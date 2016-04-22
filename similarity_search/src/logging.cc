/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
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
//  To shutup Microsoft Compiler who complains about localtime being unsafe
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <iostream>
#include <string>

#include "logging.h"

const char* log_severity[] = {"INFO", "WARNING", "ERROR", "FATAL"};

using namespace std;

// allocate the static member
std::ofstream Logger::logfile_;

static struct voidstream : public ostream {
  template <class T> ostream& operator<< (T) { return *this; }
  template <class T> ostream& operator<< (T*) { return *this; }
  template <class T> ostream& operator<< (T&) { return *this; }
  voidstream() : ostream(0) {}
} voidstream_;

std::string LibGetCurrentTime() {
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  char time_string[50];
  strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", timeinfo);
  return std::string(time_string);
}

ostream* Logger::currstrm_ = &cerr;

Logger::Logger(LogSeverity severity, const std::string& _file, int line, const char* function)
    : severity_(severity) {
  std::string file = _file;
  size_t n = file.rfind('/');
  if (n != std::string::npos) {
    file.erase(file.begin(), file.begin() + n + 1);
  }
  stream() << LibGetCurrentTime() << " " << file << ":" << line
           << " (" << function << ") [" << log_severity[severity] << "] ";
}

Logger::~Logger() {
  stream() << '\n';
  if (severity_ == LIB_FATAL) {
    stream().flush();
    if (logfile_.is_open())
      logfile_.close();
    // TODO(@leo/@bileg) do we want to abort here?
    //abort();
    exit(1); 
  }
}

void InitializeLogger(LogChoice choice, const char* logfile) {
  if (choice == LIB_LOGNONE) {
    Logger::currstrm_ = &voidstream_;
  }
  if (choice == LIB_LOGFILE) {
    Logger::logfile_.open(logfile);
    if (!Logger::logfile_) {
      LOG(LIB_FATAL) << "Can't open the logfile: '" << logfile << "'";
    }
    Logger::currstrm_ = &Logger::logfile_;
  }
  if (choice == LIB_LOGSTDERR) {
    Logger::currstrm_ = &cerr;
  }
}


RuntimeErrorWrapper::RuntimeErrorWrapper(const std::string& _file, int line, const char* function) {
  std::string file = _file;
  size_t n = file.rfind('/');
  if (n != std::string::npos) {
    file.erase(file.begin(), file.begin() + n + 1);
  }
  stream() << LibGetCurrentTime() << " " << file << ":" << line
           << " (" << function << ") ";
}
