module;

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <unordered_map>

export module parser;

struct ParsedMessage {
	std::map<std::string, std::string> tags;	// все теги Twitch IRC
	std::string prefix;                			// часть вида user!user@user.tmi.twitch.tv
	std::string command;               			// PRIVMSG, JOIN, PART, PING и т.д.

	std::string user_id;
	std::string display_name;
	std::string username;
	std::string timestamp;
	std::string channel;
	std::string badges;
	std::string color;
	std::string text;
};

inline const std::unordered_map<std::string, std::string> BADGE_MAP = {
	{"moderator",   "⚔️"},
	{"vip",         "🩷"},
	{"subscriber",  "✨"},
};

export ParsedMessage parse_irc_message(const std::string &raw) {
	ParsedMessage msg;

	std::istringstream iss(raw);
	std::string token;

	/* tags (begin with @) */
	if (!raw.empty() && raw[0] == '@') {
		std::getline(iss, token, ' ');
		/* token = "@badge-info=;badges=...;display-name=User" */
		token.erase(0, 1); /* rm '@' */
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

	/* prefix (begin with :) */
	if (iss.peek() == ':') {
		iss.get();
		std::getline(iss, msg.prefix, ' ');
	}

	/* command (PRIVMSG, PING, NOTICE…) */
	iss >> msg.command;

	/* channel (e.g. #mychannel) */
	iss >> msg.channel;

	/* user_id */
	msg.user_id = msg.tags.contains("user-id") ? msg.tags.at("user-id") : "";

	/* display name */
	msg.display_name = msg.tags.contains("display-name")
						? msg.tags.at("display-name") : msg.prefix;

	/* username */
	auto exclam = msg.prefix.find('!');
	msg.username = (exclam != std::string::npos) ? msg.prefix.substr(0, exclam) : msg.prefix;

	/* timestamp (tmi-sent-ts) SO 8601 */
	if (msg.tags.contains("tmi-sent-ts")) {
		try {
			std::chrono::milliseconds ms(std::stoll(msg.tags.at("tmi-sent-ts")));
			std::chrono::system_clock::time_point tp(ms);
			std::time_t t = std::chrono::system_clock::to_time_t(tp) + 3 * 3600;
			std::tm tm{};
			gmtime_r(&t, &tm);
			char buf[32];
			strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
			msg.timestamp = buf;
		} catch(...) {
			msg.timestamp = "";
		}
	}

	/* bages */
	if (msg.tags.contains("badges")) {
		std::istringstream iss(msg.tags.at("badges"));
		std::string badge;
		while (std::getline(iss, badge, ',')) {
			auto pos = badge.find('/');
			std::string key = (pos != std::string::npos)
								? badge.substr(0,pos) : badge;
			if (BADGE_MAP.contains(key)) {
				msg.badges += key + " ";
			}
		}
	}

	/* color */
	msg.color = msg.tags.contains("color") ? msg.tags.at("color") : "";

	/* text msg (begin with :) */
	std::getline(iss, msg.text);
	if (!msg.text.empty() && msg.text[0] == ' ')
		msg.text.erase(0, 1);
	if (!msg.text.empty() && msg.text[0] == ':')
		msg.text.erase(0, 1);

	return msg;
}

export std::string apply_ansi_color(const std::string &hex_color, const std::string &text) {
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

export std::string format_chat_message(const ParsedMessage &msg) {
	std::ostringstream out;

	/* bages */
	std::string badges_str;
	std::istringstream iss(msg.badges);
	std::string badge;
	while (iss >> badge) {
		if (BADGE_MAP.contains(badge)) {
			badges_str += BADGE_MAP.at(badge) + " ";
		}
	}

	/* format str */
	size_t start = msg.timestamp.find('T') + 1;
	out << msg.timestamp.substr(start, 5) << " [" << badges_str
		<< apply_ansi_color(msg.color, msg.display_name) << "] " << msg.text;

	return out.str();
}