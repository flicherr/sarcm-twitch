module;

#include <asio.hpp>
#include <iostream>
#include <functional>
#include <string>
#include <thread>

export module connection;

import auth;
import parser;

using asio::ip::tcp;

export class Connection {
public:
	explicit Connection() : _socket(_io_context) {}

	bool connect(const std::string &host,
				 const std::string &port,
				 const std::string &nick,
				 const std::string &channel)
	{
		_nick = nick;
		_channel = channel;

		try {
			tcp::resolver resolver(_io_context);
			auto endpoints = resolver.resolve(host, port);
			asio::connect(_socket, endpoints);

			/* authorize IRC */
			auto &auth = AuthManager::instance({
				"xy75wpl6n3erfhkkde0wkcmoz3ykdi",
				"http://localhost:8080/callback"
			});
			if (!auth.get_oauth_irc_token().empty()) {
				_oauth_token = auth.get_oauth_irc_token();
			}

			send_line("PASS " + _oauth_token);
			send_line("NICK " + _nick);
			send_line("JOIN #" + _channel);

			send_line("CAP REQ :twitch.tv/tags twitch.tv/commands twitch.tv/membership");

			do_read();
			return true;

		} catch (std::exception &e) {
			std::cerr << e.what() << '\n';
			return false;
		}
	}

	void close() {
		_io_context.stop();
		if (_socket.is_open()) {
			_socket.close();
		}
	}

private:
	asio::io_context _io_context;
	tcp::socket _socket;
	asio::streambuf _buffer;

	std::string _nick;
	std::string _channel;
	std::string _oauth_token;

	void send_line(const std::string &line) {
		std::string msg = line + "\r\n";
		asio::write(_socket, asio::buffer(msg));
	}

	void do_read() {
		asio::async_read_until(_socket, _buffer, "\r\n",
			[this](std::error_code ec, std::size_t bytes_transferred) {
				if (!ec) {
					std::istream is(&_buffer);
					std::string msg;
					std::getline(is, msg);

					if (msg.find("PING :tmi.twitch.tv") != std::string::npos) {
					send_line("PONG :tmi.twitch.tv");
				}
					if (msg.find("PRIVMSG") != std::string::npos) {
						auto chat = parse_irc_message(msg);
						std::cout << format_chat_message(chat) << "\n";
					}

					do_read();
				} else {
					std::cerr << ec.message() << '\n';
				}
			});
		_io_context.run();
	}
};