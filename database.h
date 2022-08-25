#ifndef PARROT_BOT_DATABASE_H
#define PARROT_BOT_DATABASE_H

#include <leveldb/c.h>
#include <stdbool.h>

struct database;
struct iterator;

char* database_open(struct database** database);
char* database_write(struct database* database, char* key, char* value);
char* database_read(struct database* database, char* key, char** output);
struct iterator* database_iterator_create(struct database* database);
void database_iterator_seek(struct iterator* iterator, char* key);
void database_iterator_seek_to_first(struct iterator* iterator);
void database_iterator_key(struct iterator* iterator, char** output);
void database_iterator_value(struct iterator* iterator, char** output);
bool database_iterator_valid(struct iterator* iterator);
void database_iterator_next(struct iterator* iterator);
void database_iterator_destroy(struct iterator* iterator);
void database_free(void* data);
void database_close(struct database* database);

#endif  // PARROT_BOT_DATABASE_H
