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

  file (RENAME ${CTEST_DASHBOARD_ROOT}/${CTEST_USE_TAR_SOURCE} ${CTEST_SOURCE_DIRECTORY})
  set (LOCAL_SKIP_UPDATE "TRUE")
else ()
  if (LOCAL_UPDATE)
    if (CTEST_USE_GIT_SOURCE)
      find_program (CTEST_GIT_COMMAND NAMES git git.cmd)
      set (CTEST_GIT_UPDATE_OPTIONS)

      if (NOT EXISTS "${CTEST_SOURCE_DIRECTORY}")
        set (NEED_REPOSITORY_CHECKOUT 1)
      endif ()

      if (${NEED_REPOSITORY_CHECKOUT})
        if (REPOSITORY_BRANCH)
          set (CTEST_GIT_options "clone \"${REPOSITORY_URL}\" --branch  \"${REPOSITORY_BRANCH}\" --single-branch \"${CTEST_SOURCE_DIRECTORY}\" --recurse-submodules")
        else ()
          set (CTEST_GIT_options "clone \"${REPOSITORY_URL}\" \"${CTEST_SOURCE_DIRECTORY}\" --recurse-submodules")
        endif ()
        set (CTEST_CHECKOUT_COMMAND "${CTEST_GIT_COMMAND} ${CTEST_GIT_options}")
      else ()
        set (CTEST_GIT_options "pull")
      endif ()
      set (CTEST_UPDATE_COMMAND "${CTEST_GIT_COMMAND}")
    endif ()
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
# Send the main script as a note.
list (APPEND CTEST_NOTES_FILES
    "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}"
    "${CMAKE_CURRENT_LIST_FILE}"
    "${CTEST_SOURCE_DIRECTORY}/config/cmake/cacheinit.cmake"
)
if (EXISTS "${CTEST_SCRIPT_DIRECTORY}/SkipTests.log")
    list(APPEND CTEST_NOTES_FILES
      "${CTEST_SCRIPT_DIRECTORY}/SkipTests.log"
    )
endif ()

#-----------------------------------------------------------------------------
# Check for required variables.
# --------------------------
foreach (req
    CTEST_CMAKE_GENERATOR
    CTEST_SITE
    CTEST_BUILD_NAME
  )
  if (NOT DEFINED ${req})
    message (FATAL_ERROR "The containing script must set ${req}")
  endif ()
