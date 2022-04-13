#pragma once

#include <dirent.h>

DIR *opendirat(int dir_fd, char const *dir, int extra_flags, int *pnew_fd);
bool DIRforeach_dir(DIR *dir, void(*callback)(int, struct dirent *, bool *));
bool foreach_dir(const char *path, void(*callback)(int, struct dirent *, bool *));
int read_full(int fd, void *buf, size_t count);
int write_full(int fd, const void *buf, size_t count);