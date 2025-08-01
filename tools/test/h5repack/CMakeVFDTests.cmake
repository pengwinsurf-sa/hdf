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

##############################################################################
##############################################################################
###           T E S T I N G                                                ###
##############################################################################
##############################################################################
H5_CREATE_VFD_DIR()

set (H5REPACK_VFD_subfiling_SKIP_TESTS
  h5repacktest
)

##############################################################################
##############################################################################
###           T H E   T E S T S  M A C R O S                               ###
##############################################################################
##############################################################################
set (H5REPACK_CLEANFILES
      bounds_latest_latest.h5
      h5repack_attr.h5
      h5repack_attr_refs.h5
      h5repack_deflate.h5
      h5repack_early.h5
      h5repack_ext.h5
      h5repack_fill.h5
      h5repack_filters.h5
      h5repack_fletcher.h5
      h5repack_hlink.h5
      h5repack_layout.h5
      h5repack_layouto.h5
      h5repack_layout2.h5
      h5repack_layout3.h5
      h5repack_layout.UD.h5
      h5repack_named_dtypes.h5
      h5repack_nested_8bit_enum.h5
      h5repack_nested_8bit_enum_deflated.h5
      h5repack_nbit.h5
      h5repack_objs.h5
      h5repack_refs.h5
      h5repack_shuffle.h5
      h5repack_soffset.h5
      h5repack_szip.h5
      # fsm
      h5repack_aggr.h5
      h5repack_fsm_aggr_nopersist.h5
      h5repack_fsm_aggr_persist.h5
      h5repack_none.h5
      h5repack_paged_nopersist.h5
      h5repack_paged_persist.h5
)
macro (ADD_VFD_TEST vfdname resultcode)
  if (NOT HDF5_ENABLE_USING_MEMCHECKER)
    add_test (
        NAME H5REPACK-${vfdname}-h5repacktest-clear-objects
        COMMAND ${CMAKE_COMMAND} -E remove ${H5REPACK_CLEANFILES}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${vfdname}
    )
    if (NOT "h5repacktest" IN_LIST H5REPACK_VFD_${vfdname}_SKIP_TESTS)
      add_test (
          NAME H5REPACK_VFD-${vfdname}-h5repacktest
          COMMAND "${CMAKE_COMMAND}"
              -D "TEST_EMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}"
              -D "TEST_PROGRAM=$<TARGET_FILE:h5repacktest>"
              -D "TEST_ARGS:STRING="
              -D "TEST_VFD:STRING=${vfdname}"
              -D "TEST_EXPECT=${resultcode}"
              -D "TEST_OUTPUT=${vfdname}-h5repacktest.out"
              -D "TEST_FOLDER=${PROJECT_BINARY_DIR}/${vfdname}"
              -P "${HDF_RESOURCES_DIR}/vfdTest.cmake"
      )
      set_tests_properties (H5REPACK_VFD-${vfdname}-h5repacktest PROPERTIES
          DEPENDS H5REPACK_VFD-${vfdname}-h5repacktest-clear-objects
          TIMEOUT ${CTEST_SHORT_TIMEOUT}
      )
      if ("H5REPACK_VFD-${vfdname}-h5repacktest" MATCHES "${HDF5_DISABLE_TESTS_REGEX}")
        set_tests_properties (H5REPACK_VFD-${vfdname}-h5repacktest PROPERTIES DISABLED true)
      endif ()
      add_test (
          NAME H5REPACK_VFD-${vfdname}-h5repacktest-clean-objects
          COMMAND ${CMAKE_COMMAND} -E remove
              ${H5REPACK_CLEANFILES}
      )
      set_tests_properties (H5REPACK_VFD-${vfdname}-h5repacktest-clean-objects PROPERTIES
          DEPENDS H5REPACK_VFD-${vfdname}-h5repacktest
          WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${vfdname}
      )
    endif ()
  endif ()
endmacro ()

##############################################################################
##############################################################################
###           T H E   T E S T S                                            ###
##############################################################################
##############################################################################

# Run test with different Virtual File Driver
foreach (vfd ${VFD_LIST})
  ADD_VFD_TEST (${vfd} 0)
endforeach ()