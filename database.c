#include "database.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

char* database_open(struct database* database) {
    char* error = NULL;

    database->leveldb_options = leveldb_options_create();
    leveldb_options_set_create_if_missing(database->leveldb_options, true);

    database->leveldb =
        leveldb_open(database->leveldb_options, "parrot-bot.leveldb", &error);

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

    leveldb_writeoptions_destroy(write_options);

    if (error != NULL) {
        return error;
    }

    leveldb_free(error);
    error = NULL;

    return error;
}

char* database_read(struct database* database, char* key, char** output) {
    char* error = NULL;

    leveldb_readoptions_t* read_options = leveldb_readoptions_create();

    size_t output_length;

    char* result = leveldb_get(database->leveldb, read_options, key,
                               strlen(key), &output_length, &error);

    leveldb_readoptions_destroy(read_options);

    if (error != NULL || result == NULL) {
        *output = NULL;

        return error;
    }

    // Here be dragons. Thou art forewarned
    // The value returned by LevelDB is not null-terminated; its capacity is
    // not enough for a terminator in the first place.
    //
    // We're forced to copy it to terminate it as we'd be writing into memory
    // we don't own otherwise.
    *output = malloc(output_length + 1);

    memcpy(*output, result, output_length);
    database_free(result);

    (*output)[output_length] = '\0';

    leveldb_free(error);
    error = NULL;

    return error;
}

void database_free(void* data) {
    leveldb_free(data);
}

void database_close(struct database* database) {
    leveldb_close(database->leveldb);
    leveldb_options_destroy(database->leveldb_options);
}