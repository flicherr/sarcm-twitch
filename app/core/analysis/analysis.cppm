module;

#include <iostream>
#include <string>
#include <vector>

export module analysis;

import storage;

using UserActivity = UserMessageCount;

export class AnalysisManager {
public:
	explicit AnalysisManager()
		: _storage(StorageManager::instance()) {}

	std::vector<UserActivity> get_top_users(
		const std::string &channel, int limit = 10) {
		return _storage.get_users(channel, limit);
	}

	// std::vector<HourlyActivity> get_activity_by_hour(
	// 	const std::string &channel_id) {
	// 	return _storage.get_messages_by_channel(channel_id);
	// }

private:
	StorageManager &_storage;
};