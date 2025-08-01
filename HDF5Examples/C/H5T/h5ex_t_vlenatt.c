/************************************************************

  This example shows how to read and write variable-length
  datatypes to an attribute.  The program first writes two
  variable-length integer arrays to the attribute then
  closes the file.  Next, it reopens the file, reads back
  the data, and outputs it to the screen.

Note: This example includes older cases from previous versions
  of HDF5 for historical reference and to illustrate how to
  migrate older code to newer functions. However, readers are
  encouraged to avoid using deprecated functions and earlier
  schemas from those versions.

 ************************************************************/

#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>

#define FILENAME  "h5ex_t_vlenatt.h5"
#define DATASET   "DS1"
#define ATTRIBUTE "A1"
#define LEN0      3
#define LEN1      12

int
main(void)
{
    hid_t file, filetype, memtype, space, dset, attr;
    /* Handles */
    herr_t status;
    hvl_t  wdata[2], /* Array of vlen structures */
        *rdata;      /* Pointer to vlen structures */
    hsize_t dims[1] = {2};
    int    *ptr, ndims;
    hsize_t i, j;

    /*
     * Initialize variable-length data.  wdata[0] is a countdown of
     * length LEN0, wdata[1] is a Fibonacci sequence of length LEN1.
     */
    wdata[0].len = LEN0;
    ptr          = (int *)malloc(wdata[0].len * sizeof(int));
    for (i = 0; i < wdata[0].len; i++)
        ptr[i] = wdata[0].len - (size_t)i; /* 3 2 1 */
    wdata[0].p = (void *)ptr;

    wdata[1].len = LEN1;
    ptr          = (int *)malloc(wdata[1].len * sizeof(int));
    ptr[0]       = 1;
    ptr[1]       = 1;
    for (i = 2; i < wdata[1].len; i++)
        ptr[i] = ptr[i - 1] + ptr[i - 2]; /* 1 1 2 3 5 8 etc. */
    wdata[1].p = (void *)ptr;

    /*
     * Create a new file using the default properties.
     */
    file = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create variable-length datatype for file and memory.
     */
    filetype = H5Tvlen_create(H5T_STD_I32LE);
    memtype  = H5Tvlen_create(H5T_NATIVE_INT);

    /*
     * Create dataset with a null dataspace.
     */
    space  = H5Screate(H5S_NULL);
    dset   = H5Dcreate(file, DATASET, H5T_STD_I32LE, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Sclose(space);

    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    space = H5Screate_simple(1, dims, NULL);

    /*
     * Create the attribute and write the variable-length data to it
     */
    attr   = H5Acreate(dset, ATTRIBUTE, filetype, space, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, memtype, wdata);

    /*
     * Close and release resources.  Note the use of H5Dvlen_reclaim
     * removes the need to manually free() the previously malloc'ed
     * data.
     */
#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_110_API) && !defined(H5_USE_18_API) && !defined(H5_USE_16_API)
    status = H5Treclaim(memtype, space, H5P_DEFAULT, wdata);
#else
    status = H5Dvlen_reclaim(memtype, space, H5P_DEFAULT, wdata);
#endif
    status = H5Aclose(attr);
    status = H5Dclose(dset);
    status = H5Sclose(space);
    status = H5Tclose(filetype);
    status = H5Tclose(memtype);
    status = H5Fclose(file);

    /*
     * Now we begin the read section of this example.  Here we assume
     * the attribute has the same name and rank, but can have any size.
     * Therefore we must allocate a new array to read in data using
     * malloc().
     */

    /*
     * Open file, dataset, and attribute.
     */
    file = H5Fopen(FILENAME, H5F_ACC_RDONLY, H5P_DEFAULT);
    dset = H5Dopen(file, DATASET, H5P_DEFAULT);
    attr = H5Aopen(dset, ATTRIBUTE, H5P_DEFAULT);

    /*
     * Get dataspace and allocate memory for array of vlen structures.
     * This does not actually allocate memory for the vlen data, that
     * will be done by the library.
     */
    space = H5Aget_space(attr);
    ndims = H5Sget_simple_extent_dims(space, dims, NULL);
    rdata = (hvl_t *)malloc(dims[0] * sizeof(hvl_t));

    /*
     * Create the memory datatype.
     */
    memtype = H5Tvlen_create(H5T_NATIVE_INT);

    /*
     * Read the data.
     */
    status = H5Aread(attr, memtype, rdata);

    /*
     * Output the variable-length data to the screen.
     */
    for (i = 0; i < dims[0]; i++) {
        printf("%s[%" PRIuHSIZE "]:\n  {", ATTRIBUTE, i);
        ptr = rdata[i].p;
        for (j = 0; j < rdata[i].len; j++) {
            printf(" %d", ptr[j]);
            if ((j + 1) < rdata[i].len)
                printf(",");
        }
        printf(" }\n");
    }

    /*
     * Close and release resources.  Note we must still free the
     * top-level pointer "rdata", as H5Dvlen_reclaim only frees the
     * actual variable-length data, and not the structures themselves.
     */
#if H5_VERSION_GE(1, 12, 0) && !defined(H5_USE_110_API) && !defined(H5_USE_18_API) && !defined(H5_USE_16_API)
    status = H5Treclaim(memtype, space, H5P_DEFAULT, rdata);
#else
    status = H5Dvlen_reclaim(memtype, space, H5P_DEFAULT, rdata);
#endif
    free(rdata);
    status = H5Aclose(attr);
    status = H5Dclose(dset);
    status = H5Sclose(space);
    status = H5Tclose(memtype);
    status = H5Fclose(file);

    return 0;
}
