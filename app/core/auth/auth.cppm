module;

#include <asio.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

export module auth;

import storage;

using asio::ip::tcp;

struct Config {
	const std::string client_id {"xy75wpl6n3erfhkkde0wkcmoz3ykdi"};
	const std::string redirect_uri {"http://localhost:8080/callback"};
};

export class AuthManager {
public:
	static AuthManager &instance() {
		static AuthManager inst;
		return inst;
	}

	AuthManager(const AuthManager&) = delete;
	AuthManager &operator=(const AuthManager&) = delete;

	std::string get_token() const {
		std::lock_guard<std::mutex> lock(_mutex);
		return _token.access_token;
	}

	std::string get_oauth_irc_token() const {
		return "oauth:" + get_token();
	}

	void authorize() {
		std::thread server_thread([this]() { run_local_server(); });
		
		std::string auth_url = get_auth_url();
		std::cout << "Open link to browser:\n" << auth_url << "\n";

		/* waiting for the server to get token */
		{
			std::unique_lock<std::mutex> lock(_token_mutex);
			_token_cv.wait(lock, [this]{ return _token_received; });
			lock.unlock();
		}
		server_thread.join();
		save_token();
	}

private:
	std::string get_auth_url() const {
		std::ostringstream oss;
		oss << "https://id.twitch.tv/oauth2/authorize"
			   "?response_type=token"
			   "&client_id=" + _config.client_id +
			   "&redirect_uri=" + _config.redirect_uri +
			   "&scope=chat:read";
		return oss.str();
	}

	bool is_expirate(const std::string &expires_at) const {
		if (expires_at.empty()) {
			return true;
		}
		
		bool is_expirate = std::chrono::system_clock::now()
			.time_since_epoch().count() >= std::stoll(expires_at);
		if (is_expirate) {
			StorageManager::instance().clear_old_tokens();
		}
		return is_expirate;
	}

	void save_token() const {
		StorageManager::instance().save_token(_token);
	}

	void load_token() {
		std::lock_guard lock(_mutex);
		
		_token = StorageManager::instance().get_token();
		if (_token.access_token.empty() || is_expirate(_token.expires_at)) {
			authorize();
		}
	}

	void run_local_server() {
		try {
			asio::io_context io_context;
			tcp::acceptor acceptor(io_context,
				tcp::endpoint(tcp::v4(), 8080));

			while (!_token_received) {
				tcp::socket socket(io_context);
				acceptor.accept(socket);

				char data[2048];
				size_t length = socket.read_some(asio::buffer(data));
				std::string request(data, length);
				/* checked path */
				if (request.find("GET /callback") == 0) {
					std::string page =
						"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
						"<!DOCTYPE html><body>"
						"<h1>Twitch account authorisation is being performed.</h1>"
						"<script>"
						"const hash = window.location.hash.substring(1);"
						"const params = new URLSearchParams(hash);"
						"const token = params.get('access_token');"
						"if(token) fetch('http://localhost:8080/token?access_token='+token)"
						".then(()=>setTimeout(()=>window.close(),1500));"
						"</script>"
						"</body></html>";
					asio::write(socket, asio::buffer(page));
				} else if (request.find("GET /token?access_token=") == 0) {
					auto pos = request.find("access_token=") + 13;
					auto end = request.find(' ', pos);
					std::string token_str = request.substr(pos, end - pos);

					{
						_token.access_token = token_str;
						_token.expires_at = std::to_string((std::chrono::system_clock
							::now() + std::chrono::hours(59*24)).time_since_epoch().count());
					}
					_token_received = true;
					_token_cv.notify_one();

					/* simple response */
					std::string response =
						"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
						"<html><body>Token received!</body></html>";
					asio::write(socket, asio::buffer(response));
				}
			}
		} catch (std::exception& e) {
			std::cerr << "[AuthManager] server error: " << e.what() << "\n";
		}
	}

private:
	AuthManager() : _token_received(false) {
		if (_token.access_token.find("invalid") != std::string::npos) {
			load_token();
		}
	}

	Config _config;
	Token _token {"invalid"};
	mutable std::mutex _mutex;

	std::mutex _token_mutex;
	std::condition_variable _token_cv;
	bool _token_received;
};