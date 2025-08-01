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
#-------------------------------------------------------------------------------
macro (H5_SET_LIB_OPTIONS libtarget libname libtype libpackage)
  set (LIB_OUT_NAME "${libname}")
  # SOVERSION passed in ARGN when shared
  if (${libtype} MATCHES "SHARED")
    set (PACKAGE_SOVERSION ${HDF5_${libpackage}_PACKAGE_SOVERSION})
    set (PACKAGE_COMPATIBILITY ${H5_${libpackage}_SOVERS_INTERFACE}.0.0)
    set (PACKAGE_CURRENT ${H5_${libpackage}_SOVERS_INTERFACE}.${H5_${libpackage}_SOVERS_MINOR}.0)
    if (WIN32)
      set (LIBHDF_VERSION ${HDF5_PACKAGE_VERSION_MAJOR})
    else ()
      set (LIBHDF_VERSION ${HDF5_${libpackage}_PACKAGE_SOVERSION_MAJOR})
    endif ()
    set_target_properties (${libtarget} PROPERTIES VERSION ${PACKAGE_SOVERSION})
    if (WIN32)
        set (${LIB_OUT_NAME} "${LIB_OUT_NAME}-${LIBHDF_VERSION}")
    else ()
        set_target_properties (${libtarget} PROPERTIES SOVERSION ${LIBHDF_VERSION})
    endif ()
    if (CMAKE_C_OSX_CURRENT_VERSION_FLAG)
      set_property (TARGET ${libtarget} APPEND PROPERTY
          LINK_FLAGS "${CMAKE_C_OSX_CURRENT_VERSION_FLAG}${PACKAGE_CURRENT} ${CMAKE_C_OSX_COMPATIBILITY_VERSION_FLAG}${PACKAGE_COMPATIBILITY}"
      )
    endif ()
  endif ()
  HDF_SET_LIB_OPTIONS (${libtarget} ${LIB_OUT_NAME} ${libtype})

  #-- Apple Specific install_name for libraries
  if (APPLE)
    cmake_dependent_option (HDF5_BUILD_WITH_INSTALL_NAME "Build with library install_name set to the installation path" OFF APPLE OFF)
    mark_as_advanced (HDF5_BUILD_WITH_INSTALL_NAME)
    if (HDF5_BUILD_WITH_INSTALL_NAME)
      set_target_properties (${libtarget} PROPERTIES
          INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
          BUILD_WITH_INSTALL_RPATH ${HDF5_BUILD_WITH_INSTALL_NAME}
      )
    endif ()
    if (HDF5_BUILD_FRAMEWORKS)
      if (${libtype} MATCHES "SHARED")
        # adapt target to build frameworks instead of dylibs
        set_target_properties (${libtarget} PROPERTIES
            XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
            FRAMEWORK TRUE
            FRAMEWORK_VERSION ${HDF5_PACKAGE_VERSION_MAJOR}
            MACOSX_FRAMEWORK_IDENTIFIER org.hdfgroup.${libtarget}
            MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${HDF5_PACKAGE_VERSION_MAJOR}
            MACOSX_FRAMEWORK_BUNDLE_VERSION ${HDF5_PACKAGE_VERSION_MAJOR})
      endif ()
    endif ()
  endif ()
endmacro ()

# Initialize the list of VFDs to be used for testing and create a test folder for each VFD
macro (H5_SET_VFD_LIST)
  set (VFD_LIST
      sec2
      stdio
      core
      core_paged
      split
      multi
      family
      # Splitter VFD currently can't be tested with the h5_fileaccess()
      # approach due to it trying to lock the same W/O file when two
      # files are created/opened with the same FAPL that has the VFD
      # set on it. When tested with the environment variable and a
      # default FAPL, the VFD appends "_wo" to the filename when the
      # W/O path isn't specified, which works for all the tests.
      #splitter
      # Log VFD currently has file space allocation bugs
      #log
      # Onion VFD not currently tested with VFD tests
      #onion
  )

  if (H5_HAVE_DIRECT)
    list (APPEND VFD_LIST direct)
  endif ()
  if (H5_HAVE_PARALLEL)
    # MPI I/O VFD is currently incompatible with too many tests in the VFD test set
    # list (APPEND VFD_LIST mpio)
  endif ()
  if (H5_HAVE_MIRROR_VFD)
    # Mirror VFD needs network configuration, etc. and isn't easy to set
    # reasonable defaults for that info.
    # list (APPEND VFD_LIST mirror)
  endif ()
  if (H5_HAVE_ROS3_VFD)
    # This would require a custom test suite
    # list (APPEND VFD_LIST ros3)
  endif ()
  if (H5_HAVE_LIBHDFS)
    # This would require a custom test suite
    # list (APPEND VFD_LIST hdfs)
  endif ()
  if (H5_HAVE_SUBFILING_VFD)
    # Subfiling has a few VFD test failures to be resolved
    # list (APPEND VFD_LIST subfiling)
  endif ()
  if (H5_HAVE_WINDOWS)
    list (APPEND VFD_LIST windows)
  endif ()
endmacro ()

# Initialize the list of VFDs to be used for testing and create a test folder for each VFD
macro (H5_CREATE_VFD_DIR)
  foreach (vfdtest ${VFD_LIST})
    file (MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/${vfdtest}")
  endforeach ()
endmacro ()
