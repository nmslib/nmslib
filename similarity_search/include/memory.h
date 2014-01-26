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

#ifndef _MEMORY_USAGE_H_
#define _MEMORY_USAGE_H_

#ifdef __linux

#include <sys/types.h>
#include <unistd.h>

class MemUsage {
 public:
  MemUsage(int pid = getpid()) {
    snprintf(status_file_, sizeof(status_file_),
             "/proc/%d/status", pid);
  }

  ~MemUsage() {}

  /** returns the current virtual memory size of
      the process with pid in MB **/
  double get_vmsize() {
    FILE* f = fopen(status_file_, "rt");
    if (!f) {
      return -1.0;
    }
    char buf[100];
    int vmsize = -1024;
    while (fgets(buf, sizeof(buf), f)) {
      if (strncmp(buf, "VmSize:", 7) == 0) {
        sscanf(buf + 7, "%d", &vmsize);
        break;
      }
    }
    fclose(f);
    return vmsize / 1024.0;
  }

 private:
  char status_file_[50];
};

#else

class MemUsage {
 public:
  double get_vmsize() {
    return -1.0;
  }
};

#endif

#endif      // _MEMORY_USAGE_H_

