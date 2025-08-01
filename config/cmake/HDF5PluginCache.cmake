# CMake cache file for external HDF5 filter plugins when building the plugins during a HDF5 build

#########################
# EXTERNAL cache entries
#########################

# examples are the tests for plugins
set (H5PL_BUILD_TESTING ON CACHE BOOL "Enable H5PL testing" FORCE)
set (H5PL_BUILD_EXAMPLES ${HDF5_BUILD_EXAMPLES} CACHE BOOL "Build H5PL Examples" FORCE)

#preset HDF5 cache vars to this projects libraries instead of searching
set (H5PL_HDF5_HEADER "H5pubconf.h" CACHE STRING "Name of HDF5 header" FORCE)
set (H5PL_HDF5_LINK_LIBS ${HDF5_LIBSH_TARGET} CACHE STRING "HDF5 target" FORCE)
#set (H5PL_HDF5_INCLUDE_DIRS $<TARGET_PROPERTY:${HDF5_LIBSH_TARGET},INCLUDE_DIRECTORIES> CACHE PATH "HDF5 include dirs" FORCE)
set (H5PL_HDF5_INCLUDE_DIRS "${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR}" CACHE PATH "HDF5 include dirs" FORCE)
set (H5PL_HDF5_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE STRING "HDF5 build folder" FORCE)

set (H5PL_HDF5_DUMP_EXECUTABLE $<TARGET_FILE:h5dump> CACHE STRING "HDF5 h5dump target" FORCE)
set (H5PL_HDF5_REPACK_EXECUTABLE $<TARGET_FILE:h5repack> CACHE STRING "HDF5 h5repack target" FORCE)

if (NOT DEFINED H5PL_ALLOW_EXTERNAL_SUPPORT)
  set (H5PL_ALLOW_EXTERNAL_SUPPORT "${HDF5_ALLOW_EXTERNAL_SUPPORT}" CACHE STRING "Allow External Library Building (NO GIT TGZ)" FORCE)
endif ()

if (H5PL_ALLOW_EXTERNAL_SUPPORT MATCHES "GIT")
  set (ENABLE_BITGROOM OFF CACHE BOOL "" FORCE)
  set (ENABLE_JPEG OFF CACHE BOOL "" FORCE)
  set (ENABLE_LZF OFF CACHE BOOL "" FORCE)
endif ()

if (WIN32 AND (CMAKE_C_COMPILER_ID MATCHES "Intel[Ll][Ll][Vv][Mm]" OR CMAKE_C_COMPILER_ID MATCHES "Intel"))
  set (ENABLE_ZFP OFF CACHE BOOL "" FORCE)
endif ()

if (NOT DEFINED H5PL_TGZPATH)
  set (H5PL_TGZPATH "${TGZPATH}" CACHE PATH "PATH for finding plugin tgz file" FORCE)
endif ()

set (H5PL_GIT_URL "${PLUGIN_GIT_URL}" CACHE STRING "Use plugins from HDF Group repository" FORCE)
set (H5PL_GIT_BRANCH "${PLUGIN_GIT_BRANCH}" CACHE STRING "" FORCE)

set (H5PL_TGZ_NAME "${PLUGIN_TGZ_NAME}" CACHE STRING "Use plugins from compressed file" FORCE)

set (PL_PACKAGE_NAME "${PLUGIN_PACKAGE_NAME}" CACHE STRING "Name of plugins package" FORCE)
set (H5PL_CPACK_ENABLE OFF CACHE BOOL "Enable CPack include and components" FORCE)

set (H5PL_USE_GNU_DIRS ${HDF5_USE_GNU_DIRS} CACHE BOOL "ON to use GNU Coding Standard install directory variables, OFF to use historical settings" FORCE)
