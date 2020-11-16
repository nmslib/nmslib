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
#ifndef _UTILS_H_
#define _UTILS_H_

#include <vector>
#include <string>

namespace sqfd {

float Normalize(float val, float min_val, float max_val);
float Denormalize(float val, float min_val, float max_val);
bool Startswith(std::string s, std::string prefix);
bool Endswith(std::string s, std::string suffix);
bool IsFileExists(const std::string& path);
bool IsDirectoryExists(const std::string& path);
bool IsImageFile(const std::string& file);
void MakeDirectory(const std::string& path);
std::vector<std::string> GetAllFiles(const std::string& path);
std::vector<std::string> GetImageFiles(const std::string& path);
std::string GetBasename(std::string filename);
std::string GetDirname(std::string filename);

}

#endif    // _UTILS_H_
