module;

#include <asio.hpp>
#include <iostream>
#include <string>

export module connection;

import auth;
import parser;
import storage;

using asio::ip::tcp;

export class Connection {
public:
	explicit Connection() : _socket(_io_context) {}

	void connect(const std::string &nick, const std::string &channel) {
		try {
			tcp::resolver resolver(_io_context);
			auto endpoints = resolver.resolve("irc.chat.twitch.tv", "6667");
			asio::connect(_socket, endpoints);

			const auto &auth = AuthManager::instance();

			send_line("PASS " + auth.get_oauth_irc_token());
			send_line("NICK " + nick);
			send_line("JOIN #" + channel);

			send_line("CAP REQ :twitch.tv/tags twitch.tv/commands twitch.tv/membership");

			do_read_once();
			_io_context.run();

		} catch (std::exception &e) {
			std::cerr << "[Connection] " << e.what() << '\n';
		}
	}

	void close() {
		_io_context.stop();
		if (_socket.is_open()) {
			_socket.close();
		}
	}

private:
	void send_line(const std::string &line) {
		std::string msg = line + "\r\n";
		asio::write(_socket, asio::buffer(msg));
	}

	void do_read_once() {
		asio::async_read_until(_socket, _buffer, "\r\n",
			[this](std::error_code ec, std::size_t bytes_transferred) {
				if (ec) {
					std::cerr << ec.message() << '\n';
					return;
				}

				std::istream is(&_buffer);
				std::string msg;
				std::getline(is, msg);

				if (msg.find("PING :tmi.twitch.tv") != std::string::npos) {
					send_line("PONG :tmi.twitch.tv");
				}
				if (msg.find("PRIVMSG") != std::string::npos) {
					auto parsed = parse_irc_message(msg);
					auto &storage = StorageManager::instance();
					storage.upsert_user({parsed.user_id, parsed.username, parsed.display_name});
					storage.upsert_user_tags({parsed.user_id, parsed.channel, parsed.badges, parsed.color});
					storage.save_message({parsed.channel, parsed.user_id, parsed.text, parsed.timestamp});
					// std::cout << format_chat_message(parsed) << '\n';
				}
				asio::post(_io_context, [this]() { do_read_once(); });
			});
	}

private:
	asio::io_context _io_context;
	tcp::socket _socket;
	asio::streambuf _buffer;
	bool _connected = true;
};