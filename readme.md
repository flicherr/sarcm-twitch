# sarcm-Twitch
A console application for saving, analyzing and replaying chat messages Twitch stream.

### Features
- Saving the actions of chatters in Twitch live stream in a local SQLite database:
  - nickname, roles (mod, sub, VIP);
  - message, emotions;
  - channel, stream;
  - time of sending.
- Analyze stream chat activity:
  - number of messages by time (activity graph);
  - most active users;
  - popular phrases/words;
  - emotion usage statistics;
  - activity of moderators, VIPs, and subs.
- Replay saved chat according to message timestamps.

### Installation
Building for your current system architecture:
```bash
git clone https://github.com/flicherr/twitch-sarcm.git
cmake -S . -B --build -G Ninja
cmake --build build
```

### Use examples
```bash
./build/sarcm <your twitch-nikname> <channel>
```
