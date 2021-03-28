#pragma once

#include <stddef.h>
#include <Kernel/Interface/stat.h>

typedef struct {
    int fd;
} DIR;

DIR* opendir(const char *path);
struct dirent* readdir(DIR *dirp);
int closedir(DIR *dirp);
