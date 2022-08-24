#include <concord/discord.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void on_ready(struct discord* client, const struct discord_ready* event) {
    u64snowflake application_id = event->application->id;

    struct discord_application_command_option options[] = {
        {.name = "add",
         .description = "Add a response",
         .type = DISCORD_APPLICATION_OPTION_SUB_COMMAND,
         .options =
             &(struct discord_application_command_options){
                 .size = 2,
                 .array =
                     (struct discord_application_command_option[]){
                         {
                             .name = "key",
                             .description =
                                 "Text to which the bot will respond",
                             .type = DISCORD_APPLICATION_OPTION_STRING,
                             .required = true,
                         },
                         {
                             .name = "value",
                             .description =
                                 "Text with which the bot will respond",
                             .type = DISCORD_APPLICATION_OPTION_STRING,
                             .required = true,
                         }}}},
        {.name = "get",
         .description = "Get all responses",
         .type = DISCORD_APPLICATION_OPTION_SUB_COMMAND,
         .options = &(struct discord_application_command_options){0}}};

    struct discord_create_global_application_command application_commands[] = {
        {.name = "responses",
         .description = "View and manage responses",
         .type = DISCORD_APPLICATION_CHAT_INPUT,
         .options = &(struct discord_application_command_options){
             .size = sizeof(options) / sizeof(options[0]),
             .array = options,
         }}};

    int application_commands_amount =
        sizeof(application_commands) / sizeof(application_commands[0]);

    for (int i = 0; i < application_commands_amount; ++i) {
        discord_create_global_application_command(
            client, application_id, &application_commands[0], NULL);
    }
}

void handle_add_response_subcommand(
    struct discord* client,
    struct discord_application_command_interaction_data_options* options) {
    char *key, *value;

    for (int i = 0; i < options->size; ++i) {
        struct discord_application_command_interaction_data_option option =
            options->array[i];

        if (!strcmp(option.name, "key")) {
            key = option.value;

            continue;
        }

        if (!strcmp(option.name, "value")) {
            value = option.value;

            continue;
        }
    }

    // TODO: respond
}

void handle_get_response_subcommand(
    struct discord* client,
    struct discord_application_command_interaction_data_options* options) {
    // TODO: respond
}

void handle_responses_command(struct discord* client,
                              const struct discord_interaction* event) {
    for (int i = 0; i < event->data->options->size; ++i) {
        struct discord_application_command_interaction_data_option option =
            event->data->options->array[i];

        printf("looking at option %s\n", option.name);
        if (!strcmp(option.name, "add")) {
            handle_add_response_subcommand(client, option.options);

            return;
        }

        if (!strcmp(option.name, "get")) {
            handle_get_response_subcommand(client, option.options);

            return;
        }
    }
}

void on_interaction(struct discord* client,
                    const struct discord_interaction* event) {
    if (event->type == DISCORD_INTERACTION_APPLICATION_COMMAND) {
        if (!strcmp(event->data->name, "responses")) {
            handle_responses_command(client, event);
        }
    }
}

int main(void) {
    char* bot_token = getenv("PARROT_BOT_TOKEN");

    struct discord* client = discord_init(bot_token);

    discord_set_on_ready(client, &on_ready);
    discord_set_on_interaction_create(client, &on_interaction);

    return discord_run(client);
}
