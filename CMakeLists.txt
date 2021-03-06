# © Copyright 2010 - 2013 BlackTopp Studios Inc.
# This file is part of The DAGFrameScheduler.
#
#    The DAGFrameScheduler is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    The DAGFrameScheduler is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with The DAGFrameScheduler.  If not, see <http://www.gnu.org/licenses/>.
#
#   The original authors have included a copy of the license specified above in the
#   'doc' folder. See 'gpl.txt'
#
#   We welcome the use of the DAGFrameScheduler to anyone, including companies who wish to
#   Build professional software and charge for their product.
#
#   However there are some practical restrictions, so if your project involves
#   any of the following you should contact us and we will try to work something
#   out:
#    - DRM or Copy Protection of any kind(except Copyrights)
#    - Software Patents You Do Not Wish to Freely License
#    - Any Kind of Linking to Non-GPL licensed Works
#    - Are Currently In Violation of Another Copyright Holder's GPL License
#    - If You want to change our code and not add a few hundred MB of stuff to
#        your distribution
#
#   These and other limitations could cause serious legal problems if you ignore
#   them, so it is best to simply contact us or the Free Software Foundation, if
#   you have any questions.
#
#   Joseph Toppi - toppij@gmail.com
#   John Blackwood - makoenergy02@gmail.com

cmake_minimum_required (VERSION 2.8)
project (DAGFrameScheduler)

message (STATUS "${PROJECT_NAME} - Begin")

##############################################################################
# Some variables
set(RootProjectSourceDir ${${PROJECT_NAME}_SOURCE_DIR}/)
Message(STATUS "Using Source From: ${ProjectSourceDir}")
set(RootProjectBinaryDir ${${PROJECT_NAME}_BINARY_DIR}/)
Message(STATUS "Putting all major output in: ${ProjectBinaryDir}")

set(RootProjectTestDir ${${PROJECT_NAME}_SOURCE_DIR}/tests)
Message(STATUS "Looking for tests output in: ${RootProjectTestDir}")
set(RootProjectIncludeDir ${${PROJECT_NAME}_SOURCE_DIR}/src/ ${RootProjectTestDir})
Message(STATUS "Include Search path has: ${RootProjectIncludeDir}")

set(LIBRARY_OUTPUT_PATH "${RootProjectBinaryDir}bin/")
message( STATUS "Putting complete binaries in: ${LIBRARY_OUTPUT_PATH}")
set(EXECUTABLE_OUTPUT_PATH "${RootProjectBinaryDir}bin/")
message( STATUS "Putting complete binaries in: ${EXECUTABLE_OUTPUT_PATH}")

Set(LibName DAGFrameScheduler)
Message(STATUS "Creating a library called: ${LibName}")
set(TestsName SchedulerTests)
Message(STATUS "Creating an executable called: ${TestsName}")

##############################################################################
# Options

# Allow the developer to select if Dynamic or Static libraries are built
option(Mezz_BuildSharedLib "Build Shared Library" OFF)
set(LibType STATIC)
if (Mezz_BuildSharedLib)
    set(LibType SHARED)
endif (Mezz_BuildSharedLib)

#How long should the default length on rolling averages and other multiframe periods be.
set(Mezz_FramesToTrack 10 CACHE STRING "How long should frame durations be tracked for")

# Allow the developer to select if New thread should be created each frame or if an atomic barrier should be used to synchronized threads.
option(Mezz_MinimizeThreadsEachFrame "Used atomics to minimize thread creation" OFF)
set(MinimizeThreads THREADSEACHFRAME)
if (Mezz_MinimizeThreadsEachFrame)
    set(MinimizeThreads MINTHREADS)
endif (Mezz_MinimizeThreadsEachFrame)

# Allow the developer to disable mandatory compiler wanrings, in case it breaks stuff
option(Mezz_AllWarnings "Use atomics to minimize thread creation" ON)
if (Mezz_AllWarnings)
    if(MSVC)
    # Force to always compile with W4
        if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
            string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
        endif()
    elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
            # Update if necessary
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
    endif()
endif (Mezz_AllWarnings)

