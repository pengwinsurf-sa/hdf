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
#include "hdf5_hl.h"
#include <stdlib.h>

/*-------------------------------------------------------------------------
 * Table API example
 *
 * H5TBmake_table
 * H5TBread_table
 *
 *-------------------------------------------------------------------------
 */

#define NFIELDS    (hsize_t)5
#define NRECORDS   (hsize_t)8
#define TABLE_NAME "table"
#define FILENAME   "h5ex_table_01.h5"

int
main(void)
{
    typedef struct Particle {
        char   name[16];
        int    lati;
        int    longi;
        float  pressure;
        double temperature;
    } Particle;

    Particle dst_buf[NRECORDS];

    /* Calculate the size and the offsets of our struct members in memory */
    size_t dst_size            = sizeof(Particle);
    size_t dst_offset[NFIELDS] = {HOFFSET(Particle, name), HOFFSET(Particle, lati), HOFFSET(Particle, longi),
                                  HOFFSET(Particle, pressure), HOFFSET(Particle, temperature)};

    size_t dst_sizes[NFIELDS] = {sizeof(dst_buf[0].name), sizeof(dst_buf[0].lati), sizeof(dst_buf[0].longi),
                                 sizeof(dst_buf[0].pressure), sizeof(dst_buf[0].temperature)};

    /* Define an array of Particles */
    Particle p_data[NRECORDS] = {{"zero", 0, 1, 0.2F, 3.0},    {"one", 10, 11, 1.2F, 13.0},
                                 {"two", 20, 21, 2.2F, 23.0},  {"three", 30, 31, 3.2F, 33.0},
                                 {"four", 40, 41, 4.2F, 43.0}, {"five", 50, 51, 5.2F, 53.0},
                                 {"six", 60, 61, 6.2F, 63.0},  {"seven", 70, 71, 7.2F, 73.0}};

    /* Define field information */
    const char *field_names[NFIELDS] = {"Name", "Latitude", "Longitude", "Pressure", "Temperature"};
    hid_t       field_type[NFIELDS];
    hid_t       string_type;
    hid_t       file_id;
    hsize_t     chunk_size = 10;
    int        *fill_data  = NULL;
    int         compress   = 0;
    int         i;

    /* Initialize field_type */
    string_type = H5Tcopy(H5T_C_S1);
    H5Tset_size(string_type, 16);
    field_type[0] = string_type;
    field_type[1] = H5T_NATIVE_INT;
    field_type[2] = H5T_NATIVE_INT;
    field_type[3] = H5T_NATIVE_FLOAT;
    field_type[4] = H5T_NATIVE_DOUBLE;

    /* Create a new file using default properties. */
    file_id = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*-------------------------------------------------------------------------
     * H5TBmake_table
     *-------------------------------------------------------------------------
     */

    H5TBmake_table("Table Title", file_id, TABLE_NAME, NFIELDS, NRECORDS, dst_size, field_names, dst_offset,
                   field_type, chunk_size, fill_data, compress, p_data);

    /*-------------------------------------------------------------------------
     * H5TBread_table
     *-------------------------------------------------------------------------
     */

    H5TBread_table(file_id, TABLE_NAME, dst_size, dst_offset, dst_sizes, dst_buf);

    /* print it by rows */
    for (i = 0; i < NRECORDS; i++) {
        printf("%-5s %-5d %-5d %-5f %-5f", dst_buf[i].name, dst_buf[i].lati, dst_buf[i].longi,
               dst_buf[i].pressure, dst_buf[i].temperature);
        printf("\n");
    }

    /*-------------------------------------------------------------------------
     * end
     *-------------------------------------------------------------------------
     */

    /* close type */
    H5Tclose(string_type);

    /* close the file */
    H5Fclose(file_id);

    return 0;
}
