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

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

using std::ostream;
using std::ofstream;
using std::cerr;
using std::stringstream;
using std::runtime_error;
using std::string;

enum LogSeverity {LIB_INFO, LIB_WARNING, LIB_ERROR, LIB_FATAL};
enum LogChoice  {LIB_LOGNONE, LIB_LOGFILE, LIB_LOGSTDERR};

std::string LibGetCurrentTime();

// write log to file
void InitializeLogger(LogChoice choice = LIB_LOGNONE, const char* logfile = NULL);

class Logger {
 public:
  Logger(LogSeverity severity, const std::string& file, int line, const char* function);
  ~Logger();

  static std::ostream& stream() { return *currstrm_ ; }

 private:
  LogSeverity severity_;

  static ofstream logfile_;

  static ostream* currstrm_;

  // If choice != LIB_LOGFILE the second argument is ignored
  friend void InitializeLogger(LogChoice choice, const char* logfile);
};

class RuntimeErrorWrapper {
 public:
  RuntimeErrorWrapper(const std::string& file, int line, const char* function);

  stringstream& stream() { return currstrm_ ; }

 private:
  stringstream currstrm_;
};


#define LOG(severity) \
  Logger(severity, __FILE__, __LINE__, __FUNCTION__).stream()

#define CHECK(condition) \
  if (!(condition)) {\
    LOG(LIB_ERROR) << "Check failed: " << #condition;  \
    throw runtime_error("Check failed: it's either a bug or inconsistent data!"); \
  }

#define CHECK_MSG(condition,message) \
  if (!(condition)) {\
    LOG(LIB_ERROR) << "Check failed: " << #condition;  \
    throw runtime_error("Check failed: " + string(message)); \
  }

// debug only check and log
#ifndef NDEBUG
#define DCHECK(condition) CHECK(condition)
#define DLOG(severity) LOG(severity)
#else
// NDEBUG disables standard C assertions
#define DCHECK(condition) while (0) CHECK(condition)
#define DLOG(severity) while (0) LOG(severity)
#endif

#define PREPARE_RUNTIME_ERR(var) \
  RuntimeErrorWrapper var(__FILE__, __LINE__, __FUNCTION__); var.stream()
#define THROW_RUNTIME_ERR(var) \
  throw runtime_error(var.stream().str())

#endif     // _LOGGING_H_

