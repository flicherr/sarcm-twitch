module;

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <unordered_map>

export module parser;

struct ChatMessage {
	std::string raw;                   			// оригинальное сообщение
	std::map<std::string, std::string> tags;	// все теги Twitch IRC
	std::string prefix;                			// часть вида user!user@user.tmi.twitch.tv
	std::string command;               			// PRIVMSG, JOIN, PART, PING и т.д.
	std::string channel;               			// #channel
	std::string text;                  			// текст сообщения
};

export ChatMessage parse_irc_message(const std::string &raw) {
	ChatMessage msg;
	msg.raw = raw;

	std::istringstream iss(raw);
	std::string token;

	// 1. Теги (начинаются с @)
	if (!raw.empty() && raw[0] == '@') {
		std::getline(iss, token, ' ');
		// token = "@badge-info=;badges=...;display-name=User"
		token.erase(0, 1); // убрать '@'
		std::istringstream tags_stream(token);
		std::string kv;
		while (std::getline(tags_stream, kv, ';')) {
			auto pos = kv.find('=');
			if (pos != std::string::npos) {
				std::string key = kv.substr(0, pos);
				std::string val = kv.substr(pos + 1);
				msg.tags[key] = val;
			} else {
				msg.tags[kv] = "";
			}
		}
	}

	// 2. Префикс (начинается с :)
	if (iss.peek() == ':') {
		iss.get();
		std::getline(iss, msg.prefix, ' ');
	}

	// 3. Команда (PRIVMSG, PING, NOTICE…)
	iss >> msg.command;

	// 4. Канал (например, #mychannel)
	iss >> msg.channel;

	// 5. Текст (начинается с :)
	std::getline(iss, msg.text);
	if (!msg.text.empty() && msg.text[0] == ' ')
		msg.text.erase(0, 1);
	if (!msg.text.empty() && msg.text[0] == ':')
		msg.text.erase(0, 1);

	return msg;
}

std::string apply_ansi_color(const std::string &hex_color,
							 const std::string &text) {
	if (hex_color.empty() || hex_color[0] != '#') return text;

	int r = 255, g = 255, b = 255; // fallback белый

	if (hex_color.size() == 7) {
		// #RRGGBB
		unsigned int rv, gv, bv;
		if (sscanf(hex_color.c_str() + 1, "%02x%02x%02x", &rv, &gv, &bv) == 3) {
			r = rv; g = gv; b = bv;
		}
	} else if (hex_color.size() == 4) {
		// #RGB → #RRGGBB
		unsigned int rv, gv, bv;
		if (sscanf(hex_color.c_str() + 1, "%1x%1x%1x", &rv, &gv, &bv) == 3) {
			r = rv * 17; g = gv * 17; b = bv * 17;
		}
	}

	std::ostringstream out;
	out << "\033[38;2;" << r << ";" << g << ";" << b << "m" << text << "\033[0m";
	return out.str();
}

inline const std::unordered_map<std::string, std::string> BADGE_MAP = {
	{"broadcaster", "🎥"},
	{"moderator",   "🛡"},
	{"vip",         "💎"},
	{"subscriber",  "⭐"},
	{"founder",     "🚀"},
	{"turbo",       "⚡"},
	{"premium",     "👑"},
	{"artist-badge","🎨"},
};

export std::string format_chat_message(const ChatMessage &msg) {
	std::ostringstream out;

	// Ник
	std::string user = msg.tags.contains("display-name") ? msg.tags.at("display-name") : msg.prefix;
	std::string color = msg.tags.contains("color") ? msg.tags.at("color") : "";

	// Бейджи
	std::string badges_str;
	if (msg.tags.contains("badges")) {
		std::istringstream iss(msg.tags.at("badges"));
		std::string badge;
		while (std::getline(iss, badge, ',')) {
			auto pos = badge.find('/');
			std::string key = (pos != std::string::npos) ? badge.substr(0,pos) : badge;
			if (BADGE_MAP.contains(key)) {
				badges_str += BADGE_MAP.at(key) + " ";
			}
		}
	}

	// Форматируем строку
	out << "[" << badges_str << apply_ansi_color(color, user) << "] ";
	out << msg.text;

	return out.str();
}

//export ChatMessage parse_message(const std::string &raw) {
//	ChatMessage msg;
//
//	// display-name=имя
//	std::regex display_name_re(R"(display-name=([^;]*))");
//	std::smatch m;
//	if (std::regex_search(raw, m, display_name_re)) {
//		msg.display_name = m[1].str();
//	}
//
//	// ник из :username!
//	std::regex user_re(R"(:([a-zA-Z0-9_]+)!.* PRIVMSG)");
//	if (std::regex_search(raw, m, user_re)) {
//		msg.user = m[1].str();
//	}
//
//	// само сообщение (после PRIVMSG #channel :)
//	std::regex text_re(R"(PRIVMSG\s+#[^\s]+\s+:(.*))");
//	if (std::regex_search(raw, m, text_re)) {
//		msg.text = m[1].str();
//	}
//
//	return msg;
//}