#When looking for work use atomics to advance iterators past complete work units.
option(Mezz_DecacheWorkUnits "Use atomics to minimize how many WorkUnits are likely to be in CPU cache]." ON)
set(DecacheWork NODECACHEWORKUNIT)
if (Mezz_DecacheWorkUnits)
    set(DecacheWork DECACHEWORKUNIT)
endif (Mezz_DecacheWorkUnits)

###############################################################################
# Doxygen options
option (Mezz_Doc "Refresh Doxygen documentation during a build" OFF)

if(Mezz_Doc)
    Message(STATUS "Starting Doxygen Configuration.")
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(MezzanineDocumentation ALL
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doc/DoxyFiles/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/DoxyFiles/
            COMMENT "Generating API documentation with Doxygen" VERBATIM
        )
    else(DOXYGEN_FOUND)
        Message(STATUS "Doxygen not found.")
    endif(DOXYGEN_FOUND)
    Message(STATUS "Finishing Doxygen Configuration.")
endif(Mezz_Doc)

# todo add options
# An option that will turn off all of the threading header and use std::thread via typedefs instead.
# An option that will turn off the mutex class and use std::classes via typedefs instead.
# An option that will turn off the lock_guard class and use std::classes via typedefs instead.

###############################################################################
# Sources
File(GLOB ${PROJECT_NAME}_test "${RootProjectTestDir}/"  "${RootProjectTestDir}/*.h")

message( STATUS "Looking for Tests in ${RootProjectTestDir}")
foreach(CurrentFileName ${${PROJECT_NAME}_test})
    String(REGEX MATCH "/([^/]*)\\.h$" DirtyName "${CurrentFileName}")
    String(LENGTH ${DirtyName} DirtyNameLength)
    MATH(EXPR TargetLength "${DirtyNameLength} - 3")
    String(SUBSTRING "${DirtyName}" "1" "${TargetLength}" TargetName)
    #message (${TargetLength})
    #message (${DirtyName})
    #message("${TargetName}")
    list(APPEND FoundTests ${TargetName})
endforeach(CurrentFileName ${${PROJECT_NAME}_test})

message(STATUS "Found Tests: ")
foreach(CurrentTest ${FoundTests})
    message(STATUS "   ${CurrentTest}")
endforeach(CurrentTest ${FoundTests})

