module;

#include <iostream>
#include <sstream>
#include <ranges>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

export module tcli;

import connection;
import analysis;
import parser;

class ChatWorker {
public:
	ChatWorker(std::string username, std::string channel) :
	_username(std::move(username)), _channel(std::move(channel)), _thread(&ChatWorker::run, this) {}

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
			std::cout << "Already tracking channel: " << channel << "\n";
			return;
		}
		_workers[channel] =
			std::make_unique<ChatWorker>(username, channel);
		std::cout << "Started tracking channel: " << channel << "\n";
	}

	void stop_channel(const std::string &channel) {
		if (_workers.contains(channel)) {
			_workers[channel]->stop();
			_workers.erase(channel);
			std::cout << "Stopped tracking channel: " << channel << "\n";
		} else {
			std::cout << "Channel not active: " << channel << "\n";
		}
	}

	void stop_all() {
		for (auto &worker: _workers | std::views::values) {
			worker->stop();
			worker.release();
		}
		_workers.clear();
		std::cout << "Stopped all channels.\n";
	}

	void status() {
		if (_workers.empty()) {
			std::cout << "No active channels.\n";
		} else {
			std::cout << "Active channels:\n";
			for (const auto &ch: _workers | std::views::keys) {
				std::cout << "  - " << ch << "\n";
			}
		}
	}

	void top_users_channel(const std::string &channel, const int &limit = 10) {
		AnalysisManager analysis;
		auto users = analysis.get_top_users(channel, limit);
		if (users.empty()) {
			std::cout << "No data about the channel " << channel << "\n";
			return;
		}

		std::cout << "Top " << limit << " users from channel " << channel << ":\n";
		for (const auto &[user, color, msg_count] : users) {
			std::cout << apply_ansi_color(color, user) << ' ' << msg_count << '\n';
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
		if (cmd == "status" || cmd == "stat") {
			cli.status();
		} else if (cmd.starts_with("stop ")) {
			std::string _, ch;
			iss >> _ >> ch;
			if (!ch.empty()) {
				cli.stop_channel(ch.data());
			}
		} else if (cmd == "stop-all" || cmd == "q") {
			cli.stop_all();
			break;
		} else if (cmd.starts_with("start ")) {
			std::string _, nick, ch;
			iss >> _ >> nick >> ch;
			if (!nick.empty() && !ch.empty()) {
				cli.start_channel(nick.data(), ch.data());
			}
		} else if (cmd.starts_with("top-us ")) {
			std::string _, ch;
			int limit = -1;
			iss >> _ >> ch >> limit;
			if (!ch.empty() && limit == -1) {
				cli.top_users_channel(ch.data());
			} else if (!ch.empty() && limit > 0) {
				cli.top_users_channel(ch.data(), limit);
			}
		} else if (cmd.starts_with("!")) {
			std::system(cmd.erase(0, 1).c_str());
		} else {
			std::cout << "Unknown command: " << cmd << "\n";
		}
	}
}