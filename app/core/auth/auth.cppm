module;

#include <asio.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <optional>
#include <mutex>
#include <thread>

export module auth;

using asio::ip::tcp;

struct Config {
	std::string client_id;
	std::string redirect_uri;
};

struct Token {
	std::string access_token;
	std::chrono::system_clock::time_point expires_at;

	bool is_expired() const {
		return std::chrono::system_clock::now() >= expires_at;
	}
};

export class AuthManager {
public:
	static AuthManager &instance(const Config &config) {
		static AuthManager inst(config);
		return inst;
	}

	AuthManager(const AuthManager&) = delete;
	AuthManager &operator=(const AuthManager&) = delete;

	Token get_token() const {
		std::lock_guard<std::mutex> lock(_mutex);
		if (_token.access_token.empty())
			throw std::runtime_error("No token set");
		if (_token.is_expired())
			throw std::runtime_error("Token expired");
		return _token;
	}

	std::string get_oauth_irc_token() const {
		return "oauth:" + get_token().access_token;
	}

	void authorize() {
		std::thread server_thread([this]() { run_local_server(); });

		std::string auth_url = get_auth_url();
		std::cout << "Open link to browser:\n" << auth_url << "\n";

		/* waiting for the server to get token */
		{
			std::unique_lock<std::mutex> lock(_token_mutex);
			_token_cv.wait(lock, [this]{ return _token_received; });
		}

		server_thread.join();
		save_token(_token, "token.txt");
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

	void save_token(const Token &token, const std::string &path) {
		std::lock_guard<std::mutex> lock(_mutex);
		std::ofstream file(path, std::ios::trunc);
		if (!file.is_open()) {
			throw std::runtime_error("Cannot open file: " + path);
		}

		file << token.access_token << "\n"
			 << std::chrono::duration_cast<std::chrono::seconds>(
					 token.expires_at.time_since_epoch()).count() << "\n";
	}

	void run_local_server() {
		try {
			asio::io_context io_context;
			tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

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
						"<p>Twitch account authorisation is being performed.</p>"
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
						std::lock_guard<std::mutex> lock(_mutex);
						_token.access_token = token_str;
						_token.expires_at = std::chrono::system_clock::now() +
											std::chrono::hours(4);
					}

					_token_received = true;
					_token_cv.notify_one();

					/* simple response */
					std::string response =
						"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
						"<html><body>token received!</body></html>";
					asio::write(socket, asio::buffer(response));
				}
			}
		} catch (std::exception& e) {
			std::cerr << "[AuthManager] server error: " << e.what() << "\n";
		}
	}

private:
	AuthManager(const Config &config)
		: _config(config), _token_received(false) {}

	Config _config;
	Token _token;
	mutable std::mutex _mutex;

	std::mutex _token_mutex;
	std::condition_variable _token_cv;
	bool _token_received;
};
