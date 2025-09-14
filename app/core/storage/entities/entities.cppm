module;

#include <string>

export module storage.entities;

export {
	struct User {
		std::string id;
		std::string login;
		std::string display_name;
	};

	struct UserChannelTags {
		std::string user_id;
		std::string channel_id; // тоже user.id
		std::string badges;
		std::string color;
	};

	struct ChatMessage {
		std::string channel_id;
		std::string user_id;
		std::string text;
		std::string timestamp;
	};
}