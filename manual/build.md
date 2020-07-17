# Prerequisites

1. A modern compiler that supports C++11: G++ 4.7, Intel compiler 14, Clang 3.4, or Visual Studio 14 (version 12 can probably be used as well, but the project files need to be downgraded).
2. **64-bit** Linux is recommended, but most of our code builds on **64-bit** Windows and MACOS as well. 
3. Only for Linux/MACOS: CMake (GNU make is also required) 
4. An Intel or AMD processor that supports SSE 4.2 is recommended
5. Extended version of the library requires a development version of the following libraries: Boost, GNU scientific library, and Eigen3.

To install additional prerequisite packages on Ubuntu, type the following

```
sudo apt-get install libboost-all-dev libgsl0-dev libeigen3-dev
```

## Quick Start on Linux/Mac

To compile, go to the directory **similarity_search** and type:  
```
cmake .
make  
```
To build an extended version (need extra library):
```
cmake . -DWITH_EXTRAS=1
make  
```

## Additional CMAKE options

The compiler is chosen by setting two environment variables: ``CXX`` and ``CC``. In the case of GNU
C++ (version 8), you may need to type:
```
export CCX=g++-8 CC=gcc-8
```

To create makefiles for a release version of the code, type:
```
cmake -DCMAKE_BUILD_TYPE=Release .
```

If you did not create any makeles before, you can shortcut by typing:
```
cmake .
```

To create makefiles for a debug version of the code, type:
```
cmake -DCMAKE_BUILD_TYPE=Debug .
```

When makefiles are created, just type:

```make```

**Important note**: a shortcut command:
``cmake .``
(re)-creates makefiles for the previously created build. When you type ``cmake .``
for the first time, it creates release makefiles. However, if you create debug 
makefiles and then type ``cmake .``, this will not lead to creation of release makefiles!
To prevent this, you need to to delete the cmake cache and makefiles, before
running cmake. For example, you can do the following (assuming the
current directory is similarity search):

```
rm -rf `find . -name CMakeFiles CMakeCache.txt`
```

Also note that, for some reason, cmake might sometimes ignore environmental
variables ``CXX`` and ``CC``. In this unlikely case, you can specify the compiler directly
through cmake arguments. For example, in the case of the GNU C++ and the
release build, this can be done as follows:

```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-8 \
-DCMAKE_GCC_COMPILER=gcc-8 CMAKE_CC_COMPILER=gcc-8 .
```

Finally, if cmake cannot find the Boost libraries, their location can be specified
manually as follows:

```
export BOOST_ROOT=$HOME/boost_download_dir
```

## Installing and building via vcpkg
You can download and install nmslib using the [vcpkg](https://github.com/Microsoft/vcpkg) dependency manager on both Windows and Mac/Linux.
On an Unix-like system do the following:
```
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg install nmslib
```

The nmslib port in vcpkg is kept up to date by Microsoft team members and community contributors. If the version is out of date, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.

# Building on Windows
## Building from sources

Building on Windows requires [Visual Studio 2015 Express for Desktop](https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx). Although it is generally possible and [AppVeyor (see config file for details)](/.appveyor.yml) does create binary Windows versions of NMSLIB, it is often difficult due to subtle differences in the building environment. An easier solution is to build using **vcpkg** (see below). 

AppVeyor builds only Python bindings. To create other binaries, one needs [CMake for Windows](https://cmake.org/download/). First, generate Visual Studio solution file for 64 bit architecture using CMake **GUI**. You have to specify both the platform and the version of Visual Studio. Then, the generated solution can be built using Visual Studio (but, again, it is a bit tricky). 
It is easier to use vcpkg:
```
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install nmslib
```

# Testing the Correctness of Implementations

We have two main testing utilities ``bunit`` and ``test_integr`` (``experiment.exe`` and
``test_integr.exe`` on Windows).
Both utilities accept the single optional argument: the name of the log file.
If the log file is not specified, a lot of informational messages are printed to the screen.

The ``bunit`` verifies some basic functitionality akin to unit testing.
In particular, it checks that an optimized version of the, e.g., Eucledian, distance
returns results that are very similar to the results returned by unoptimized and simpler version.
The utility ``bunit`` is expected to always run without errors.

The utility ``test_integr`` runs complete implementations of many methods
and checks if several effectiveness and efficiency characteristics
meet the expectations.
The expectations are encoded as an array of instances of the class ``MethodTestCase``
(see [the code here](/similarity_search/test/test_integr.cc#L65)).
For example, we expect that the recall falls in a certain pre-recorded range.

Because almost all our methods are randomized, there is a great deal of variance
in the observed performance characteristics. Thus, some tests
may fail infrequently, if e.g., the actual recall value is slightly lower or higher 
than an expected minimum  or maximum.
From an error message, it should be clear if the discrepancy is substantial, i.e.,
something went wrong, or not, i.e., we observe an unlikely outcome due to randomization.



