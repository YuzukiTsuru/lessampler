#include "FileExists.h"

#include <sys/stat.h>

#include <fstream>

bool FileExists::exists(const char* name) {
  struct stat buffer;
  return stat(name, &buffer) == 0;
}
