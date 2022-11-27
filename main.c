// Because portable code is for pussies.
#define _GNU_SOURCE
#define PCRE2_CODE_UNIT_WIDTH 8
#define STB_DS_IMPLEMENTATION

#include <assert.h>
#include <chan/chan.h>
#include <concord/discord.h>
#include <errno.h>
#include <magic.h>
#include <openssl/sha.h>
#include <pcre2.h>
#include <signal.h>
#include <sqlite3.h>
#include <stb_ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "download.h"
#include "init_sql.h"
#include "vector.h"
#include "s3.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define PAGE_SIZE 10
#define PAGINATION_ID_PREFIX "page:"

#define DISCORD_CUSTOM_ID_LENGTH (100 + 1)

#define SHA256_OUTPUT_LENGTH (SHA256_DIGEST_LENGTH * 2 + 1)

char* g_database_path;
sqlite3* g_database;

struct discord* g_client;

pcre2_code* g_url_regex;

pthread_t g_asset_thread;
chan_t* g_asset_chan;

magic_t g_magic;

struct s3* g_s3;

struct asset {
    char* key;
    char* value;
};

void sha256(unsigned char* data,
            size_t data_length,
            char outputBuffer[SHA256_OUTPUT_LENGTH]) {
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_CTX sha256 = {0};

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, data_length);
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[SHA256_OUTPUT_LENGTH - 1] = 0;
}

struct match {
    PCRE2_SIZE start, end;
};

struct vector* regex_match(pcre2_code* regex,
                           PCRE2_SPTR subject,
                           size_t subject_length) {
#define APPEND_RESULT(i)                                 \
    {                                                    \
        struct match* match = calloc(1, sizeof(*match)); \
        match->start = matches_vector[2 * (i)];          \
        match->end = matches_vector[2 * (i) + 1];        \
        vector_push(result_vector, match);               \
    }

    pcre2_match_data* match_data =
        pcre2_match_data_create_from_pattern(regex, NULL);

    int rc =
        pcre2_match(regex, subject, subject_length, 0, 0, match_data, NULL);

    if (rc < 0) {
        pcre2_match_data_free(match_data);

        return NULL;
    }

    struct vector* result_vector = vector_create();

    PCRE2_SIZE* matches_vector = pcre2_get_ovector_pointer(match_data);

    for (int i = 0; i < rc; i += 2) {
        APPEND_RESULT(i)
    }

    int crlf_is_newline;
    size_t option_bits;
    uint32_t newline;

    (void)pcre2_pattern_info(regex, PCRE2_INFO_ALLOPTIONS, &option_bits);
    int utf8 = (option_bits & PCRE2_UTF) != 0;

    (void)pcre2_pattern_info(regex, PCRE2_INFO_NEWLINE, &newline);
    crlf_is_newline = newline == PCRE2_NEWLINE_ANY ||
                      newline == PCRE2_NEWLINE_CRLF ||
                      newline == PCRE2_NEWLINE_ANYCRLF;

    for (;;) {
        uint32_t options = 0;
        PCRE2_SIZE start_offset = matches_vector[1];

        if (matches_vector[0] == matches_vector[1]) {
            if (matches_vector[0] == subject_length) {
                break;
            }

            options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
        }

        rc = pcre2_match(regex, subject, subject_length, start_offset, options,
                         match_data, NULL);

        if (rc == PCRE2_ERROR_NOMATCH) {
            if (options == 0) {
                break;
            }

            matches_vector[1] = start_offset + 1;
            if (crlf_is_newline && start_offset < subject_length - 1 &&
                subject[start_offset] == '\r' &&
                subject[start_offset + 1] == '\n') {
                matches_vector[1] += 1;
            } else if (utf8) {
                while (matches_vector[1] < subject_length) {
                    if ((subject[matches_vector[1]] & 0xc0) != 0x80) {
                        break;
                    }

                    matches_vector[1] += 1;
                }
            }

            continue;
        }

        if (rc < 0) {
            pcre2_match_data_free(match_data);
            vector_destroy(result_vector);

            return NULL;
        }

        for (int i = 0; i < rc; i += 2) {
            APPEND_RESULT(i)
        }
    }

    pcre2_match_data_free(match_data);

    return result_vector;
}

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

