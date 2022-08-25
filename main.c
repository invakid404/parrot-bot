#include <concord/discord.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"

struct database* g_database;
struct discord* g_client;

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

    struct discord_application_command application_commands[] = {
        {.name = "responses",
         .description = "View and manage responses",
         .type = DISCORD_APPLICATION_CHAT_INPUT,
         .options = &(struct discord_application_command_options){
             .size = sizeof(options) / sizeof(options[0]),
             .array = options,
         }}};

    int application_commands_amount =
        sizeof(application_commands) / sizeof(application_commands[0]);

    struct discord_application_commands existing_commands = {0};

    struct discord_ret_application_commands existing_commands_response = {
        .sync = &existing_commands,
    };

    discord_get_global_application_commands(client, application_id,
                                            &existing_commands_response);

    if (existing_commands.size != application_commands_amount) {
        struct discord_application_commands commands = {
            .size = application_commands_amount,
            .array = application_commands,
        };

        discord_bulk_overwrite_global_application_commands(
            client, application_id, &commands, NULL);
    }
}

void handle_add_response_subcommand(
    struct discord* client,
    const struct discord_interaction* event,
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

    char* existing_value;

    char* error = database_read(g_database, key, &existing_value);
    if (error != NULL) {
        fprintf(stderr, "failed to read key from database: %s\n", error);

        exit(1);
    }

    if (existing_value != NULL) {
        free(existing_value);

        char message[DISCORD_MAX_MESSAGE_LEN];
        snprintf(message, sizeof(message),
                 "Response with key `%s` already exists!", key);

        struct discord_interaction_response response = {
            .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
            .data = &(struct discord_interaction_callback_data){
                .content = message,
                .flags = DISCORD_MESSAGE_EPHEMERAL,
            }};

        discord_create_interaction_response(client, event->id, event->token,
                                            &response, NULL);

        return;
    }

    error = database_write(g_database, key, value);
    if (error != NULL) {
        fprintf(stderr, "failed to write response to database: %s\n", error);

        exit(1);
    }

    char message[DISCORD_MAX_MESSAGE_LEN];
    snprintf(message, sizeof(message), "Successfully added key `%s`!", key);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){
            .content = message,
            .flags = DISCORD_MESSAGE_EPHEMERAL,
        }};

    discord_create_interaction_response(client, event->id, event->token,
                                        &response, NULL);
}

void handle_get_response_subcommand(
    struct discord* client,
    const struct discord_interaction* event,
    struct discord_application_command_interaction_data_options* options) {
    // TODO: respond
}

void handle_responses_command(struct discord* client,
                              const struct discord_interaction* event) {
    for (int i = 0; i < event->data->options->size; ++i) {
        struct discord_application_command_interaction_data_option option =
            event->data->options->array[i];

        if (!strcmp(option.name, "add")) {
            handle_add_response_subcommand(client, event, option.options);

            return;
        }

        if (!strcmp(option.name, "get")) {
            handle_get_response_subcommand(client, event, option.options);

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

void on_message(struct discord* client, const struct discord_message* event) {
    if (event->author->bot) {
        return;
    }

    char* response_value;

    char* error = database_read(g_database, event->content, &response_value);
    if (error != NULL) {
        fprintf(stderr, "failed to read response from database: %s\n", error);

        exit(1);
    }

    if (response_value == NULL) {
        return;
    }

    struct discord_create_message message = {
        .content = response_value,
    };

    discord_create_message(client, event->channel_id, &message, NULL);

    free(response_value);
}

void signal_handler(void) {
    exit(0);
}

void cleanup(void) {
    database_close(g_database);

    discord_cleanup(g_client);
    ccord_global_cleanup();
}

int main(void) {
    char* bot_token = getenv("PARROT_BOT_TOKEN");
    if (bot_token == NULL) {
        fprintf(stderr, "no bot token provided\n" );

        return 1;
    }

    char* error = database_open(&g_database);
    if (error != NULL) {
        fprintf(stderr, "failed to open database: %s\n", error);

        return 1;
    }

    atexit(cleanup);
    signal(SIGTERM, (__sighandler_t)signal_handler);
    signal(SIGINT, (__sighandler_t)signal_handler);

    g_client = discord_init(bot_token);

    discord_add_intents(g_client, DISCORD_GATEWAY_MESSAGE_CONTENT);

    discord_set_on_ready(g_client, &on_ready);
    discord_set_on_interaction_create(g_client, &on_interaction);
    discord_set_on_message_create(g_client, &on_message);

    return discord_run(g_client);
}
