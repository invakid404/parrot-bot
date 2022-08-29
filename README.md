# parrot-bot

Your very own meme repository within Discord.

This is a Discord bot, that you can program to respond in a specific way to certain messages. This is mostly useful for
keeping track of your favorite memes.

## Quick Start

```bash
$ cmake .
$ cmake --build .
$ PARROT_BOT_TOKEN=<TOKEN> ./parrot-bot
```

## Dependencies:

- [concord](https://github.com/Cogmasters/concord) (vendored): discord API
- [libcurl](https://curl.se/libcurl/): fetching http resources (also needed for concord)
- [sqlite3](https://www.sqlite.org/index.html): database
- [libpcre2](https://sourceforge.net/projects/pcre/): regex engine
- [chan](https://github.com/tylertreat/chan.git) (vendored): communication mechanism
- [openssl::crypto](https://www.openssl.org): hashing
- [libmagic](https://www.darwinsys.com/file/): determining file types
- [stb](https://github.com/nothings/stb) (vendored): hash table
- [pthread](https://manned.org/pthread.3): multithreading
- [glibc](https://www.gnu.org/software/libc/): some non-standard libraries

## FAQ

- **Why C?**: Why not?
- **Why should I care?**: You shouldn't.