cmake_minimum_required (VERSION 3.18)
project (HDF5_HL_SRC C)

#-----------------------------------------------------------------------------
# List Source files
#-----------------------------------------------------------------------------
set (HL_SOURCES
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DO.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DS.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5IM.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LT.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTanalyze.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTparse.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PT.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5TB.c
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LD.c
)

set (HL_HEADERS
    ${HL_HEADERS}
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DOpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DSpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5IMpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PTpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5TBpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LDpublic.h
    ${HDF5_HL_SRC_SOURCE_DIR}/hdf5_hl.h
)

set (HL_PRIVATE_HEADERS
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTparse.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5DSprivate.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5IMprivate.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PTprivate.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5HLprivate2.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LTprivate.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5PTprivate.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5TBprivate.h
    ${HDF5_HL_SRC_SOURCE_DIR}/H5LDprivate.h
)

if (BUILD_STATIC_LIBS)
  add_library (${HDF5_HL_LIB_TARGET} STATIC ${HL_SOURCES} ${HL_HEADERS} ${HL_PRIVATE_HEADERS})
  target_include_directories (${HDF5_HL_LIB_TARGET}
      PRIVATE "${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
  )
  target_compile_options(${HDF5_HL_LIB_TARGET} PRIVATE "${HDF5_CMAKE_C_FLAGS}")
  TARGET_C_PROPERTIES (${HDF5_HL_LIB_TARGET} STATIC)
  target_link_libraries (${HDF5_HL_LIB_TARGET} PUBLIC ${HDF5_LIB_TARGET})
  H5_SET_LIB_OPTIONS (${HDF5_HL_LIB_TARGET} ${HDF5_HL_LIB_NAME} STATIC 0)
  set_target_properties (${HDF5_HL_LIB_TARGET} PROPERTIES FOLDER libraries/hl)
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_LIB_TARGET}")
  set (install_targets ${HDF5_HL_LIB_TARGET})
endif ()

if (BUILD_SHARED_LIBS)
  add_library (${HDF5_HL_LIBSH_TARGET} SHARED ${HL_SOURCES} ${HL_HEADERS} ${HL_PRIVATE_HEADERS})
  target_include_directories (${HDF5_HL_LIBSH_TARGET}
      PRIVATE "${HDF5_SRC_INCLUDE_DIRS};${HDF5_SRC_BINARY_DIR};$<$<BOOL:${HDF5_ENABLE_PARALLEL}>:${MPI_C_INCLUDE_DIRS}>"
      INTERFACE "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>"
  )
  target_compile_options(${HDF5_HL_LIBSH_TARGET} PRIVATE "${HDF5_CMAKE_C_FLAGS}")
  target_compile_definitions(${HDF5_HL_LIBSH_TARGET} PUBLIC "H5_BUILT_AS_DYNAMIC_LIB")
  TARGET_C_PROPERTIES (${HDF5_HL_LIBSH_TARGET} SHARED)
  target_link_libraries (${HDF5_HL_LIBSH_TARGET} PUBLIC ${HDF5_LIBSH_TARGET})
  H5_SET_LIB_OPTIONS (${HDF5_HL_LIBSH_TARGET} ${HDF5_HL_LIB_NAME} SHARED "HL")
  set_target_properties (${HDF5_HL_LIBSH_TARGET} PROPERTIES FOLDER libraries/hl)
  set_global_variable (HDF5_LIBRARIES_TO_EXPORT "${HDF5_LIBRARIES_TO_EXPORT};${HDF5_HL_LIBSH_TARGET}")
  set (install_targets ${install_targets} ${HDF5_HL_LIBSH_TARGET})
endif ()

