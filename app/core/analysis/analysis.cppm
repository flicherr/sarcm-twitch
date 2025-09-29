module;

#include <iostream>
#include <string>
#include <vector>

export module analysis;

import storage;

using UserActivity = UserMessageCount;
using HourActivity = HourMessageCount;

export class AnalysisManager {
public:
	explicit AnalysisManager()
		: _storage(StorageManager::instance()) {}

	std::vector<UserActivity> top_chatters(
		std::string_view channel, const int limit = 10) const {
		return _storage.get_users(channel, limit);
	}

	std::vector<HourActivity> hourly_active(
		std::string_view channel, std::string_view date) {
		return _storage.get_hourly_activity(channel, date);
	}

private:
	StorageManager &_storage;
};