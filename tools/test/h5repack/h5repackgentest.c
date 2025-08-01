/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "hdf5.h"
#include "H5private.h"
#include "h5repackgentest.h"
#include "h5tools.h"
#include "h5tools_utils.h"
#include "h5test.h"

#define MAX_NAME_SIZE     256
#define PAGE_SIZE_DEFAULT 4096

#define FILE_INT32LE_1 "h5repack_int32le_1d"
#define FILE_INT32LE_2 "h5repack_int32le_2d"
#define FILE_INT32LE_3 "h5repack_int32le_3d"
#define FILE_UINT8BE   "h5repack_uint8be"
#define FILE_F32LE     "h5repack_f32le"

#define H5REPACKGENTEST_OOPS                                                                                 \
    {                                                                                                        \
        ret_value = -1;                                                                                      \
        goto done;                                                                                           \
    }

#define H5REPACKGENTEST_COMMON_CLEANUP(dcpl, file, space)                                                    \
    {                                                                                                        \
        if ((dcpl) != H5P_DEFAULT && (dcpl) != H5I_INVALID_HID) {                                            \
            (void)H5Pclose((dcpl));                                                                          \
        }                                                                                                    \
        if ((file) != H5I_INVALID_HID) {                                                                     \
            (void)H5Fclose((file));                                                                          \
        }                                                                                                    \
        if ((space) != H5I_INVALID_HID) {                                                                    \
            (void)H5Sclose((space));                                                                         \
        }                                                                                                    \
    }

struct external_def {
    hsize_t  type_size;
    unsigned n_elts_per_file;
    unsigned n_elts_total;
};

#define NELMTS(X) (sizeof(X) / sizeof(X[0])) /* # of elements */

#define DIM1  40
#define DIM2  20
#define CDIM1 (DIM1 / 2)
#define CDIM2 (DIM2 / 2)
#define RANK  2

/* obj and region references */
#define NAME_OBJ_DS1    "Dset1"
#define NAME_OBJ_GRP    "Group"
#define NAME_OBJ_NDTYPE "NamedDatatype"
#define NAME_OBJ_DS2    "Dset2"
#define REG_REF_DS1     "Dset_REGREF"

/*-------------------------------------------------------------------------
 * prototypes
 *-------------------------------------------------------------------------
 */
static int make_all_objects(hid_t loc_id);
static int make_attributes(hid_t loc_id);
static int make_hlinks(hid_t loc_id);
static int make_early(void);
static int make_layout(hid_t loc_id);
static int make_layout2(hid_t loc_id);
static int make_layout3(hid_t loc_id);
#ifdef H5_HAVE_FILTER_SZIP
static int make_szip(hid_t loc_id);
#endif /* H5_HAVE_FILTER_SZIP */
static int make_deflate(hid_t loc_id);
static int make_shuffle(hid_t loc_id);
static int make_fletcher32(hid_t loc_id);
static int make_nbit(hid_t loc_id);
static int make_scaleoffset(hid_t loc_id);
static int make_all_filters(hid_t loc_id);
static int make_fill(hid_t loc_id);
static int make_big(hid_t loc_id);
static int write_dset_in(hid_t loc_id, const char *dset_name, hid_t file_id, int make_diffs);
static int write_attr_in(hid_t loc_id, const char *dset_name, hid_t fid, int make_diffs);
static int write_dset(hid_t loc_id, int rank, hsize_t *dims, const char *dset_name, hid_t tid, void *buf);
static int make_dset(hid_t loc_id, const char *name, hid_t sid, hid_t dcpl, void *buf);
static int make_attr(hid_t loc_id, int rank, hsize_t *dims, const char *attr_name, hid_t tid, void *buf);
static int make_dset_reg_ref(hid_t loc_id);
static int make_external(hid_t loc_id);
static int make_userblock(void);
static int make_userblock_file(void);
static int make_named_dtype(hid_t loc_id);
static int make_references(hid_t loc_id);
static int make_complex_attr_references(hid_t loc_id);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Helper function to create and write a dataset to file.
 * Returns 0 on success, -1 on failure.
 */
static int
make_dataset(hid_t file_id, const char *dset_name, hid_t mem_type_id, hid_t space_id, hid_t dcpl_id,
             void *wdata)
{
    hid_t dset_id   = H5I_INVALID_HID;
    int   ret_value = 0;

    dset_id = H5Dcreate2(file_id, dset_name, mem_type_id, space_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT);
    if (dset_id == H5I_INVALID_HID)
        H5REPACKGENTEST_OOPS;

    if (H5Dwrite(dset_id, mem_type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata) < 0)
        H5REPACKGENTEST_OOPS;

done:
    if (dset_id != H5I_INVALID_HID)
        (void)H5Dclose(dset_id);

    return ret_value;
} /* end make_dataset() */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Helper function to populate the DCPL external storage list.
 * Creates external files for the DCPL, with each file name following the
 * convention "<filename>_ex-<num>.dat". Will append `n_external_files` to
 * the filename list, with each file having space for `n_elts` items of the
 * type (of size `elt_size`). The numeric inputs are not sanity-checked.
 * Returns 0 on success, -1 on failure.
 */
static int
set_dcpl_external_list(hid_t dcpl, const char *filename, unsigned n_elts_per_file, unsigned n_elts_total,
                       hsize_t elt_size)
{
    char     name[MAX_NAME_SIZE];
    unsigned n_external_files = 0;
    unsigned i                = 0;

    if (NULL == filename || '\0' == *filename)
        return -1;

    n_external_files = n_elts_total / n_elts_per_file;
    if (n_elts_total != (n_external_files * n_elts_per_file))
        return -1;

    for (i = 0; i < n_external_files; i++) {
        if (snprintf(name, MAX_NAME_SIZE, "%s_ex-%u.dat", filename, i) >= MAX_NAME_SIZE)
            return -1;

        if (H5Pset_external(dcpl, name, 0, n_elts_per_file * elt_size) < 0)
            return -1;
    }
    return 0;
} /* end set_dcpl_external_list() */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Generalized utility function to write a file with the specified data and
 * dataset configuration. If `ext` is provided, will attempt to use external
 * storage.
 * Returns 0 on success, -1 on failure.
 */
static int
make_file(const char *basename, struct external_def *ext, hid_t type_id, hsize_t rank, hsize_t *dims,
          void *wdata)
{
    char  filename[MAX_NAME_SIZE];
    hid_t file_id   = H5I_INVALID_HID;
    hid_t dcpl_id   = H5P_DEFAULT;
    hid_t space_id  = H5I_INVALID_HID;
    int   ret_value = 0;

    if (snprintf(filename, MAX_NAME_SIZE, "%s%s.h5", basename, (NULL != ext) ? "_ex" : "") >= MAX_NAME_SIZE)
        H5REPACKGENTEST_OOPS;

    if (NULL != ext) {
        dcpl_id = H5Pcreate(H5P_DATASET_CREATE);
        if (dcpl_id == H5I_INVALID_HID)
            H5REPACKGENTEST_OOPS;

        if (set_dcpl_external_list(dcpl_id, basename, ext->n_elts_per_file, ext->n_elts_total,
                                   ext->type_size) < 0)
            H5REPACKGENTEST_OOPS;
    }

    space_id = H5Screate_simple((int)rank, dims, NULL);
    if (space_id == H5I_INVALID_HID)
        H5REPACKGENTEST_OOPS;

    file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id == H5I_INVALID_HID)
        H5REPACKGENTEST_OOPS;

    if (make_dataset(file_id, "dset", type_id, space_id, dcpl_id, wdata) < 0)
        H5REPACKGENTEST_OOPS;

done:
    H5REPACKGENTEST_COMMON_CLEANUP(dcpl_id, file_id, space_id);
    return ret_value;
} /* end make_file() */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Returns 0 on success, -1 on failure.
 */
int
generate_int32le_1d(bool external)
{
    int32_t              wdata[12];
    hsize_t              dims[]    = {12};
    struct external_def *def_ptr   = NULL;
    struct external_def  def       = {(hsize_t)sizeof(int32_t), 6, 12};
    int32_t              n         = 0;
    int                  ret_value = 0;

    /* Generate values
     */
    for (n = 0; n < 12; n++) {
        wdata[n] = n - 6;
    }

    def_ptr = (true == external) ? (&def) : NULL;
    if (make_file(FILE_INT32LE_1, def_ptr, H5T_STD_I32LE, 1, dims, wdata) < 0)
        ret_value = -1;

    return ret_value;
} /* end generate_int32le_1d() */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Returns 0 on success, -1 on failure.
 */
int
generate_int32le_2d(bool external)
{
    int32_t              wdata[64];
    hsize_t              dims[]    = {8, 8};
    struct external_def *def_ptr   = NULL;
    struct external_def  def       = {(hsize_t)sizeof(int32_t), 64, 64};
    int32_t              n         = 0;
    int                  ret_value = 0;

    /* Generate values
     */
    for (n = 0; n < 64; n++) {
        wdata[n] = n - 32;
    }

    def_ptr = (true == external) ? (&def) : NULL;
    if (make_file(FILE_INT32LE_2, def_ptr, H5T_STD_I32LE, 2, dims, wdata) < 0)
        ret_value = -1;

    return ret_value;
} /* end generate_int32le_2d() */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Returns 0 on success, -1 on failure.
 */
int
generate_int32le_3d(bool external)
{
    hsize_t              dims[] = {8, 8, 8};
    int32_t              wdata[512]; /* 8^3, from dims */
    struct external_def *def_ptr   = NULL;
    struct external_def  def       = {(hsize_t)sizeof(int32_t), 512, 512};
    int32_t              n         = 0;
    int                  i         = 0;
    int                  j         = 0;
    int                  k         = 0;
    int                  ret_value = 0;

    /* generate values, alternating positive and negative
     */
    for (i = 0, n = 0; (hsize_t)i < dims[0]; i++) {
        for (j = 0; (hsize_t)j < dims[1]; j++) {
            for (k = 0; (hsize_t)k < dims[2]; k++, n++) {
                wdata[n] = (k + j * 512 + i * 4096) * ((n & 1) ? (-1) : (1));
            }
        }
    }

    def_ptr = (true == external) ? (&def) : NULL;
    if (make_file(FILE_INT32LE_3, def_ptr, H5T_STD_I32LE, 3, dims, wdata) < 0)
        ret_value = -1;

    return ret_value;
} /* end generate_int32le_3d() */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Returns 0 on success, -1 on failure.
 */
int
generate_uint8be(bool external)
{
    hsize_t              dims[] = {4, 8, 8};
    uint8_t              wdata[256]; /* 4*8*8, from dims */
    struct external_def *def_ptr   = NULL;
    struct external_def  def       = {(hsize_t)sizeof(uint8_t), 64, 256};
    uint8_t              n         = 0;
    int                  i         = 0;
    int                  j         = 0;
    int                  k         = 0;
    int                  ret_value = 0;

    /* Generate values, ping-pong from ends of range
     */
    for (i = 0, n = 0; (hsize_t)i < dims[0]; i++) {
        for (j = 0; (hsize_t)j < dims[1]; j++) {
            for (k = 0; (hsize_t)k < dims[2]; k++, n++) {
                wdata[n] = (uint8_t)((n & 1) ? -n : n);
            }
        }
    }

    def_ptr = (true == external) ? (&def) : NULL;
    if (make_file(FILE_UINT8BE, def_ptr, H5T_STD_U8BE, 3, dims, wdata) < 0)
        ret_value = -1;

    return ret_value;
} /* end generate_uint8be() */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Returns 0 on success, -1 on failure.
 */
int
generate_f32le(bool external)
{
    hsize_t              dims[] = {12, 6};
    float                wdata[72]; /* 12*6, from dims */
    struct external_def *def_ptr   = NULL;
    struct external_def  def       = {(hsize_t)sizeof(float), 72, 72};
    float                n         = 0;
    int                  i         = 0;
    int                  j         = 0;
    int                  k         = 0;
    int                  ret_value = 0;

    /* Generate values */
    for (i = 0, k = 0, n = 0; (hsize_t)i < dims[0]; i++) {
        for (j = 0; (hsize_t)j < dims[1]; j++, k++, n++) {
            wdata[k] = n * 801.1F * ((k % 5 == 1) ? (-1) : (1));
        }
    }

    def_ptr = (true == external) ? (&def) : NULL;
    if (make_file(FILE_F32LE, def_ptr, H5T_IEEE_F32LE, 2, dims, wdata) < 0)
        ret_value = -1;

    return ret_value;
} /* end generate_f32le() */

/*-------------------------------------------------------------------------
 * Function: make_h5repack_testfiles
 *
 * Purpose:  make a test file with all types of HDF5 objects,
 *           datatypes and filters
 *
 *-------------------------------------------------------------------------
 */
