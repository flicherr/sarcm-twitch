module;

#include <string>

export module storage:entities;

export struct Token {
	std::string access_token;
	std::string expires_at;
};

struct User {
	std::string id;
	std::string login;
	std::string display_name;
};

struct UserChannelTags {
	std::string user_id;
	std::string channel_id; // user.id
	std::string badges;
	std::string color;
};

struct ChatMessage {
	std::string channel_id;
	std::string user_id;
	std::string text;
	std::string timestamp;
};