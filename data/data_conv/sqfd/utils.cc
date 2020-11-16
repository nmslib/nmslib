/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/nmslib/nmslib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <unordered_set>
#include "utils.h"

namespace sqfd {

const std::unordered_set<std::string> extensions = {"jpg", "jpeg", "png"};

bool Startswith(std::string s, std::string prefix) {
  size_t pos = s.find(prefix);
  return pos != std::string::npos && pos == 0;
}

bool Endswith(std::string s, std::string suffix) {
  size_t pos = s.rfind(suffix);
  return pos != std::string::npos && pos + suffix.size() == s.size();
}

std::string GetFullPath(const std::string& path, const std::string& file) {
  if (path.empty()) {
    return file;
  }
  return path[path.length() - 1] == '/' ? path + file : path + "/" + file;
}

bool IsFileExists(const std::string& path) {
  return access(path.c_str(), F_OK) == 0;
}

bool IsDirectoryExists(const std::string& path) {
  struct stat st;
  int ret = stat(path.c_str(), &st);
  if (ret == -1) {
    return false;
  } else {
    return S_ISDIR(st.st_mode) ? true : false;
  }
}

bool IsImageFile(const std::string& file) {
  size_t pos = file.rfind(".");
  if (pos == std::string::npos) {
    return false;
  }
  std::string ext = file.substr(pos + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  return extensions.count(ext) != 0;
}

void MakeDirectory(const std::string& path) {
  system(("mkdir -p " + path).c_str());
}

std::vector<std::string> GetAllFiles(const std::string& path) {
  struct dirent* dir;
  DIR* d = opendir(path.c_str());
  std::vector<std::string> files;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      std::string filename = dir->d_name;
      files.push_back(GetFullPath(path, filename));
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

std::vector<std::string> GetImageFiles(const std::string& path) {
  struct dirent* dir;
  DIR* d = opendir(path.c_str());
  std::vector<std::string> files;
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      std::string filename = dir->d_name;
      if (IsImageFile(filename)) {
        files.push_back(GetFullPath(path, filename));
      }
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

std::string GetBasename(std::string filename) {
  size_t pos = filename.rfind("/");
  if (pos != std::string::npos) {
    filename = filename.substr(pos + 1);
  }
  /*
  pos = filename.rfind(".");
  if (pos != std::string::npos) {
    filename = filename.substr(0, pos);
  }
  */
  return filename;
}

std::string GetDirname(std::string filename) {
  size_t pos = filename.rfind("/");
  if (pos != std::string::npos) {
    return filename.substr(0, pos + 1);
  }
  return "./";
}

float Normalize(float val, float min_val, float max_val) {
  val = std::max(min_val, val);
  val = std::min(max_val, val);
  assert(min_val <= val && val <= max_val);
  return (val - min_val) / (max_val - min_val);
}

float Denormalize(float val, float min_val, float max_val) {
  assert(min_val < max_val);
  val = val * (max_val - min_val) + min_val;
  assert(min_val <= val && val <= max_val);
  return val;
}

}