int
make_h5repack_testfiles(void)
{
    hid_t    fid  = H5I_INVALID_HID;
    hid_t    fcpl = H5I_INVALID_HID; /* File creation property list */
    hid_t    fapl = H5I_INVALID_HID; /* File access property list */
    unsigned j;                      /* Local index variable */
    bool     driver_is_parallel;

    if (h5_using_parallel_driver(H5P_DEFAULT, &driver_is_parallel) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file for general copy test
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME0, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_fill(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create another file for general copy test (all datatypes)
     *-------------------------------------------------------------------------
     */
    if (!driver_is_parallel) {
        if ((fid = H5Fcreate(H5REPACK_FNAME1, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
            return -1;
        if (make_all_objects(fid) < 0)
            goto out;
        if (H5Fclose(fid) < 0)
            return -1;
    }

    /*-------------------------------------------------------------------------
     * create a file for attributes copy test
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME2, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_attributes(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file for hard links test
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME3, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_hlinks(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file for layouts test
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME4, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_layout(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file for layout conversion test
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME18, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;

    if (make_layout2(fid) < 0)
        goto out;

    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * for test layout conversions form chunk with unlimited max dims
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME19, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;

    if (make_layout3(fid) < 0)
        goto out;

    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file for the H5D_ALLOC_TIME_EARLY test
     *-------------------------------------------------------------------------
     */
    if (make_early() < 0)
        goto out;

        /*-------------------------------------------------------------------------
         * create a file with the SZIP filter
         *-------------------------------------------------------------------------
         */
#ifdef H5_HAVE_FILTER_SZIP
    if ((fid = H5Fcreate(H5REPACK_FNAME7, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_szip(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;
#endif /* H5_HAVE_FILTER_SZIP */

    /*-------------------------------------------------------------------------
     * create a file with the deflate filter
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME8, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_deflate(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file with the shuffle filter
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME9, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_shuffle(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file with the fletcher32 filter
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME10, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_fletcher32(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file with all the filters
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME11, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_all_filters(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file with the nbit filter
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME12, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_nbit(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file with the scaleoffset filter
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME13, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_scaleoffset(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a big dataset
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME14, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_big(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a file with external dataset
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME15, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_external(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    if (h5_using_default_driver(NULL)) {
        /*-------------------------------------------------------------------------
         * create a file with userblock
         *-------------------------------------------------------------------------
         */
        if (make_userblock() < 0)
            goto out;

        /*-------------------------------------------------------------------------
         * create a userblock file
         *-------------------------------------------------------------------------
         */
        if (make_userblock_file() < 0)
            goto out;
    }

    /*-------------------------------------------------------------------------
     * create a file with named datatypes
     *-------------------------------------------------------------------------
     */
    if ((fid = H5Fcreate(H5REPACK_FNAME17, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (make_named_dtype(fid) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        return -1;

    if (!driver_is_parallel) {
        /*-------------------------------------------------------------------------
         * create obj and region reference type datasets (bug1814)
         * add attribute with int type (bug1726)
         * add attribute with obj and region reference type (bug1726)
         *-------------------------------------------------------------------------
         */
        if ((fid = H5Fcreate(H5REPACK_FNAME_REF, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
            return -1;
        /* create reference type datasets */
        if (make_references(fid) < 0)
            goto out;
        if (H5Fclose(fid) < 0)
            return -1;

        /*-------------------------------------------------------------------------
         * create a file with obj and region references in attribute of compound and
         * vlen datatype
         *-------------------------------------------------------------------------*/
        if ((fid = H5Fcreate(H5REPACK_FNAME_ATTR_REF, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
            return -1;
        if (make_complex_attr_references(fid) < 0)
            goto out;
        if (H5Fclose(fid) < 0)
            return -1;
    }

    /*-------------------------------------------------------------------------
     * create 8 files with combinations ???
     *------------------------------------------------------------------------- */

    /* Create file access property list */
    if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        return -1;

    /* Set to use latest library format */
    if (H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) < 0)
        return -1;

    /*
     * #0 -- h5repack_latest.h5
     * default: strategy=FSM_AGGR, persist=false, threshold=1
     * default: inpage=PAGE_SIZE_DEFAULT
     */
    j = 0;
    if ((fid = H5Fcreate(H5REPACK_FSPACE_FNAMES[j], H5F_ACC_TRUNC, H5P_DEFAULT, fapl)) < 0)
        return -1;
    if (H5Fclose(fid) < 0)
        return -1;

    /*
     * #1 -- h5repack_default.h5
     * default: strategy=FSM_AGGR, persist=false, threshold=1
     * default: inpage=PAGE_SIZE_DEFAULT
     */
    assert(j < NELMTS(H5REPACK_FSPACE_FNAMES));
    if ((fid = H5Fcreate(H5REPACK_FSPACE_FNAMES[++j], H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (H5Fclose(fid) < 0)
        return -1;

    if (h5_using_default_driver(NULL)) {
        /*
         * #2 -- h5repack_page_persist.h5
         * Setting:
         *    strategy=PAGE, persist=true, threshold=1
         *    inpage=512
         *  latest format
         */
        /* Create file creation property list */
        if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
            return -1;
        if (H5Pset_file_space_page_size(fcpl, (hsize_t)512) < 0)
            return -1;
        if (H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_PAGE, true, (hsize_t)1) < 0)
            return -1;
        assert(j < NELMTS(H5REPACK_FSPACE_FNAMES));
        if ((fid = H5Fcreate(H5REPACK_FSPACE_FNAMES[++j], H5F_ACC_TRUNC, fcpl, fapl)) < 0)
            return -1;
        if (H5Fclose(fid) < 0)
            return -1;
        if (H5Pclose(fcpl) < 0)
            return -1;

        /*
         * #3 -- h5repack_fsm_aggr_persist.h5
         * Setting:
         *    strategy=FSM_AGGR, persist=true, threshold=1
         *  default: inpage=PAGE_SIZE_DEFAULT
         */
        /* Create file creation property list */
        if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
            return -1;
        if (H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_FSM_AGGR, true, (hsize_t)1) < 0)
            return -1;
        assert(j < NELMTS(H5REPACK_FSPACE_FNAMES));
        if ((fid = H5Fcreate(H5REPACK_FSPACE_FNAMES[++j], H5F_ACC_TRUNC, fcpl, H5P_DEFAULT)) < 0)
            return -1;
        if (H5Fclose(fid) < 0)
            return -1;
        if (H5Pclose(fcpl) < 0)
            return -1;

        /*
         * #4 -- h5repack_page_threshold.h5
         * Setting:
         *    strategy=PAGE, persist=false, threshold=3
         *  inpage=8192
         *  latest format
         */

        /* Create file creation property list */
        if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
            return -1;
        if (H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_PAGE, false, (hsize_t)3) < 0)
            return -1;
        if (H5Pset_file_space_page_size(fcpl, (hsize_t)8192) < 0)
            return -1;
        assert(j < NELMTS(H5REPACK_FSPACE_FNAMES));
        if ((fid = H5Fcreate(H5REPACK_FSPACE_FNAMES[++j], H5F_ACC_TRUNC, fcpl, fapl)) < 0)
            return -1;
        if (H5Fclose(fid) < 0)
            return -1;
        if (H5Pclose(fcpl) < 0)
            return -1;

        /*
         * #5 -- h5repack_fsm_aggr_threshold.h5
         * Setting:
         *    strategy=FSM_AGGR, persist=false, threshold=3
         *    inpage=PAGE_SIZE_MEDIUM
         */

        /* Create file creation property list */
        if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
            return -1;
        if (H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_FSM_AGGR, false, (hsize_t)3) < 0)
            return -1;
        if (H5Pset_file_space_page_size(fcpl, (hsize_t)PAGE_SIZE_DEFAULT) < 0)
            return -1;
        assert(j < NELMTS(H5REPACK_FSPACE_FNAMES));
        if ((fid = H5Fcreate(H5REPACK_FSPACE_FNAMES[++j], H5F_ACC_TRUNC, fcpl, H5P_DEFAULT)) < 0)
            return -1;
        if (H5Fclose(fid) < 0)
            return -1;
        if (H5Pclose(fcpl) < 0)
            return -1;

        /*
         * #6 -- h5repack_aggr.h5
         * Setting:
         *     strategy=AGGR, persist=false, threshold=1
         *  latest format
         */

        /* Create file creation property list */
        if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
            return -1;
        if (H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_AGGR, false, (hsize_t)1) < 0)
            return -1;
        assert(j < NELMTS(H5REPACK_FSPACE_FNAMES));
        if ((fid = H5Fcreate(H5REPACK_FSPACE_FNAMES[++j], H5F_ACC_TRUNC, fcpl, fapl)) < 0)
            return -1;
        if (H5Fclose(fid) < 0)
            return -1;
        if (H5Pclose(fcpl) < 0)
            return -1;
    }

    /*
     * #7 -- h5repack_none.h5
     * Setting:
     *    strategy=NONE, persist=false, threshold=1
     *     inpage=8192
     */

    /* Create file creation property list */
    if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
        return -1;
    if (H5Pset_file_space_strategy(fcpl, H5F_FSPACE_STRATEGY_NONE, false, (hsize_t)1) < 0)
        return -1;
    if (H5Pset_file_space_page_size(fcpl, (hsize_t)8192) < 0)
        return -1;
    assert(j < NELMTS(H5REPACK_FSPACE_FNAMES));
    if ((fid = H5Fcreate(H5REPACK_FSPACE_FNAMES[++j], H5F_ACC_TRUNC, fcpl, H5P_DEFAULT)) < 0)
        return -1;
    if (H5Fclose(fid) < 0)
        return -1;
    if (H5Pclose(fcpl) < 0)
        return -1;

    if (H5Pclose(fapl) < 0)
        return -1;

    return 0;

out:
    H5Fclose(fid);
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_all_objects
 *
 * Purpose:  make a test file with all types of HDF5 objects
 *
 *-------------------------------------------------------------------------
 */
static int
make_all_objects(hid_t loc_id)
{
    hid_t   did     = H5I_INVALID_HID;
    hid_t   gid     = H5I_INVALID_HID;
    hid_t   tid     = H5I_INVALID_HID;
    hid_t   rid     = H5I_INVALID_HID;
    hid_t   sid     = H5I_INVALID_HID;
    hid_t   gcplid  = H5I_INVALID_HID;
    hsize_t dims[1] = {2};
    /* compound datatype */
    typedef struct s_t {
        int   a;
        float b;
    } s_t;

    /*-------------------------------------------------------------------------
     * H5G_DATASET
     *-------------------------------------------------------------------------
     */
    if ((sid = H5Screate_simple(1, dims, NULL)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "dset_referenced", H5T_NATIVE_INT, sid, H5P_DEFAULT, H5P_DEFAULT,
                          H5P_DEFAULT)) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5G_GROUP
     *-------------------------------------------------------------------------
     */
    if ((gid = H5Gcreate2(loc_id, "g1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Gclose(gid) < 0)
        goto out;

    /* create a group "g2" with H5P_CRT_ORDER_TRACKED set */
    if ((gcplid = H5Pcreate(H5P_GROUP_CREATE)) < 0)
        goto out;
    if (H5Pset_link_creation_order(gcplid, H5P_CRT_ORDER_TRACKED) < 0)
        goto out;
    if ((gid = H5Gcreate2(loc_id, "g2", H5P_DEFAULT, gcplid, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Gclose(gid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5G_TYPE
     *-------------------------------------------------------------------------
     */

    /* create a compound datatype */
    if ((tid = H5Tcreate(H5T_COMPOUND, sizeof(s_t))) < 0)
        goto out;
    if (H5Tinsert(tid, "a", HOFFSET(s_t, a), H5T_NATIVE_INT) < 0)
        goto out;
    if (H5Tinsert(tid, "b", HOFFSET(s_t, b), H5T_NATIVE_FLOAT) < 0)
        goto out;
    if (H5Tcommit2(loc_id, "type", tid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5G_LINK
     *-------------------------------------------------------------------------
     */

    if (H5Lcreate_soft("dset", loc_id, "link", H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5G_UDLINK
     *-------------------------------------------------------------------------
     */
    /* Create an external link. Other UD links are not supported by h5repack */
    if (H5Lcreate_external("file", "path", loc_id, "ext_link", H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * write a series of datasets at root
     *-------------------------------------------------------------------------
     */

    if ((rid = H5Gopen2(loc_id, "/", H5P_DEFAULT)) < 0)
        goto out;
    if (write_dset_in(rid, "dset_referenced", loc_id, 0) < 0)
        goto out;
    if (H5Gclose(rid) < 0)
        goto out;

    /* close */
    if (H5Dclose(did) < 0)
        goto out;
    if (H5Sclose(sid) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    if (H5Pclose(gcplid) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Dclose(did);
        H5Gclose(gid);
        H5Gclose(rid);
        H5Sclose(sid);
        H5Tclose(tid);
        H5Pclose(gcplid);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_attributes
 *
 * Purpose:  make a test file with all types of attributes
 *
 *-------------------------------------------------------------------------
 */
static int
make_attributes(hid_t loc_id)
{
    hid_t   did     = H5I_INVALID_HID;
    hid_t   gid     = H5I_INVALID_HID;
    hid_t   rid     = H5I_INVALID_HID;
    hid_t   sid     = H5I_INVALID_HID;
    hsize_t dims[1] = {2};

    /*-------------------------------------------------------------------------
     * H5G_DATASET
     *-------------------------------------------------------------------------
     */
    if ((sid = H5Screate_simple(1, dims, NULL)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "dset", H5T_NATIVE_INT, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5G_GROUP
     *-------------------------------------------------------------------------
     */
    if ((gid = H5Gcreate2(loc_id, "g1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if ((rid = H5Gopen2(loc_id, "/", H5P_DEFAULT)) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * write a series of attributes on the dataset, group, and root group
     *-------------------------------------------------------------------------
     */

    if (write_attr_in(did, "dset", loc_id, 0) < 0)
        goto out;
    if (write_attr_in(gid, "dset", loc_id, 0) < 0)
        goto out;
    if (write_attr_in(rid, "dset", loc_id, 0) < 0)
        goto out;

    /* close */
    if (H5Dclose(did) < 0)
        goto out;
    if (H5Gclose(gid) < 0)
        goto out;
    if (H5Gclose(rid) < 0)
        goto out;
    if (H5Sclose(sid) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Dclose(did);
        H5Gclose(gid);
        H5Gclose(rid);
        H5Sclose(sid);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_hlinks
 *
 * Purpose:  make a test file with hard links
 *
 *-------------------------------------------------------------------------
 */
static int
make_hlinks(hid_t loc_id)
{
    hid_t   g1id      = -1;
    hid_t   g2id      = H5I_INVALID_HID;
    hid_t   g3id      = H5I_INVALID_HID;
    hsize_t dims[2]   = {3, 2};
    int     buf[3][2] = {{1, 1}, {1, 2}, {2, 2}};

    /*-------------------------------------------------------------------------
     * create a dataset and some hard links to it
     *-------------------------------------------------------------------------
     */

    if (write_dset(loc_id, 2, dims, "dset", H5T_NATIVE_INT, buf) < 0)
        return -1;
    if (H5Lcreate_hard(loc_id, "dset", H5L_SAME_LOC, "link1 to dset", H5P_DEFAULT, H5P_DEFAULT) < 0)
        return -1;
    if (H5Lcreate_hard(loc_id, "dset", H5L_SAME_LOC, "link2 to dset", H5P_DEFAULT, H5P_DEFAULT) < 0)
        return -1;
    if (H5Lcreate_hard(loc_id, "dset", H5L_SAME_LOC, "link3 to dset", H5P_DEFAULT, H5P_DEFAULT) < 0)
        return -1;

    /*-------------------------------------------------------------------------
     * create a group and some hard links to it
     *-------------------------------------------------------------------------
     */

    if ((g1id = H5Gcreate2(loc_id, "g1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if ((g2id = H5Gcreate2(g1id, "g2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if ((g3id = H5Gcreate2(g2id, "g3", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;

    if (H5Lcreate_hard(loc_id, "g1", g2id, "link1 to g1", H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;
    if (H5Lcreate_hard(g1id, "g2", g3id, "link1 to g2", H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;

    /* close */
    if (H5Gclose(g1id) < 0)
        goto out;
    if (H5Gclose(g2id) < 0)
        goto out;
    if (H5Gclose(g3id) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Gclose(g1id);
        H5Gclose(g2id);
        H5Gclose(g3id);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_szip
 *
 * Purpose:  make a dataset with the SZIP filter
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_FILTER_SZIP
static int
make_szip(hid_t loc_id)
{
    hid_t    dcpl                  = H5I_INVALID_HID; /* dataset creation property list */
    hid_t    sid                   = H5I_INVALID_HID; /* dataspace ID */
    unsigned szip_options_mask     = H5_SZIP_ALLOW_K13_OPTION_MASK | H5_SZIP_NN_OPTION_MASK;
    unsigned szip_pixels_per_block = 8;
    hsize_t  dims[RANK]            = {DIM1, DIM2};
    hsize_t  chunk_dims[RANK]      = {CDIM1, CDIM2};
    int      szip_can_encode       = 0;

    /* Create and fill array */
    struct {
        int arr[DIM1][DIM2];
    } *buf = malloc(sizeof(*buf));
    if (NULL == buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(buf, int);

    /* create a space */
    if ((sid = H5Screate_simple(RANK, dims, NULL)) < 0)
        goto error;
    /* create a dcpl */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    /* set up chunk */
    if (H5Pset_chunk(dcpl, RANK, chunk_dims) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * SZIP
     *-------------------------------------------------------------------------
     */
    /* Make sure encoding is enabled */
    if (h5tools_can_encode(H5Z_FILTER_SZIP) == 1)
        szip_can_encode = 1;

    if (szip_can_encode) {
        /* set szip data */
        if (H5Pset_szip(dcpl, szip_options_mask, szip_pixels_per_block) < 0)
            goto error;
        if (make_dset(loc_id, "dset_szip", sid, dcpl, buf) < 0)
            goto error;
    }
    else
        /* WARNING? SZIP is decoder only, can't generate test files */

        if (H5Sclose(sid) < 0)
            goto error;
    if (H5Pclose(dcpl) < 0)
        goto error;

    free(buf);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Pclose(dcpl);
        H5Sclose(sid);
    }
    H5E_END_TRY

    free(buf);

    return -1;
}
#endif /* H5_HAVE_FILTER_SZIP */

/*-------------------------------------------------------------------------
 * Function: make_deflate
 *
 * Purpose: make a dataset with the deflate filter
 *
 *-------------------------------------------------------------------------
 */
static int
make_deflate(hid_t loc_id)
{
    hid_t   dcpl             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   sid              = H5I_INVALID_HID; /* dataspace ID */
    hsize_t dims[RANK]       = {DIM1, DIM2};
    hsize_t chunk_dims[RANK] = {CDIM1, CDIM2};
#if defined(H5_HAVE_FILTER_DEFLATE)
    hobj_ref_t bufref[1]; /* reference */
    hsize_t    dims1r[1] = {1};
#else
    (void)loc_id;
#endif

    /* Create and fill array */
    struct {
        int arr[DIM1][DIM2];
    } *buf = malloc(sizeof(*buf));
    if (NULL == buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(buf, int);

    /* create a space */
    if ((sid = H5Screate_simple(RANK, dims, NULL)) < 0)
        goto error;
    /* create a dcpl */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    /* set up chunk */
    if (H5Pset_chunk(dcpl, RANK, chunk_dims) < 0)
        goto error;

        /*-------------------------------------------------------------------------
         * GZIP
         *-------------------------------------------------------------------------
         */
#if defined(H5_HAVE_FILTER_DEFLATE)
    /* set deflate data */
    if (H5Pset_deflate(dcpl, 9) < 0)
        goto error;
    if (make_dset(loc_id, "dset_deflate", sid, dcpl, buf) < 0)
        goto error;

    /* create a reference to the dataset, test second seeep of file for references */

    if (H5Rcreate(&bufref[0], loc_id, "dset_deflate", H5R_OBJECT, (hid_t)-1) < 0)
        goto error;
    if (write_dset(loc_id, 1, dims1r, "ref", H5T_STD_REF_OBJ, bufref) < 0)
        goto error;
#endif

    /*-------------------------------------------------------------------------
     * close space and dcpl
     *-------------------------------------------------------------------------
     */
    if (H5Sclose(sid) < 0)
        goto error;
    if (H5Pclose(dcpl) < 0)
        goto error;

    free(buf);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Pclose(dcpl);
        H5Sclose(sid);
    }
    H5E_END_TRY

    free(buf);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_shuffle
 *
 * Purpose: make a dataset with the shuffle filter
 *
 *-------------------------------------------------------------------------
 */
static int
make_shuffle(hid_t loc_id)
{
    hid_t   dcpl             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   sid              = H5I_INVALID_HID; /* dataspace ID */
    hsize_t dims[RANK]       = {DIM1, DIM2};
    hsize_t chunk_dims[RANK] = {CDIM1, CDIM2};

    /* Create and fill array */
    struct {
        int arr[DIM1][DIM2];
    } *buf = malloc(sizeof(*buf));
    if (NULL == buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(buf, int);

    /* create a space */
    if ((sid = H5Screate_simple(RANK, dims, NULL)) < 0)
        goto error;
    /* create a dcpl */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    /* set up chunk */
    if (H5Pset_chunk(dcpl, RANK, chunk_dims) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * shuffle
     *-------------------------------------------------------------------------
     */

    /* set the shuffle filter */
    if (H5Pset_shuffle(dcpl) < 0)
        goto error;
    if (make_dset(loc_id, "dset_shuffle", sid, dcpl, buf) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * close space and dcpl
     *-------------------------------------------------------------------------
     */
    if (H5Sclose(sid) < 0)
        goto error;
    if (H5Pclose(dcpl) < 0)
        goto error;

    free(buf);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Pclose(dcpl);
        H5Sclose(sid);
    }
    H5E_END_TRY

    free(buf);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_fletcher32
 *
 * Purpose: make a dataset with the fletcher32 filter
 *
 *-------------------------------------------------------------------------
 */
static int
make_fletcher32(hid_t loc_id)
{
    hid_t   dcpl             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   sid              = H5I_INVALID_HID; /* dataspace ID */
    hsize_t dims[RANK]       = {DIM1, DIM2};
    hsize_t chunk_dims[RANK] = {CDIM1, CDIM2};

    /* Create and fill array */
    struct {
        int arr[DIM1][DIM2];
    } *buf = malloc(sizeof(*buf));
    if (NULL == buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(buf, int);

    /* create a space */
    if ((sid = H5Screate_simple(RANK, dims, NULL)) < 0)
        goto error;
    /* create a dataset creation property list; the same DCPL is used for all dsets */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    /* set up chunk */
    if (H5Pset_chunk(dcpl, RANK, chunk_dims) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * fletcher32
     *-------------------------------------------------------------------------
     */

    /* remove the filters from the dcpl */
    if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
        goto error;
    /* set the checksum filter */
    if (H5Pset_fletcher32(dcpl) < 0)
        goto error;
    if (make_dset(loc_id, "dset_fletcher32", sid, dcpl, buf) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * close space and dcpl
     *-------------------------------------------------------------------------
     */
    if (H5Sclose(sid) < 0)
        goto error;
    if (H5Pclose(dcpl) < 0)
        goto error;

    free(buf);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Pclose(dcpl);
        H5Sclose(sid);
    }
    H5E_END_TRY

    free(buf);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_nbit
 *
 * Purpose: make a dataset with the nbit filter
 *
 *-------------------------------------------------------------------------
 */
static int
make_nbit(hid_t loc_id)
{
    hid_t   dcpl             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   sid              = H5I_INVALID_HID; /* dataspace ID */
    hid_t   dtid             = H5I_INVALID_HID;
    hid_t   dsid             = H5I_INVALID_HID;
    hid_t   dxpl             = H5P_DEFAULT;
    hsize_t dims[RANK]       = {DIM1, DIM2};
    hsize_t chunk_dims[RANK] = {CDIM1, CDIM2};

    /* Create and fill array */
    struct {
        int arr[DIM1][DIM2];
    } *buf = malloc(sizeof(*buf));
    if (NULL == buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(buf, int);

    /* create a space */
    if ((sid = H5Screate_simple(RANK, dims, NULL)) < 0)
        goto error;
    /* create a dataset creation property list; the same DCPL is used for all dsets */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    /* set up chunk */
    if (H5Pset_chunk(dcpl, RANK, chunk_dims) < 0)
        goto error;

#ifdef H5_HAVE_PARALLEL
    {
        bool driver_is_parallel;

        /* Set up collective writes for parallel driver */
        if (h5_using_parallel_driver(H5P_DEFAULT, &driver_is_parallel) < 0)
            goto error;
        if (driver_is_parallel) {
            if ((dxpl = H5Pcreate(H5P_DATASET_XFER)) < 0)
                goto error;
            if (H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE) < 0)
                goto error;
        }
    }
#endif

    dtid = H5Tcopy(H5T_NATIVE_INT);
    if (H5Tset_precision(dtid, (H5Tget_precision(dtid) - 1)) < 0)
        goto error;

    /* remove the filters from the dcpl */
    if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
        goto error;
    if (H5Pset_nbit(dcpl) < 0)
        goto error;
    if ((dsid = H5Dcreate2(loc_id, "dset_nbit", dtid, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
        goto error;
    if (H5Dwrite(dsid, dtid, H5S_ALL, H5S_ALL, dxpl, buf) < 0)
        goto error;
    H5Dclose(dsid);

    if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
        goto error;
    if ((dsid = H5Dcreate2(loc_id, "dset_int31", dtid, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
        goto error;
    if (H5Dwrite(dsid, dtid, H5S_ALL, H5S_ALL, dxpl, buf) < 0)
        goto error;
    H5Dclose(dsid);

    /*-------------------------------------------------------------------------
     * close
     *-------------------------------------------------------------------------
     */
    if (dxpl != H5P_DEFAULT && H5Pclose(dxpl) < 0)
        goto error;
    if (H5Sclose(sid) < 0)
        goto error;
    if (H5Pclose(dcpl) < 0)
        goto error;
    if (H5Tclose(dtid) < 0)
        goto error;

    free(buf);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Tclose(dtid);
        H5Pclose(dxpl);
        H5Pclose(dcpl);
        H5Sclose(sid);
        H5Dclose(dsid);
    }
    H5E_END_TRY

    free(buf);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_scaleoffset
 *
 * Purpose: make a dataset with the scaleoffset filter
 *
 *-------------------------------------------------------------------------
 */
static int
make_scaleoffset(hid_t loc_id)
{
    hid_t   dcpl             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   sid              = H5I_INVALID_HID; /* dataspace ID */
    hid_t   dtid             = H5I_INVALID_HID;
    hid_t   dsid             = H5I_INVALID_HID;
    hid_t   dxpl             = H5P_DEFAULT;
    hsize_t dims[RANK]       = {DIM1, DIM2};
    hsize_t chunk_dims[RANK] = {CDIM1, CDIM2};

    /* Create and fill array */
    struct {
        int arr[DIM1][DIM2];
    } *buf = malloc(sizeof(*buf));
    if (NULL == buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(buf, int);

    /* create a space */
    if ((sid = H5Screate_simple(RANK, dims, NULL)) < 0)
        goto error;
    /* create a dataset creation property list; the same DCPL is used for all dsets */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    /* set up chunk */
    if (H5Pset_chunk(dcpl, RANK, chunk_dims) < 0)
        goto error;

#ifdef H5_HAVE_PARALLEL
    {
        bool driver_is_parallel;

        if (h5_using_parallel_driver(H5P_DEFAULT, &driver_is_parallel) < 0)
            goto error;
        /* Set up collective writes for parallel driver */
        if (driver_is_parallel) {
            if ((dxpl = H5Pcreate(H5P_DATASET_XFER)) < 0)
                goto error;
            if (H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE) < 0)
                goto error;
        }
    }
#endif

    dtid = H5Tcopy(H5T_NATIVE_INT);

    /* remove the filters from the dcpl */
    if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
        goto error;
    if (H5Pset_scaleoffset(dcpl, H5Z_SO_INT, 31) < 0)
        goto error;
    if ((dsid = H5Dcreate2(loc_id, "dset_scaleoffset", dtid, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
        goto error;
    if (H5Dwrite(dsid, dtid, H5S_ALL, H5S_ALL, dxpl, buf) < 0)
        goto error;
    H5Dclose(dsid);
    if ((dsid = H5Dcreate2(loc_id, "dset_none", dtid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto error;
    if (H5Dwrite(dsid, dtid, H5S_ALL, H5S_ALL, dxpl, buf) < 0)
        goto error;
    H5Tclose(dtid);
    H5Dclose(dsid);

    /*-------------------------------------------------------------------------
     * close space, dxpl and dcpl
     *-------------------------------------------------------------------------
     */
    if (dxpl != H5P_DEFAULT && H5Pclose(dxpl) < 0)
        goto error;
    if (H5Sclose(sid) < 0)
        goto error;
    if (H5Pclose(dcpl) < 0)
        goto error;

    free(buf);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Pclose(dxpl);
        H5Dclose(dsid);
        H5Tclose(dtid);
        H5Pclose(dcpl);
        H5Sclose(sid);
    }
    H5E_END_TRY

    free(buf);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_all_filters
 *
 * Purpose:  make a file with all filters
 *
 *-------------------------------------------------------------------------
 */
static int
make_all_filters(hid_t loc_id)
{
    hid_t dcpl = H5I_INVALID_HID; /* dataset creation property list */
    hid_t sid  = H5I_INVALID_HID; /* dataspace ID */
    hid_t dtid = H5I_INVALID_HID;
    hid_t dsid = H5I_INVALID_HID;
    hid_t dxpl = H5P_DEFAULT;
#if defined(H5_HAVE_FILTER_SZIP)
    unsigned szip_options_mask     = H5_SZIP_ALLOW_K13_OPTION_MASK | H5_SZIP_NN_OPTION_MASK;
    unsigned szip_pixels_per_block = 8;
#endif /* H5_HAVE_FILTER_SZIP */
    hsize_t dims[RANK]       = {DIM1, DIM2};
    hsize_t chunk_dims[RANK] = {CDIM1, CDIM2};
#if defined(H5_HAVE_FILTER_SZIP)
    int szip_can_encode = 0;
#endif

    /* Create and fill array */
    struct {
        int arr[DIM1][DIM2];
    } *buf = malloc(sizeof(*buf));
    if (NULL == buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(buf, int);

    /* create a space */
    if ((sid = H5Screate_simple(RANK, dims, NULL)) < 0)
        goto error;
    /* create a dataset creation property list; the same DCPL is used for all dsets */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    /* set up chunk */
    if (H5Pset_chunk(dcpl, RANK, chunk_dims) < 0)
        goto error;

#ifdef H5_HAVE_PARALLEL
    {
        bool driver_is_parallel;

        if (h5_using_parallel_driver(H5P_DEFAULT, &driver_is_parallel) < 0)
            goto error;
        /* Set up collective writes for parallel driver */
        if (driver_is_parallel) {
            if ((dxpl = H5Pcreate(H5P_DATASET_XFER)) < 0)
                goto error;
            if (H5Pset_dxpl_mpio(dxpl, H5FD_MPIO_COLLECTIVE) < 0)
                goto error;
        }
    }
#endif

    /* set the shuffle filter */
    if (H5Pset_shuffle(dcpl) < 0)
        goto error;

    /* set the checksum filter */
    if (H5Pset_fletcher32(dcpl) < 0)
        goto error;

#if defined(H5_HAVE_FILTER_SZIP)
    if (h5tools_can_encode(H5Z_FILTER_SZIP) == 1) {
        szip_can_encode = 1;
    }
    if (szip_can_encode) {
        /* set szip data */
        if (H5Pset_szip(dcpl, szip_options_mask, szip_pixels_per_block) < 0)
            goto error;
    }
    else {
        /* WARNING? SZIP is decoder only, can't generate test data using szip */
    }
#endif

#if defined(H5_HAVE_FILTER_DEFLATE)
    /* set deflate data */
    if (H5Pset_deflate(dcpl, 9) < 0)
        goto error;
#endif

    if (make_dset(loc_id, "dset_all", sid, dcpl, buf) < 0)
        goto error;

    /* remove the filters from the dcpl */
    if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
        goto error;
    /* set the checksum filter */
    if (H5Pset_fletcher32(dcpl) < 0)
        goto error;
    if (make_dset(loc_id, "dset_fletcher32", sid, dcpl, buf) < 0)
        goto error;

        /* Make sure encoding is enabled */
#if defined(H5_HAVE_FILTER_SZIP)
    if (szip_can_encode) {
        /* remove the filters from the dcpl */
        if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
            goto error;
        /* set szip data */
        if (H5Pset_szip(dcpl, szip_options_mask, szip_pixels_per_block) < 0)
            goto error;
        if (make_dset(loc_id, "dset_szip", sid, dcpl, buf) < 0)
            goto error;
    }
    else {
        /* WARNING? SZIP is decoder only, can't generate test dataset */
    }
#endif

    /* remove the filters from the dcpl */
    if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
        goto error;
    /* set the shuffle filter */
    if (H5Pset_shuffle(dcpl) < 0)
        goto error;
    if (make_dset(loc_id, "dset_shuffle", sid, dcpl, buf) < 0)
        goto error;

#if defined(H5_HAVE_FILTER_DEFLATE)
    /* remove the filters from the dcpl */
    if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
        goto error;
    /* set deflate data */
    if (H5Pset_deflate(dcpl, 1) < 0)
        goto error;
    if (make_dset(loc_id, "dset_deflate", sid, dcpl, buf) < 0)
        goto error;
#endif

    /* remove the filters from the dcpl */
    if (H5Premove_filter(dcpl, H5Z_FILTER_ALL) < 0)
        goto error;
    /* set the shuffle filter */
    if (H5Pset_nbit(dcpl) < 0)
        goto error;
    if ((dtid = H5Tcopy(H5T_NATIVE_INT)) < 0)
        goto error;
    if (H5Tset_precision(dtid, (H5Tget_precision(dtid) - 1)) < 0)
        goto error;
    if ((dsid = H5Dcreate2(loc_id, "dset_nbit", dtid, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
        goto error;
    if (H5Dwrite(dsid, dtid, H5S_ALL, H5S_ALL, dxpl, buf) < 0)
        goto error;

    /* close */
    if (H5Tclose(dtid) < 0)
        goto error;
    if (H5Dclose(dsid) < 0)
        goto error;

    if (H5Sclose(sid) < 0)
        goto error;
    if (dxpl != H5P_DEFAULT && H5Pclose(dxpl) < 0)
        goto error;
    if (H5Pclose(dcpl) < 0)
        goto error;

    free(buf);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Tclose(dtid);
        H5Dclose(dsid);
        H5Pclose(dxpl);
        H5Pclose(dcpl);
        H5Sclose(sid);
    }
    H5E_END_TRY

    free(buf);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_early
 *
 * Purpose: create a file for the H5D_ALLOC_TIME_EARLY test
 *
 *-------------------------------------------------------------------------
 */
static int
make_early(void)
{
    hsize_t dims[1]  = {3000};
    hsize_t cdims[1] = {30};
    hid_t   fid      = H5I_INVALID_HID;
    hid_t   did      = H5I_INVALID_HID;
    hid_t   sid      = H5I_INVALID_HID;
    hid_t   tid      = H5I_INVALID_HID;
    hid_t   dcpl     = H5I_INVALID_HID;
    int     i;
    char    name[16];
    int     iter = 100;

    if ((fid = H5Fcreate(H5REPACK_FNAME5, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;
    if (H5Fclose(fid) < 0)
        goto out;

    if ((sid = H5Screate_simple(1, dims, NULL)) < 0)
        goto out;
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto out;
    if (H5Pset_chunk(dcpl, 1, cdims) < 0)
        goto out;
    if (H5Pset_alloc_time(dcpl, H5D_ALLOC_TIME_EARLY) < 0)
        goto out;

    for (i = 0; i < iter; i++) {
        if ((fid = H5Fopen(H5REPACK_FNAME5, H5F_ACC_RDWR, H5P_DEFAULT)) < 0)
            goto out;
        if ((did = H5Dcreate2(fid, "early", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
            goto out;
        if ((tid = H5Tcopy(H5T_NATIVE_DOUBLE)) < 0)
            goto out;
        snprintf(name, sizeof(name), "%d", i);
        if ((H5Tcommit2(fid, name, tid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
            goto out;
        if (H5Tclose(tid) < 0)
            goto out;
        if (H5Dclose(did) < 0)
            goto out;
        if (H5Ldelete(fid, "early", H5P_DEFAULT) < 0)
            goto out;
        if (H5Fclose(fid) < 0)
            goto out;
    }

    /*-------------------------------------------------------------------------
     * do the same without close/opening the file and creating the dataset
     *-------------------------------------------------------------------------
     */

    if ((fid = H5Fcreate(H5REPACK_FNAME6, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        return -1;

    for (i = 0; i < iter; i++) {
        if ((tid = H5Tcopy(H5T_NATIVE_DOUBLE)) < 0)
            goto out;
        snprintf(name, sizeof(name), "%d", i);
        if ((H5Tcommit2(fid, name, tid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
            goto out;
        if (H5Tclose(tid) < 0)
            goto out;
    }

    if (H5Sclose(sid) < 0)
        goto out;
    if (H5Pclose(dcpl) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Tclose(tid);
        H5Pclose(dcpl);
        H5Sclose(sid);
        H5Dclose(did);
        H5Fclose(fid);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_layout
 *
 * Purpose: make several datasets with several layouts in location LOC_ID
 *
 *-------------------------------------------------------------------------
 */
static int
make_layout(hid_t loc_id)
{
    hid_t   dcpl             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   sid              = H5I_INVALID_HID; /* dataspace ID */
    hsize_t dims[RANK]       = {DIM1, DIM2};
    hsize_t chunk_dims[RANK] = {CDIM1, CDIM2};
    int     i;
    char    name[16];

    /* Create and fill array */
    struct {
        int arr[DIM1][DIM2];
    } *buf = malloc(sizeof(*buf));
    if (NULL == buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(buf, int);

    /*-------------------------------------------------------------------------
     * make several dataset with no filters
     *-------------------------------------------------------------------------
     */
    for (i = 0; i < 4; i++) {
        snprintf(name, sizeof(name), "dset%d", i + 1);
        if (write_dset(loc_id, RANK, dims, name, H5T_NATIVE_INT, buf) < 0)
            goto error;
    }

    /*-------------------------------------------------------------------------
     * make several dataset with several layout options
     *-------------------------------------------------------------------------
     */
    /* create a space */
    if ((sid = H5Screate_simple(RANK, dims, NULL)) < 0)
        goto error;
    /* create a dataset creation property list; the same DCPL is used for all dsets */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * H5D_COMPACT
     *-------------------------------------------------------------------------
     */
    if (H5Pset_layout(dcpl, H5D_COMPACT) < 0)
        goto error;
    if (make_dset(loc_id, "dset_compact", sid, dcpl, buf) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * H5D_CONTIGUOUS
     *-------------------------------------------------------------------------
     */
    if (H5Pset_layout(dcpl, H5D_CONTIGUOUS) < 0)
        goto error;
    if (make_dset(loc_id, "dset_contiguous", sid, dcpl, buf) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * H5D_CHUNKED
     *-------------------------------------------------------------------------
     */
    if (H5Pset_chunk(dcpl, RANK, chunk_dims) < 0)
        goto error;
    if (make_dset(loc_id, "dset_chunk", sid, dcpl, buf) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * close space and dcpl
     *-------------------------------------------------------------------------
     */
    if (H5Sclose(sid) < 0)
        goto error;
    if (H5Pclose(dcpl) < 0)
        goto error;

    free(buf);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Pclose(dcpl);
        H5Sclose(sid);
    }
    H5E_END_TRY

    free(buf);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_layout2
 *
 * Purpose: create datasets with contiguous and chunked layouts:
 *
 *  contig_small: < 1k, fixed dims datspace
 *  chunked_small_fixed: < 1k, fixed dims dataspace
 *
 *-------------------------------------------------------------------------
 */
#define S_DIM1        4
#define S_DIM2        10
#define CONTIG_S      "contig_small"
#define CHUNKED_S_FIX "chunked_small_fixed"

static int
make_layout2(hid_t loc_id)
{

    hid_t contig_dcpl  = H5I_INVALID_HID; /* dataset creation property list */
    hid_t chunked_dcpl = H5I_INVALID_HID; /* dataset creation property list */

    int   ret_value = -1;              /* Return value */
    hid_t s_sid     = H5I_INVALID_HID; /* dataspace ID */

    hsize_t s_dims[RANK]     = {S_DIM1, S_DIM2};         /* Dataspace (< 1 k) */
    hsize_t chunk_dims[RANK] = {S_DIM1 / 2, S_DIM2 / 2}; /* Dimension sizes for chunks */

    /* Create and fill array */
    struct {
        int arr[S_DIM1][S_DIM2];
    } *s_buf = malloc(sizeof(*s_buf));
    if (NULL == s_buf)
        goto error;
    H5TEST_FILL_2D_HEAP_ARRAY(s_buf, int);

    /* Create dataspaces */
    if ((s_sid = H5Screate_simple(RANK, s_dims, NULL)) < 0)
        goto error;

    /* Create contiguous datasets */
    if ((contig_dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    if (H5Pset_layout(contig_dcpl, H5D_CONTIGUOUS) < 0)
        goto error;
    if (make_dset(loc_id, CONTIG_S, s_sid, contig_dcpl, s_buf) < 0)
        goto error;

    /* Create chunked datasets */
    if ((chunked_dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;
    if (H5Pset_chunk(chunked_dcpl, RANK, chunk_dims) < 0)
        goto error;
    if (make_dset(loc_id, CHUNKED_S_FIX, s_sid, chunked_dcpl, s_buf) < 0)
        goto error;

    ret_value = 0;

error:
    H5E_BEGIN_TRY
    {
        H5Pclose(contig_dcpl);
        H5Pclose(chunked_dcpl);

        H5Sclose(s_sid);
    }
    H5E_END_TRY

    free(s_buf);

    return (ret_value);

} /* make_layout2() */

/*-------------------------------------------------------------------------
 * Function: make_layout3
 *
 * Purpose: make chunked datasets with unlimited max dim and chunk dim is
 *          bigger than current dim. (HDFFV-7933)
 *          Test for converting chunk to chunk , chunk to conti and chunk
 *          to compact.
 *          - The chunk to chunk changes layout bigger than any current dim
 *            again.
 *          - The chunk to compact test dataset bigger than 64K, should
 *            remain original layout.*
 *
 *-------------------------------------------------------------------------
 */
#define DIM1_L3 300
#define DIM2_L3 200
/* small size */
#define SDIM1_L3 4
#define SDIM2_L3 50
static int
make_layout3(hid_t loc_id)
{
    hid_t   dcpl1             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   dcpl2             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   dcpl3             = H5I_INVALID_HID; /* dataset creation property list */
    hid_t   sid1              = H5I_INVALID_HID; /* dataspace ID */
    hid_t   sid2              = H5I_INVALID_HID; /* dataspace ID */
    hsize_t dims1[RANK]       = {DIM1_L3, DIM2_L3};
    hsize_t dims2[RANK]       = {SDIM1_L3, SDIM2_L3};
    hsize_t maxdims[RANK]     = {H5S_UNLIMITED, H5S_UNLIMITED};
    hsize_t chunk_dims1[RANK] = {DIM1_L3 * 2, 5};
    hsize_t chunk_dims2[RANK] = {SDIM1_L3 + 2, SDIM2_L3 / 2};
    hsize_t chunk_dims3[RANK] = {SDIM1_L3 - 2, SDIM2_L3 / 2};

    /* Create arrays */
    struct {
        int arr[DIM1_L3][DIM2_L3];
    } *buf1 = malloc(sizeof(*buf1));
    struct {
        int arr[SDIM1_L3][SDIM2_L3];
    } *buf2 = malloc(sizeof(*buf2));

    if (NULL == buf1 || NULL == buf2)
        goto error;

    /* Fill arrays */
    H5TEST_FILL_2D_HEAP_ARRAY(buf1, int);
    H5TEST_FILL_2D_HEAP_ARRAY(buf2, int);

    /*-------------------------------------------------------------------------
     * make chunked dataset with
     *  - dset maxdims are UNLIMIT
     *  - a chunk dim is bigger than dset dim
     *  - dset size bigger than compact max (64K)
     *-------------------------------------------------------------------------
     */
    /* create a space */
    if ((sid1 = H5Screate_simple(RANK, dims1, maxdims)) < 0)
        goto error;
    /* create a dataset creation property list; the same DCPL is used for all dsets */
    if ((dcpl1 = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;

    if (H5Pset_chunk(dcpl1, RANK, chunk_dims1) < 0)
        goto error;
    if (make_dset(loc_id, "chunk_unlimit1", sid1, dcpl1, buf1) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * make chunked dataset with
     *  - dset maxdims are UNLIMIT
     *  - a chunk dim is bigger than dset dim
     *  - dset size smaller than compact (64K)
     *-------------------------------------------------------------------------
     */

    /* create a space */
    if ((sid2 = H5Screate_simple(RANK, dims2, maxdims)) < 0)
        goto error;
    /* create a dataset creation property list; the same DCPL is used for all dsets */
    if ((dcpl2 = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;

    if (H5Pset_chunk(dcpl2, RANK, chunk_dims2) < 0)
        goto error;

    if (make_dset(loc_id, "chunk_unlimit2", sid2, dcpl2, buf2) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * make chunked dataset with
     *  - dset maxdims are UNLIMIT
     *  - a chunk dims are smaller than dset dims
     *  - dset size smaller than compact (64K)
     *-------------------------------------------------------------------------
     */
    /* create a dataset creation property list; the same DCPL is used for all dsets */
    if ((dcpl3 = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto error;

    if (H5Pset_chunk(dcpl3, RANK, chunk_dims3) < 0)
        goto error;

    if (make_dset(loc_id, "chunk_unlimit3", sid2, dcpl3, buf2) < 0)
        goto error;

    /*-------------------------------------------------------------------------
     * close space and dcpl
     *-------------------------------------------------------------------------
     */
    if (H5Sclose(sid1) < 0)
        goto error;
    if (H5Sclose(sid2) < 0)
        goto error;
    if (H5Pclose(dcpl1) < 0)
        goto error;
    if (H5Pclose(dcpl2) < 0)
        goto error;
    if (H5Pclose(dcpl3) < 0)
        goto error;

    free(buf1);
    free(buf2);

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Sclose(sid1);
        H5Sclose(sid2);
        H5Pclose(dcpl1);
        H5Pclose(dcpl2);
        H5Pclose(dcpl3);
    }
    H5E_END_TRY

    free(buf1);
    free(buf2);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make a file with an integer dataset with a fill value
 *
 * Purpose: test copy of fill values
 *
 *-------------------------------------------------------------------------
 */
static int
make_fill(hid_t loc_id)
{
    hid_t   did = H5I_INVALID_HID;
    hid_t   sid = H5I_INVALID_HID;
    hid_t   dcpl;
    hsize_t dims[2]   = {3, 2};
    int     buf[3][2] = {{1, 1}, {1, 2}, {2, 2}};
    int     fillvalue = 2;

    /*-------------------------------------------------------------------------
     * H5T_INTEGER, write a fill value
     *-------------------------------------------------------------------------
     */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto out;
    if (H5Pset_fill_value(dcpl, H5T_NATIVE_INT, &fillvalue) < 0)
        goto out;
    if ((sid = H5Screate_simple(2, dims, NULL)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "dset_fill", H5T_NATIVE_INT, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Dwrite(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) < 0)
        goto out;

    /* close */
    if (H5Sclose(sid) < 0)
        goto out;
    if (H5Pclose(dcpl) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Pclose(dcpl);
        H5Sclose(sid);
        H5Dclose(did);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_big
 *
 * Purpose: used in test read by hyperslabs. Creates a 128MB dataset.
 *  Only 1 1024Kb hyperslab is written.
 *
 *-------------------------------------------------------------------------
 */
static int
make_big(hid_t loc_id)
{
    hid_t        did   = H5I_INVALID_HID;
    hid_t        f_sid = H5I_INVALID_HID;
    hid_t        m_sid = H5I_INVALID_HID;
    hid_t        tid;
    hid_t        dcpl;
    hsize_t      dims[1] = {H5TOOLS_MALLOCSIZE + 1}; /* dataset dimensions */
    hsize_t      hs_size[1];                         /* hyperslab dimensions */
    hsize_t      hs_start[1];                        /* hyperslab start */
    hsize_t      chunk_dims[1] = {1024};             /* chunk dimensions */
    size_t       size;
    size_t       nelmts    = (size_t)1024;
    signed char  fillvalue = -1;
    signed char *buf       = NULL;

    /* write one 1024 byte hyperslab */
    hs_start[0] = 0;
    hs_size[0]  = 1024;

    /* create */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto out;
    if (H5Pset_fill_value(dcpl, H5T_NATIVE_SCHAR, &fillvalue) < 0)
        goto out;
    if (H5Pset_chunk(dcpl, 1, chunk_dims) < 0)
        goto out;
    if ((f_sid = H5Screate_simple(1, dims, NULL)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "dset", H5T_NATIVE_SCHAR, f_sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
        goto out;
    if ((m_sid = H5Screate_simple(1, hs_size, hs_size)) < 0)
        goto out;
    if ((tid = H5Dget_type(did)) < 0)
        goto out;
    if ((size = H5Tget_size(tid)) <= 0)
        goto out;

    /* initialize buffer to 0  */
    buf = (signed char *)calloc(nelmts, size);

    if (H5Sselect_hyperslab(f_sid, H5S_SELECT_SET, hs_start, NULL, hs_size, NULL) < 0)
        goto out;
    if (H5Dwrite(did, H5T_NATIVE_SCHAR, m_sid, f_sid, H5P_DEFAULT, buf) < 0)
        goto out;

    free(buf);
    buf = NULL;

    /* close */
    if (H5Sclose(f_sid) < 0)
        goto out;
    if (H5Sclose(m_sid) < 0)
        goto out;
    if (H5Pclose(dcpl) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Pclose(dcpl);
        H5Sclose(f_sid);
        H5Sclose(m_sid);
        H5Dclose(did);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_external
 *
 * Purpose: create a external dataset
 *
 *-------------------------------------------------------------------------
 */
static int
make_external(hid_t loc_id)
{
    hid_t   did = H5I_INVALID_HID;
    hid_t   sid = H5I_INVALID_HID;
    hid_t   dcpl;
    int     buf[2] = {1, 2};
    hsize_t cur_size[1]; /* data space current size  */
    hsize_t max_size[1]; /* data space maximum size  */
    hsize_t size;

    cur_size[0] = max_size[0] = 2;
    size                      = max_size[0] * sizeof(int);

    /* create */
    if ((dcpl = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto out;
    if (H5Pset_external(dcpl, H5REPACK_EXTFILE, 0, size) < 0)
        goto out;
    if ((sid = H5Screate_simple(1, cur_size, max_size)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "external", H5T_NATIVE_INT, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Dwrite(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf) < 0)
        goto out;

    /* close */
    if (H5Sclose(sid) < 0)
        goto out;
    if (H5Pclose(dcpl) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Pclose(dcpl);
        H5Sclose(sid);
        H5Dclose(did);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_userblock
 *
 * Purpose: create a file for the userblock copying test
 *
 *-------------------------------------------------------------------------
 */
static int
make_userblock(void)
{
    hid_t                         fid  = H5I_INVALID_HID;
    hid_t                         fcpl = H5I_INVALID_HID;
    int                           fd   = -1;          /* File descriptor for writing userblock */
    char                          ub[USERBLOCK_SIZE]; /* User block data */
    ssize_t H5_ATTR_NDEBUG_UNUSED nwritten;           /* # of bytes written */
    size_t                        u;                  /* Local index variable */

    /* Create file creation property list with userblock set */
    if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
        goto out;
    if (H5Pset_userblock(fcpl, (hsize_t)USERBLOCK_SIZE) < 0)
        goto out;

    /* Create file with userblock */
    if ((fid = H5Fcreate(H5REPACK_FNAME16, H5F_ACC_TRUNC, fcpl, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Fclose(fid) < 0)
        goto out;

    /* Close file creation property list */
    if (H5Pclose(fcpl) < 0)
        goto out;

    /* Initialize userblock data */
    for (u = 0; u < USERBLOCK_SIZE; u++)
        ub[u] = (char)('a' + (char)(u % 26));

    /* Re-open HDF5 file, as "plain" file */
    if ((fd = HDopen(H5REPACK_FNAME16, O_WRONLY)) < 0)
        goto out;

    /* Write userblock data */
    nwritten = HDwrite(fd, ub, (size_t)USERBLOCK_SIZE);
    assert(nwritten == USERBLOCK_SIZE);

    /* Close file */
    HDclose(fd);

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Pclose(fcpl);
        H5Fclose(fid);
    }
    H5E_END_TRY
    if (fd >= 0)
        HDclose(fd);

    return -1;
} /* end make_userblock() */

/*-------------------------------------------------------------------------
 * Function: verify_userblock
 *
 * Purpose: Verify that the userblock was copied correctly
 *
 *-------------------------------------------------------------------------
 */
int
verify_userblock(const char *filename)
{
    hid_t                         fid  = H5I_INVALID_HID;
    hid_t                         fcpl = H5I_INVALID_HID;
    int                           fd   = -1;          /* File descriptor for writing userblock */
    char                          ub[USERBLOCK_SIZE]; /* User block data */
    hsize_t                       ub_size = 0;        /* User block size */
    ssize_t H5_ATTR_NDEBUG_UNUSED nread;              /* # of bytes read */
    size_t                        u;                  /* Local index variable */

    /* Open file with userblock */
    if ((fid = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT)) < 0)
        goto out;

    /* Retrieve file creation property list & userblock size */
    if ((fcpl = H5Fget_create_plist(fid)) < 0)
        goto out;
    if (H5Pget_userblock(fcpl, &ub_size) < 0)
        goto out;

    /* Verify userblock size is correct */
    if (ub_size != USERBLOCK_SIZE)
        goto out;

    /* Close file creation property list */
    if (H5Pclose(fcpl) < 0)
        goto out;

    if (H5Fclose(fid) < 0)
        goto out;

    /* Re-open HDF5 file, as "plain" file */
    if ((fd = HDopen(filename, O_RDONLY)) < 0)
        goto out;

    /* Read userblock data */
    nread = HDread(fd, ub, (size_t)USERBLOCK_SIZE);
    assert(nread == USERBLOCK_SIZE);

    /* Verify userblock data */
    for (u = 0; u < USERBLOCK_SIZE; u++)
        if (ub[u] != (char)('a' + (u % 26)))
            goto out;

    /* Close file */
    HDclose(fd);

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Pclose(fcpl);
        H5Fclose(fid);
    }
    H5E_END_TRY
    if (fd >= 0)
        HDclose(fd);

    return -1;
} /* end verify_userblock() */

/*-------------------------------------------------------------------------
 * Function: make_userblock_file
 *
 * Purpose: create a file for the userblock add test
 *
 *-------------------------------------------------------------------------
 */
static int
make_userblock_file(void)
{
    int                           fd = -1;            /* File descriptor for writing userblock */
    char                          ub[USERBLOCK_SIZE]; /* User block data */
    ssize_t H5_ATTR_NDEBUG_UNUSED nwritten;           /* # of bytes written */
    size_t                        u;                  /* Local index variable */

    /* initialize userblock data */
    for (u = 0; u < USERBLOCK_SIZE; u++)
        ub[u] = (char)('a' + (char)(u % 26));

    /* open file */
    if ((fd = HDopen(H5REPACK_FNAME_UB, O_WRONLY | O_CREAT | O_TRUNC, H5_POSIX_CREATE_MODE_RW)) < 0)
        goto out;

    /* write userblock data */
    nwritten = HDwrite(fd, ub, (size_t)USERBLOCK_SIZE);
    assert(nwritten == USERBLOCK_SIZE);

    /* close file */
    HDclose(fd);

    return 0;

out:

    if (fd >= 0)
        HDclose(fd);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: write_dset_in
 *
 * Purpose: write datasets in LOC_ID
 *
 *-------------------------------------------------------------------------
 */
static int
write_dset_in(hid_t loc_id, const char *dset_name, /* for saving reference to dataset*/
              hid_t file_id, int make_diffs /* flag to modify data buffers */)
{
    /* compound datatype */
    typedef struct s_t {
        char   a;
        double b;
    } s_t;

    typedef enum { RED, GREEN } e_t;

    hid_t    did = H5I_INVALID_HID;
    hid_t    sid = H5I_INVALID_HID;
    hid_t    tid = H5I_INVALID_HID;
    hid_t    pid = H5I_INVALID_HID;
    unsigned i, j;
    int      val, k, n;
    float    f;

    /* create 1D attributes with dimension [2], 2 elements */
    hsize_t    dims[1]    = {2};
    hsize_t    dims1r[1]  = {2};
    char       buf1[2][3] = {"ab", "de"};            /* string */
    char       buf2[2]    = {1, 2};                  /* bitfield, opaque */
    s_t        buf3[2]    = {{1, 2}, {3, 4}};        /* compound */
    hobj_ref_t buf4[2];                              /* reference */
    e_t        buf45[2] = {RED, GREEN};              /* enum */
    hvl_t      buf5[2];                              /* vlen */
    hsize_t    dimarray[1] = {3};                    /* array dimension */
    int        buf6[2][3]  = {{1, 2, 3}, {4, 5, 6}}; /* array */
    int        buf7[2]     = {1, 2};                 /* integer */
    float      buf8[2]     = {1, 2};                 /* float */
    float      buf9[4]     = {1, 2, 3, 4};           /* complex */

    /* create 2D attributes with dimension [3][2], 6 elements */
    hsize_t    dims2[2]    = {3, 2};
    hsize_t    dims2r[2]   = {1, 1};
    char       buf12[6][3] = {"ab", "cd", "ef", "gh", "ij", "kl"};                /* string */
    char       buf22[3][2] = {{1, 2}, {3, 4}, {5, 6}};                            /* bitfield, opaque */
    s_t        buf32[6]    = {{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}, {11, 12}}; /* compound */
    hobj_ref_t buf42[1][1];                                                       /* reference */
    hvl_t      buf52[3][2];                                                       /* vlen */
    int buf62[6][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}, {13, 14, 15}, {16, 17, 18}}; /* array */
    int buf72[3][2] = {{1, 2}, {3, 4}, {5, 6}};                              /* integer */
    float buf82[3][2] = {{1, 2}, {3, 4}, {5, 6}};                            /* float */
    float buf92[6][2] = {{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}, {11, 12}}; /* complex */

    /* create 3D attributes with dimension [4][3][2], 24 elements */
    hsize_t dims3[3]     = {4, 3, 2};
    hsize_t dims3r[3]    = {1, 1, 1};
    char    buf13[24][2] = {
        "ab", "cd", "ef", "gh", "ij", "kl", "mn", "pq", "rs", "tu", "vw", "xz", "AB",
        "CD", "EF", "GH", "IJ", "KL", "MN", "PQ", "RS", "TU", "VW", "XZ"}; /* string, NO NUL fixed length */
    char       buf23[4][3][2];                                                /* bitfield, opaque */
    s_t        buf33[4][3][2];                                                /* compound */
    hobj_ref_t buf43[1][1][1];                                                /* reference */
    hvl_t      buf53[4][3][2];                                                /* vlen */
    int        buf63[24][3];                                                  /* array */
    int        buf73[4][3][2];                                                /* integer */
    float      buf83[4][3][2];                                                /* float */
    float      buf93[24][2];                                                  /* complex */

    /*-------------------------------------------------------------------------
     * 1D
     *-------------------------------------------------------------------------
     */

    /*-------------------------------------------------------------------------
     * H5T_STRING
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++)
            for (j = 0; j < 2; j++)
                buf1[i][j] = 'z';
    }

    if ((tid = H5Tcopy(H5T_C_S1)) < 0)
        goto out;
    if (H5Tset_size(tid, (size_t)2) < 0)
        goto out;
    if (write_dset(loc_id, 1, dims, "string", tid, buf1) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /* create hard link */
    if (H5Lcreate_hard(loc_id, "string", H5L_SAME_LOC, "string_link", H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_BITFIELD
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++)
            buf2[i] = buf2[1] = 0;
    }

    if ((tid = H5Tcopy(H5T_STD_B8LE)) < 0)
        goto out;
    if (write_dset(loc_id, 1, dims, "bitfield", tid, buf2) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_OPAQUE
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++) {
            buf3[i].a = 0;
            buf3[i].b = 0;
        }
    }

    if ((tid = H5Tcreate(H5T_OPAQUE, (size_t)1)) < 0)
        goto out;
    if (H5Tset_tag(tid, "1-byte opaque type") < 0)
        goto out;
    if (write_dset(loc_id, 1, dims, "opaque", tid, buf2) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_COMPOUND
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++)
            buf45[i] = GREEN;
    }

    if ((tid = H5Tcreate(H5T_COMPOUND, sizeof(s_t))) < 0)
        goto out;
    if (H5Tinsert(tid, "a", HOFFSET(s_t, a), H5T_NATIVE_CHAR) < 0)
        goto out;
    if (H5Tinsert(tid, "b", HOFFSET(s_t, b), H5T_NATIVE_DOUBLE) < 0)
        goto out;
    if (write_dset(loc_id, 1, dims, "compound", tid, buf3) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_REFERENCE
     *-------------------------------------------------------------------------
     */
    /* object references ( H5R_OBJECT ) */
    buf4[0] = 0;
    buf4[1] = 0;
    if (dset_name) {
        if (H5Rcreate(&buf4[0], file_id, dset_name, H5R_OBJECT, (hid_t)-1) < 0)
            goto out;
        if (write_dset(loc_id, 1, dims1r, "refobj", H5T_STD_REF_OBJ, buf4) < 0)
            goto out;
    }

    /* Dataset region reference ( H5R_DATASET_REGION  )  */
    if (make_dset_reg_ref(loc_id) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_ENUM
     *-------------------------------------------------------------------------
     */
    if ((tid = H5Tcreate(H5T_ENUM, sizeof(e_t))) < 0)
        goto out;
    val = 0;
    if (H5Tenum_insert(tid, "RED", &val) < 0)
        goto out;
    val = 1;
    if (H5Tenum_insert(tid, "GREEN", &val) < 0)
        goto out;
    if (write_dset(loc_id, 1, dims, "enum", tid, buf45) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_VLEN
     *-------------------------------------------------------------------------
     */

    /* Allocate and initialize VL dataset to write */

    buf5[0].len           = 1;
    buf5[0].p             = malloc(1 * sizeof(int));
    ((int *)buf5[0].p)[0] = 1;
    buf5[1].len           = 2;
    buf5[1].p             = malloc(2 * sizeof(int));
    ((int *)buf5[1].p)[0] = 2;
    ((int *)buf5[1].p)[1] = 3;

    if (make_diffs) {
        ((int *)buf5[0].p)[0] = 0;
        ((int *)buf5[1].p)[0] = 0;
        ((int *)buf5[1].p)[1] = 0;
    }

    if ((sid = H5Screate_simple(1, dims, NULL)) < 0)
        goto out;
    if ((tid = H5Tvlen_create(H5T_NATIVE_INT)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "vlen", tid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Dwrite(did, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf5) < 0)
        goto out;
    if (H5Treclaim(tid, sid, H5P_DEFAULT, buf5) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    if (H5Sclose(sid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_ARRAY
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++)
            for (j = 0; j < 3; j++)
                buf6[i][j] = 0;
    }

    if ((tid = H5Tarray_create2(H5T_NATIVE_INT, 1, dimarray)) < 0)
        goto out;
    if (write_dset(loc_id, 1, dims, "array", tid, buf6) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    {

        hsize_t  TEST_BUFSIZE = (128 * 1024 * 1024); /* 128MB */
        double  *dbuf;                               /* information to write */
        size_t   size;
        hsize_t  sdims[] = {1};
        hsize_t  tdims[] = {TEST_BUFSIZE / sizeof(double) + 1};
        unsigned u;

        /* allocate and initialize array data to write */
        size = (TEST_BUFSIZE / sizeof(double) + 1) * sizeof(double);
        dbuf = (double *)malloc(size);
        if (NULL == dbuf) {
            printf("\nError: Cannot allocate memory for \"arrayd\" data buffer size %dMB.\n",
                   (int)size / 1000000);
            goto out;
        }

        for (u = 0; u < TEST_BUFSIZE / sizeof(double) + 1; u++)
            dbuf[u] = u;

        if (make_diffs) {
            dbuf[5] = 0;
            dbuf[6] = 0;
        }

        /* create a type larger than TEST_BUFSIZE */
        if ((tid = H5Tarray_create2(H5T_NATIVE_DOUBLE, 1, tdims)) < 0) {
            free(dbuf);
            goto out;
        }
        size = H5Tget_size(tid);
        if ((sid = H5Screate_simple(1, sdims, NULL)) < 0) {
            free(dbuf);
            goto out;
        }
        if ((did = H5Dcreate2(loc_id, "arrayd", tid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
            free(dbuf);
            goto out;
        }
#if defined(WRITE_ARRAY)
        H5Dwrite(did, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, dbuf);
#endif

        /* close */
        H5Dclose(did);
        H5Tclose(tid);
        H5Sclose(sid);
        free(dbuf);
    }

    /*-------------------------------------------------------------------------
     * H5T_INTEGER and H5T_FLOAT
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++) {
            buf7[i] = 0;
            buf8[i] = 0;
        }
    }

    if (write_dset(loc_id, 1, dims, "integer", H5T_NATIVE_INT, buf7) < 0)
        goto out;
    if (write_dset(loc_id, 1, dims, "float", H5T_NATIVE_FLOAT, buf8) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_COMPLEX
     *-------------------------------------------------------------------------
     */

    if (make_diffs)
        memset(buf9, 0, sizeof(buf9));

    if ((tid = H5Tcomplex_create(H5T_NATIVE_FLOAT)) < 0)
        goto out;
    if (write_dset(loc_id, 1, dims, "complex", tid, buf9) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * 2D
     *-------------------------------------------------------------------------
     */

    /*-------------------------------------------------------------------------
     * H5T_STRING
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf12, 'z', sizeof buf12);
    }

    if ((tid = H5Tcopy(H5T_C_S1)) < 0)
        goto out;
    if (H5Tset_size(tid, (size_t)2) < 0)
        goto out;
    if (write_dset(loc_id, 2, dims2, "string2D", tid, buf12) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_BITFIELD
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf22, 0, sizeof buf22);
    }

    if ((tid = H5Tcopy(H5T_STD_B8LE)) < 0)
        goto out;
    if (write_dset(loc_id, 2, dims2, "bitfield2D", tid, buf22) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_OPAQUE
     *-------------------------------------------------------------------------
     */
    if ((tid = H5Tcreate(H5T_OPAQUE, (size_t)1)) < 0)
        goto out;
    if (H5Tset_tag(tid, "1-byte opaque type") < 0)
        goto out;
    if (write_dset(loc_id, 2, dims2, "opaque2D", tid, buf22) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_COMPOUND
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf32, 0, sizeof buf32);
    }

    if ((tid = H5Tcreate(H5T_COMPOUND, sizeof(s_t))) < 0)
        goto out;
    if (H5Tinsert(tid, "a", HOFFSET(s_t, a), H5T_NATIVE_CHAR) < 0)
        goto out;
    if (H5Tinsert(tid, "b", HOFFSET(s_t, b), H5T_NATIVE_DOUBLE) < 0)
        goto out;
    if (write_dset(loc_id, 2, dims2, "compound2D", tid, buf32) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_REFERENCE
     *-------------------------------------------------------------------------
     */
    /* Create references to dataset */
    if (dset_name) {
        if (H5Rcreate(&buf42[0][0], file_id, dset_name, H5R_OBJECT, (hid_t)-1) < 0)
            goto out;
        if (write_dset(loc_id, 2, dims2r, "refobj2D", H5T_STD_REF_OBJ, buf42) < 0)
            goto out;
    }

    /*-------------------------------------------------------------------------
     * H5T_ENUM
     *-------------------------------------------------------------------------
     */

    if ((tid = H5Tcreate(H5T_ENUM, sizeof(e_t))) < 0)
        goto out;
    val = 0;
    if (H5Tenum_insert(tid, "RED", &val) < 0)
        goto out;
    val = 1;
    if (H5Tenum_insert(tid, "GREEN", &val) < 0)
        goto out;
    if (write_dset(loc_id, 2, dims2, "enum2D", tid, 0) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_VLEN
     *-------------------------------------------------------------------------
     */

    /* Allocate and initialize VL dataset to write */
    n = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 2; j++) {
            unsigned l;

            buf52[i][j].p   = malloc((i + 1) * sizeof(int));
            buf52[i][j].len = (size_t)(i + 1);
            for (l = 0; l < i + 1; l++) {
                if (make_diffs)
                    ((int *)buf52[i][j].p)[l] = 0;
                else
                    ((int *)buf52[i][j].p)[l] = n++;
            }
        }
    }

    if ((sid = H5Screate_simple(2, dims2, NULL)) < 0)
        goto out;
    if ((tid = H5Tvlen_create(H5T_NATIVE_INT)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "vlen2D", tid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Dwrite(did, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf52) < 0)
        goto out;
    if (H5Treclaim(tid, sid, H5P_DEFAULT, buf52) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    if (H5Sclose(sid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_ARRAY
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf62, 0, sizeof buf62);
    }

    if ((tid = H5Tarray_create2(H5T_NATIVE_INT, 1, dimarray)) < 0)
        goto out;
    if (write_dset(loc_id, 2, dims2, "array2D", tid, buf62) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_INTEGER, write a fill value
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf72, 0, sizeof buf72);
        memset(buf82, 0, sizeof buf82);
    }

    if ((pid = H5Pcreate(H5P_DATASET_CREATE)) < 0)
        goto out;
    if ((sid = H5Screate_simple(2, dims2, NULL)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "integer2D", H5T_NATIVE_INT, sid, H5P_DEFAULT, pid, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Dwrite(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf72) < 0)
        goto out;
    if (H5Pclose(pid) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;
    if (H5Sclose(sid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_FLOAT
     *-------------------------------------------------------------------------
     */

    if (write_dset(loc_id, 2, dims2, "float2D", H5T_NATIVE_FLOAT, buf82) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_COMPLEX
     *-------------------------------------------------------------------------
     */

    if (make_diffs)
        memset(buf92, 0, sizeof(buf92));

    if ((tid = H5Tcomplex_create(H5T_NATIVE_FLOAT)) < 0)
        goto out;
    if (write_dset(loc_id, 2, dims2, "complex2D", tid, buf92) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * 3D
     *-------------------------------------------------------------------------
     */

    /*-------------------------------------------------------------------------
     * H5T_STRING
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf13, 'z', sizeof buf13);
    }

    if ((tid = H5Tcopy(H5T_C_S1)) < 0)
        goto out;
    if (H5Tset_size(tid, (size_t)2) < 0)
        goto out;
    if (write_dset(loc_id, 3, dims3, "string3D", tid, buf13) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_BITFIELD
     *-------------------------------------------------------------------------
     */

    n = 1;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                if (make_diffs)
                    buf23[i][j][k] = 0;
                else
                    buf23[i][j][k] = (char)(n++);
            }
        }
    }

    if ((tid = H5Tcopy(H5T_STD_B8LE)) < 0)
        goto out;
    if (write_dset(loc_id, 3, dims3, "bitfield3D", tid, buf23) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_OPAQUE
     *-------------------------------------------------------------------------
     */
    if ((tid = H5Tcreate(H5T_OPAQUE, (size_t)1)) < 0)
        goto out;
    if (H5Tset_tag(tid, "1-byte opaque type") < 0)
        goto out;
    if (write_dset(loc_id, 3, dims3, "opaque3D", tid, buf23) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_COMPOUND
     *-------------------------------------------------------------------------
     */

    n = 1;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                if (make_diffs) {
                    buf33[i][j][k].a = 0;
                    buf33[i][j][k].b = 0;
                }
                else {
                    buf33[i][j][k].a = (char)(n++);
                    buf33[i][j][k].b = n++;
                }
            }
        }
    }

    if ((tid = H5Tcreate(H5T_COMPOUND, sizeof(s_t))) < 0)
        goto out;
    if (H5Tinsert(tid, "a", HOFFSET(s_t, a), H5T_NATIVE_CHAR) < 0)
        goto out;
    if (H5Tinsert(tid, "b", HOFFSET(s_t, b), H5T_NATIVE_DOUBLE) < 0)
        goto out;
    if (write_dset(loc_id, 3, dims3, "compound3D", tid, buf33) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_REFERENCE
     *-------------------------------------------------------------------------
     */
    /* Create references to dataset */
    if (dset_name) {
        if (H5Rcreate(&buf43[0][0][0], file_id, dset_name, H5R_OBJECT, (hid_t)-1) < 0)
            goto out;
        if (write_dset(loc_id, 3, dims3r, "refobj3D", H5T_STD_REF_OBJ, buf43) < 0)
            goto out;
    }

    /*-------------------------------------------------------------------------
     * H5T_ENUM
     *-------------------------------------------------------------------------
     */

    if ((tid = H5Tcreate(H5T_ENUM, sizeof(e_t))) < 0)
        goto out;
    val = 0;
    if (H5Tenum_insert(tid, "RED", &val) < 0)
        goto out;
    val = 1;
    if (H5Tenum_insert(tid, "GREEN", &val) < 0)
        goto out;
    if (write_dset(loc_id, 3, dims3, "enum3D", tid, 0) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_VLEN
     *-------------------------------------------------------------------------
     */

    /* Allocate and initialize VL dataset to write */
    n = 0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                unsigned l;

                buf53[i][j][k].p   = malloc((i + 1) * sizeof(int));
                buf53[i][j][k].len = (size_t)(i + 1);
                for (l = 0; l < i + 1; l++) {
                    if (make_diffs)
                        ((int *)buf53[i][j][k].p)[l] = 0;
                    else
                        ((int *)buf53[i][j][k].p)[l] = n++;
                }
            }
        }
    }

    if ((sid = H5Screate_simple(3, dims3, NULL)) < 0)
        goto out;
    if ((tid = H5Tvlen_create(H5T_NATIVE_INT)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "vlen3D", tid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Dwrite(did, tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf53) < 0)
        goto out;

    if (H5Treclaim(tid, sid, H5P_DEFAULT, buf53) < 0)
        goto out;

    if (H5Dclose(did) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    if (H5Sclose(sid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_ARRAY
     *-------------------------------------------------------------------------
     */

    n = 1;
    for (i = 0; i < 24; i++) {
        for (j = 0; j < dimarray[0]; j++) {
            if (make_diffs)
                buf63[i][j] = 0;
            else
                buf63[i][j] = n++;
        }
    }

    if ((tid = H5Tarray_create2(H5T_NATIVE_INT, 1, dimarray)) < 0)
        goto out;
    if (write_dset(loc_id, 3, dims3, "array3D", tid, buf63) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_INTEGER and H5T_FLOAT
     *-------------------------------------------------------------------------
     */
    n = 1;
    f = 1;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                if (make_diffs) {
                    buf73[i][j][k] = 0;
                    buf83[i][j][k] = 0;
                }
                else {
                    buf73[i][j][k] = n++;
                    buf83[i][j][k] = f++;
                }
            }
        }
    }
    if (write_dset(loc_id, 3, dims3, "integer3D", H5T_NATIVE_INT, buf73) < 0)
        goto out;
    if (write_dset(loc_id, 3, dims3, "float3D", H5T_NATIVE_FLOAT, buf83) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * H5T_COMPLEX
     *-------------------------------------------------------------------------
     */

    f = 1;
    for (i = 0; i < 24; i++)
        for (j = 0; j < 2; j++) {
            if (make_diffs)
                buf93[i][j] = 0;
            else
                buf93[i][j] = f++;
        }

    if ((tid = H5Tcomplex_create(H5T_NATIVE_FLOAT)) < 0)
        goto out;
    if (write_dset(loc_id, 3, dims3, "complex3D", tid, buf93) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Pclose(pid);
        H5Sclose(sid);
        H5Dclose(did);
        H5Tclose(tid);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_dset_reg_ref
 *
 * Purpose: write dataset region references
 *
 *-------------------------------------------------------------------------
 */

#define SPACE1_RANK 1
#define SPACE1_DIM1 1
#define SPACE2_RANK 2
#define SPACE2_DIM1 10
#define SPACE2_DIM2 10
static int
make_dset_reg_ref(hid_t loc_id)
{
    hid_t            did1    = H5I_INVALID_HID; /* Dataset ID   */
    hid_t            did2    = H5I_INVALID_HID; /* Dereferenced dataset ID */
    hid_t            sid1    = H5I_INVALID_HID; /* Dataspace ID #1  */
    hid_t            sid2    = H5I_INVALID_HID; /* Dataspace ID #2  */
    hsize_t          dims1[] = {SPACE1_DIM1};
    hsize_t          dims2[] = {SPACE2_DIM1, SPACE2_DIM2};
    hsize_t          start[SPACE2_RANK];  /* Starting location of hyperslab */
    hsize_t          stride[SPACE2_RANK]; /* Stride of hyperslab */
    hsize_t          count[SPACE2_RANK];  /* Element count of hyperslab */
    hsize_t          block[SPACE2_RANK];  /* Block size of hyperslab */
    hdset_reg_ref_t *wbuf  = NULL;        /* buffer to write to disk */
    int             *dwbuf = NULL;        /* Buffer for writing numeric data to disk */
    int              i;                   /* counting variables */
    int              retval = -1;         /* return value */

    /* Allocate write & read buffers */
    wbuf  = (hdset_reg_ref_t *)calloc(SPACE1_DIM1, sizeof(hdset_reg_ref_t));
    dwbuf = (int *)malloc((SPACE2_DIM1 * SPACE2_DIM2) * sizeof(int));

    /* Create dataspace for datasets */
    if ((sid2 = H5Screate_simple(SPACE2_RANK, dims2, NULL)) < 0)
        goto out;

    /* Create a dataset */
    if ((did2 = H5Dcreate2(loc_id, "dsetreg", H5T_NATIVE_UCHAR, sid2, H5P_DEFAULT, H5P_DEFAULT,
                           H5P_DEFAULT)) < 0)
        goto out;

    for (i = 0; i < SPACE2_DIM1 * SPACE2_DIM2; i++)
        dwbuf[i] = i * 3;

    /* Write selection to disk */
    if (H5Dwrite(did2, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dwbuf) < 0)
        goto out;

    /* Create dataspace for the reference dataset */
    if ((sid1 = H5Screate_simple(SPACE1_RANK, dims1, NULL)) < 0)
        goto out;

    /* Create a dataset */
    if ((did1 = H5Dcreate2(loc_id, "refreg", H5T_STD_REF_DSETREG, sid1, H5P_DEFAULT, H5P_DEFAULT,
                           H5P_DEFAULT)) < 0)
        goto out;

    /* Select 6x6 hyperslab for first reference */
    start[0]  = 2;
    start[1]  = 2;
    stride[0] = 1;
    stride[1] = 1;
    count[0]  = 6;
    count[1]  = 6;
    block[0]  = 1;
    block[1]  = 1;
    if (H5Sselect_hyperslab(sid2, H5S_SELECT_SET, start, stride, count, block) < 0)
        goto out;

    /* Store dataset region */
    if (H5Rcreate(&wbuf[0], loc_id, "dsetreg", H5R_DATASET_REGION, sid2) < 0)
        goto out;

    /* Write selection to disk */
    if (H5Dwrite(did1, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL, H5P_DEFAULT, wbuf) < 0)
        goto out;

    /* Close all objects */
    if (H5Sclose(sid1) < 0)
        goto out;
    if (H5Dclose(did1) < 0)
        goto out;
    if (H5Sclose(sid2) < 0)
        goto out;
    if (H5Dclose(did2) < 0)
        goto out;

    retval = 0;

out:
    if (wbuf)
        free(wbuf);
    if (dwbuf)
        free(dwbuf);

    H5E_BEGIN_TRY
    {
        H5Sclose(sid1);
        H5Sclose(sid2);
        H5Dclose(did1);
        H5Dclose(did2);
    }
    H5E_END_TRY

    return retval;
}

/*-------------------------------------------------------------------------
 * Function: write_attr_in
 *
 * Purpose: write attributes in LOC_ID (dataset, group, named datatype)
 *
 *-------------------------------------------------------------------------
 */

static int
write_attr_in(hid_t loc_id, const char *dset_name, /* for saving reference to dataset*/
              hid_t fid,                           /* for reference create */
              int   make_diffs /* flag to modify data buffers */)
{
    /* Compound datatype */
    typedef struct s_t {
        char   a;
        double b;
    } s_t;

    typedef enum { RED, GREEN } e_t;

    hid_t    aid = H5I_INVALID_HID;
    hid_t    sid = H5I_INVALID_HID;
    hid_t    tid = H5I_INVALID_HID;
    int      val, j, k, n;
    unsigned i;
    float    f;

    /* create 1D attributes with dimension [2], 2 elements */
    hsize_t    dims[1]    = {2};
    char       buf1[2][2] = {"ab", "de"};            /* string, NO NUL fixed length */
    char       buf2[2]    = {1, 2};                  /* bitfield, opaque */
    s_t        buf3[2]    = {{1, 2}, {3, 4}};        /* compound */
    hobj_ref_t buf4[2];                              /* reference */
    e_t        buf45[2] = {RED, RED};                /* enum */
    hvl_t      buf5[2];                              /* vlen */
    hsize_t    dimarray[1] = {3};                    /* array dimension */
    int        buf6[2][3]  = {{1, 2, 3}, {4, 5, 6}}; /* array */
    int        buf7[2]     = {1, 2};                 /* integer */
    float      buf8[2]     = {1, 2};                 /* float */

    /* create 2D attributes with dimension [3][2], 6 elements */
    hsize_t    dims2[2]    = {3, 2};
    char       buf12[6][2] = {"ab", "cd", "ef", "gh", "ij", "kl"}; /* string, NO NUL fixed length */
    char       buf22[3][2] = {{1, 2}, {3, 4}, {5, 6}};             /* bitfield, opaque */
    s_t        buf32[6]    = {{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}, {11, 12}}; /* compound */
    hobj_ref_t buf42[3][2];                                                       /* reference */
    e_t        buf452[3][2];                                                      /* enum */
    hvl_t      buf52[3][2];                                                       /* vlen */
    int buf62[6][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}, {13, 14, 15}, {16, 17, 18}}; /* array */
    int buf72[3][2] = {{1, 2}, {3, 4}, {5, 6}};   /* integer */
    float buf82[3][2] = {{1, 2}, {3, 4}, {5, 6}}; /* float */

    /* create 3D attributes with dimension [4][3][2], 24 elements */
    hsize_t dims3[3]     = {4, 3, 2};
    char    buf13[24][2] = {
        "ab", "cd", "ef", "gh", "ij", "kl", "mn", "pq", "rs", "tu", "vw", "xz", "AB",
        "CD", "EF", "GH", "IJ", "KL", "MN", "PQ", "RS", "TU", "VW", "XZ"}; /* string, NO NUL fixed length */
    char       buf23[4][3][2];                                                /* bitfield, opaque */
    s_t        buf33[4][3][2];                                                /* compound */
    hobj_ref_t buf43[4][3][2];                                                /* reference */
    e_t        buf453[4][3][2];                                               /* enum */
    hvl_t      buf53[4][3][2];                                                /* vlen */
    int        buf63[24][3];                                                  /* array */
    int        buf73[4][3][2];                                                /* integer */
    float      buf83[4][3][2];                                                /* float */

    /*-------------------------------------------------------------------------
     * 1D attributes
     *-------------------------------------------------------------------------
     */

    /*-------------------------------------------------------------------------
     * H5T_STRING
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++)
            for (j = 0; j < 2; j++) {
                buf1[i][j] = 'z';
            }
    }
    /*
    buf1[2][2]= {"ab","de"};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    Attribute:   <string> and <string>
    position      string of </g1>  string of </g1> difference
    ------------------------------------------------------------
    [ 0 ]          a                z
    [ 0 ]          b                z
    [ 1 ]          d                z
    [ 1 ]          e                z
    */
    if ((tid = H5Tcopy(H5T_C_S1)) < 0)
        goto out;
    if (H5Tset_size(tid, (size_t)2) < 0)
        goto out;
    if (make_attr(loc_id, 1, dims, "string", tid, buf1) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_BITFIELD
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++)
            buf2[i] = buf2[1] = 0;
    }
    /*
    buf2[2]= {1,2};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    Attribute:   <bitfield> and <bitfield>
    position      bitfield of </g1> bitfield of </g1> difference
    position        opaque of </g1> opaque of </g1> difference
    ------------------------------------------------------------
    [ 0 ]          1               0               1
    [ 1 ]          2               0               2
    */

    if ((tid = H5Tcopy(H5T_STD_B8LE)) < 0)
        goto out;
    if (make_attr(loc_id, 1, dims, "bitfield", tid, buf2) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_OPAQUE
     *-------------------------------------------------------------------------
     */

    /*
    buf2[2]= {1,2};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    Attribute:   <opaque> and <opaque>
    position     opaque of </g1> opaque of </g1> difference
    position        opaque of </g1> opaque of </g1> difference
    ------------------------------------------------------------
    [ 0 ]          1               0               1
    [ 1 ]          2               0               2
    */

    if ((tid = H5Tcreate(H5T_OPAQUE, (size_t)1)) < 0)
        goto out;
    if (H5Tset_tag(tid, "1-byte opaque type") < 0)
        goto out;
    if (make_attr(loc_id, 1, dims, "opaque", tid, buf2) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_COMPOUND
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++) {
            buf3[i].a = 0;
            buf3[i].b = 0;
        }
    }

    /*
    buf3[2]= {{1,2},{3,4}};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    Attribute:   <compound> and <compound>
    position        compound of </g1> compound of </g1> difference
    ------------------------------------------------------------
    [ 0 ]          1               5               4
    [ 0 ]          2               5               3
    [ 1 ]          3               5               2
    [ 1 ]          4               5               1
    */

    if ((tid = H5Tcreate(H5T_COMPOUND, sizeof(s_t))) < 0)
        goto out;
    if (H5Tinsert(tid, "a", HOFFSET(s_t, a), H5T_NATIVE_CHAR) < 0)
        goto out;
    if (H5Tinsert(tid, "b", HOFFSET(s_t, b), H5T_NATIVE_DOUBLE) < 0)
        goto out;
    if (make_attr(loc_id, 1, dims, "compound", tid, buf3) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_REFERENCE
     *-------------------------------------------------------------------------
     */
    /* object references ( H5R_OBJECT  */
    if (dset_name) {
        if (H5Rcreate(&buf4[0], fid, dset_name, H5R_OBJECT, (hid_t)-1) < 0)
            goto out;
        if (H5Rcreate(&buf4[1], fid, dset_name, H5R_OBJECT, (hid_t)-1) < 0)
            goto out;
        if (make_attr(loc_id, 1, dims, "reference", H5T_STD_REF_OBJ, buf4) < 0)
            goto out;
    }

    /*-------------------------------------------------------------------------
     * H5T_ENUM
     *-------------------------------------------------------------------------
     */
    if (make_diffs) {
        for (i = 0; i < 2; i++) {
            buf45[i] = GREEN;
        }
    }
    /*
    buf45[2]= {RED,RED};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    Attribute:   <enum> and <enum>
    position     enum of </g1>   enum of </g1>   difference
    ------------------------------------------------------------
    [ 0 ]          RED              GREEN
    [ 1 ]          RED              GREEN
    */
    if ((tid = H5Tcreate(H5T_ENUM, sizeof(e_t))) < 0)
        goto out;
    val = 0;
    if (H5Tenum_insert(tid, "RED", &val) < 0)
        goto out;
    val = 1;
    if (H5Tenum_insert(tid, "GREEN", &val) < 0)
        goto out;
    if (make_attr(loc_id, 1, dims, "enum", tid, buf45) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_VLEN
     *-------------------------------------------------------------------------
     */

    /* Allocate and initialize VL dataset to write */

    buf5[0].len           = 1;
    buf5[0].p             = malloc(1 * sizeof(int));
    ((int *)buf5[0].p)[0] = 1;
    buf5[1].len           = 2;
    buf5[1].p             = malloc(2 * sizeof(int));
    ((int *)buf5[1].p)[0] = 2;
    ((int *)buf5[1].p)[1] = 3;

    if (make_diffs) {
        ((int *)buf5[0].p)[0] = 0;
        ((int *)buf5[1].p)[0] = 0;
        ((int *)buf5[1].p)[1] = 0;
    }
    /*
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    position        vlen of </g1>   vlen of </g1>   difference
    ------------------------------------------------------------
    [ 0 ]          1               0               1
    [ 1 ]          2               0               2
    [ 1 ]          3               0               3
    */

    if ((sid = H5Screate_simple(1, dims, NULL)) < 0)
        goto out;
    if ((tid = H5Tvlen_create(H5T_NATIVE_INT)) < 0)
        goto out;
    if ((aid = H5Acreate2(loc_id, "vlen", tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Awrite(aid, tid, buf5) < 0)
        goto out;
    if (H5Treclaim(tid, sid, H5P_DEFAULT, buf5) < 0)
        goto out;
    if (H5Aclose(aid) < 0)
        goto out;
    aid = H5I_INVALID_HID;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;
    if (H5Sclose(sid) < 0)
        goto out;
    sid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_ARRAY
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++)
            for (j = 0; j < 3; j++) {
                buf6[i][j] = 0;
            }
    }
    /*
    buf6[2][3]= {{1,2,3},{4,5,6}};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    Attribute:   <array> and <array>
    position        array of </g1>  array of </g1>  difference
    ------------------------------------------------------------
    [ 0 ]          1               0               1
    [ 0 ]          2               0               2
    [ 0 ]          3               0               3
    [ 1 ]          4               0               4
    [ 1 ]          5               0               5
    [ 1 ]          6               0               6
    */
    if ((tid = H5Tarray_create2(H5T_NATIVE_INT, 1, dimarray)) < 0)
        goto out;
    if (make_attr(loc_id, 1, dims, "array", tid, buf6) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_INTEGER and H5T_FLOAT
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        for (i = 0; i < 2; i++) {
            buf7[i] = 0;
            buf8[i] = 0;
        }
    }

    /*
    buf7[2]= {1,2};
    buf8[2]= {1,2};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    position        integer of </g1> integer of </g1> difference
    ------------------------------------------------------------
    [ 0 ]          1               0               1
    [ 1 ]          2               0               2
    position        float of </g1>  float of </g1>  difference
    ------------------------------------------------------------
    [ 0 ]          1               0               1
    [ 1 ]          2               0               2
    */
    if (make_attr(loc_id, 1, dims, "integer", H5T_NATIVE_INT, buf7) < 0)
        goto out;
    if (make_attr(loc_id, 1, dims, "float", H5T_NATIVE_FLOAT, buf8) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * 2D attributes
     *-------------------------------------------------------------------------
     */

    /*-------------------------------------------------------------------------
     * H5T_STRING
     *-------------------------------------------------------------------------
     */
    if (make_diffs) {
        memset(buf12, 'z', sizeof buf12);
    }

    /*
    buf12[6][2]= {"ab","cd","ef","gh","ij","kl"};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Attribute:   <string2D> and <string2D>
    position        string2D of </g1> string2D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 ]          a                z
    [ 0 0 ]          b                z
    [ 0 1 ]          c                z
    [ 0 1 ]          d                z
    [ 1 0 ]          e                z
    [ 1 0 ]          f                z
    [ 1 1 ]          g                z
    [ 1 1 ]          h                z
    [ 2 0 ]          i                z
    [ 2 0 ]          j                z
    [ 2 1 ]          k                z
    [ 2 1 ]          l                z
    */

    if ((tid = H5Tcopy(H5T_C_S1)) < 0)
        goto out;
    if (H5Tset_size(tid, (size_t)2) < 0)
        goto out;
    if (make_attr(loc_id, 2, dims2, "string2D", tid, buf12) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_BITFIELD
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf22, 0, sizeof buf22);
    }

    /*
    buf22[3][2]= {{1,2},{3,4},{5,6}};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Attribute:   <bitfield2D> and <bitfield2D>
    position        bitfield2D of </g1> bitfield2D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 ]          1               0               1
    [ 0 1 ]          2               0               2
    [ 1 0 ]          3               0               3
    [ 1 1 ]          4               0               4
    [ 2 0 ]          5               0               5
    [ 2 1 ]          6               0               6
    */

    if ((tid = H5Tcopy(H5T_STD_B8LE)) < 0)
        goto out;
    if (make_attr(loc_id, 2, dims2, "bitfield2D", tid, buf22) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_OPAQUE
     *-------------------------------------------------------------------------
     */

    /*
    buf22[3][2]= {{1,2},{3,4},{5,6}};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Attribute:   <opaque2D> and <opaque2D>
    position        opaque2D of </g1> opaque2D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 ]          1               0               1
    [ 0 1 ]          2               0               2
    [ 1 0 ]          3               0               3
    [ 1 1 ]          4               0               4
    [ 2 0 ]          5               0               5
    [ 2 1 ]          6               0               6
    */
    if ((tid = H5Tcreate(H5T_OPAQUE, (size_t)1)) < 0)
        goto out;
    if (H5Tset_tag(tid, "1-byte opaque type") < 0)
        goto out;
    if (make_attr(loc_id, 2, dims2, "opaque2D", tid, buf22) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_COMPOUND
     *-------------------------------------------------------------------------
     */
    if (make_diffs) {
        memset(buf32, 0, sizeof buf32);
    }

    /*
    buf32[6]= {{1,2},{3,4},{5,6},{7,8},{9,10},{11,12}};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Attribute:   <opaque2D> and <opaque2D>
    position        opaque2D of </g1> opaque2D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 ]          1               0               1
    [ 0 1 ]          2               0               2
    [ 1 0 ]          3               0               3
    [ 1 1 ]          4               0               4
    [ 2 0 ]          5               0               5
    [ 2 1 ]          6               0               6
    */

    if ((tid = H5Tcreate(H5T_COMPOUND, sizeof(s_t))) < 0)
        goto out;
    if (H5Tinsert(tid, "a", HOFFSET(s_t, a), H5T_NATIVE_CHAR) < 0)
        goto out;
    if (H5Tinsert(tid, "b", HOFFSET(s_t, b), H5T_NATIVE_DOUBLE) < 0)
        goto out;
    if (make_attr(loc_id, 2, dims2, "compound2D", tid, buf32) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_REFERENCE
     *-------------------------------------------------------------------------
     */
    /* Create references to dataset */
    if (dset_name) {
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 2; j++) {
                if (H5Rcreate(&buf42[i][j], fid, dset_name, H5R_OBJECT, (hid_t)-1) < 0)
                    goto out;
            }
        }
        if (make_attr(loc_id, 2, dims2, "reference2D", H5T_STD_REF_OBJ, buf42) < 0)
            goto out;
    }

    /*-------------------------------------------------------------------------
     * H5T_ENUM
     *-------------------------------------------------------------------------
     */
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 2; j++) {
            if (make_diffs)
                buf452[i][j] = GREEN;
            else
                buf452[i][j] = RED;
        }
    }

    /*
    Attribute:   <enum2D> and <enum2D>
    position        enum2D of </g1> enum2D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 ]          RED              GREEN
    [ 0 1 ]          RED              GREEN
    [ 1 0 ]          RED              GREEN
    [ 1 1 ]          RED              GREEN
    [ 2 0 ]          RED              GREEN
    [ 2 1 ]          RED              GREEN
    */

    if ((tid = H5Tcreate(H5T_ENUM, sizeof(e_t))) < 0)
        goto out;
    val = 0;
    if (H5Tenum_insert(tid, "RED", &val) < 0)
        goto out;
    val = 1;
    if (H5Tenum_insert(tid, "GREEN", &val) < 0)
        goto out;
    if (make_attr(loc_id, 2, dims2, "enum2D", tid, buf452) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_VLEN
     *-------------------------------------------------------------------------
     */

    /* Allocate and initialize VL dataset to write */
    n = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 2; j++) {
            unsigned l;

            buf52[i][j].p   = malloc((i + 1) * sizeof(int));
            buf52[i][j].len = (size_t)(i + 1);
            for (l = 0; l < i + 1; l++)
                if (make_diffs)
                    ((int *)buf52[i][j].p)[l] = 0;
                else
                    ((int *)buf52[i][j].p)[l] = n++;
        }
    }

    /*
    position        vlen2D of </g1> vlen2D of </g1> difference
    ------------------------------------------------------------
    [ 0 1 ]          1               0               1
    [ 1 0 ]          2               0               2
    [ 1 0 ]          3               0               3
    [ 1 1 ]          4               0               4
    [ 1 1 ]          5               0               5
    [ 2 0 ]          6               0               6
    [ 2 0 ]          7               0               7
    [ 2 0 ]          8               0               8
    [ 2 1 ]          9               0               9
    [ 2 1 ]          10              0               10
    [ 2 1 ]          11              0               11
    */

    if ((sid = H5Screate_simple(2, dims2, NULL)) < 0)
        goto out;
    if ((tid = H5Tvlen_create(H5T_NATIVE_INT)) < 0)
        goto out;
    if ((aid = H5Acreate2(loc_id, "vlen2D", tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Awrite(aid, tid, buf52) < 0)
        goto out;
    if (H5Treclaim(tid, sid, H5P_DEFAULT, buf52) < 0)
        goto out;
    if (H5Aclose(aid) < 0)
        goto out;
    aid = H5I_INVALID_HID;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;
    if (H5Sclose(sid) < 0)
        goto out;
    sid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_ARRAY
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf62, 0, sizeof buf62);
    }
    /*
    buf62[6][3]= {{1,2,3},{4,5,6},{7,8,9},{10,11,12},{13,14,15},{16,17,18}};
    $h5diff file7.h5 file6.h5 g1 g1 -v
    Group:       </g1> and </g1>
    Attribute:   <array2D> and <array2D>
    position        array2D of </g1> array2D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 ]          1               0               1
    [ 0 0 ]          2               0               2
    [ 0 0 ]          3               0               3
    [ 0 1 ]          4               0               4
    [ 0 1 ]          5               0               5
    [ 0 1 ]          6               0               6
    [ 1 0 ]          7               0               7
    [ 1 0 ]          8               0               8
    [ 1 0 ]          9               0               9
    [ 1 1 ]          10              0               10
    [ 1 1 ]          11              0               11
    [ 1 1 ]          12              0               12
    [ 2 0 ]          13              0               13
    [ 2 0 ]          14              0               14
    [ 2 0 ]          15              0               15
    [ 2 1 ]          16              0               16
    [ 2 1 ]          17              0               17
    [ 2 1 ]          18              0               18
    */
    if ((tid = H5Tarray_create2(H5T_NATIVE_INT, 1, dimarray)) < 0)
        goto out;
    if (make_attr(loc_id, 2, dims2, "array2D", tid, buf62) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_INTEGER and H5T_FLOAT
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf72, 0, sizeof buf72);
        memset(buf82, 0, sizeof buf82);
    }
    /*
    Attribute:   <integer2D> and <integer2D>
    position        integer2D of </g1> integer2D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 ]          1               0               1
    [ 0 1 ]          2               0               2
    [ 1 0 ]          3               0               3
    [ 1 1 ]          4               0               4
    [ 2 0 ]          5               0               5
    [ 2 1 ]          6               0               6
    6 differences found
    Attribute:   <float2D> and <float2D>
    position        float2D of </g1> float2D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 ]          1               0               1
    [ 0 1 ]          2               0               2
    [ 1 0 ]          3               0               3
    [ 1 1 ]          4               0               4
    [ 2 0 ]          5               0               5
    [ 2 1 ]          6               0               6
    */

    if (make_attr(loc_id, 2, dims2, "integer2D", H5T_NATIVE_INT, buf72) < 0)
        goto out;
    if (make_attr(loc_id, 2, dims2, "float2D", H5T_NATIVE_FLOAT, buf82) < 0)
        goto out;

    /*-------------------------------------------------------------------------
     * 3D attributes
     *-------------------------------------------------------------------------
     */

    /*-------------------------------------------------------------------------
     * H5T_STRING
     *-------------------------------------------------------------------------
     */

    if (make_diffs) {
        memset(buf13, 'z', sizeof buf13);
    }

    /*
    buf13[24][2]= {"ab","cd","ef","gh","ij","kl","mn","pq",
    "rs","tu","vw","xz","AB","CD","EF","GH",
    "IJ","KL","MN","PQ","RS","TU","VW","XZ"};

    Attribute:   <string3D> and <string3D>
    position        string3D of </g1> string3D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 0 ]          a                z
    [ 0 0 0 ]          b                z
    [ 0 0 1 ]          c                z
    [ 0 0 1 ]          d                z
    [ 0 1 0 ]          e                z
    [ 0 1 0 ]          f                z
    [ 0 1 1 ]          g                z
    [ 0 1 1 ]          h                z
    [ 0 2 0 ]          i                z
    [ 0 2 0 ]          j                z
    [ 0 2 1 ]          k                z
    [ 0 2 1 ]          l                z
    [ 1 0 0 ]          m                z
    [ 1 0 0 ]          n                z
    [ 1 0 1 ]          p                z
    [ 1 0 1 ]          q                z
    [ 1 1 0 ]          r                z
    [ 1 1 0 ]          s                z
    [ 1 1 1 ]          t                z
    [ 1 1 1 ]          u                z
    [ 1 2 0 ]          v                z
    [ 1 2 0 ]          w                z
    [ 1 2 1 ]          x                z
    [ 2 0 0 ]          A                z
    [ 2 0 0 ]          B                z
    [ 2 0 1 ]          C                z
    [ 2 0 1 ]          D                z
    [ 2 1 0 ]          E                z
    [ 2 1 0 ]          F                z
    [ 2 1 1 ]          G                z
    [ 2 1 1 ]          H                z
    [ 2 2 0 ]          I                z
    [ 2 2 0 ]          J                z
    [ 2 2 1 ]          K                z
    [ 2 2 1 ]          L                z
    [ 3 0 0 ]          M                z
    [ 3 0 0 ]          N                z
    [ 3 0 1 ]          P                z
    [ 3 0 1 ]          Q                z
    [ 3 1 0 ]          R                z
    [ 3 1 0 ]          S                z
    [ 3 1 1 ]          T                z
    [ 3 1 1 ]          U                z
    [ 3 2 0 ]          V                z
    [ 3 2 0 ]          W                z
    [ 3 2 1 ]          X                z
    [ 3 2 1 ]          Z                z
    */

    if ((tid = H5Tcopy(H5T_C_S1)) < 0)
        goto out;
    if (H5Tset_size(tid, (size_t)2) < 0)
        goto out;
    if (make_attr(loc_id, 3, dims3, "string3D", tid, buf13) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_BITFIELD
     *-------------------------------------------------------------------------
     */

    n = 1;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                if (make_diffs)
                    buf23[i][j][k] = 0;
                else
                    buf23[i][j][k] = (char)(n++);
            }
        }
    }

    /*
    position        bitfield3D of </g1> bitfield3D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 0 ]          1               0               1
    [ 0 0 1 ]          2               0               2
    [ 0 1 0 ]          3               0               3
    [ 0 1 1 ]          4               0               4
    [ 0 2 0 ]          5               0               5
    [ 0 2 1 ]          6               0               6
    [ 1 0 0 ]          7               0               7
    [ 1 0 1 ]          8               0               8
    [ 1 1 0 ]          9               0               9
    [ 1 1 1 ]          10              0               10
    [ 1 2 0 ]          11              0               11
    [ 1 2 1 ]          12              0               12
    [ 2 0 0 ]          13              0               13
    [ 2 0 1 ]          14              0               14
    [ 2 1 0 ]          15              0               15
    [ 2 1 1 ]          16              0               16
    [ 2 2 0 ]          17              0               17
    [ 2 2 1 ]          18              0               18
    [ 3 0 0 ]          19              0               19
    [ 3 0 1 ]          20              0               20
    [ 3 1 0 ]          21              0               21
    [ 3 1 1 ]          22              0               22
    [ 3 2 0 ]          23              0               23
    [ 3 2 1 ]          24              0               24
    */

    if ((tid = H5Tcopy(H5T_STD_B8LE)) < 0)
        goto out;
    if (make_attr(loc_id, 3, dims3, "bitfield3D", tid, buf23) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_OPAQUE
     *-------------------------------------------------------------------------
     */
    if ((tid = H5Tcreate(H5T_OPAQUE, (size_t)1)) < 0)
        goto out;
    if (H5Tset_tag(tid, "1-byte opaque type") < 0)
        goto out;
    if (make_attr(loc_id, 3, dims3, "opaque3D", tid, buf23) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_COMPOUND
     *-------------------------------------------------------------------------
     */

    n = 1;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                if (make_diffs) {
                    buf33[i][j][k].a = 0;
                    buf33[i][j][k].b = 0;
                }
                else {
                    buf33[i][j][k].a = (char)(n++);
                    buf33[i][j][k].b = n++;
                }
            }
        }
    }
    /*position        compound3D of </g1> compound3D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 0 ]          1               0               1
    [ 0 0 0 ]          2               0               2
    [ 0 0 1 ]          3               0               3
    [ 0 0 1 ]          4               0               4
    [ 0 1 0 ]          5               0               5
    [ 0 1 0 ]          6               0               6
    [ 0 1 1 ]          7               0               7
    [ 0 1 1 ]          8               0               8
    [ 0 2 0 ]          9               0               9
    [ 0 2 0 ]          10              0               10
    [ 0 2 1 ]          11              0               11
    [ 0 2 1 ]          12              0               12
    [ 1 0 0 ]          13              0               13
    [ 1 0 0 ]          14              0               14
    [ 1 0 1 ]          15              0               15
    [ 1 0 1 ]          16              0               16
    [ 1 1 0 ]          17              0               17
    [ 1 1 0 ]          18              0               18
    [ 1 1 1 ]          19              0               19
    [ 1 1 1 ]          20              0               20
    [ 1 2 0 ]          21              0               21
    [ 1 2 0 ]          22              0               22
    [ 1 2 1 ]          23              0               23
    [ 1 2 1 ]          24              0               24
    [ 2 0 0 ]          25              0               25
    [ 2 0 0 ]          26              0               26
    [ 2 0 1 ]          27              0               27
    [ 2 0 1 ]          28              0               28
    [ 2 1 0 ]          29              0               29
    [ 2 1 0 ]          30              0               30
    [ 2 1 1 ]          31              0               31
    [ 2 1 1 ]          32              0               32
    [ 2 2 0 ]          33              0               33
    [ 2 2 0 ]          34              0               34
    [ 2 2 1 ]          35              0               35
    [ 2 2 1 ]          36              0               36
    [ 3 0 0 ]          37              0               37
    [ 3 0 0 ]          38              0               38
    [ 3 0 1 ]          39              0               39
    [ 3 0 1 ]          40              0               40
    [ 3 1 0 ]          41              0               41
    [ 3 1 0 ]          42              0               42
    [ 3 1 1 ]          43              0               43
    [ 3 1 1 ]          44              0               44
    [ 3 2 0 ]          45              0               45
    [ 3 2 0 ]          46              0               46
    [ 3 2 1 ]          47              0               47
    [ 3 2 1 ]          48              0               48
    */

    if ((tid = H5Tcreate(H5T_COMPOUND, sizeof(s_t))) < 0)
        goto out;
    if (H5Tinsert(tid, "a", HOFFSET(s_t, a), H5T_NATIVE_CHAR) < 0)
        goto out;
    if (H5Tinsert(tid, "b", HOFFSET(s_t, b), H5T_NATIVE_DOUBLE) < 0)
        goto out;
    if (make_attr(loc_id, 3, dims3, "compound3D", tid, buf33) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_REFERENCE
     *-------------------------------------------------------------------------
     */
    /* Create references to dataset */
    if (dset_name) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++) {
                for (k = 0; k < 2; k++)
                    if (H5Rcreate(&buf43[i][j][k], fid, dset_name, H5R_OBJECT, (hid_t)-1) < 0)
                        goto out;
            }
        }
        if (make_attr(loc_id, 3, dims3, "reference3D", H5T_STD_REF_OBJ, buf43) < 0)
            goto out;
    }

    /*-------------------------------------------------------------------------
     * H5T_ENUM
     *-------------------------------------------------------------------------
     */

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                if (make_diffs) {
                    buf453[i][j][k] = RED;
                }
                else {
                    buf453[i][j][k] = GREEN;
                }
            }
        }
    }

    /*
    position        enum3D of </g1> enum3D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 0 ]          GREEN            RED
    [ 0 0 1 ]          GREEN            RED
    [ 0 1 0 ]          GREEN            RED
    [ 0 1 1 ]          GREEN            RED
    [ 0 2 0 ]          GREEN            RED
    [ 0 2 1 ]          GREEN            RED
    [ 1 0 0 ]          GREEN            RED
    [ 1 0 1 ]          GREEN            RED
    [ 1 1 0 ]          GREEN            RED
    [ 1 1 1 ]          GREEN            RED
    [ 1 2 0 ]          GREEN            RED
    [ 1 2 1 ]          GREEN            RED
    [ 2 0 0 ]          GREEN            RED
    [ 2 0 1 ]          GREEN            RED
    [ 2 1 0 ]          GREEN            RED
    [ 2 1 1 ]          GREEN            RED
    [ 2 2 0 ]          GREEN            RED
    [ 2 2 1 ]          GREEN            RED
    [ 3 0 0 ]          GREEN            RED
    [ 3 0 1 ]          GREEN            RED
    [ 3 1 0 ]          GREEN            RED
    [ 3 1 1 ]          GREEN            RED
    [ 3 2 0 ]          GREEN            RED
    [ 3 2 1 ]          GREEN            RED
    */

    if ((tid = H5Tcreate(H5T_ENUM, sizeof(e_t))) < 0)
        goto out;
    val = 0;
    if (H5Tenum_insert(tid, "RED", &val) < 0)
        goto out;
    val = 1;
    if (H5Tenum_insert(tid, "GREEN", &val) < 0)
        goto out;
    if (make_attr(loc_id, 3, dims3, "enum3D", tid, buf453) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_VLEN
     *-------------------------------------------------------------------------
     */

    /* Allocate and initialize VL dataset to write */
    n = 0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                unsigned l;

                buf53[i][j][k].p   = malloc((i + 1) * sizeof(int));
                buf53[i][j][k].len = (size_t)i + 1;
                for (l = 0; l < i + 1; l++)
                    if (make_diffs)
                        ((int *)buf53[i][j][k].p)[l] = 0;
                    else
                        ((int *)buf53[i][j][k].p)[l] = n++;
            }
        }
    }
    /*
    position        vlen3D of </g1> vlen3D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 1 ]          1               0               1
    [ 0 1 0 ]          2               0               2
    [ 0 1 1 ]          3               0               3
    [ 0 2 0 ]          4               0               4
    [ 0 2 1 ]          5               0               5
    [ 1 0 0 ]          6               0               6
    [ 1 0 0 ]          7               0               7
    [ 1 0 1 ]          8               0               8
    [ 1 0 1 ]          9               0               9
    [ 1 1 0 ]          10              0               10
    etc
    */
    if ((sid = H5Screate_simple(3, dims3, NULL)) < 0)
        goto out;
    if ((tid = H5Tvlen_create(H5T_NATIVE_INT)) < 0)
        goto out;
    if ((aid = H5Acreate2(loc_id, "vlen3D", tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Awrite(aid, tid, buf53) < 0)
        goto out;
    if (H5Treclaim(tid, sid, H5P_DEFAULT, buf53) < 0)
        goto out;
    if (H5Aclose(aid) < 0)
        goto out;
    aid = H5I_INVALID_HID;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;
    if (H5Sclose(sid) < 0)
        goto out;
    sid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_ARRAY
     *-------------------------------------------------------------------------
     */
    n = 1;
    for (i = 0; i < 24; i++) {
        for (j = 0; j < (int)dimarray[0]; j++) {
            if (make_diffs)
                buf63[i][j] = 0;
            else
                buf63[i][j] = n++;
        }
    }
    /*
    position        array3D of </g1> array3D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 0 ]          1               0               1
    [ 0 0 0 ]          2               0               2
    [ 0 0 0 ]          3               0               3
    [ 0 0 1 ]          4               0               4
    [ 0 0 1 ]          5               0               5
    [ 0 0 1 ]          6               0               6
    [ 0 1 0 ]          7               0               7
    etc
    */

    if ((tid = H5Tarray_create2(H5T_NATIVE_INT, 1, dimarray)) < 0)
        goto out;
    if (make_attr(loc_id, 3, dims3, "array3D", tid, buf63) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    tid = H5I_INVALID_HID;

    /*-------------------------------------------------------------------------
     * H5T_INTEGER and H5T_FLOAT
     *-------------------------------------------------------------------------
     */
    n = 1;
    f = 1;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                if (make_diffs) {
                    buf73[i][j][k] = 0;
                    buf83[i][j][k] = 0;
                }
                else {
                    buf73[i][j][k] = n++;
                    buf83[i][j][k] = f++;
                }
            }
        }
    }

    /*
    position        integer3D of </g1> integer3D of </g1> difference
    ------------------------------------------------------------
    [ 0 0 0 ]          1               0               1
    [ 0 0 1 ]          2               0               2
    [ 0 1 0 ]          3               0               3
    [ 0 1 1 ]          4               0               4
    [ 0 2 0 ]          5               0               5
    [ 0 2 1 ]          6               0               6
    [ 1 0 0 ]          7               0               7
    [ 1 0 1 ]          8               0               8
    [ 1 1 0 ]          9               0               9
    [ 1 1 1 ]          10              0               10
    etc
    */
    if (make_attr(loc_id, 3, dims3, "integer3D", H5T_NATIVE_INT, buf73) < 0)
        goto out;
    if (make_attr(loc_id, 3, dims3, "float3D", H5T_NATIVE_FLOAT, buf83) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Aclose(aid);
        H5Sclose(sid);
        H5Tclose(tid);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_dset
 *
 * Purpose: utility function to create and write a dataset in LOC_ID
 *
 *-------------------------------------------------------------------------
 */
static int
make_dset(hid_t loc_id, const char *name, hid_t sid, hid_t dcpl, void *buf)
{
    hid_t did     = H5I_INVALID_HID;
    hid_t dxpl_id = H5P_DEFAULT;

    if ((did = H5Dcreate2(loc_id, name, H5T_NATIVE_INT, sid, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0)
        return -1;

#ifdef H5_HAVE_PARALLEL
    {
        bool driver_is_parallel;

        if (h5_using_parallel_driver(H5P_DEFAULT, &driver_is_parallel) < 0)
            goto out;
        /* Set up collective writes for parallel driver */
        if (driver_is_parallel) {
            if ((dxpl_id = H5Pcreate(H5P_DATASET_XFER)) < 0)
                goto out;
            if (H5Pset_dxpl_mpio(dxpl_id, H5FD_MPIO_COLLECTIVE) < 0)
                goto out;
        }
    }
#endif

    if (H5Dwrite(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, dxpl_id, buf) < 0)
        goto out;
    if (dxpl_id != H5P_DEFAULT && H5Pclose(dxpl_id) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        return -1;
    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Dclose(did);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: write_dset
 *
 * Purpose: utility function to create and write a dataset in LOC_ID
 *
 *-------------------------------------------------------------------------
 */
static int
write_dset(hid_t loc_id, int rank, hsize_t *dims, const char *dset_name, hid_t tid, void *buf)
{
    hid_t did     = H5I_INVALID_HID;
    hid_t sid     = H5I_INVALID_HID;
    hid_t dxpl_id = H5P_DEFAULT;

    if ((sid = H5Screate_simple(rank, dims, NULL)) < 0)
        return -1;
    if ((did = H5Dcreate2(loc_id, dset_name, tid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (buf) {
#ifdef H5_HAVE_PARALLEL
        {
            bool driver_is_parallel;

            if (h5_using_parallel_driver(H5P_DEFAULT, &driver_is_parallel) < 0)
                goto out;
            /* Set up collective writes for parallel driver */
            if (driver_is_parallel) {
                if ((dxpl_id = H5Pcreate(H5P_DATASET_XFER)) < 0)
                    goto out;
                if (H5Pset_dxpl_mpio(dxpl_id, H5FD_MPIO_COLLECTIVE) < 0)
                    goto out;
            }
        }
#endif

        if (H5Dwrite(did, tid, H5S_ALL, H5S_ALL, dxpl_id, buf) < 0)
            goto out;
    }
    if (dxpl_id != H5P_DEFAULT && H5Pclose(dxpl_id) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;
    if (H5Sclose(sid) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Pclose(dxpl_id);
        H5Dclose(did);
        H5Sclose(sid);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_attr
 *
 * Purpose: utility function to write an attribute in LOC_ID
 *
 *-------------------------------------------------------------------------
 */
static int
make_attr(hid_t loc_id, int rank, hsize_t *dims, const char *attr_name, hid_t tid, void *buf)
{
    hid_t aid;
    hid_t sid;

    if ((sid = H5Screate_simple(rank, dims, NULL)) < 0)
        return -1;
    if ((aid = H5Acreate2(loc_id, attr_name, tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (buf) {
        if (H5Awrite(aid, tid, buf) < 0)
            goto out;
    }
    if (H5Aclose(aid) < 0)
        goto out;
    if (H5Sclose(sid) < 0)
        goto out;
    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Aclose(aid);
        H5Sclose(sid);
    }
    H5E_END_TRY
    return -1;
}

/*-------------------------------------------------------------------------
 * Function: make_named_dtype
 *
 * Purpose: create a file with named datatypes in various configurations
 *
 *-------------------------------------------------------------------------
 */
static int
make_named_dtype(hid_t loc_id)
{
    hsize_t dims[1] = {3};
    hid_t   did     = H5I_INVALID_HID;
    hid_t   aid     = H5I_INVALID_HID;
    hid_t   sid     = H5I_INVALID_HID;
    hid_t   tid     = H5I_INVALID_HID;
    hid_t   gid     = H5I_INVALID_HID;

    if ((sid = H5Screate_simple(1, dims, NULL)) < 0)
        goto out;

    /* Create a dataset with an anonymous committed datatype as the first thing
     * h5repack sees */
    if ((tid = H5Tcopy(H5T_STD_I16LE)) < 0)
        goto out;
    if (H5Tcommit_anon(loc_id, tid, H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "A", tid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /* Create an attribute on that dataset that uses a committed datatype in
     * a remote group */
    if ((gid = H5Gcreate2(loc_id, "M", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Gclose(gid) < 0)
        goto out;
    if ((gid = H5Gcreate2(loc_id, "M/M", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Gclose(gid) < 0)
        goto out;
    if ((tid = H5Tcopy(H5T_STD_I16BE)) < 0)
        goto out;
    if (H5Tcommit2(loc_id, "/M/M/A", tid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;
    if ((aid = H5Acreate2(did, "A", tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Aclose(aid) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;

    /* Create a dataset in the remote group that uses a committed datatype in
     * the root group */
    if ((tid = H5Tcopy(H5T_STD_I32LE)) < 0)
        goto out;
    if (H5Tcommit2(loc_id, "N", tid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "M/M/B", tid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /* Create an attribute on the remote dataset that uses an anonymous
     * committed datatype */
    if ((tid = H5Tcopy(H5T_STD_I32BE)) < 0)
        goto out;
    if (H5Tcommit_anon(loc_id, tid, H5P_DEFAULT, H5P_DEFAULT) < 0)
        goto out;
    if ((aid = H5Acreate2(did, "A", tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Aclose(aid) < 0)
        goto out;

    /* Create another attribute that uses the same anonymous datatype */
    if ((aid = H5Acreate2(did, "B", tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Aclose(aid) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;

    /* Create a dataset in the root group that uses the committed datatype in
     * the root group */
    if ((tid = H5Topen2(loc_id, "N", H5P_DEFAULT)) < 0)
        goto out;
    if ((did = H5Dcreate2(loc_id, "O", tid, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Dclose(did) < 0)
        goto out;

    /* Create 2 attributes on the committed datatype that use that datatype */
    if ((aid = H5Acreate2(tid, "A", tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Aclose(aid) < 0)
        goto out;
    if ((aid = H5Acreate2(tid, "B", tid, sid, H5P_DEFAULT, H5P_DEFAULT)) < 0)
        goto out;
    if (H5Aclose(aid) < 0)
        goto out;
    if (H5Tclose(tid) < 0)
        goto out;

    /* Close */
    if (H5Sclose(sid) < 0)
        goto out;

    return 0;

out:
    H5E_BEGIN_TRY
    {
        H5Tclose(tid);
        H5Aclose(aid);
        H5Sclose(sid);
        H5Dclose(did);
        H5Gclose(gid);
    }
    H5E_END_TRY
    return -1;
} /* end make_named_dtype() */

/*-------------------------------------------------------------------------
 * Function: add_attr_with_objref
 *
 * Purpose:
 *  Create attributes with object reference to objects (dset,
 *  group, datatype).
 *
 * Note:
 *  this function depends on locally created objects, however can be modified
 *  to be independent as necessary
 *
 *------------------------------------------------------------------------*/
static herr_t
add_attr_with_objref(hid_t file_id, hid_t obj_id)
{
    int ret = SUCCEED;
    int status;
    /* attr obj ref */
    hsize_t    dim_attr_objref[1] = {3};
    hobj_ref_t data_attr_objref[3];

    /* --------------------------------
     * add attribute with obj ref type
     */
    /* ref to dset */
    status = H5Rcreate(&data_attr_objref[0], file_id, NAME_OBJ_DS1, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* ref to group */
    status = H5Rcreate(&data_attr_objref[1], file_id, NAME_OBJ_GRP, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* ref to datatype */
    status = H5Rcreate(&data_attr_objref[2], file_id, NAME_OBJ_NDTYPE, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* create attr with obj ref type */
    status = make_attr(obj_id, 1, dim_attr_objref, "Attr_OBJREF", H5T_STD_REF_OBJ, data_attr_objref);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> make_attr failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

out:

    return ret;
}

/*-------------------------------------------------------------------------
 * Function: add_attr_with_regref
 *
 * Purpose:
 *  Create attributes with region reference to dset
 *
 * Note:
 *  this function depends on locally created objects, however can be modified
 *  to be independent as necessary
 *
 *------------------------------------------------------------------------*/
static herr_t
add_attr_with_regref(hid_t file_id, hid_t obj_id)
{
    int ret = SUCCEED;
    int status;

    /* attr region ref */
    hid_t           sid_regrefed_dset          = 0;
    hsize_t         dim_regrefed_dset[2]       = {3, 6};
    hsize_t         coords_regrefed_dset[3][2] = {{0, 1}, {1, 2}, {2, 3}};
    hsize_t         dim_attr_regref[1]         = {1}; /* dim of  */
    hdset_reg_ref_t data_attr_regref[1];

    /* -----------------------------------
     * add attribute with region ref type
     */
    sid_regrefed_dset = H5Screate_simple(2, dim_regrefed_dset, NULL);
    if (sid_regrefed_dset < 0) {
        fprintf(stderr, "Error: %s %d> H5Screate_simple failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* select elements space for reference */
    status = H5Sselect_elements(sid_regrefed_dset, H5S_SELECT_SET, (size_t)3, coords_regrefed_dset[0]);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Sselect_elements failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* create region reference from elements space */
    status = H5Rcreate(&data_attr_regref[0], file_id, NAME_OBJ_DS2, H5R_DATASET_REGION, sid_regrefed_dset);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* create attr with region ref type */
    status = make_attr(obj_id, 1, dim_attr_regref, "Attr_REGREF", H5T_STD_REF_DSETREG, data_attr_regref);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> make_attr failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

out:
    if (sid_regrefed_dset > 0)
        H5Sclose(sid_regrefed_dset);

    return ret;
}

/*-------------------------------------------------------------------------
 * Function: gen_refered_objs
 *
 * Purpose:
 *  Create objects (dataset, group, datatype) to be referenced
 *
 * Note:
 *  This function is to use along with gen_obj_ref() gen_region_ref()
 *
 *------------------------------------------------------------------------*/
static herr_t
gen_refered_objs(hid_t loc_id)
{
    int    status;
    herr_t ret = SUCCEED;

    /* objects (dset, group, datatype) */
    hid_t   sid = 0, did1 = 0, gid = 0, tid = 0;
    hsize_t dims1[1] = {3};
    int     data[3]  = {10, 20, 30};

    /* Dset2 */
    hid_t   sid2 = 0, did2 = 0;
    hsize_t dims2[2]     = {3, 16};
    char    data2[3][16] = {"The quick brown", "fox jumps over ", "the 5 lazy dogs"};

    /*-----------------------
     * add short dataset
     * (define NAME_OBJ_DS1)
     */
    sid = H5Screate_simple(1, dims1, NULL);
    if (sid < 0) {
        fprintf(stderr, "Error: %s %d> H5Screate_simple failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    did1 = H5Dcreate2(loc_id, NAME_OBJ_DS1, H5T_NATIVE_INT, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (did1 < 0) {
        fprintf(stderr, "Error: %s %d> H5Dcreate2 failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    status = H5Dwrite(did1, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Dwrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*--------------
     * add group
     * (define NAME_OBJ_GRP)
     */
    gid = H5Gcreate2(loc_id, NAME_OBJ_GRP, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (gid < 0) {
        fprintf(stderr, "Error: %s %d> H5Gcreate2 failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*----------------------
     * add named datatype
     * (define NAME_OBJ_NDTYPE)
     */
    tid    = H5Tcopy(H5T_NATIVE_INT);
    status = H5Tcommit2(loc_id, NAME_OBJ_NDTYPE, tid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Tcommit2 failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*--------------------------
     * create long dataset
     * (define NAME_OBJ_DS2)
     */
    sid2 = H5Screate_simple(2, dims2, NULL);
    if (sid2 < 0) {
        fprintf(stderr, "Error: %s %d> H5Screate_simple failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* create normal dataset which is referred */
    did2 = H5Dcreate2(loc_id, NAME_OBJ_DS2, H5T_STD_I8LE, sid2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (did2 < 0) {
        fprintf(stderr, "Error: %s %d> H5Dcreate2 failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* write values to dataset */
    status = H5Dwrite(did2, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT, data2);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Dwrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

out:
    if (did1 > 0)
        H5Dclose(did1);
    if (gid > 0)
        H5Gclose(gid);
    if (tid > 0)
        H5Tclose(tid);
    if (sid > 0)
        H5Sclose(sid);

    if (did2 > 0)
        H5Dclose(did2);
    if (sid2 > 0)
        H5Sclose(sid2);
    return ret;
}

/*-------------------------------------------------------------------------
 * Function: gen_obj_ref
 *
 * Purpose:
 *  Generate object references to objects (dataset,group and named datatype)
 *
 * Note:
 *  copied from h5copygentest.c and update to create named datatype
 *
 *------------------------------------------------------------------------*/
static herr_t
gen_obj_ref(hid_t loc_id)
{
    int    status;
    herr_t ret = SUCCEED;

    hid_t   sid = 0, oid = 0;
    hsize_t dims_dset_objref[1] = {3};

    /* attr with int type */
    hsize_t dim_attr_int[1]  = {2};
    int     data_attr_int[2] = {10, 20};

    /* write buffer for obj reference */
    hobj_ref_t objref_buf[3];

    /*---------------------------------------------------------
     * create obj references to the previously created objects.
     * Passing -1 as reference is an object.*/

    /* obj ref to dataset */
    status = H5Rcreate(&objref_buf[0], loc_id, NAME_OBJ_DS1, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* obj ref to group */
    status = H5Rcreate(&objref_buf[1], loc_id, NAME_OBJ_GRP, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* obj ref to named-datatype */
    status = H5Rcreate(&objref_buf[2], loc_id, NAME_OBJ_NDTYPE, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*---------------------------------------------------------
     * create dataset contain references
     */
    sid = H5Screate_simple(1, dims_dset_objref, NULL);
    if (sid < 0) {
        fprintf(stderr, "Error: %s %d> H5Screate_simple failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    oid = H5Dcreate2(loc_id, "Dset_OBJREF", H5T_STD_REF_OBJ, sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (oid < 0) {
        fprintf(stderr, "Error: %s %d> H5Dcreate2 failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    status = H5Dwrite(oid, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, objref_buf);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Dwrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* add attribute with int type */
    if (make_attr(oid, 1, dim_attr_int, "integer", H5T_NATIVE_INT, data_attr_int) < 0)
        goto out;

    /* add attribute with obj ref */
    status = add_attr_with_objref(loc_id, oid);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> add_attr_with_objref failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* add attribute with region ref */
    status = add_attr_with_regref(loc_id, oid);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> add_attr_with_regref failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

out:
    if (oid > 0)
        H5Dclose(oid);
    if (sid > 0)
        H5Sclose(sid);

    return ret;
}

/*-------------------------------------------------------------------------
 * Function: gen_region_ref
 *
 * Purpose: Generate dataset region references
 *
 * Note:
 *  copied from h5copygentest.c
 *
 *------------------------------------------------------------------------*/
static herr_t
gen_region_ref(hid_t loc_id)
{
    int    status;
    herr_t ret = SUCCEED;

    /* target dataset */
    hid_t   sid_trg     = 0;
    hsize_t dims_trg[2] = {3, 16};

    /* dset with region ref type */
    hid_t sid_ref = 0, oid_ref = 0;

    /* region ref to target dataset */
    hsize_t         coords[4][2] = {{0, 1}, {2, 11}, {1, 0}, {2, 4}};
    hdset_reg_ref_t rr_data[2];
    hsize_t         start[2]  = {0, 0};
    hsize_t         stride[2] = {2, 11};
    hsize_t         count[2]  = {2, 2};
    hsize_t         block[2]  = {1, 3};
    hsize_t         dims1[1]  = {2};

    /* attr with int type */
    hsize_t dim_attr_int[1]  = {2};
    int     data_attr_int[2] = {10, 20};

    sid_trg = H5Screate_simple(2, dims_trg, NULL);
    if (sid_trg < 0) {
        fprintf(stderr, "Error: %s %d> H5Screate_simple failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* select elements space for reference */
    status = H5Sselect_elements(sid_trg, H5S_SELECT_SET, (size_t)4, coords[0]);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Sselect_elements failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* create region reference from elements space */
    status = H5Rcreate(&rr_data[0], loc_id, NAME_OBJ_DS2, H5R_DATASET_REGION, sid_trg);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* select hyperslab space for reference */
    status = H5Sselect_hyperslab(sid_trg, H5S_SELECT_SET, start, stride, count, block);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Sselect_hyperslab failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* create region reference from hyperslab space */
    status = H5Rcreate(&rr_data[1], loc_id, NAME_OBJ_DS2, H5R_DATASET_REGION, sid_trg);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* Create dataspace. */
    sid_ref = H5Screate_simple(1, dims1, NULL);
    if (sid_ref < 0) {
        fprintf(stderr, "Error: %s %d> H5Screate_simple failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* create region reference dataset */
    oid_ref =
        H5Dcreate2(loc_id, REG_REF_DS1, H5T_STD_REF_DSETREG, sid_ref, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (oid_ref < 0) {
        fprintf(stderr, "Error: %s %d> H5Dcreate2 failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* write data as region references */
    status = H5Dwrite(oid_ref, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL, H5P_DEFAULT, rr_data);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Dwrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* add attribute with int type */
    if (make_attr(oid_ref, 1, dim_attr_int, "integer", H5T_NATIVE_INT, data_attr_int) < 0)
        goto out;

    /* add attribute with obj ref */
    status = add_attr_with_objref(loc_id, oid_ref);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> add_attr_with_objref failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* add attribute with region ref */
    status = add_attr_with_regref(loc_id, oid_ref);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> add_attr_with_regref failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

out:
    if (oid_ref > 0)
        H5Dclose(oid_ref);
    if (sid_ref > 0)
        H5Sclose(sid_ref);
    if (sid_trg > 0)
        H5Sclose(sid_trg);

    return ret;
}

/*-------------------------------------------------------------------------
 * Function: make_references
 *
 * Purpose: create a file with obj and region references
 *
 *-------------------------------------------------------------------------
 */
static herr_t
make_references(hid_t loc_id)
{
    herr_t ret = SUCCEED;
    herr_t status;

    /* add target objects */
    status = gen_refered_objs(loc_id);
    if (status == FAIL) {
        fprintf(stderr, "Failed to generate referenced object.\n");
        ret = FAIL;
    }

    /* add object reference */
    status = gen_obj_ref(loc_id);
    if (status == FAIL) {
        fprintf(stderr, "Failed to generate object reference.\n");
        ret = FAIL;
    }

    /* add region reference */
    status = gen_region_ref(loc_id);
    if (status == FAIL) {
        fprintf(stderr, "Failed to generate region reference.\n");
        ret = FAIL;
    }

    return ret;
}

/*-------------------------------------------------------------------------
 * Function: make_complex_attr_references
 *
 * Purpose:
 *   create a file with :
 *   1. obj ref in attribute of compound type
 *   2. region ref in attribute of compound type
 *   3. obj ref in attribute of vlen type
 *   4. region ref in attribute of vlen type
 *
 *-------------------------------------------------------------------------
 */
/* obj dset */
#define RANK_OBJ 2
#define DIM0_OBJ 6
#define DIM1_OBJ 10
/* container dset */
#define RANK_DSET 1
#define DIM_DSET  4
/* 1. obj references in compound attr */
#define RANK_COMP_OBJREF 1
#define DIM_COMP_OBJREF  3 /* for dataset, group, datatype */
/* 2. region references in compound attr */
#define RANK_COMP_REGREF 1
#define DIM_COMP_REGREF  1 /* for element region */
/* 3. obj references in vlen attr */
#define RANK_VLEN_OBJREF 1
#define DIM_VLEN_OBJREF  3 /* for dataset, group, datatype */
#define LEN0_VLEN_OBJREF 1 /* dataset */
#define LEN1_VLEN_OBJREF 1 /* group */
#define LEN2_VLEN_OBJREF 1 /* datatype */
/* 4. region references in vlen attr */
#define RANK_VLEN_REGREF 1
#define DIM_VLEN_REGREF  1 /*  for element region */
#define LEN0_VLEN_REGREF 1 /* element region  */

static herr_t
make_complex_attr_references(hid_t loc_id)
{
    herr_t ret = SUCCEED;
    herr_t status;
    /*
     * for objects
     */
    hid_t   objgid = 0, objdid = 0, objtid = 0, objsid = 0;
    hsize_t obj_dims[RANK_OBJ]           = {DIM0_OBJ, DIM1_OBJ};
    int     obj_data[DIM0_OBJ][DIM1_OBJ] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},           {10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
        {20, 21, 22, 23, 24, 25, 26, 27, 28, 29}, {30, 31, 32, 33, 34, 35, 36, 37, 38, 39},
        {40, 41, 42, 43, 44, 45, 46, 47, 48, 49}, {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}};

    /*
     * group main
     */
    hid_t main_gid = 0;
    /*
     * dataset which the attribute will be attached to
     */
    hsize_t main_dset_dims[RANK_DSET] = {DIM_DSET};
    hid_t   main_sid = 0, main_did = 0;
    /*
     * 1. obj references in compound attr
     */
    hid_t comp_objref_tid = 0, comp_objref_aid = 0;
    typedef struct comp_objref_t {
        hobj_ref_t val_objref;
        int        val_int;
    } comp_objref_t;
    comp_objref_t comp_objref_data[DIM_COMP_OBJREF];
    hid_t         comp_objref_attr_sid              = 0;
    hsize_t       comp_objref_dim[RANK_COMP_OBJREF] = {DIM_COMP_OBJREF};

    /*
     * 2. region references in compound attr
     */
    hid_t comp_regref_tid = 0, comp_regref_aid = 0;
    typedef struct comp_regref_t {
        hdset_reg_ref_t val_regref;
        int             val_int;
    } comp_regref_t;
    comp_regref_t comp_regref_data[DIM_COMP_REGREF];
    hid_t         comp_regref_attr_sid              = 0;
    hsize_t       comp_regref_dim[RANK_COMP_REGREF] = {DIM_COMP_REGREF};
    hsize_t       coords[4][2]                      = {{0, 1}, {2, 3}, {3, 4}, {4, 5}};

    /*
     * 3. obj references in vlen attr
     */
    hid_t   vlen_objref_attr_tid = 0, vlen_objref_attr_sid = 0;
    hid_t   vlen_objref_attr_id = 0;
    hvl_t   vlen_objref_data[DIM_VLEN_OBJREF];
    hsize_t vlen_objref_dims[RANK_VLEN_OBJREF] = {DIM_VLEN_OBJREF};

    /*
     * 4. region references in vlen attr
     */
    hid_t   vlen_regref_attr_tid = 0, vlen_regref_attr_sid = 0;
    hid_t   vlen_regref_attr_id = 0;
    hvl_t   vlen_regref_data[DIM_VLEN_REGREF];
    hsize_t vlen_regref_dim[RANK_VLEN_REGREF] = {DIM_VLEN_REGREF};

    /* ---------------------------------------
     * create objects which to be referenced
     */
    /* object1 group */
    objgid = H5Gcreate2(loc_id, NAME_OBJ_GRP, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /* object2 dataset */
    objsid = H5Screate_simple(RANK_OBJ, obj_dims, NULL);
    objdid = H5Dcreate2(loc_id, NAME_OBJ_DS1, H5T_NATIVE_INT, objsid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite(objdid, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, obj_data[0]);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Dwrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* object3 named datatype */
    objtid = H5Tcopy(H5T_NATIVE_INT);
    status = H5Tcommit2(loc_id, NAME_OBJ_NDTYPE, objtid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Tcommit2 failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* ---------------------------------------------
     *  Put testing objs in this group
     * create group contain dataset with attribute and the attribute has
     * compound type which contain obj and region reference */
    main_gid = H5Gcreate2(loc_id, "group_main", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (main_gid < 0) {
        fprintf(stderr, "Error: %s %d> H5Gcreate2 failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*----------------------------------------------------------
     * create dataset which the attribute will be attached to
     */
    main_sid = H5Screate_simple(RANK_DSET, main_dset_dims, NULL);

    main_did =
        H5Dcreate2(main_gid, "dset_main", H5T_NATIVE_INT, main_sid, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite(main_did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, obj_data[0]);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Dwrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*-------------------------------------------------------------------
     * 1. create obj references in a attribute of compound type
     */

    /*
     * create compound type for attribute
     */
    comp_objref_tid = H5Tcreate(H5T_COMPOUND, sizeof(comp_objref_t));

    H5Tinsert(comp_objref_tid, "value_objref", HOFFSET(comp_objref_t, val_objref), H5T_STD_REF_OBJ);
    H5Tinsert(comp_objref_tid, "value_int", HOFFSET(comp_objref_t, val_int), H5T_NATIVE_INT);

    /*
     * Create the object references into compound type
     */
    /* references to dataset */
    status = H5Rcreate(&(comp_objref_data[0].val_objref), loc_id, NAME_OBJ_DS1, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }
    comp_objref_data[0].val_int = 0;

    /* references to group */
    status = H5Rcreate(&(comp_objref_data[1].val_objref), loc_id, NAME_OBJ_GRP, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }
    comp_objref_data[1].val_int = 10;

    /* references to datatype */
    status = H5Rcreate(&(comp_objref_data[2].val_objref), loc_id, NAME_OBJ_NDTYPE, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }
    comp_objref_data[2].val_int = 20;

    /*
     * create attribute and write the object ref
     */
    comp_objref_attr_sid = H5Screate_simple(RANK_COMP_OBJREF, comp_objref_dim, NULL);
    comp_objref_aid =
        H5Acreate2(main_did, "Comp_OBJREF", comp_objref_tid, comp_objref_attr_sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(comp_objref_aid, comp_objref_tid, comp_objref_data);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Awrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*-------------------------------------------------------------------
     * 2. create region references in attribute of compound type
     */
    /*
     * create compound type for attribute
     */
    comp_regref_tid = H5Tcreate(H5T_COMPOUND, sizeof(comp_regref_t));

    H5Tinsert(comp_regref_tid, "value_regref", HOFFSET(comp_regref_t, val_regref), H5T_STD_REF_DSETREG);
    H5Tinsert(comp_regref_tid, "value_int", HOFFSET(comp_regref_t, val_int), H5T_NATIVE_INT);

    /*
     * create the region reference
     */
    status = H5Sselect_elements(objsid, H5S_SELECT_SET, (size_t)4, coords[0]);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Sselect_elements failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }
    status = H5Rcreate(&(comp_regref_data[0].val_regref), loc_id, NAME_OBJ_DS1, H5R_DATASET_REGION, objsid);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }
    comp_regref_data[0].val_int = 10;

    /*
     * create attribute and write the region ref
     */
    comp_regref_attr_sid = H5Screate_simple(RANK_COMP_REGREF, comp_regref_dim, NULL);
    comp_regref_aid =
        H5Acreate2(main_did, "Comp_REGREF", comp_regref_tid, comp_regref_attr_sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(comp_regref_aid, comp_regref_tid, comp_regref_data);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Awrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*-------------------------------------------------------------------
     * 3. create obj references in attribute of vlen type
     */
    /*
     * prepare vlen data
     */
    vlen_objref_data[0].len = LEN0_VLEN_OBJREF;
    vlen_objref_data[0].p   = malloc(vlen_objref_data[0].len * sizeof(hobj_ref_t));
    vlen_objref_data[1].len = LEN1_VLEN_OBJREF;
    vlen_objref_data[1].p   = malloc(vlen_objref_data[1].len * sizeof(hobj_ref_t));
    vlen_objref_data[2].len = LEN2_VLEN_OBJREF;
    vlen_objref_data[2].p   = malloc(vlen_objref_data[2].len * sizeof(hobj_ref_t));

    /*
     * create obj references
     */
    /* reference to dataset */
    status =
        H5Rcreate(&((hobj_ref_t *)vlen_objref_data[0].p)[0], loc_id, NAME_OBJ_DS1, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }
    /* reference to group */
    status =
        H5Rcreate(&((hobj_ref_t *)vlen_objref_data[1].p)[0], loc_id, NAME_OBJ_GRP, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }
    /* reference to datatype */
    status =
        H5Rcreate(&((hobj_ref_t *)vlen_objref_data[2].p)[0], loc_id, NAME_OBJ_NDTYPE, H5R_OBJECT, (hid_t)-1);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*
     * create vlen type with obj reference
     */
    vlen_objref_attr_tid = H5Tvlen_create(H5T_STD_REF_OBJ);
    vlen_objref_attr_sid = H5Screate_simple(RANK_VLEN_OBJREF, vlen_objref_dims, NULL);

    /*
     * create attribute and write the object reference
     */
    vlen_objref_attr_id = H5Acreate2(main_did, "Vlen_OBJREF", vlen_objref_attr_tid, vlen_objref_attr_sid,
                                     H5P_DEFAULT, H5P_DEFAULT);
    status              = H5Awrite(vlen_objref_attr_id, vlen_objref_attr_tid, vlen_objref_data);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Awrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* close resource for vlen data */
    status = H5Treclaim(vlen_objref_attr_tid, vlen_objref_attr_sid, H5P_DEFAULT, vlen_objref_data);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Treclaim failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*-------------------------------------------------------------------
     * 4. create region references in a attribute of vlen type
     */

    /*
     * prepare vlen data
     */
    vlen_regref_data[0].len = LEN0_VLEN_REGREF;
    vlen_regref_data[0].p   = malloc(vlen_regref_data[0].len * sizeof(hdset_reg_ref_t));

    /*
     * create region reference
     */
    status = H5Sselect_elements(objsid, H5S_SELECT_SET, (size_t)4, coords[0]);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Sselect_elements failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }
    status = H5Rcreate(&((hdset_reg_ref_t *)vlen_regref_data[0].p)[0], loc_id, NAME_OBJ_DS1,
                       H5R_DATASET_REGION, objsid);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Rcreate failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /*
     * create vlen type with region reference
     */
    vlen_regref_attr_tid = H5Tvlen_create(H5T_STD_REF_DSETREG);
    vlen_regref_attr_sid = H5Screate_simple(RANK_VLEN_REGREF, vlen_regref_dim, NULL);

    /*
     * create attribute and write the region reference
     */
    vlen_regref_attr_id = H5Acreate2(main_did, "Vlen_REGREF", vlen_regref_attr_tid, vlen_regref_attr_sid,
                                     H5P_DEFAULT, H5P_DEFAULT);
    status              = H5Awrite(vlen_regref_attr_id, vlen_regref_attr_tid, vlen_regref_data);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Awrite failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

    /* close resource for vlen data */
    status = H5Treclaim(vlen_regref_attr_tid, vlen_regref_attr_sid, H5P_DEFAULT, vlen_regref_data);
    if (status < 0) {
        fprintf(stderr, "Error: %s %d> H5Treclaim failed.\n", __func__, __LINE__);
        ret = FAIL;
        goto out;
    }

out:
    /* release resources */
    if (objgid > 0)
        H5Gclose(objgid);
    if (objsid > 0)
        H5Sclose(objsid);
    if (objdid > 0)
        H5Dclose(objdid);
    if (objtid > 0)
        H5Tclose(objtid);

    if (main_gid > 0)
        H5Gclose(main_gid);
    if (main_sid > 0)
        H5Sclose(main_sid);
    if (main_did > 0)
        H5Dclose(main_did);
    /* comp obj ref */
    if (comp_objref_tid > 0)
        H5Tclose(comp_objref_tid);
    if (comp_objref_aid > 0)
        H5Aclose(comp_objref_aid);
    if (comp_objref_attr_sid > 0)
        H5Sclose(comp_objref_attr_sid);
    /* comp region ref */
    if (comp_regref_tid > 0)
        H5Tclose(comp_regref_tid);
    if (comp_regref_aid > 0)
        H5Aclose(comp_regref_aid);
    if (comp_regref_attr_sid > 0)
        H5Sclose(comp_regref_attr_sid);
    /* vlen obj ref */
    if (vlen_objref_attr_id > 0)
        H5Aclose(vlen_objref_attr_id);
    if (vlen_objref_attr_sid > 0)
        H5Sclose(vlen_objref_attr_sid);
    if (vlen_objref_attr_tid > 0)
        H5Tclose(vlen_objref_attr_tid);
    /* vlen region ref */
    if (vlen_regref_attr_id > 0)
        H5Aclose(vlen_regref_attr_id);
    if (vlen_regref_attr_sid > 0)
        H5Sclose(vlen_regref_attr_sid);
    if (vlen_regref_attr_tid > 0)
        H5Tclose(vlen_regref_attr_tid);

    return ret;
}