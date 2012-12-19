# Compiling the DAGFrameScheduler #

To build this library all that should be required is:
 * A C++ compiler
 * CMake

This has been tested on a variety of compilers on a variety of platforms including:
 * Ubuntu x64 with GCC 4.3.2
 * Ubuntu x64 with Clang 3.0.6-6
 * Mac OS X 10.6.8 with GCC 4.2.1
 * windows XP with MinGW 4.7.0
 * windows XP with visual studio 10
 * windows Vista with MinGW 4.6.x

If you are familiar with make based builds you can skip the rest of this document. There are no special options or configurations that must be done. You may want to read the 'Build Options' section so you can read about what might be done.

## Build Options ##
During cmake configuration there are several options to change the end results.

### Mezz_AllWarnings ###
When this is enabled (the default configuration) this library attempts to enable all compiler warnings. If you build project files for visual studio the `/W4` argument will be passed to the compiler. All other compilers will be passed `-Wall -Wno-long-long -pedantic`, this should work with GCC derived/compatible compilers (GCC, Clang, mingw, ICC ad others).

There are several visual studio compiler warnings that are disabled in the source code with `#pragma warning( disable : 4XXX )` in the source code. These are all labeled with remarks stating what they prevent.

### Mezz_BuildSharedLib ###
If this is enabled (default is disabled), this library will be compiled into a .dll, .so or similar library for you plaform. When disabled this will create a static library. There is a measured performance decrease of around 5% slowdown in execution time on Ubuntu x64 when using this as shared library. Since this library is intended to be used in performace sensitive innner loops, it is recommended to leave this off unless you must have teh flexibility  that shared libraries provide.

### Mezz_MinimizeThreadsEachFrame ###
By default this library creates and destroys threads each frame. This allows the algorithms employed to be simple and avoid costly synchronizations. When enabled this library tries to use atomic operations to implement this synchronization. Some compilers will throw errors if the underlying hardware does not suppport atomic operations, and other compilers will use OS synchronization primitives.

This could change performance on a per platform and maybe even per workload basis. On platforms where thread creation is inexpensive, the default configuration performs measurably faster. On systems with slower thread creation, OS implemented (rather than CPU instruction) synchronization primitives, or slow atomic operations this could easily be significantly faster in the other direction. On Ubuntu x64 it is 3% to 10% faster to create threads. It is recommended that this be tested and measured in both configurations for maximum performance before deployment. 

### Mezz_DecacheWorkUnits ###
When a thread is looking for work, it simply iterates over a container of potential workunits and executes the first one it finds in the NotStarted status with all its dependencies. Checking its status means that it has to be loaded from RAM into the CPU's cache. If there are many workunits this potentially means saturating the Memory Bus with Workunits that will quickly fall out of cache. This option keeps a handle to the latest point in this container it makes sense to start searching from. This way all searches will take on average some very small amount of checks, which may be proportional to the amount of threads. The cost for doing this is the added complexity of the algorithm and atomic cas operations where none previously existed.

In theory the more workunits the greater the potential gain, tests with small numbers of work units show this to be disadvantageous. More testing, with larger pools of workunits is required. This is controled by the Mezz_DecacheWorkUnits CMake option.

### CMAKE_BUILD_TYPE ###
This is one of the build options intrinsic to CMake. Set it to 'DEBUG' to enable debugging options in IDEs like Code::Blocks or QT Creator. Set it to 'RELEASE' for performance. Depending in the compiler this can make between a 5x and 20x difference in performance, making the one of the most influential performance options, second only to choosing correct algorithms.

## Creating Build Files with CMake ##
The first step is getting the source code. If you have this file you probably have the source code. If not you can get the source code from the [DAGFrameScheduler Github repository][1] as a zip archive. It needs to be unzipped before compilation. If you have git installed you can use `git clone git://github.com/BlackToppStudios/DAGFrameScheduler.git` to download a copy of the source already in a revision control system.

Make a directory for the build. CMake can create the files for the actual build separate from the source files. A sibling directory relative to the source code works well.

You will need a copy of CMake software. This can be downloaded from the [CMake website][2]. If you are using the graphical UI for it, all of the options will be presented when you open it and point it to the source directory with the `CMakeLists.txt`. CMake also has an option to specify where to build the binaries, direct this towards the directory you created earlier.

To get CMake to present you the options you may need to click the 'Configure' button after you point it at the correct directories. CMake will also ask you which compiler to create build scripts for, simply select from the menu when it does. If you change any of the settings be sure to click the 'Configure' button to save your settings in the 'CMakeCache.txt'. The file 'CMakeCache.txt' is saved in the build directory and is used whenever the 'Generate' button is clicked. When the 'Generate' button is clicked CMake will attempt to generate the build scripts or project files that your compiler or IDE can use.

Use the resulting files to compile the library. Doing this with Code::Blocks, visual studio, or QT Creator is trivially easy. Use the 'Open Project' option frome the 'File' menu of any of these programs. For IDEs like Eclipse or Codelite you will want to create 'Unix Makefiles' and use the Makefile project option.

Alternatively, you can build the library from the command line. If you want to use the library and not look at its source in an IDE, you can use make from a command line. When CMake creates 'Unix makefiles' you can use just about any version of make to perform the build. On windows you can use 'mingw32-make.exe' to build 'Unix Makefiles'. The nmake tool shipped with visual studio cannot build Unix makefiles, however when choosing the compiler in CMake there is an option for 'NMake Makefiles'.

## Skipping CMake Entirely ##
You can also add the source code to an existing project. If you already have a project and you wish to tightly integrate this with that there are only a few technical steps.

Be aware of licensing concerns. If you are not comfortable with the GPL v3 as specified in the LICENSE file, you should contact Joe (toppij@blacktoppstudios.com) or John (blackwoodj@blacktoppstudios.com) about your licensing concerns. They will work with you to get a license you can use.

You will need to configure your existing project to provide preprocessor directives for any of the the build options you want to use. The source code checks the if `_MEZZ_STATIC_BUILD_` or `_MEZZ_SHARED_BUILD_` are defined to determine if it is being build statically of dynamically. This mostly matters on windows when compiling a dynamic, because the default is to not export symbols. The other big option that needs to be define in the pre-processor is whether to minimize thread of to create threads each frame, this can be specified by defining `_MEZZ_MINTHREADS_` or `_MEZZ_THREADSEACHFRAME_`.



[1]: https://github.com/BlackToppStudios/DAGFrameScheduler "DAGFrameScheduler Github repository"
[2]: http://www.cmake.org/cmake/resources/software.html "CMake Downloads"

