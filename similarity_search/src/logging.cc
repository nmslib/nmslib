/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <iostream>
#include <string>

#include "logging.h"

const char* log_severity[] = {"INFO", "WARNING", "ERROR", "FATAL"};

// allocate the static member
std::ofstream Logger::logfile_;

std::ostream& Logger::stream() {
  return logfile_.is_open() ? logfile_ : std::cerr;
}

std::string GetCurrentTime() {
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  char time_string[50];
  strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", timeinfo);
  return std::string(time_string);
}

Logger::Logger(LogSeverity severity, const std::string& _file, int line, const char* function)
    : severity_(severity) {
  std::string file = _file;
  size_t n = file.rfind('/');
  if (n != std::string::npos) {
    file.erase(file.begin(), file.begin() + n + 1);
  }
  stream() << GetCurrentTime() << " " << file << ":" << line
           << " (" << function << ") [" << log_severity[severity] << "] ";
}

Logger::~Logger() {
  stream() << '\n';
  if (severity_ == FATAL) {
    // stream() << std::flush;
    if (logfile_.is_open())
      logfile_.close();
    // TODO(@leo/@bileg) do we want to abort here?
    //abort();
    exit(1); 
  }
}

void InitializeLogger(const char* logfile) {
  Logger::logfile_.open(logfile);
  assert(Logger::logfile_.is_open());
}

