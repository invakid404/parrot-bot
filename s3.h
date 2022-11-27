#ifndef PARROT_BOT_S3_H
#define PARROT_BOT_S3_H

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
void s3_put_object(struct s3* s3, char* data, char* key, char* mime_type);
void s3_destroy(struct s3* s3);

#endif  // PARROT_BOT_S3_H
