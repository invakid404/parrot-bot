#include "database.h"
#include <stdbool.h>
#include <string.h>

char* database_open(struct database* database) {
    char* error = NULL;

    leveldb_options_t* leveldb_options = leveldb_options_create();
    leveldb_options_set_create_if_missing(leveldb_options, true);

    database->leveldb =
        leveldb_open(leveldb_options, "parrot-bot.leveldb", &error);
    if (error != NULL) {
        return error;
    }

    leveldb_free(error);
    error = NULL;

    return error;
}

char* database_write(struct database* database, char* key, char* value) {
    char* error = NULL;

    leveldb_writeoptions_t* write_options = leveldb_writeoptions_create();

    leveldb_put(database->leveldb, write_options, key, strlen(key), value,
                strlen(value), &error);
    if (error != NULL) {
        return error;
    }

    leveldb_free(error);
    error = NULL;

    return error;
}

char* database_read(struct database* database,
                    char* key,
                    char** output,
                    size_t* output_length) {
    char* error = NULL;

    leveldb_readoptions_t* read_options = leveldb_readoptions_create();

    char* result = leveldb_get(database->leveldb, read_options, key,
                               strlen(key), output_length, &error);
    if (error != NULL) {
        return error;
    }

    *output = result;

    leveldb_free(error);
    error = NULL;

    return error;
}

void database_free(void* data) {
    leveldb_free(data);
}

void database_close(struct database* database) {
    leveldb_close(database->leveldb);
}