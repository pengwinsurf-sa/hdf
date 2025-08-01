cmake_minimum_required (VERSION 3.18)
project (HDF5_HL_CPP_SRC CXX)

#-----------------------------------------------------------------------------
# Define Sources
#-----------------------------------------------------------------------------

set (HDF5_HL_CPP_SOURCES ${HDF5_HL_CPP_SRC_SOURCE_DIR}/H5PacketTable.cpp)
set (HDF5_HL_CPP_HDRS ${HDF5_HL_CPP_SRC_SOURCE_DIR}/H5PacketTable.h)

if (BUILD_STATIC_LIBS)
  add_library (${HDF5_HL_CPP_LIB_TARGET} STATIC ${HDF5_HL_CPP_SOURCES} ${HDF5_HL_CPP_HDRS})
  target_include_directories (${HDF5_HL_CPP_LIB_TARGET}
      PRIVATE "${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
  )
  target_compile_options(${HDF5_HL_CPP_LIB_TARGET} PRIVATE "${HDF5_CMAKE_CXX_FLAGS}")
  TARGET_C_PROPERTIES (${HDF5_HL_CPP_LIB_TARGET} STATIC)
  target_link_libraries (${HDF5_HL_CPP_LIB_TARGET} PUBLIC ${HDF5_HL_LIB_TARGET} ${HDF5_LIB_TARGET})
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_CPP_LIB_TARGET}")
  H5_SET_LIB_OPTIONS (${HDF5_HL_CPP_LIB_TARGET} ${HDF5_HL_CPP_LIB_NAME} STATIC 0)
  set_target_properties (${HDF5_HL_CPP_LIB_TARGET} PROPERTIES FOLDER libraries/hl)
  set (install_targets ${HDF5_HL_CPP_LIB_TARGET})
endif ()

if (BUILD_SHARED_LIBS)
  add_library (${HDF5_HL_CPP_LIBSH_TARGET} SHARED ${HDF5_HL_CPP_SOURCES})
  target_include_directories (${HDF5_HL_CPP_LIBSH_TARGET}
      PRIVATE "${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
  )
  target_compile_options(${HDF5_HL_CPP_LIBSH_TARGET} PRIVATE "${HDF5_CMAKE_CXX_FLAGS}")
  target_compile_definitions(${HDF5_HL_CPP_LIBSH_TARGET} PUBLIC "H5_BUILT_AS_DYNAMIC_LIB")
  TARGET_C_PROPERTIES (${HDF5_HL_CPP_LIBSH_TARGET} SHARED)
  target_link_libraries (${HDF5_HL_CPP_LIBSH_TARGET} PUBLIC ${HDF5_HL_LIBSH_TARGET} ${HDF5_LIBSH_TARGET})
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_CPP_LIBSH_TARGET}")
  H5_SET_LIB_OPTIONS (${HDF5_HL_CPP_LIBSH_TARGET} ${HDF5_HL_CPP_LIB_NAME} SHARED "HL_CXX")
  set_target_properties (${HDF5_HL_CPP_LIBSH_TARGET} PROPERTIES FOLDER libraries/hl)
  set (install_targets ${install_targets} ${HDF5_HL_CPP_LIBSH_TARGET})
endif ()

#-----------------------------------------------------------------------------
# Add Target to clang-format
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_FORMATTERS)
  if (BUILD_STATIC_LIBS)
    clang_format (HDF5_HL_CPP_SRC_FORMAT ${HDF5_HL_CPP_LIB_TARGET})
  else ()
    clang_format (HDF5_HL_CPP_SRC_FORMAT ${HDF5_HL_CPP_LIBSH_TARGET})
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
install (
    FILES
        ${HDF5_HL_CPP_HDRS}
    DESTINATION
        ${HDF5_INSTALL_INCLUDE_DIR}
    COMPONENT
        hlcppheaders
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_HL_CPP_LIBSH_TARGET} ${HDF5_INSTALL_BIN_DIR} hlcpplibraries)
  endif ()
  if (BUILD_STATIC_LIBS)
    INSTALL_TARGET_PDB (${HDF5_HL_CPP_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} hlcpplibraries)
  endif ()

  install (
      TARGETS
          ${install_targets}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT hlcpplibraries
      ARCHIVE DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT hlcpplibraries
      RUNTIME DESTINATION ${HDF5_INSTALL_BIN_DIR} COMPONENT hlcpplibraries
      FRAMEWORK DESTINATION ${HDF5_INSTALL_FWRK_DIR} COMPONENT hlcpplibraries
      INCLUDES DESTINATION include
  )
endif ()

#-----------------------------------------------------------------------------
# Create pkgconfig files
#-----------------------------------------------------------------------------
set (_PKG_CONFIG_PREFIX ${CMAKE_INSTALL_PREFIX})
set (_PKG_CONFIG_EXEC_PREFIX \${prefix})
set (_PKG_CONFIG_LIBDIR \${exec_prefix}/${HDF5_INSTALL_LIB_DIR})
set (_PKG_CONFIG_INCLUDEDIR \${prefix}/${HDF5_INSTALL_INCLUDE_DIR})
set (_PKG_CONFIG_LIBNAME "${HDF5_HL_CPP_LIB_NAME}")
set (_PKG_CONFIG_VERSION "${HDF5_PACKAGE_VERSION}")
set (PKG_CONFIG_LIBNAME "${HDF5_HL_CPP_LIB_NAME}")
if (${HDF_CFG_NAME} MATCHES "Debug" OR ${HDF_CFG_NAME} MATCHES "Developer")
  set (PKG_CONFIG_LIBNAME "${PKG_CONFIG_LIBNAME}${CMAKE_DEBUG_POSTFIX}")
endif ()

set (_PKG_CONFIG_LIBS_PRIVATE)

if (BUILD_STATIC_LIBS)
  set (_PKG_CONFIG_LIBS "${_PKG_CONFIG_LIBS} -l${PKG_CONFIG_LIBNAME}")
endif ()
if (BUILD_SHARED_LIBS)
  set (_PKG_CONFIG_SH_LIBS "${_PKG_CONFIG_SH_LIBS} -l${PKG_CONFIG_LIBNAME}")
endif ()

set (_PKG_CONFIG_REQUIRES "${HDF5_HL_LIB_NAME} = ${HDF5_PACKAGE_VERSION}")
set (_PKG_CONFIG_REQUIRES_PRIVATE "${HDF5_HL_LIB_NAME} = ${HDF5_PACKAGE_VERSION}")

configure_file (
    ${HDF_CONFIG_DIR}/libhdf5.pc.in
    ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_HL_CPP_LIB_CORENAME}.pc
    @ONLY
)
install (
    FILES ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_HL_CPP_LIB_CORENAME}.pc
    DESTINATION ${HDF5_INSTALL_LIB_DIR}/pkgconfig
    COMPONENT hlcpplibraries
)

if (H5_HAVE_PKGCONFIG)
  set (_PKG_CONFIG_COMPILER ${HDF5_H5CC_CXX_COMPILER})
  configure_file (
      ${HDF_CONFIG_DIR}/libh5cc.in
      ${HDF5_BINARY_DIR}/CMakeFiles/h5c++
      @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/h5c++
      DESTINATION ${HDF5_INSTALL_BIN_DIR}
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      COMPONENT cpplibraries
  )
endif ()
