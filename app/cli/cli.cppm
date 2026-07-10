module;

#include <iostream>
#include <sstream>
#include <ranges>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <sys/ioctl.h>
#include <unistd.h>

export module tcli;

import connection;
import analysis;
import parser;

class ChatWorker {
public:
	ChatWorker(std::string username, std::string channel) :
		_username(std::move(username)),
		_channel(std::move(channel)),
		_thread(&ChatWorker::run, this) {}

	void stop() {
		_conn.close();
		if (_thread.joinable()) {
			_thread.join();
		}
	}

	const std::string &channel() const {
		return _channel;
	}

private:
	void run() {
		_conn.connect(_username, _channel);
	}

	std::string	_username;
	std::string	_channel;
	std::thread	_thread;
	Connection	_conn;
};

class CliManager {
public:
	void start_channel(const std::string &username, const std::string &channel) {
		if (_workers.contains(channel)) {
			std::cout << "  already tracking channel: " << channel << "\n";
			return;
		}
		_workers[channel] =
			std::make_unique<ChatWorker>(username, channel);
		std::cout << "  started tracking channel: " << channel << "\n";
	}

	void stop_channel(const std::string &channel) {
		if (_workers.contains(channel)) {
			_workers[channel]->stop();
			_workers.erase(channel);
			std::cout << "  stopped tracking channel: " << channel << "\n";
		} else {
			std::cout << "  channel not active: " << channel << "\n";
		}
	}

	void stop_all() {
		for (auto &worker: _workers | std::views::values) {
			worker->stop();
			worker.reset();
		}
		_workers.clear();
		std::cout << "  stopped all channels.\n";
	}

	void status() {
		if (_workers.empty()) {
			std::cout << "  no active channels.\n";
		} else {
			std::cout << "  active channels:\n";
			for (const auto &ch: _workers | std::views::keys) {
				std::cout << "  - " << ch << "\n";
			}
		}
	}

	void top_chatters(std::string_view channel, const int limit = 10) const {
		AnalysisManager analysis;
		auto users = analysis.top_chatters(channel, limit);
		if (users.empty()) {
			std::cout << "  no data about the channel " << channel << "\n";
			return;
		}

		for (const auto &[user, color, msg_count] : users) {
			std::cout << apply_ansi_color(color, user) << ' ' << msg_count << '\n';
		}
	}

	void hourly_active(std::string_view channel, std::string_view date) const {
		AnalysisManager analysis;
		auto activity = analysis.hourly_active(channel, date);
		if (activity.empty()) {
			std::cout << "  no data about the channel " << channel <<
				" in date " << date << "\n";
			return;
		}

		auto max_count_it = std::max_element(
			activity.begin(), activity.end(),
			[](const auto &a, const auto &b) {
				return a.msg_count < b.msg_count;
			}
		);
		int max_count = max_count_it != activity.end() ? max_count_it->msg_count : 1;

		auto get_console_width = [] -> unsigned short {
			struct winsize w{};
			if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
				return 80;
			}
			return w.ws_col;
		};
		int console_width = get_console_width();
		constexpr int label_width = 8;
		const int max_bar_width = console_width - label_width - 6;

		for (auto& [hour, count] : activity) {
			const int bar_length = static_cast<int>(count / static_cast<double>(max_count) * max_bar_width);

			std::cout << (hour < 10 ? "0" : "") << hour << ".00 | ";
			for (int i = 0; i < bar_length; ++i) std::cout << "█";
			std::cout << " " << count << "\n";
		}
	}

private:
	std::unordered_map<std::string, std::unique_ptr<ChatWorker>> _workers;
};

export void cli_start() {
	CliManager cli;

	std::string cmd;
	while (true) {
		std::cout << "> ";
		if (!std::getline(std::cin, cmd)) break;

		std::istringstream iss(cmd);
		if (cmd == "status" || cmd == "s") {
			cli.status();
		} else if (cmd.starts_with("stop ") || cmd.starts_with("rm ")) {
			std::string _, ch;
			iss >> _ >> ch;
			if (!ch.empty()) {
				cli.stop_channel(ch);
			}
		} else if (cmd == "stop-all" || cmd == "rma") {
			cli.stop_all();
		} else if (cmd.starts_with("start ") || cmd.starts_with("add ")) {
			std::string _, nick, ch;
			iss >> _ >> nick >> ch;
			if (!nick.empty() && !ch.empty()) {
				cli.start_channel(nick, ch);
			}
		} else if (cmd.starts_with("top-chatters ") || cmd.starts_with("tc ")) {
			std::string _, ch;
			int limit = -1;
			iss >> _ >> ch >> limit;
			if (!ch.empty() && limit == -1) {
				cli.top_chatters(ch);
			} else if (!ch.empty() && limit > 0) {
				cli.top_chatters(ch, limit);
			}
		} else if (cmd.starts_with("hourly-active ") || cmd.starts_with("ha ")) {
			std::string _, ch, dt;
			iss >> _ >> ch >> dt;
			if (!dt.empty() && !ch.empty()) {
				cli.hourly_active(ch, dt);
			}
		} else if (cmd == "q") {
			break;
		} else if (cmd.starts_with("!")) {
			std::system(cmd.erase(0, 1).c_str());
		} else {
			std::cout << "Unknown command: " << cmd << "\n";
		}
	}
}