void process_assets(char* key, char* value) {
    char* key_copy = strdup(key);
    char* value_copy = strdup(value);

    struct asset* asset = calloc(1, sizeof(*asset));

    asset->key = key_copy;
    asset->value = value_copy;

    chan_send(g_asset_chan, asset);
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

    const char* insert_query =
        "INSERT INTO `responses` (`key`, `value`) VALUES (?, ?)";

    sqlite3_stmt* insert_statement;
    sqlite3_prepare_v2(g_database, insert_query, -1, &insert_statement, NULL);

    sqlite3_bind_text(insert_statement, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(insert_statement, 2, value, -1, SQLITE_STATIC);

    int result = sqlite3_step(insert_statement);

    sqlite3_finalize(insert_statement);

    if (result == SQLITE_CONSTRAINT) {
        char message[DISCORD_MAX_MESSAGE_LEN];
        snprintf(message, sizeof(message),
                 "Response with key %s already exists!", key);

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

    char message[DISCORD_MAX_MESSAGE_LEN];
    snprintf(message, sizeof(message), "Successfully added key %s!", key);

    struct discord_interaction_response response = {
        .type = DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE,
        .data = &(struct discord_interaction_callback_data){
            .content = message,
            .flags = DISCORD_MESSAGE_EPHEMERAL,
        }};

    discord_create_interaction_response(client, event->id, event->token,
                                        &response, NULL);

    process_assets(key, value);
}

void handle_responses_interaction(struct discord* client,
                                  const struct discord_interaction* event,
                                  enum discord_interaction_callback_types type,
                                  int page) {
    const char* get_page_query =
        "SELECT `key` FROM `responses` LIMIT ? OFFSET ?";

    sqlite3_stmt* get_page_statement;
    sqlite3_prepare_v2(g_database, get_page_query, -1, &get_page_statement,
                       NULL);

    sqlite3_bind_int(get_page_statement, 1, PAGE_SIZE);
    sqlite3_bind_int(get_page_statement, 2, page * PAGE_SIZE);

    char embed_description[DISCORD_EMBED_DESCRIPTION_LEN] = {0};

    for (int index = 0; sqlite3_step(get_page_statement) == SQLITE_ROW;
         ++index) {
        if (index > 0) {
            strcat(embed_description, "\n");
        }

        if (index % 2 == 0) {
            strcat(embed_description, "**");
        }

        strcat(embed_description,
               (char*)sqlite3_column_text(get_page_statement, 0));

        if (index % 2 == 0) {
            strcat(embed_description, "**");
        }
    }

    sqlite3_finalize(get_page_statement);

    if (strlen(embed_description) <= 0) {
        handle_responses_interaction(client, event, type, page - 1);

        return;
    }

    struct discord_embed embeds[] = {{
        .title = "Responses",
        .description = embed_description,
    }};

    char prev_id[DISCORD_CUSTOM_ID_LENGTH], next_id[DISCORD_CUSTOM_ID_LENGTH];

    sprintf(prev_id, PAGINATION_ID_PREFIX "%d", MAX(page - 1, 0));
    sprintf(next_id, PAGINATION_ID_PREFIX "%d", page + 1);

    struct discord_component buttons[] = {{.type = DISCORD_COMPONENT_BUTTON,
                                           .label = "<",
                                           .style = DISCORD_BUTTON_PRIMARY,
                                           .custom_id = prev_id},
                                          {
                                              .type = DISCORD_COMPONENT_BUTTON,
                                              .label = ">",
                                              .style = DISCORD_BUTTON_PRIMARY,
                                              .custom_id = next_id,
                                          }};

    struct discord_component components[] = {
        {.type = DISCORD_COMPONENT_ACTION_ROW,
         .components = &(struct discord_components){
             .size = sizeof(buttons) / sizeof(buttons[0]),
             .array = buttons,
         }}};

    struct discord_interaction_callback_data callback_data = {
        .embeds =
            &(struct discord_embeds){
                .size = sizeof(embeds) / sizeof(embeds[0]),
                .array = embeds,
            },
        .components =
            &(struct discord_components){
                .size = sizeof(components) / sizeof(components[0]),
                .array = components,
            },
        .flags = DISCORD_MESSAGE_EPHEMERAL};

    struct discord_interaction_response response = {
        .type = type,
        .data = &callback_data,
    };

    discord_create_interaction_response(client, event->id, event->token,
                                        &response, NULL);
}

void handle_get_response_subcommand(
    struct discord* client,
    const struct discord_interaction* event,
    struct discord_application_command_interaction_data_options*) {
    handle_responses_interaction(
        client, event, DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE, 0);
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

void handle_pagination(struct discord* client,
                       const struct discord_interaction* event) {
    char* page_str = event->data->custom_id + strlen(PAGINATION_ID_PREFIX);

    handle_responses_interaction(client, event,
                                 DISCORD_INTERACTION_UPDATE_MESSAGE,
                                 (int)strtol(page_str, NULL, 10));
}

void on_interaction(struct discord* client,
                    const struct discord_interaction* event) {
    if (event->type == DISCORD_INTERACTION_APPLICATION_COMMAND) {
        if (!strcmp(event->data->name, "responses")) {
            handle_responses_command(client, event);
        }

        return;
    }

    if (event->type == DISCORD_INTERACTION_MESSAGE_COMPONENT) {
        if (event->data->component_type == DISCORD_COMPONENT_BUTTON &&
            !strncmp(event->data->custom_id, PAGINATION_ID_PREFIX,
                     strlen(PAGINATION_ID_PREFIX))) {
            handle_pagination(client, event);
        }
    }
}

void on_message(struct discord* client, const struct discord_message* event) {
    if (event->author->bot) {
        return;
    }

    const char* select_query =
        "SELECT `value` FROM `responses` WHERE `key` = ?";

    sqlite3_stmt* select_statement;
    sqlite3_prepare_v2(g_database, select_query, -1, &select_statement, NULL);

    sqlite3_bind_text(select_statement, 1, event->content, -1, SQLITE_STATIC);

    int result = sqlite3_step(select_statement);
    if (result == SQLITE_ROW) {
        char* value = (char*)sqlite3_column_text(select_statement, 0);

        struct discord_create_message message = {
            .content = value,
        };

        discord_create_message(client, event->channel_id, &message, NULL);
    }

    sqlite3_finalize(select_statement);
}

void process_matches(struct asset* asset, struct vector* matches) {
    struct {
        char* key;
        char* value;
    }* url_map = NULL;
    sh_new_arena(url_map);

    for (int i = 0; i < matches->size; ++i) {
        struct match* match = matches->data[i];

        size_t length = match->end - match->start;

        char* url = strndup(asset->value + match->start, length);

        char* result = shget(url_map, url);
        if (result != NULL) {
            free(url);

            continue;
        }

        struct download_buffer* file_buffer = download_file(url);

        const char* mime_type =
            magic_buffer(g_magic, file_buffer->memory, file_buffer->size);

        char file_hash[SHA256_OUTPUT_LENGTH];
        sha256(file_buffer->memory, file_buffer->size, file_hash);

        char target_path[PATH_MAX];
        sprintf(target_path, "%.2s/%.2s/%s", file_hash,
                file_hash + 2, file_hash);

        char* target_url = calloc(PATH_MAX, sizeof(*target_url));
        sprintf(target_url, "%s/%s/%s", g_s3->host, g_s3->bucket_name, target_path);

        s3_put_object(g_s3, (char*)file_buffer->memory, file_buffer->size, target_path, (char*)mime_type);

        shput(url_map, url, target_url);

        download_buffer_destroy(file_buffer);
        free(url);
    }

    sqlite3* transaction;
    sqlite3_open(g_database_path, &transaction);

    sqlite3_exec(transaction, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    const char* replace_sql =
        "UPDATE `responses` SET `value` = REPLACE(`value`, ?1, ?2) WHERE `key` "
        "= ?3;";

    for (int i = 0; i < shlen(url_map); ++i) {
        sqlite3_stmt* insert_statement = {0};
        sqlite3_prepare_v2(transaction, replace_sql, -1, &insert_statement,
                           NULL);

        sqlite3_bind_text(insert_statement, 1, url_map[i].key, -1,
                          SQLITE_STATIC);
        sqlite3_bind_text(insert_statement, 2, url_map[i].value, -1,
                          SQLITE_STATIC);
        sqlite3_bind_text(insert_statement, 3, asset->key, -1, SQLITE_STATIC);

        sqlite3_step(insert_statement);

        sqlite3_finalize(insert_statement);

        free(url_map[i].value);
    }

    sqlite3_exec(transaction, "COMMIT;", NULL, NULL, NULL);

    sqlite3_close(transaction);

    shfree(url_map);
}

void* process_assets_thread(void*) {
    for (;;) {
        struct asset* asset;

        int result = chan_recv(g_asset_chan, (void**)&asset);
        if (result < 0) {
            break;
        }

        struct vector* matches = regex_match(
            g_url_regex, (PCRE2_SPTR)asset->value, strlen(asset->value));

        if (matches != NULL) {
            process_matches(asset, matches);

            vector_destroy(matches);
        }

        free(asset->key);
        free(asset->value);

        free(asset);
    }

    return NULL;
}

void signal_handler(void) {
    exit(0);
}

void cleanup(void) {
    if (g_asset_chan) {
        chan_close(g_asset_chan);
        chan_dispose(g_asset_chan);
    }

    if (g_asset_thread) {
        pthread_join(g_asset_thread, NULL);
    }

    if (g_magic) {
        magic_close(g_magic);
    }

    if (g_database) {
        sqlite3_close(g_database);
    }

    if (g_client) {
        discord_cleanup(g_client);
        ccord_global_cleanup();
    }

    if (g_url_regex) {
        pcre2_code_free(g_url_regex);
    }

    if (g_s3) {
        s3_destroy(g_s3);
    }
}

int main(void) {
    char* bot_token = getenv("PARROT_BOT_TOKEN");
    if (bot_token == NULL) {
        fprintf(stderr, "no bot token provided\n");

        return 1;
    }

    g_database_path = getenv("PARROT_BOT_DATABASE_PATH");
    if (g_database_path == NULL) {
        g_database_path = "parrot_bot.db";
    }

    char* s3_host = getenv("PARROT_BOT_S3_HOST");
    if (s3_host == NULL) {
        fprintf(stderr, "no s3 host provided\n");

        return 1;
    }

    char* s3_bucket_name = getenv("PARROT_BOT_S3_BUCKET_NAME");
    if (s3_bucket_name == NULL) {
        fprintf(stderr, "no s3 bucket name provided\n");

        return 1;
    }

    char* s3_public_key = getenv("PARROT_BOT_S3_PUBLIC_KEY");
    if (s3_public_key == NULL) {
        fprintf(stderr, "no s3 public key provided\n");

        return 1;
    }

    char* s3_private_key = getenv("PARROT_BOT_S3_PRIVATE_KEY");
    if (s3_private_key == NULL) {
        fprintf(stderr, "no s3 private key provided\n");

        return 1;
    }

    atexit(cleanup);
    signal(SIGTERM, (__sighandler_t)signal_handler);
    signal(SIGINT, (__sighandler_t)signal_handler);

    PCRE2_SPTR url_pattern = (PCRE2_SPTR) "(?:https?:\\/\\/)\\S+";

    int error_number;
    PCRE2_SIZE error_offset;

    g_url_regex = pcre2_compile(url_pattern, PCRE2_ZERO_TERMINATED, 0,
                                &error_number, &error_offset, NULL);
    assert(g_url_regex != NULL);

    g_asset_chan = chan_init(0);

    g_magic = magic_open(MAGIC_EXTENSION);
    int result = magic_load(g_magic, NULL);
    if (result < 0) {
        fprintf(stderr, "failed to load magic database: %s\n",
                magic_error(g_magic));

        return 1;
    }

    magic_setflags(g_magic, MAGIC_MIME);

    pthread_create(&g_asset_thread, NULL, process_assets_thread, NULL);

    result = sqlite3_open(g_database_path, &g_database);
    if (result != SQLITE_OK) {
        fprintf(stderr, "failed to open database: %s\n",
                sqlite3_errstr(result));

        return 1;
    }

    // Initialize database
    result = sqlite3_exec(g_database, INIT_SQL, NULL, NULL, NULL);
    if (result != SQLITE_OK) {
        fprintf(stderr, "failed to initialize database: %s\n",
                sqlite3_errstr(result));

        return 1;
    }

    g_s3 = s3_create(s3_host, s3_bucket_name, s3_public_key, s3_private_key);

    g_client = discord_init(bot_token);

    discord_add_intents(g_client, DISCORD_GATEWAY_MESSAGE_CONTENT);

    discord_set_on_ready(g_client, &on_ready);
    discord_set_on_interaction_create(g_client, &on_interaction);
    discord_set_on_message_create(g_client, &on_message);

    return discord_run(g_client);
}
