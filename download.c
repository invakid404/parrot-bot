#include "download.h"
#include <curl/curl.h>
#include <memory.h>
#include <stdio.h>

#define USER_AGENT "parrotbot/dev"

static size_t write_memory(void* contents,
                           size_t size,
                           size_t members_amount,
                           void* user_data) {
    size_t real_size = size * members_amount;
    struct download_buffer* download_buffer =
        (struct download_buffer*)user_data;

    unsigned char* ptr =
        realloc(download_buffer->memory, download_buffer->size + real_size + 1);
    if (!ptr) {
        fprintf(stderr, "out of memory while downloading file\n");

        exit(1);
    }

    download_buffer->memory = ptr;
    memcpy(&(download_buffer->memory[download_buffer->size]), contents,
           real_size);
    download_buffer->size += real_size;
    download_buffer->memory[download_buffer->size] = 0;

    return real_size;
}

struct download_buffer* download_file(char* url) {
    CURL* curl_handle = curl_easy_init();
    struct download_buffer* result = calloc(1, sizeof(*result));

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)result);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, USER_AGENT);

    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

        free(result);

        return NULL;
    }

    curl_easy_cleanup(curl_handle);

    return result;
}

void download_buffer_destroy(struct download_buffer* download_buffer) {
    if (download_buffer->memory != NULL) {
        free(download_buffer->memory);
    }

    free(download_buffer);
}