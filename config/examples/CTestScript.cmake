#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the LICENSE file, which can be found at the root of the source code
# distribution tree, or in https://www.hdfgroup.org/licenses.
# If you do not have access to either file, you may request a copy from
# help@hdfgroup.org.
#
cmake_minimum_required (VERSION 3.18)
########################################################
# For any comments please contact help@hdfgroup.org
#
########################################################
# -----------------------------------------------------------
# -- Get environment
# -----------------------------------------------------------
if (NOT SITE_OS_NAME)
  ## machine name not provided - attempt to discover with uname
  ## -- set hostname
  ## --------------------------
  find_program (HOSTNAME_CMD NAMES hostname)
  execute_process (COMMAND ${HOSTNAME_CMD} OUTPUT_VARIABLE HOSTNAME OUTPUT_STRIP_TRAILING_WHITESPACE)
  set (CTEST_SITE  "${HOSTNAME}${CTEST_SITE_EXT}")
  find_program (UNAME NAMES uname)
  macro (getuname name flag)
    execute_process (COMMAND "${UNAME}" "${flag}" OUTPUT_VARIABLE "${name}" OUTPUT_STRIP_TRAILING_WHITESPACE)
  endmacro ()

  getuname (osname -s)
  string(STRIP ${osname} osname)
  getuname (osrel  -r)
  string(STRIP ${osrel} osrel)
  getuname (cpu    -m)
  string(STRIP ${cpu} cpu)
  message (STATUS "Dashboard script uname output: ${osname}-${osrel}-${cpu}\n")

  set (CTEST_BUILD_NAME  "${osname}-${osrel}-${cpu}")
else ()
  ## machine name provided
  ## --------------------------
  if (CMAKE_HOST_UNIX)
    set (CTEST_BUILD_NAME "${SITE_OS_NAME}-${SITE_OS_VERSION}-${SITE_OS_BITS}-${SITE_COMPILER_NAME}-${SITE_COMPILER_VERSION}")
  else ()
    set (CTEST_BUILD_NAME "${SITE_OS_NAME}-${SITE_OS_VERSION}-${SITE_COMPILER_NAME}")
  endif ()
endif ()
if (SITE_BUILDNAME_SUFFIX)
  set (CTEST_BUILD_NAME  "${SITE_BUILDNAME_SUFFIX}-${CTEST_BUILD_NAME}")
