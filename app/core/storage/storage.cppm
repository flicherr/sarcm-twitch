module;

#include <sqlite3.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string>

export module storage;

import storage.entities;

export class StorageManager {
public:
	static StorageManager &instance() {
		static StorageManager inst;
		return inst;
	}

	bool init(const std::string &db_path = "sarcm.db") {
		if (sqlite3_open(db_path.c_str(), &_db) != SQLITE_OK) {
			std::cerr << "DB open error: " << sqlite3_errmsg(_db) << "\n";
			return false;
		}
		sqlite3_exec(_db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);

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

	void upsert_user(const User &user) {
		sqlite3_stmt *stmt;
		const char *sql = R"SQL(
	        INSERT INTO users (id, login, display_name)
	        VALUES (?, ?, ?)
	        ON CONFLICT(id) DO UPDATE SET
	            login = excluded.login,
	            display_name = excluded.display_name;
	    )SQL";

		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error("Failed to prepare statement for users");

		sqlite3_bind_text(stmt, 1, user.id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, user.login.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, user.display_name.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << "Insert user failed: " << sqlite3_errmsg(_db) << "\n";
		}
		sqlite3_finalize(stmt);
	}

	void upsert_user_tags(const UserChannelTags &tags) {
		sqlite3_stmt *stmt;
		const char *sql = R"SQL(
	        INSERT INTO user_channel_tags (user_id, channel_id, badges, color)
	        VALUES (?, ?, ?, ?)
	        ON CONFLICT(user_id, channel_id) DO UPDATE SET
	            badges = excluded.badges,
	            color = excluded.color;
	    )SQL";

		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			throw std::runtime_error("Failed to prepare statement for tags");
		}

		sqlite3_bind_text(stmt, 1, tags.user_id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, tags.channel_id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, tags.badges.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 4, tags.color.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << "Insert tags failed: " << sqlite3_errmsg(_db) << "\n";
		}
		sqlite3_finalize(stmt);
	}

	void save_message(const ChatMessage &msg) {
		sqlite3_stmt *stmt;
			const char *sql = R"SQL(
	        INSERT INTO messages (channel_id, user_id, text, timestamp)
	        VALUES (?, ?, ?, ?);
	    )SQL";

		if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			throw std::runtime_error("Failed to prepare statement for messages");
		}
		sqlite3_bind_text(stmt, 1, msg.channel_id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, msg.user_id.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, msg.text.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 4, msg.timestamp.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << "Insert message failed: " << sqlite3_errmsg(_db) << "\n";
		}
		sqlite3_finalize(stmt);
	}

private:
	StorageManager() = default;
	~StorageManager() {
		if (_db) sqlite3_close(_db);
	}

	StorageManager(const StorageManager&) = delete;
	StorageManager& operator=(const StorageManager&) = delete;

	void exec(const std::string &sql) {
		char* errMsg = nullptr;
		if (sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
			std::string err = errMsg ? errMsg : "unknown error";
			sqlite3_free(errMsg);
			throw std::runtime_error("SQLite error: " + err);
		}
	}

	sqlite3 *_db = nullptr;
};