endforeach ()

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
if (LOCAL_MEMCHECK_TEST)
  if(LOCAL_USE_VALGRIND)
    set (CTEST_MEMORYCHECK_COMMAND_OPTIONS "-v --tool=memcheck --leak-check=full --track-fds=yes --num-callers=50 --show-reachable=yes --track-origins=yes --malloc-fill=0xff --free-fill=0xfe")
    find_program(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
  endif()
  set (CTEST_CONFIGURE_COMMAND
      "${CTEST_CMAKE_COMMAND} -C \"${CTEST_SOURCE_DIRECTORY}/config/cmake/mccacheinit.cmake\" -DCMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE} ${BUILD_OPTIONS} \"-G${CTEST_CMAKE_GENERATOR}\" ${CTEST_CONFIGURE_ARCHITECTURE} ${CTEST_CONFIGURE_TOOLSET} \"${CTEST_SOURCE_DIRECTORY}\""
  )
else ()
  if (LOCAL_COVERAGE_TEST)
    if(LOCAL_USE_GCOV)
      find_program (CTEST_COVERAGE_COMMAND NAMES gcov)
    endif ()
  endif ()
  set (CTEST_CONFIGURE_COMMAND
      "${CTEST_CMAKE_COMMAND} -C \"${CTEST_SOURCE_DIRECTORY}/config/cmake/cacheinit.cmake\" -DCMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE} ${BUILD_OPTIONS} \"-G${CTEST_CMAKE_GENERATOR}\" ${CTEST_CONFIGURE_ARCHITECTURE} ${CTEST_CONFIGURE_TOOLSET} \"${CTEST_SOURCE_DIRECTORY}\""
  )
endif ()

#-----------------------------------------------------------------------------
## -- set output to english
set (ENV{LC_MESSAGES} "en_EN")

# Print summary information.
foreach (v
    CTEST_SITE
    CTEST_BUILD_NAME
    CTEST_SOURCE_DIRECTORY
    CTEST_BINARY_DIRECTORY
    CTEST_CMAKE_GENERATOR
    CTEST_CONFIGURATION_TYPE
    CTEST_GIT_COMMAND
    CTEST_CHECKOUT_COMMAND
    CTEST_CONFIGURE_COMMAND
    CTEST_SCRIPT_DIRECTORY
    CTEST_USE_LAUNCHERS
  )
  set (vars "${vars}  ${v}=[${${v}}]\n")
endforeach ()
message (STATUS "Dashboard script configuration:\n${vars}\n")

#-----------------------------------------------------------------------------

###################################################################
#########       Following is for submission to CDash   ############
###################################################################
if (NOT DEFINED MODEL)
  set (MODEL "Experimental")
endif ()

set (ENV{CI_SITE_NAME} ${CTEST_SITE})
set (ENV{CI_BUILD_NAME} ${CTEST_BUILD_NAME})
set (ENV{CI_MODEL} ${MODEL})

#-----------------------------------------------------------------------------
  ## NORMAL process
  ## -- LOCAL_UPDATE updates the source folder from svn
  ## -- LOCAL_SUBMIT reports to CDash server
  ## -- LOCAL_SKIP_TEST skips the test process (only builds)
  ## -- LOCAL_MEMCHECK_TEST executes the Valgrind testing
  ## -- LOCAL_COVERAGE_TEST executes code coverage process
  ## --------------------------
  ctest_start (${MODEL} GROUP ${MODEL})
  if (LOCAL_UPDATE)
    ctest_update (SOURCE "${CTEST_SOURCE_DIRECTORY}")
  endif ()
  configure_file (${CTEST_SOURCE_DIRECTORY}/config/CTestCustom.cmake ${CTEST_BINARY_DIRECTORY}/CTestCustom.cmake)
  ctest_read_custom_files ("${CTEST_BINARY_DIRECTORY}")
  ctest_configure (BUILD "${CTEST_BINARY_DIRECTORY}" RETURN_VALUE res)
  if (LOCAL_SUBMIT)
    ctest_submit (PARTS Update Configure Notes)
  endif ()
  if (${res} LESS 0 OR ${res} GREATER 0)
    file (APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed Configure: ${res}\n")
  endif ()

  # On Cray XC40, configuring fails in the Fortran section when using the craype-mic-knl module.
  # When the configure phase is done with the craype-haswell module and the build phase is done
  # with the craype-mic-knl module, configure succeeds and tests pass on the knl compute nodes
  # for Intel, Cray, GCC and Clang compilers.  If the variables aren't set or if not
  # cross compiling, the module switch will not occur.
  if (CMAKE_CROSSCOMPILING AND COMPILENODE_HWCOMPILE_MODULE AND COMPUTENODE_HWCOMPILE_MODULE)
      execute_process (COMMAND module switch ${COMPILENODE_HWCOMPILE_MODULE} ${COMPUTENODE_HWCOMPILE_MODULE})
  endif ()

  ctest_build (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND RETURN_VALUE res NUMBER_ERRORS errval)
  if (LOCAL_SUBMIT)
    ctest_submit (PARTS Build)
  endif ()
  if (${res} LESS 0 OR ${res} GREATER 0 OR ${errval} GREATER 0)
    file (APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed ${errval} Build: ${res}\n")
  endif ()

  if (NOT LOCAL_SKIP_TEST)
    if (NOT LOCAL_MEMCHECK_TEST)
      if (NOT LOCAL_BATCH_TEST)
        ctest_test (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND ${ctest_test_args} RETURN_VALUE res)
      else ()
        file(STRINGS ${CTEST_BINARY_DIRECTORY}/Testing/TAG TAG_CONTENTS REGEX "^2([0-9]+)[-]([0-9]+)$")
        if (LOCAL_BATCH_SCRIPT_COMMAND STREQUAL "raybsub")
          execute_process (COMMAND ${CTEST_BINARY_DIRECTORY}/${LOCAL_BATCH_SCRIPT_COMMAND} ${LOCAL_BATCH_SCRIPT_ARGS} ${CTEST_BINARY_DIRECTORY}/${LOCAL_BATCH_SCRIPT_NAME})
        else ()
          if (LOCAL_BATCH_SCRIPT_COMMAND STREQUAL "qsub")
            execute_process (COMMAND ${CTEST_BINARY_DIRECTORY}/${LOCAL_BATCH_SCRIPT_NAME} ctestS.out)
          else ()
            execute_process (COMMAND ${LOCAL_BATCH_SCRIPT_COMMAND} ${LOCAL_BATCH_SCRIPT_ARGS} ${CTEST_BINARY_DIRECTORY}/${LOCAL_BATCH_SCRIPT_NAME})
          endif()
        endif ()
        message(STATUS "Check for existence of ${CTEST_BINARY_DIRECTORY}/ctestS.done")
        execute_process(COMMAND ls ${CTEST_BINARY_DIRECTORY}/ctestS.done RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
        while(result)
          ctest_sleep(60)
          execute_process(COMMAND ls ${CTEST_BINARY_DIRECTORY}/ctestS.done RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
        endwhile(result)
        message(STATUS "Serial tests completed.")
        if (LOCAL_SUBMIT)
          ctest_submit (PARTS Test)
        endif ()
        if (LOCAL_BATCH_SCRIPT_PARALLEL_NAME)
          unset(result CACHE)
          if (LOCAL_BATCH_SCRIPT_COMMAND STREQUAL "raybsub")
            execute_process (COMMAND ${CTEST_BINARY_DIRECTORY}/${LOCAL_BATCH_SCRIPT_COMMAND} ${LOCAL_BATCH_SCRIPT_ARGS} ${CTEST_BINARY_DIRECTORY}/${LOCAL_BATCH_SCRIPT_PARALLEL_NAME})
          else ()
            if (LOCAL_BATCH_SCRIPT_COMMAND STREQUAL "qsub")
              execute_process (COMMAND ${CTEST_BINARY_DIRECTORY}/${LOCAL_BATCH_SCRIPT_NAME} ctestP.out)
            else ()
              execute_process (COMMAND ${LOCAL_BATCH_SCRIPT_COMMAND} ${LOCAL_BATCH_SCRIPT_ARGS} ${CTEST_BINARY_DIRECTORY}/${LOCAL_BATCH_SCRIPT_PARALLEL_NAME})
            endif ()
          endif ()
          message(STATUS "Check for existence of ${CTEST_BINARY_DIRECTORY}/ctestP.done")
          execute_process(COMMAND ls ${CTEST_BINARY_DIRECTORY}/ctestP.done RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
          while(result)
            ctest_sleep(60)
            execute_process(COMMAND ls ${CTEST_BINARY_DIRECTORY}/ctestP.done RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
          endwhile(result)
          message(STATUS "parallel tests completed.")
        endif()
      endif ()
      if (LOCAL_SUBMIT)
        ctest_submit (PARTS Test)
      endif ()
      if (${res} LESS 0 OR ${res} GREATER 0)
        file (APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed Tests: ${res}\n")
      endif ()
    else ()
      ctest_memcheck (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND ${ctest_test_args})
      if (LOCAL_SUBMIT)
        ctest_submit (PARTS MemCheck)
      endif ()
    endif ()
    if (LOCAL_COVERAGE_TEST)
      ctest_coverage (BUILD "${CTEST_BINARY_DIRECTORY}" APPEND)
      if (LOCAL_SUBMIT)
        ctest_submit (PARTS Coverage)
      endif ()
    endif ()
    if (LOCAL_SUBMIT)
      ctest_submit (PARTS Done)
    endif ()
  endif ()

  if (NOT LOCAL_MEMCHECK_TEST AND NOT LOCAL_NO_PACKAGE AND NOT LOCAL_SKIP_BUILD)
    ##-----------------------------------------------
    ## Package the product
    ##-----------------------------------------------
    execute_process (COMMAND cpack -C ${CTEST_CONFIGURATION_TYPE} -V
      WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
      RESULT_VARIABLE cpackResult
      OUTPUT_VARIABLE cpackLog
      ERROR_VARIABLE cpackLog.err
    )
    file (WRITE ${CTEST_BINARY_DIRECTORY}/cpack.log "${cpackLog.err}" "${cpackLog}")
    if (cpackResult GREATER 0)
      file (APPEND ${CTEST_SCRIPT_DIRECTORY}/FailedCTest.txt "Failed packaging: ${cpackResult}:${cpackLog.err} \n")
    endif ()
  endif ()
#-----------------------------------------------------------------------------
