#include "directory.h"

// std
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>

bool create_dir(const char *dir_name) {
  DIR *dir;
  if ((dir = opendir(dir_name)) == NULL) {
    if (ENOENT == errno) {
      mkdir(dir_name, 0777);
    } else {
      return false;
    }
  }

  closedir(dir);

  return true;
}
