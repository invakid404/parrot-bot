#include <concord/discord.h>
#include <stdio.h>
#include <stdlib.h>

void on_ready(struct discord*, const struct discord_ready* event) {
    printf("Client is ready: %s#%s\n", event->user->username,
           event->user->discriminator);
}

int main(void) {
    char* bot_token = getenv("PARROT_BOT_TOKEN");

    struct discord* client = discord_init(bot_token);
    discord_set_on_ready(client, &on_ready);

    return discord_run(client);
}
