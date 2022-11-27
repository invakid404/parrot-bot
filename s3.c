#include "s3.h"
#include <curl/curl.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct s3* s3_create(char* host,
                     char* bucket_name,
                     char* public_key,
                     char* private_key) {
    struct s3* s3 = malloc(sizeof(*s3));

    s3->host = host;
    s3->bucket_name = bucket_name;
    s3->public_key = public_key;
    s3->private_key = private_key;

    return s3;
}

void s3_put_object(struct s3* s3, char* data, char* key, char* mime_type) {
    char request[2048];

    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    time_t expires = spec.tv_sec + 300;

    sprintf(request,
            "PUT\n"
            "\n"
            "%s\n"
            "%ld\n"
            "/%s/%s",
            mime_type, expires, s3->bucket_name, key);

    unsigned char* hmac = malloc(20);
    unsigned int hmac_len = 0;

    HMAC(EVP_sha1(), s3->private_key, (int)strlen(s3->private_key),
         (unsigned char*)request, strlen(request), hmac, &hmac_len);

    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    bio = BIO_push(b64, bio);
    BIO_write(bio, hmac, (int)hmac_len);
    BIO_flush(bio);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);

    CURL* curl = curl_easy_init();
    char* signature = curl_easy_escape(curl, bptr->data, (int)bptr->length - 1);

    char target_url[2048];
    sprintf(target_url,
            "%s/%s/"
            "%s?AWSAccessKeyId=%s&Expires=%ld&Signature=%s",
            s3->host, s3->bucket_name, key, s3->public_key, expires, signature);

    curl_easy_setopt(curl, CURLOPT_URL, target_url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    struct curl_slist* headers = {0};

    char content_type[2048];
    sprintf(content_type, "Content-Type: %s", mime_type);

    headers = curl_slist_append(headers, content_type);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_perform(curl);

    curl_slist_free_all(headers);

    free(hmac);
    BIO_free_all(bio);
    curl_free(signature);
}

void s3_destroy(struct s3* s3) {
    free(s3);
}
