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

#include "memory.h"


#ifdef __linux

#include <sys/types.h>
#include <unistd.h>

#endif

#ifdef _MSC_VER

#include<windows.h>
#include<stdio.h>   
#include<tchar.h>

#endif

namespace similarity {

MemUsage::MemUsage() {
#ifdef __linux
            int pid = getpid());
            snprintf(status_file_, sizeof(status_file_),
                "/proc/%d/status", pid);
#endif
}

double 
MemUsage::get_vmsize() {
#ifdef __linux
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
#endif
#ifdef _MSC_VER
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof(statex);

    GlobalMemoryStatusEx(&statex);
    return statex.ullAvailVirtual / 1024.0 / 1024.0;
#endif
    return -1.0;
}



}
