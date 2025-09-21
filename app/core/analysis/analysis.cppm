module;

#include <iostream>
#include <string>
#include <vector>

export module analysis;

import storage;
import parser;

using UserActivity = UserMessageCount;

export class AnalysisManager {
public:
	explicit AnalysisManager()
		: _storage(StorageManager::instance()) {}

	std::vector<UserActivity> get_top_users(
		const std::string &channel, int limit = 10) {
		auto users = _storage.get_users(channel, limit);

		for (const auto &user : users) {
			std::cout << apply_ansi_color(user.color, user.display_name)
				<< ' ' << user.message_count << '\n';
		}
		std::cout << '\n';

		return users;
	}

	// std::vector<HourlyActivity> get_activity_by_hour(
	// 	const std::string &channel_id) {
	// 	return _storage.get_messages_by_channel(channel_id);
	// }

private:
	StorageManager &_storage;
};