#-----------------------------------------------------------------------------
# Add Target to clang-format
#-----------------------------------------------------------------------------
if (HDF5_ENABLE_FORMATTERS)
  if (BUILD_STATIC_LIBS)
    clang_format (HDF5_HL_SRC_FORMAT ${HDF5_HL_LIB_TARGET})
  else ()
    clang_format (HDF5_HL_SRC_FORMAT ${HDF5_HL_LIBSH_TARGET})
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
install (
    FILES
        ${HL_HEADERS}
    DESTINATION
        ${HDF5_INSTALL_INCLUDE_DIR}
    COMPONENT
        hlheaders
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install for import into other projects
#-----------------------------------------------------------------------------
if (HDF5_EXPORTED_TARGETS)
  if (BUILD_SHARED_LIBS)
    INSTALL_TARGET_PDB (${HDF5_HL_LIBSH_TARGET} ${HDF5_INSTALL_BIN_DIR} hllibraries)
  endif ()
  if (BUILD_STATIC_LIBS)
    INSTALL_TARGET_PDB (${HDF5_HL_LIB_TARGET} ${HDF5_INSTALL_LIB_DIR} hllibraries)
  endif ()

  install (
      TARGETS
          ${install_targets}
      EXPORT
          ${HDF5_EXPORTED_TARGETS}
      LIBRARY DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT hllibraries
      ARCHIVE DESTINATION ${HDF5_INSTALL_LIB_DIR} COMPONENT hllibraries
      RUNTIME DESTINATION ${HDF5_INSTALL_BIN_DIR} COMPONENT hllibraries
      FRAMEWORK DESTINATION ${HDF5_INSTALL_FWRK_DIR} COMPONENT hllibraries
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
set (_PKG_CONFIG_LIBNAME "${HDF5_HL_LIB_NAME}")
set (_PKG_CONFIG_VERSION "${HDF5_PACKAGE_VERSION}")
set (PKG_CONFIG_LIBNAME "${HDF5_HL_LIB_NAME}")
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

set (_PKG_CONFIG_REQUIRES "${HDF5_LIB_NAME} = ${HDF5_PACKAGE_VERSION}")
set (_PKG_CONFIG_REQUIRES_PRIVATE "${HDF5_LIB_NAME} = ${HDF5_PACKAGE_VERSION}")

configure_file (
    ${HDF_CONFIG_DIR}/libhdf5.pc.in
    ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_HL_LIB_CORENAME}.pc
    @ONLY
)
install (
    FILES ${HDF5_BINARY_DIR}/CMakeFiles/${HDF5_HL_LIB_CORENAME}.pc
    DESTINATION ${HDF5_INSTALL_LIB_DIR}/pkgconfig
    COMPONENT hllibraries
)

if (H5_HAVE_PKGCONFIG)
  set (_PKG_CONFIG_COMPILER ${HDF5_H5CC_C_COMPILER})
  configure_file (
      ${HDF_CONFIG_DIR}/libh5cc.in
      ${HDF5_BINARY_DIR}/CMakeFiles/h5cc
      @ONLY
  )
  install (
      FILES ${HDF5_BINARY_DIR}/CMakeFiles/h5cc
      DESTINATION ${HDF5_INSTALL_BIN_DIR}
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      COMPONENT libraries
  )
  if (HDF5_ENABLE_PARALLEL AND MPI_C_FOUND)
    #legacy requires a different name
    configure_file (
        ${HDF_CONFIG_DIR}/libh5cc.in
        ${HDF5_BINARY_DIR}/CMakeFiles/h5pcc
        @ONLY
    )
    install (
        FILES ${HDF5_BINARY_DIR}/CMakeFiles/h5pcc
        DESTINATION ${HDF5_INSTALL_BIN_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        COMPONENT libraries
    )
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build documentation
#-----------------------------------------------------------------------------
if (DOXYGEN_FOUND)
# This cmake function requires that the non-default doxyfile settings are provided with set (DOXYGEN_xxx) commands
# In addition the doxyfile aliases @INCLUDE option is not supported and would need to be provided in a set (DOXYGEN_ALIASES) command.
#  doxygen_add_docs (hdf5lib_doc
##      ${HL_SOURCES} ${HL_HEADERS} ${HDF5_DOXYGEN_DIR}/dox
#      ${DOXYGEN_INPUT_DIRECTORY}
#      ALL
#      WORKING_DIRECTORY ${HDF5_HL_SRC_DIR}
#      COMMENT "Generating HDF5 HL library Source Documentation"
#  )

# This custom target and doxygen/configure work together
  # Replace variables inside @@ with the current values
  add_custom_target (hdf5hllib_doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${HDF5_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${HDF5_HL_SRC_DIR}
        COMMENT "Generating HDF5 HL library Source API documentation with Doxygen"
        VERBATIM )

  if (NOT TARGET doxygen)
    add_custom_target (doxygen)
  endif ()

  add_dependencies (doxygen hdf5hllib_doc)
endif ()
