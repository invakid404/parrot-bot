#ifndef PARROT_BOT_DOWNLOAD_H
#define PARROT_BOT_DOWNLOAD_H

#include <stdlib.h>

struct download_buffer {
    unsigned char* memory;
    size_t size;
};

struct download_buffer* download_file(char* url);
void download_buffer_destroy(struct download_buffer* download_buffer);

#endif  // PARROT_BOT_DOWNLOAD_H
