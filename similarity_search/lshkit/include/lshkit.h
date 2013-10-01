/* 
    Copyright (C) 2008 Wei Dong <wdong@princeton.edu>. All Rights Reserved.
  
    This file is part of LSHKIT.
  
    LSHKIT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LSHKIT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LSHKIT.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * \mainpage LSHKIT: A C++ Locality Sensitive Hashing Library
 *
 * \author Wei Dong  wdong [@] cs.princeton.edu 
 * \author 2008-2009
 * \section update_sec UPDATES
 * \include ../NEWS
 *  - Download the CVS snapshot at http://lshkit.sourceforge.net/release/lshkit-snapshot.tar.gz
 * \section download_sec 1. Getting LSH
 *
 *  - Sourceforge projet page: http://sourceforge.net/projects/lshkit/
 *  - Browser source code: http://lshkit.cvs.sourceforge.net/lshkit/
 *  - Download a release: http://sourceforge.net/project/showfiles.php?group_id=253020
 *  - Mailing list: http://sourceforge.net/mailarchive/forum.php?forum_name=lshkit-users
 *  - Sample dataset: http://www.cs.princeton.edu/~wdong/lshkit/audio.tar.gz
 *
 * The sample dataset include the data file audio.data and a benchmark file
 * audio.query.  Audio.data contains 54,387 192 dimensional vectors and the
 * benchmark contains 10,000 queries of 200-NNs.
 *
 * \section build_sec 2. Building LSHKIT
 *
 * NOTE: You don't have to build LSHKIT separatly if you only want to use the
 * library but not the tool programs.  Instead, you can directly merge the LSHKIT
 * source files to your project.  You can avoid installing CMake by doing this.
 * See Section 3.2 for details.
 *
 * LSHKIT requires the following libraries:
 * - Gnu Scientific Library
 * - [OPTIONAL] Boost Library > 1.36, the following packages are used:
 *   - program_options
 *
 * LSHKIT uses the CMake cross platform building system. 
 *
 * \subsection build_windows 2.1 Microsoft Visual C++ Express 2008
 *
 * The method would probably work for other versions of visual C++, but I haven't tested.
 *
 * You'll have to install GSL and CMake first if you haven't.
 * Refer to the following pages:
 *  - GSL: http://www.quantcode.com/modules/smartfaq/faq.php?faqid=33
 *  - CMake: http://www.cmake.org/
 *  
 * After installing everything, you still need the following configurations so
 * that CMake can find GSL:
 *  - Add an environment variable GSL_ROOT_DIR, so that \%GSL_ROOT_DIR\%\\include
 *    contains the GSL header files and \%GSL_ROOT_DIR\%\\lib contains the GSL libraries.
 *  - Copy the file lshkit\FindGSL.cmake to <CMAKE_HOME>\\share\\cmake-xxx\\Modules, where
 *    <CMAKE_HOME> is where you install CMake.
 *
 * If you have Boost installed in your system, you can choose to use your own Boost installation
 * instead of the minimal one comes with LSHKIT.  To do that, add an environment
 * variable BOOST_ROOT, so that \%BOOST_ROOT\%\\include contains the Boost
 * header files and \%BOOST_ROOT\%\\lib contains the Boost libraries.
 *
 * We are now ready to build LSHKIT.  Open the Visual C++ command prompt (by clicking
 * "Start/All Programs/Microsoft Visual C++.../Visual Studio Tools/Visual
 * Studio 2008 Command Prompt") and take the following steps:
 *  -# Create a building directory, which we call BULID_DIR, change to the directory.
 *  -# Run "cmakesetup <LSHKIT_DIR>", where <LSHKIT_DIR> is the LSHKIT root directory.
 *  -# The CMake setup window will show up.  Click the "Configure" button in the bottom of the window.
 *  -# Choose "NMake Makefiles" in the popup window and confirm.
 *  -# CMake will run for a while.
 *  -# After a list of red cached values show up, click the "Configure" button again.  You might
 *    want to change the value of some variables before that, for example, change CMAKE_BUILD_TYPE
 *    from "Debug" to "Release" to enable optimization.
 *  -# The cached variables becomes gray now, click the "OK" button in the bottom.
 *  -# A Makefile has been generated, run "nmake" to make.
 *  -# The making process will last for a while.  After that, the LSHKIT library will be in the "lib"
 *    directory and the executables will be in the "bin" directory.
 *
 *  In Step 4, you can also choose "Visual Studio 9 2008" (or other versions) to create a project
 *  file, you that you can build using the Visual C++ IDE.
 *
 * \subsection build_linux 2.2 Linux
 *
 *  -# Copy the file lshkit\FindGSL.cmake to <CMAKE_HOME>/share/cmake-xxx/Modules, where
 *    <CMAKE_HOME> is where you install CMake, which is usually /usr.
 *  -# make a temporary building directory, change to it.
 *  -# Run "cmake LSHKIT_DIR", where LSHKIT_DIR is the LSHKIT root directory.
 *    You can also use "cmake -DCMAKE_BUILD_TYPE=RELEASE LSHKIT_DIR" or "cmake -DCMAKE_BUILD_TYPE=DEBUG LSHKIT_DIR" to specify the build type.
 *  -# A makefile should have been generated.  Run make.
 *  -# The making process will last for a while.  After that, the LSHKIT library will be in the "lib"
 *    directory and the executables will be in the "bin" directory.
 *
 * \section using_sec 3. Using LSHKIT in Your Project
 *
 *  Although the LSHKIT tool programs depends on boost_program_option to parse command line options,
 *  the main LSHKIT library only depends on the Boost headers.  If you use LSHKIT library in your project,
 *  you don't need to link to any of the boost library (unless you use them in other parts of your project). 
 *
 *  \subsection using_sec_1 3.1 Use LSHKIT as a library
 *  -# Build LSHKIT.
 *  -# Configure your building environment so that "LSHKIT_DIR/include" and the LSHKIT library can be found
 *    by your C++ compiler.  If you don't have Boost installed in your system, you'll also need to add
 *    "LSHKIT/3rd-party/boost" to your compiler's include file search path.
 *  -# Add "#include <lshkit.h>" to your C++ source code and you can use most of the LSHKIT functionalities.
 *  -# Link libgsl and libgslcblas (they may be named differently depending on your system) to your program.
 *
 *  \subsection using_sec_2 3.2 Directly add LSHKIT source to your project
 *  By directly using the LSHKIT source, you can avoid installing CMake.

 *  -# Configure your building environment so that "LSHKIT_DIR/include" and the LSHKIT library can be found
 *    by your C++ compiler.  If you don't have Boost installed in your system, you'll also need to add
 *    "LSHKIT/3rd-party/boost" to your compiler's include file search path.
 *  -# Make sure that GSL is properly installed in your system and your compiler can find the GSL header files.
 *  -# Add all C++ source files in "LSHKIT/src" to your project.
 *  -# Add "#include <lshkit.h>" to your C++ source code and you can use most of the LSHKIT functionalities.
 *  -# Link libgsl and libgslcblas (they may be named differently depending on your system) to your program.
 * 
 * \section usecase_sec 4. Some Specific Use Cases
 *  (See the documentation of source files pointed to.)
 *  - The file format used by the tool programs: matrix.h
 *  - (Semi-)Automatic parameter tuning for Multi-Probe LSH and run benchmarks: mplsh-tune.cpp
 *  - Building a Multi-Probe LSH index: mplsh.h
 *  - Using LSH to construct sketches: sketch.h
 *  - Using LSH to construct random histograms to match sets of features: histogram.h
 *  - The supported LSH classes: lsh.h
 *  - How to build complex LSH classses out of the basic ones: composite.h
 *  - How to add your own LSH classes: concept.h
 *
 * \section demo_sec 5. Demo Program Documentation
 * - scan.cpp
 * - lsh-run.cpp
 * - mplsh-run.cpp
 * - fitdata.cpp
 * - mplsh-predict.cpp
 * - mplsh-tune.cpp
 * - sketch-run.cpp
 *
 * \section license_sec 6. License
 *
 * LSHKIT is distributed under the terms of the GNU General Public License (GPL).
 *
 * I do not intend to use the strict GPL license.  However, the performance modeling
 * part of the package depends on the GPLed Gnu Scientific Library for regression,
 * root finding and numerical integration.  If you want to use LSHKIT in your closed source
 * software and are willing to get rid of the performance modeling part (in principle,
 * you'll still be able to use it for performance tuning so long as you don't
 * distribute it; the tuning results can be hard coded into your non-GPLed
 * software), contact me and maybe we'll be able to work out a way.  
 *                                              
 * \section citing_sec 7. Publications
 * 
 * Wei Dong, Zhe Wang, William Josephson, Moses Charikar, Kai Li. Modeling LSH for Performance Tuning. In Proceedings of the 17th Conference on Information and Knowledge Management. Napa Valley, CA, USA. October 2008.
 *
 * Wei Dong, Zhe Wang, Moses Charikar, Kai Li. Efficiently Matching Sets of Features with Random Histograms. In Proceedings of the 16th ACM International Conference on Multimedia. Vancouver, Canada. October 2008.
 *
 * Wei Dong, Moses Charikar, Kai Li. Asymmetric Distance Estimation with Sketches for Similarity Search in High-Dimensional Spaces. In Proceedings of the 31st Annual International ACM SIGIR Conference on Research & Development on Information Retrieval. Singapore. July 2008.
 *
 */

/**
 * \file lshkit.h
 * \brief The LSHKIT master header file.
 *
 * You only need to include this file to use the most functionalities of LSHKIT.
 */

#ifndef __LSHKIT_WDONG__
#define __LSHKIT_WDONG__

#include <lshkit/common.h>
#include <lshkit/composite.h>
#include <lshkit/lsh.h>
#include <lshkit/lsh-index.h>
#include <lshkit/sketch.h>
#include <lshkit/histogram.h>
#include <lshkit/metric.h>
#include <lshkit/kernel.h>
#include <lshkit/mplsh.h>
#include <lshkit/apost.h>
#include <lshkit/forest.h>
#include <lshkit/topk.h>
#include <lshkit/matrix.h>
#include <lshkit/eval.h>
#include <lshkit/spectral-hash.h>
#include <lshkit/multiprobelsh-fitdata.h>
#include <lshkit/multiprobelsh-tune.h>

#endif

