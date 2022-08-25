#include "database.h"
#include <stdlib.h>
#include <string.h>

struct database {
    leveldb_t* leveldb;
    leveldb_options_t* leveldb_options;
};

struct iterator {
    leveldb_iterator_t* leveldb_iterator;
};

__attribute__((always_inline)) char* terminate_leveldb_string(
    char* leveldb_string,
    size_t length) {
    // Here be dragons. Thou art forewarned
    // The value returned by LevelDB is not null-terminated; its capacity is
    // not enough for a terminator in the first place.
    //
    // We're forced to copy it to terminate it as we'd be writing into memory
    // we don't own otherwise.
    if (leveldb_string == NULL) {
        return NULL;
    }

    char* output = malloc(length + 1);

    memcpy(output, leveldb_string, length);
    output[length] = '\0';

    return output;
}

char* database_open(struct database** database) {
    char* error = NULL;

    (*database) = malloc(sizeof(**database));

    (*database)->leveldb_options = leveldb_options_create();
    leveldb_options_set_create_if_missing((*database)->leveldb_options, true);

    (*database)->leveldb = leveldb_open((*database)->leveldb_options,
                                        "parrot-bot.leveldb", &error);

    if (error != NULL) {
        free(*database);

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

    size_t result_length;

    char* result = leveldb_get(database->leveldb, read_options, key,
                               strlen(key), &result_length, &error);

    leveldb_readoptions_destroy(read_options);

    if (error != NULL) {
        return error;
    }

    *output = terminate_leveldb_string(result, result_length);
    leveldb_free(result);

    leveldb_free(error);
    error = NULL;

    return error;
}

struct iterator* database_iterator_create(struct database* database) {
    leveldb_readoptions_t* read_options = leveldb_readoptions_create();

    struct iterator* iterator = malloc(sizeof(*iterator));

    iterator->leveldb_iterator =
        leveldb_create_iterator(database->leveldb, read_options);

    leveldb_readoptions_destroy(read_options);

    return iterator;
}

void database_iterator_seek(struct iterator* iterator, char* key) {
    leveldb_iter_seek(iterator->leveldb_iterator, key, strlen(key));
}

void database_iterator_seek_to_first(struct iterator* iterator) {
    leveldb_iter_seek_to_first(iterator->leveldb_iterator);
}

void database_iterator_key(struct iterator* iterator, char** output) {
    size_t key_length;

    char* key =
        (char*)leveldb_iter_key(iterator->leveldb_iterator, &key_length);

    *output = terminate_leveldb_string(key, key_length);
}

void database_iterator_value(struct iterator* iterator, char** output) {
    size_t value_length;

    char* value =
        (char*)leveldb_iter_value(iterator->leveldb_iterator, &value_length);

    *output = terminate_leveldb_string(value, value_length);
}

bool database_iterator_valid(struct iterator* iterator) {
    return leveldb_iter_valid(iterator->leveldb_iterator);
}

void database_iterator_next(struct iterator* iterator) {
    leveldb_iter_next(iterator->leveldb_iterator);
}

void database_iterator_prev(struct iterator* iterator) {
    leveldb_iter_prev(iterator->leveldb_iterator);
}

void database_iterator_destroy(struct iterator* iterator) {
    leveldb_iter_destroy(iterator->leveldb_iterator);

    free(iterator);
}

void database_free(void* data) {
    leveldb_free(data);
}

void database_close(struct database* database) {
    leveldb_close(database->leveldb);
    leveldb_options_destroy(database->leveldb_options);

    free(database);
}