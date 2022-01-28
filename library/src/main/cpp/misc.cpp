#include <dirent.h>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>

DIR *opendirat (int dir_fd, char const *dir, int extra_flags, int *pnew_fd) {
  int open_flags = (O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOCTTY
                    | O_NONBLOCK | extra_flags);
  int new_fd = openat (dir_fd, dir, open_flags);

  if (new_fd < 0)
    return NULL;
  DIR *dirp = fdopendir (new_fd);
  if (dirp)
    *pnew_fd = new_fd;
  else
    {
      int fdopendir_errno = errno;
      close (new_fd);
      errno = fdopendir_errno;
    }
  return dirp;
}

bool fdforeach_dir(const int dir_fd, void(*callback)(int, struct dirent *, bool *)) {
    DIR *dir;
    struct dirent *entry;
    int fd;
    bool continue_read = true;

    if ((dir = fdopendir(dir_fd)) == nullptr)
        return false;

    fd = dirfd(dir);

    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        callback(fd, entry, &continue_read);
        if (!continue_read) break;
    }

    closedir(dir);
    return true;
}

bool foreach_dir(const char *path, void(*callback)(int, struct dirent *, bool *)) {
    DIR *dir;
    struct dirent *entry;
    int fd;
    bool continue_read = true;

    if ((dir = opendir(path)) == nullptr)
        return false;

    fd = dirfd(dir);

    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        callback(fd, entry, &continue_read);
        if (!continue_read) break;
    }

    closedir(dir);
    return true;
}

static ssize_t read_eintr(int fd, void *out, size_t len) {
    ssize_t ret;
    do {
        ret = read(fd, out, len);
    } while (ret < 0 && errno == EINTR);
    return ret;
}

int read_full(int fd, void *out, size_t len) {
    while (len > 0) {
        ssize_t ret = read_eintr(fd, out, len);
        if (ret <= 0) {
            return -1;
        }
        out = (void *) ((uintptr_t) out + ret);
        len -= ret;
    }
    return 0;
}

int write_full(int fd, const void *buf, size_t count) {
    while (count > 0) {
        ssize_t size = write(fd, buf, count < SSIZE_MAX ? count : SSIZE_MAX);
        if (size == -1) {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }

        buf = (const void *) ((uintptr_t) buf + size);
        count -= size;
    }
    return 0;
}