#ifndef PARROT_BOT_S3_H
#define PARROT_BOT_S3_H

#include <stddef.h>

struct s3 {
    char* host;
    char* bucket_name;
    char* public_key;
    char* private_key;
};

struct s3* s3_create(char* host,
                     char* bucket_name,
                     char* public_key,
                     char* private_key);
void s3_put_object(struct s3* s3,
                   char* data,
                   size_t data_size,
                   char* key,
                   char* mime_type);
void s3_destroy(struct s3* s3);

#endif  // PARROT_BOT_S3_H
