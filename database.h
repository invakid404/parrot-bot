#ifndef PARROT_BOT_DATABASE_H
#define PARROT_BOT_DATABASE_H

#include <leveldb/c.h>

struct database {
    leveldb_t* leveldb;
};

char* database_open(struct database* database);
char* database_write(struct database* database, char* key, char* value);
char* database_read(struct database* database,
                    char* key,
                    char** output,
                    size_t* output_length);
void database_free(void* data);
void database_close(struct database* database);

#endif  // PARROT_BOT_DATABASE_H