message(STATUS "Generating Test Instantiation and include Code: ")
foreach(CurrentTest ${FoundTests})
    set(CppIncludes "${CppIncludes}
    #include \"${CurrentTest}.h\" ")

    set(CppInstantiations "${CppInstantiations}
    ${CurrentTest} ${CurrentTest}Implementation;")

    set(CppNameRegistration "${CppNameRegistration}
                this->operator[](AllLower(${CurrentTest}Implementation.Name())) = &${CurrentTest}Implementation;")
endforeach(CurrentTest ${FoundTests})

configure_file( "${RootProjectSourceDir}/autodetect.h.in"  "${RootProjectBinaryDir}/autodetect.h")

include_directories(${RootProjectBinaryDir} ${RootProjectSourceDir}/testframework/)


##############################################################################
# A complete list of our source files and headers

# All the headers that the library uses, so they can be pulled out for SDK creation or whatever.
set( ${PROJECT_NAME}_lib_headers
        "${RootProjectSourceDir}src/dagframescheduler.h"    # The header to include in projects using this
        "${RootProjectSourceDir}src/crossplatformincludes.h"

        "${RootProjectSourceDir}src/datatypes.h"            # If integrating tightly into another project re-implementing this will allow for easier binary compatibility.
        "${RootProjectSourceDir}src/crossplatformexport.h"

        "${RootProjectSourceDir}src/asynchronousfileloadingworkunit.h"
        "${RootProjectSourceDir}src/asynchronousworkunit.h"
        "${RootProjectSourceDir}src/atomicoperations.h"
        "${RootProjectSourceDir}src/barrier.h"
        "${RootProjectSourceDir}src/doublebufferedresource.h"
        "${RootProjectSourceDir}src/framescheduler.h"
        "${RootProjectSourceDir}src/frameschedulerworkunits.h"
        "${RootProjectSourceDir}src/lockguard.h"
        "${RootProjectSourceDir}src/logtools.h"
        "${RootProjectSourceDir}src/monopoly.h"
        "${RootProjectSourceDir}src/mutex.h"
        "${RootProjectSourceDir}src/readwritespinlock.h"
        "${RootProjectSourceDir}src/rollingaverage.h"
        "${RootProjectSourceDir}src/spinlock.h"
        "${RootProjectSourceDir}src/systemcalls.h"
        "${RootProjectSourceDir}src/thread.h"
        "${RootProjectSourceDir}src/threadingenumerations.h"
        "${RootProjectSourceDir}src/workunit.h"
        "${RootProjectSourceDir}src/workunitkey.h"
)

# All the source files in the library
set( ${PROJECT_NAME}_lib_sources
        "${RootProjectSourceDir}src/asynchronousfileloadingworkunit.cpp"
        "${RootProjectSourceDir}src/asynchronousworkunit.cpp"
        "${RootProjectSourceDir}src/atomicoperations.cpp"
        "${RootProjectSourceDir}src/barrier.cpp"
        "${RootProjectSourceDir}src/doublebufferedresource.cpp"
        "${RootProjectSourceDir}src/framescheduler.cpp"
        "${RootProjectSourceDir}src/frameschedulerworkunits.cpp"
        "${RootProjectSourceDir}src/logtools.cpp"
        "${RootProjectSourceDir}src/monopoly.cpp"
        "${RootProjectSourceDir}src/mutex.cpp"
        "${RootProjectSourceDir}src/readwritespinlock.cpp"
        "${RootProjectSourceDir}src/spinlock.cpp"
        "${RootProjectSourceDir}src/rollingaverage.cpp"
        "${RootProjectSourceDir}src/systemcalls.cpp"
        "${RootProjectSourceDir}src/thread.cpp"
        "${RootProjectSourceDir}src/workunit.cpp"
        "${RootProjectSourceDir}src/workunitkey.cpp"
)

# Headers required for the Test to run.
set( ${PROJECT_NAME}_exe_headers
        "${RootProjectSourceDir}testframework/pugiconfig.h"
        "${RootProjectSourceDir}testframework/pugixml.h"
        "${RootProjectBinaryDir}/autodetect.h"
        "${${PROJECT_NAME}_test}"

        "${RootProjectSourceDir}testframework/consolelogic.h"
        "${RootProjectSourceDir}testframework/consolestringmanipulation.h"
        "${RootProjectSourceDir}testframework/mezztest.h"
        "${RootProjectSourceDir}testframework/testdata.h"
        "${RootProjectSourceDir}testframework/testdatatools.h"
        "${RootProjectSourceDir}testframework/testenumerations.h"
)

# Source files on the executable/tests need.
set( ${PROJECT_NAME}_exe_sources
        "${RootProjectSourceDir}testframework/pugixml.cpp"

        "${RootProjectSourceDir}testframework/consolelogic.cpp"
        "${RootProjectSourceDir}testframework/consolestringmanipulation.cpp"
        "${RootProjectSourceDir}testframework/mezztest.cpp"
        "${RootProjectSourceDir}testframework/testdata.cpp"
        "${RootProjectSourceDir}testframework/testdatatools.cpp"
        "${RootProjectSourceDir}testframework/testenumerations.cpp"
)

##############################################################################
# Define the libraries and say what they require

# This is from the old c++11 version that was before its time.
#add_definitions("-std=c++0x -lpthread")
find_package(Threads)

# The required library definitions
add_library(${LibName} ${LibType} ${${PROJECT_NAME}_lib_headers} ${${PROJECT_NAME}_lib_sources} )
target_link_libraries(${LibName} ${CMAKE_THREAD_LIBS_INIT})

add_definitions(
    -D_MEZZ_${LibType}_BUILD_                   # Trailing slash indicates that compiler header should massage this before the library consumes
    -D_MEZZ_${MinimizeThreads}_
    -D_MEZZ_${DecacheWork}_
    -D_MEZZ_FRAMESTOTRACK_=${Mezz_FramesToTrack}
)

# A basic executable that will define some tests to prove this works (at least in the small scale)
include_directories( ${RootProjectIncludeDir} )
link_directories( ${LIBRARY_OUTPUT_PATH} )

add_executable(${TestsName} ${${PROJECT_NAME}_exe_headers} ${${PROJECT_NAME}_exe_sources} )
target_link_libraries(${TestsName} ${LibName})

message (STATUS "${PROJECT_NAME} - End")
