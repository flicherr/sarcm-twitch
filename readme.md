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
git clone https://github.com/flicherr/sarcm-twitch.git
cmake -S . -B --build -G Ninja
cmake --build build
```

### Use examples
Startup:
```bash
./build/sarcm
```
Start capturing the channel chat:
```bash
> start <your_nickname> <channel>
```
Stop capturing the channel chat:
```bash
> stop <channel>
```
Stop capturing the chats of all channels:
```bash
> stop-all
```
Display information about the channel's top chatters:
```bash
> top-chatters <channel> <quantity>
```
Display chat activity <date> in the <channel> channel:
```bash
> hourly-active <channel> <date>
```
Display list active channels:
```bash
> status
```
Quit the program:
```bash
> q
```
Execute command terminal:
```bash
> !clear
```