module;

#include <sqlite3.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string>

export module storage;

import :entities;

export struct UserMessageCount {
	std::string display_name;
	std::string color;
	int message_count;
};

export struct HourMessageCount {
	int hour;
	int msg_count;
};

export class StorageManager {
public:
	static StorageManager &instance() {
		static StorageManager inst;
		return inst;
	}

	bool init(std::string_view db_path = "data/sarcm.db") {
		if (sqlite3_open(db_path.data(), &_db) != SQLITE_OK) {
			std::cerr << "[StorageManager] DB open error: " << sqlite3_errmsg(_db) << "\n";
			return false;
		}
		exec("PRAGMA journal_mode=WAL;");
		exec("PRAGMA synchronous=NORMAL;");

		exec(R"SQL(
	        CREATE TABLE IF NOT EXISTS users (
	            id TEXT PRIMARY KEY,
	            login TEXT,
	            display_name TEXT
	        );
	    )SQL");

		exec(R"SQL(
	        CREATE TABLE IF NOT EXISTS user_channel_tags (
	            user_id TEXT,
	            channel_id TEXT,
	            badges TEXT,
	            color TEXT,
	            PRIMARY KEY (user_id, channel_id),
	            FOREIGN KEY (user_id) REFERENCES users(id),
	            FOREIGN KEY (channel_id) REFERENCES users(id)
	        );
	    )SQL");

		exec(R"SQL(
	        CREATE TABLE IF NOT EXISTS messages (
	            id INTEGER PRIMARY KEY AUTOINCREMENT,
	            channel_id TEXT,
	            user_id TEXT,
	            text TEXT,
	            timestamp TEXT,
	            FOREIGN KEY (channel_id) REFERENCES users(id),
	            FOREIGN KEY (user_id) REFERENCES users(id)
	        );
	    )SQL");

		return true;
	}

	void upsert_user(const User &user) const {
		sqlite3_stmt *stmt;
		const char *sql = R"SQL(
	        INSERT INTO users (id, login, display_name)
	        VALUES (?, ?, ?)
	        ON CONFLICT(id) DO UPDATE SET
	            login = excluded.login,
	            display_name = excluded.display_name;
	    )SQL";

		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			throw std::runtime_error("[StorageManager] Failed to prepare statement for users");
		}

		sqlite3_bind_text(stmt, 1, user.id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, user.login.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, user.display_name.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << "[StorageManager] Insert user failed: " << sqlite3_errmsg(_db) << "\n";
		}
		sqlite3_finalize(stmt);
	}

	void upsert_user_tags(const UserChannelTags &tags) const {
		sqlite3_stmt *stmt;
		const char *sql = R"SQL(
		    INSERT INTO user_channel_tags (user_id, channel_id, badges, color)
	        VALUES (?, ?, ?, ?)
	      	ON CONFLICT(user_id, channel_id) DO UPDATE SET
	            badges = excluded.badges,
	            color = excluded.color;
	    )SQL";

		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			throw std::runtime_error("[StorageManager] Failed to prepare statement for tags");
		}

		sqlite3_bind_text(stmt, 1, tags.user_id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, tags.channel_id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, tags.badges.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 4, tags.color.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << "[StorageManager] Insert tags failed: " << sqlite3_errmsg(_db) << "\n";
		}
		sqlite3_finalize(stmt);
	}

	void save_message(const ChatMessage &msg) const {
		sqlite3_stmt *stmt;
		const char *sql = R"SQL(
		    INSERT INTO messages (channel_id, user_id, text, timestamp)
	        VALUES (?, ?, ?, ?);
		)SQL";

		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			throw std::runtime_error("[StorageManager] Failed to prepare statement for messages");
		}
		sqlite3_bind_text(stmt, 1, msg.channel_id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, msg.user_id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, msg.text.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 4, msg.timestamp.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << "[StorageManager] Insert message failed: " << sqlite3_errmsg(_db) << "\n";
		}
		sqlite3_finalize(stmt);
	}

	std::vector<UserMessageCount> get_users(
		std::string_view channel, const int limit = 10) const
	{
		std::vector<UserMessageCount> top_users;
		top_users.reserve(limit);

		const char *query = R"SQL(
	        SELECT u.display_name, COALESCE(t.color, '') as color, COUNT(m.id) as cnt
	        FROM users u
	        JOIN messages m ON m.user_id = u.id
			LEFT JOIN user_channel_tags t
				ON t.user_id = u.id AND t.channel_id = m.channel_id
	        WHERE m.channel_id = ?
	        GROUP BY u.display_name, t.color
	        ORDER BY cnt DESC
	        LIMIT ?
	    )SQL";

		sqlite3_stmt *stmt;
		if (sqlite3_prepare_v2(_db, query, -1, &stmt, nullptr) != SQLITE_OK) {
			std::cerr << "[StorageManager] SQLite error: " << "Failed to prepare statement" << "\n";
			return {};
		}
		sqlite3_bind_text(stmt, 1, channel.data(), -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 2, limit);

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			UserMessageCount u;
			u.display_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			u.color = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			u.message_count = sqlite3_column_int(stmt, 2);
			top_users.emplace_back(u);
		}

		sqlite3_finalize(stmt);
		return top_users;
	}

	std::vector<HourMessageCount> get_hourly_activity(
		std::string_view channel, std::string_view date)
	{
		std::vector<HourMessageCount> activity;
		const char *query = R"SQL(
            SELECT strftime('%H', timestamp) AS hour,
                   COUNT(*) as message_count
            FROM messages
            WHERE channel_id = ?
              AND date(timestamp) = ?
            GROUP BY hour
            ORDER BY hour;
        )SQL";

		sqlite3_stmt* stmt;
		if (sqlite3_prepare_v2(_db, query, -1, &stmt, nullptr) != SQLITE_OK) {
			std::cerr << "[StorageManager] SQLite error: " << "Failed to prepare statement" << "\n";
			return {};
		}
		sqlite3_bind_text(stmt, 1, channel.data(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 2, date.data(), -1, SQLITE_STATIC);

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			HourMessageCount h {
				std::stoi(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))),
				sqlite3_column_int(stmt, 1)
			};
			activity.emplace_back(h);
		}

		sqlite3_finalize(stmt);
		return activity;
	}

private:
	StorageManager() {
		init();
	}
	~StorageManager() {
		if (_db) sqlite3_close(_db);
	}

	StorageManager(const StorageManager&) = delete;
	StorageManager& operator=(const StorageManager&) = delete;

	void exec(const std::string &sql) {
		char *errMsg = nullptr;
		if (sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
			std::cerr << "[StorageManager] SQLite error: " << errMsg << "\n";
			sqlite3_free(errMsg);
		}
	}

	sqlite3 *_db = nullptr;
};