endif ()
set (BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DSITE:STRING=${CTEST_SITE} -DBUILDNAME:STRING=${CTEST_BUILD_NAME}")

# Launchers work only with Makefile and Ninja generators.
if (NOT "${CTEST_CMAKE_GENERATOR}" MATCHES "Make|Ninja" OR LOCAL_SKIP_TEST)
  set (CTEST_USE_LAUNCHERS 0)
  set (ENV{CTEST_USE_LAUNCHERS_DEFAULT} 0)
  set (BUILD_OPTIONS "${BUILD_OPTIONS} -DCTEST_USE_LAUNCHERS:BOOL=OFF")
else ()
  set (CTEST_USE_LAUNCHERS 1)
  set (ENV{CTEST_USE_LAUNCHERS_DEFAULT} 1)
  set (BUILD_OPTIONS "${BUILD_OPTIONS} -DCTEST_USE_LAUNCHERS:BOOL=ON")
endif ()

#-----------------------------------------------------------------------------
# MacOS machines need special options
#-----------------------------------------------------------------------------
if (APPLE)
  # Compiler choice
  execute_process (COMMAND xcrun --find cc OUTPUT_VARIABLE XCODE_CC OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process (COMMAND xcrun --find c++ OUTPUT_VARIABLE XCODE_CXX OUTPUT_STRIP_TRAILING_WHITESPACE)
  set (ENV{CC} "${XCODE_CC}")
  set (ENV{CXX} "${XCODE_CXX}")

  set (BUILD_OPTIONS "${BUILD_OPTIONS} -DCTEST_USE_LAUNCHERS:BOOL=ON")
endif ()

#-----------------------------------------------------------------------------
set (NEED_REPOSITORY_CHECKOUT 0)
set (CTEST_CMAKE_COMMAND "\"${CMAKE_COMMAND}\"")
if (CTEST_USE_TAR_SOURCE)
  ## --------------------------
  if (WIN32 AND NOT MINGW)
    message (STATUS "extracting... [${CMAKE_EXECUTABLE_NAME} -E tar -xvf ${CTEST_DASHBOARD_ROOT}\\${CTEST_USE_TAR_SOURCE}.zip]")
    execute_process (COMMAND ${CMAKE_EXECUTABLE_NAME} -E tar -xvf ${CTEST_DASHBOARD_ROOT}\\${CTEST_USE_TAR_SOURCE}.zip RESULT_VARIABLE rv)
  else ()
    message (STATUS "extracting... [${CMAKE_EXECUTABLE_NAME} -E tar -xvf ${CTEST_DASHBOARD_ROOT}/${CTEST_USE_TAR_SOURCE}.tar]")
    execute_process (COMMAND ${CMAKE_EXECUTABLE_NAME} -E tar -xvf ${CTEST_DASHBOARD_ROOT}/${CTEST_USE_TAR_SOURCE}.tar RESULT_VARIABLE rv)
  endif ()

  if (NOT rv EQUAL 0)
    message (STATUS "extracting... [error-(${rv}) clean up]")
    file (REMOVE_RECURSE "${CTEST_SOURCE_DIRECTORY}")
    message (FATAL_ERROR "error: extract of ${CTEST_USE_TAR_SOURCE} failed")
  endif ()
endif ()

#-----------------------------------------------------------------------------
## Clear the build directory
## --------------------------
set (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)
if (NOT EXISTS "${CTEST_BINARY_DIRECTORY}")
  file (MAKE_DIRECTORY "${CTEST_BINARY_DIRECTORY}")
else ()
  ctest_empty_binary_directory (${CTEST_BINARY_DIRECTORY})
endif ()

# Use multiple CPU cores to build
include (ProcessorCount)
ProcessorCount (N)
if (NOT N EQUAL 0)
  if (MAX_PROC_COUNT)
    if (N GREATER MAX_PROC_COUNT)
      set (N ${MAX_PROC_COUNT})
    endif ()
  endif ()
  if (NOT WIN32)
    set (CTEST_BUILD_FLAGS -j${N})
  endif ()
  set (ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${N})
endif ()

#-----------------------------------------------------------------------------
# Initialize the CTEST commands
#------------------------------
if (CMAKE_GENERATOR_TOOLSET)
  set (CTEST_CONFIGURE_TOOLSET  "\"-T${CMAKE_GENERATOR_TOOLSET}\"")
else ()
  set (CTEST_CONFIGURE_TOOLSET)
endif()
if (CMAKE_GENERATOR_ARCHITECTURE)
  set (CTEST_CONFIGURE_ARCHITECTURE  "\"-A${CMAKE_GENERATOR_ARCHITECTURE}\"")
else ()
  set (CTEST_CONFIGURE_ARCHITECTURE)
endif()
set (CTEST_CONFIGURE_COMMAND
    "${CTEST_CMAKE_COMMAND} -C \"${CTEST_SOURCE_DIRECTORY}/config/cmake/cacheinit.cmake\" -DCMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE} ${BUILD_OPTIONS} \"-G${CTEST_CMAKE_GENERATOR}\" \"${CTEST_CONFIGURE_ARCHITECTURE}\" \"${CTEST_CONFIGURE_TOOLSET}\" \"${CTEST_SOURCE_DIRECTORY}\""
)
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
## -- set output to english
set (ENV{LC_MESSAGES} "en_EN")

#-----------------------------------------------------------------------------
  configure_file(${CTEST_SOURCE_DIRECTORY}/config/CTestCustom.cmake ${CTEST_BINARY_DIRECTORY}/CTestCustom.cmake)
  ctest_read_custom_files ("${CTEST_BINARY_DIRECTORY}")
#-----------------------------------------------------------------------------
  ## NORMAL process
  ## -- LOCAL_SUBMIT reports to CDash server
  ## --------------------------
  ctest_start (Experimental)
  ctest_configure (BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE res)
  if (LOCAL_SUBMIT)
    ctest_submit (PARTS Configure Notes)
  endif ()
  if (${res} LESS 0 OR ${res} GREATER 0)
    file (APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed Configure: ${res}\n")
  endif ()

  ctest_build (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND RETURN_VALUE res NUMBER_ERRORS errval)
  if (LOCAL_SUBMIT)
    ctest_submit (PARTS Build)
  endif ()
  if (${res} LESS 0 OR ${res} GREATER 0 OR ${errval} GREATER 0)
    file (APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed ${errval} Build: ${res}\n")
  endif ()

  ctest_test (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND ${ctest_test_args} RETURN_VALUE res)
  if (LOCAL_SUBMIT)
    ctest_submit (PARTS Test)
  endif()
  if (${res} LESS 0 OR ${res} GREATER 0)
    file (APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed Tests: ${res}\n")
  endif ()
  if (${res} LESS 0 OR ${res} GREATER 0)
    message (FATAL_ERROR "tests FAILED")
  endif ()
#-----------------------------------------------------------------------------
