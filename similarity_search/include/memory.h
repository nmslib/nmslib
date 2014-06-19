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

#ifndef _MEMORY_USAGE_H_
#define _MEMORY_USAGE_H_

namespace similarity {

class MemUsage {
 public:
   MemUsage();

  /** returns the current virtual memory size of
      the process with pid in MB **/
  double get_vmsize();

 private:
  char status_file_[50];
};

}   // namespace similarity

#endif      // _MEMORY_USAGE_H_

