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

/*
 * The purpose of this test is to verify if a virtual file driver can handle:
 *    a. Large file (2GB)
 *    This should exceed 32bits I/O system since offset is a signed
 *    integral type (in order to support negative offset with respect to
 *    end of file).
 *    b. Extra Large file (4GB)
 *    This definite exceeds 32bit I/O and file systems.
 *    c. Huge file (tens of GB)
 *    This verifies the HDF5 library handles big logical file size
 *    correctly.
 * In practice, if a VFD can handle a big file size, there is no need to
 * test the smaller file sizes.  E.g., If it can handle the Huge file,
 * there is no need to test the Extra large or Large files. Therefore the
 * test starts with larger size files and continues to test the smaller size
 * files only if the large sige file tests have failed.
 *
 * Another consideration is that even if a VFD is capable to handle a
 * huge file but it is likely to take a long time to write every byte
 * of a huge file. E.g., a simple workstation may have disks of write
 * speed of 10MB/sec. A huge file of 30GB will take about an hour to
 * write it. Therefore, this test will run the huge file test only if the
 * underlying file system supports sparse file. (A Sparse file here means
 * that disk space is allocated only when the contents are actually written.
 * E.g., If one creates a new file, seeks forward 10 million bytes, writes
 * 1 bytes and closes the file, then a sparse file, will show file size of
 * 10 million bytes but actually uses only couple disk blocks, much smaller
 * than the formal file size.)
 *
 * One more consideration is that we want to distinguish an HDF5 library
 * failure from some system limits such as current free disk space or user
 * disk space quota. Therefore, the test will first attempt to verify no
 * such limits exist before running the actual VFD tests.
 */
#include "h5test.h"

#define DNAME "big.data"

#define WRT_N       50
#define WRT_SIZE    (4 * 1024)
#define FAMILY_SIZE (1024 * 1024 * 1024)

#define GB (HDoff_t)0x40000000L

#define MAX_TRIES 100

/* Define Small, Large, Extra Large, Huge File which
 * correspond to less than 2GB, 2GB, 4GB, and tens of GB file size.
 * NO_FILE stands for "no file" to be tested.
 */
typedef enum fsizes_t { SFILE, LFILE, XLFILE, HUGEFILE, NO_FILE } fsizes_t;
fsizes_t file_size = NO_FILE;

const char *FILENAME[]      = {"big", "sec2", "stdio", NULL};
int         cflag           = 1;           /* check file system before test */
int         sparse_support  = 0;           /* sparse file supported, default false */
int         have_space      = 0;           /* enough space for huge file test, default false */
hsize_t     family_size_def = FAMILY_SIZE; /* default family file size */

/* Prototypes */
static void usage(void);
static int  test_sec2(hid_t fapl);
static int  test_stdio(hid_t fapl);
static int  test_family(hid_t fapl);

/* Array used to record all addresses at which data has been written */
/* so far.  Used to prevent overlapping writes. */
static hsize_t values_used[WRT_N];

/*-------------------------------------------------------------------------
 * Function:    randll
 *
 * Purpose:    Create a random long long value.
 *         Ensures that a write at this value doesn't overlap any
 *         previous write.
 *
 * Return:    Success:    Random value
 *
 *            Failure:    Random value which overlaps another write
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
randll(hsize_t limit, int current_index)
{
    hsize_t acc     = 0;
    int     overlap = 1;
    int     i;
    int     tries = 0;

    /* Generate up to MAX_TRIES random numbers until one of them */
    /* does not overlap with any previous writes */
    while (overlap != 0 && tries < MAX_TRIES) {
        acc = (hsize_t)rand();
        acc *= (hsize_t)rand();
        acc     = acc % limit;
        overlap = 0;

        for (i = 0; i < current_index; i++) {
            if ((acc >= values_used[i]) && (acc < values_used[i] + WRT_SIZE))
                overlap = 1;
            if ((acc + WRT_SIZE >= values_used[i]) && (acc + WRT_SIZE < values_used[i] + WRT_SIZE))
                overlap = 1;
        }
        tries++;
    }

    values_used[current_index] = acc;

    return acc;
}

/*-------------------------------------------------------------------------
 * Function:    is_sparse
 *
 * Purpose:    Determines if the file system of the current working
 *             directory supports holes.
 *
 * Return:    Success:    Non-zero if holes are supported; zero
 *                otherwise.
 *
 *            Failure:    zero
 *
 *-------------------------------------------------------------------------
 */
static int
is_sparse(void)
{
    int       fd;
    h5_stat_t sb;

    if ((fd = HDopen("x.h5", O_RDWR | O_TRUNC | O_CREAT, H5_POSIX_CREATE_MODE_RW)) < 0)
        return 0;
    if (HDlseek(fd, (1024 * 1024), SEEK_SET) != 1024 * 1024)
        return 0;
    if (5 != HDwrite(fd, "hello", (size_t)5))
        return 0;
    if (HDclose(fd) < 0)
        return 0;
    memset(&sb, 0, sizeof(h5_stat_t));
    if (HDstat("x.h5", &sb) < 0)
        return 0;
    if (HDremove("x.h5") < 0)
        return 0;
#ifdef H5_HAVE_STAT_ST_BLOCKS
    return ((unsigned long)sb.st_blocks * 512 < (unsigned long)sb.st_size);
#else
    return (0);
#endif
}

/*-------------------------------------------------------------------------
 * Function:    supports_big
 *
 * Purpose:    Determines if the file system of the current working
 *             directory supports big files.
 *
 * Return:    Success:    Non-zero if big files are supported; zero
 *                otherwise.
 *
 *            Failure:    zero
 *
 *-------------------------------------------------------------------------
 */
static fsizes_t
supports_big(void)
{
    int      fd    = -1;
    fsizes_t fsize = NO_FILE;

    if ((fd = HDopen("y.h5", O_RDWR | O_TRUNC | O_CREAT, H5_POSIX_CREATE_MODE_RW)) < 0)
        goto error;

    /* Write a few byte at the beginning */
    if (5 != HDwrite(fd, "hello", (size_t)5))
        goto quit;
    fsize = SFILE;

    /* Write a few bytes at 2GB */
    if (HDlseek(fd, 2 * GB, SEEK_SET) != 2 * GB)
        goto quit;
    if (5 != HDwrite(fd, "hello", (size_t)5))
        goto quit;
    fsize = LFILE;

    /* Write a few bytes at 4GB */
    if (HDlseek(fd, 4 * GB, SEEK_SET) != 4 * GB)
        goto quit;
    if (5 != HDwrite(fd, "hello", (size_t)5))
        goto quit;
    fsize = XLFILE;

    /* If this supports sparse_file, write a few bytes at 32GB */
    if (!sparse_support)
        goto quit;
    if (HDlseek(fd, 32 * GB, SEEK_SET) != 32 * GB)
        goto quit;
    if (5 != HDwrite(fd, "hello", (size_t)5))
        goto quit;
    fsize = HUGEFILE;

quit:
    if (HDclose(fd) < 0)
        goto error;
    if (HDremove("y.h5") < 0)
        goto error;
    return fsize;

error:
    if (fd >= 0) {
        HDclose(fd);
        HDremove("y.h5");
    }
    return fsize;
}

/*-------------------------------------------------------------------------
 * Function:    enough_room
 *
 * Purpose:    Tries to create a bunch of sparse files to see if quotas will
 *        get in the way.  Some systems also have problems opening
 *        enough files and we'll check that too.
 *
 * Return:    Success:    Non-zero
 *
 *            Failure:    zero
 *
 *-------------------------------------------------------------------------
 */
static int
enough_room(hid_t fapl)
{
    int    ret_value = 0;
    int    fd[68];
    size_t i, size = (size_t)1 << 30;
    char   filename[1024], name[1024];

    /* Initialize file descriptors */
    for (i = 0; i < NELMTS(fd); i++)
        fd[i] = -1;

    /* Get file name template */
    assert(H5FD_FAMILY == H5Pget_driver(fapl));
    h5_fixname(FILENAME[0], fapl, filename, sizeof(filename));

    /* Create files */
    for (i = 0; i < NELMTS(fd); i++) {
        H5_WARN_FORMAT_NONLITERAL_OFF
        snprintf(name, sizeof(name), filename, i);
        H5_WARN_FORMAT_NONLITERAL_ON
        if ((fd[i] = HDopen(name, O_RDWR | O_CREAT | O_TRUNC, H5_POSIX_CREATE_MODE_RW)) < 0) {
            goto done;
        }
        if ((HDoff_t)size != HDlseek(fd[i], (HDoff_t)size, SEEK_SET)) {
            goto done;
        }
        if (1 != HDwrite(fd[i], "X", (size_t)1)) {
            goto done;
        }
    }
    ret_value = 1;

done:
    for (i = 0; i < NELMTS(fd) && fd[i] >= 0; i++) {
        H5_WARN_FORMAT_NONLITERAL_OFF
        snprintf(name, sizeof(name), filename, i);
        H5_WARN_FORMAT_NONLITERAL_ON
        if (HDclose(fd[i]) < 0)
            ret_value = 0;
        HDremove(name);
    }

    return ret_value;
}

/*-------------------------------------------------------------------------
 * Function:    writer
 *
 * Purpose:    Creates a *big* dataset.
 *
 * Return:    Success:    0
 *
 *            Failure:    >0
 *
 *-------------------------------------------------------------------------
 */
static int
writer(char *filename, hid_t fapl, fsizes_t testsize, int wrt_n)
{
    hsize_t size1[4] = {8, 1024, 1024, 1024};
    hsize_t size2[1] = {8LL * 1024LL * 1024LL * 1024LL};
    hsize_t hs_start[1];
    hsize_t hs_size[1];
    hid_t   file = H5I_INVALID_HID, space1 = H5I_INVALID_HID, space2 = H5I_INVALID_HID,
          mem_space = H5I_INVALID_HID, d1 = H5I_INVALID_HID, d2 = H5I_INVALID_HID;
    int  *buf = (int *)malloc(sizeof(int) * WRT_SIZE);
    int   i, j;
    FILE *out = fopen(DNAME, "w");
    hid_t dcpl;

    switch (testsize) {
        case LFILE:
            TESTING("Large dataset write(2GB)");
            /* reduce size1 to produce a 2GB dataset */
            size1[1] = 1024 / 16;
            size2[0] /= 16;
            break;

        case XLFILE:
            TESTING("Extra large dataset write(4GB)");
            /* reduce size1 to produce a 4GB dataset */
            size1[1] = 1024 / 8;
            size2[0] /= 8;
            break;

        case HUGEFILE:
            TESTING("Huge dataset write");
            /* Leave size1 as 32GB */
            break;

        case SFILE:
            TESTING("small dataset write(1GB)");
            /* reduce size1 to produce a 1GB dataset */
            size1[1] = 1024 / 32;
            size2[0] /= 32;
            break;

        case NO_FILE:
            /* what to do?? */
            fprintf(stdout, "Unexpected file size of NO_FILE\n");
            goto error;
            break;

        default:
            fprintf(stdout, "Unexpected file size(%d)\n", testsize);
            goto error;
            break;
    }

    /*
     * We might be on a machine that has 32-bit files, so create an HDF5 file
     * which is a family of files.  Each member of the family will be 1GB
     */
    if ((file = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl)) < 0) {
        goto error;
    }

    /* Create simple data spaces according to the size specified above. */
    if ((space1 = H5Screate_simple(4, size1, size1)) < 0 ||
        (space2 = H5Screate_simple(1, size2, size2)) < 0) {
        goto error;
    }

    /* Create the datasets */
    /*
     *  The fix below is provided for bug#921
     *  H5Dcreate with H5P_DEFAULT creation properties
     *  will create a set of solid 1GB files; test will crash if quotas are enforced
     *  or it will take some time to write a file.
     *  We should create a dataset allocating space late and never writing fill values.
     *  EIP 4/8/03
     */
    dcpl = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_alloc_time(dcpl, H5D_ALLOC_TIME_LATE);
    H5Pset_fill_time(dcpl, H5D_FILL_TIME_NEVER);
    if ((d1 = H5Dcreate2(file, "d1", H5T_NATIVE_INT, space1, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0 ||
        (d2 = H5Dcreate2(file, "d2", H5T_NATIVE_INT, space2, H5P_DEFAULT, dcpl, H5P_DEFAULT)) < 0) {
        goto error;
    }

    /* Write some things to them randomly */
    hs_size[0] = WRT_SIZE;
    if ((mem_space = H5Screate_simple(1, hs_size, hs_size)) < 0)
        goto error;
    for (i = 0; i < wrt_n; i++) {
        /* start position must be at least hs_size from the end */
        hs_start[0] = randll(size2[0] - hs_size[0], i);
        fprintf(out, "#%03d 0x%016" PRIxHSIZE "\n", i, hs_start[0]);
        if (H5Sselect_hyperslab(space2, H5S_SELECT_SET, hs_start, NULL, hs_size, NULL) < 0)
            goto error;
        for (j = 0; j < WRT_SIZE; j++) {
            buf[j] = i + 1;
        }
        if (H5Dwrite(d2, H5T_NATIVE_INT, mem_space, space2, H5P_DEFAULT, buf) < 0)
            goto error;
    }

    if (H5Dclose(d1) < 0)
        goto error;
    if (H5Dclose(d2) < 0)
        goto error;
    if (H5Sclose(mem_space) < 0)
        goto error;
    if (H5Sclose(space1) < 0)
        goto error;
    if (H5Sclose(space2) < 0)
        goto error;
    if (H5Fclose(file) < 0)
        goto error;
    free(buf);
    fclose(out);
    PASSED();
    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Dclose(d1);
        H5Dclose(d2);
        H5Sclose(space1);
        H5Sclose(space2);
        H5Sclose(mem_space);
        H5Fclose(file);
    }
    H5E_END_TRY
    if (buf)
        free(buf);
    if (out)
        fclose(out);
    return 1;
}

/*-------------------------------------------------------------------------
 * Function:    reader
 *
 * Purpose:    Reads some data from random locations in the dataset.
 *
 * Return:    Success:    0
 *
 *            Failure:    >0
 *
 *-------------------------------------------------------------------------
 */
static int
reader(char *filename, hid_t fapl)
{
    FILE   *script = NULL;
    hid_t   file = H5I_INVALID_HID, mspace = H5I_INVALID_HID, fspace = H5I_INVALID_HID, d2 = H5I_INVALID_HID;
    char    ln[128], *s;
    hsize_t hs_offset[1];
    hsize_t hs_size[1] = {WRT_SIZE};
    int    *buf        = (int *)malloc(sizeof(int) * WRT_SIZE);
    int     i, j, zero, wrong, nerrors = 0;

    /* Open script file */
    script = fopen(DNAME, "r");

    /* Open HDF5 file */
    if ((file = H5Fopen(filename, H5F_ACC_RDONLY, fapl)) < 0)
        FAIL_STACK_ERROR;

    /* Open the dataset */
    if ((d2 = H5Dopen2(file, "d2", H5P_DEFAULT)) < 0)
        FAIL_STACK_ERROR;
    if ((fspace = H5Dget_space(d2)) < 0)
        FAIL_STACK_ERROR;

    /* Describe `buf' */
    if ((mspace = H5Screate_simple(1, hs_size, hs_size)) < 0)
        FAIL_STACK_ERROR;

    /* Read each region */
    while (fgets(ln, (int)sizeof(ln), script)) {
        if ('#' != ln[0])
            break;
        i            = (int)strtol(ln + 1, &s, 10);
        hs_offset[0] = strtoull(s, NULL, 0);
        fprintf(stdout, "#%03d 0x%016" PRIxHSIZE "%47s", i, hs_offset[0], "");
        fflush(stdout);

        if (H5Sselect_hyperslab(fspace, H5S_SELECT_SET, hs_offset, NULL, hs_size, NULL) < 0)
            FAIL_STACK_ERROR;
        if (H5Dread(d2, H5T_NATIVE_INT, mspace, fspace, H5P_DEFAULT, buf) < 0)
            FAIL_STACK_ERROR;

        /* Check */
        for (j = zero = wrong = 0; j < WRT_SIZE; j++) {
            if (0 == buf[j])
                zero++;
            else if (buf[j] != i + 1)
                wrong++;
        }
        if (zero) {
            H5_FAILED();
            printf("    %d zero%s\n", zero, 1 == zero ? "" : "s");
        }
        else if (wrong) {
            SKIPPED();
            puts("    Possible overlap with another region.");
            nerrors++;
        }
        else {
            PASSED();
        }
    }

    if (H5Dclose(d2) < 0)
        FAIL_STACK_ERROR;
    if (H5Sclose(mspace) < 0)
        FAIL_STACK_ERROR;
    if (H5Sclose(fspace) < 0)
        FAIL_STACK_ERROR;
    if (H5Fclose(file) < 0)
        FAIL_STACK_ERROR;
    free(buf);
    fclose(script);

    return nerrors;

error:
    H5E_BEGIN_TRY
    {
        H5Dclose(d2);
        H5Sclose(mspace);
        H5Sclose(fspace);
        H5Fclose(file);
    }
    H5E_END_TRY
    if (buf)
        free(buf);
    if (script)
        fclose(script);
    return 1;
}

/*-------------------------------------------------------------------------
 * Function:    usage
 *
 * Purpose:    Print command usage
 *
 * Return:    void
 *
 *-------------------------------------------------------------------------
 */
static void
usage(void)
{
    fprintf(stdout,
            "Usage: big [-h] [-c] [-fsize <fsize>}\n"
            "\t-h\tPrint the help page\n"
            "\t-c\tFile system Checking skipped.  Caution: this test generates\n"
            "\t\tmany big files and may fill up the file system.\n"
            "\t-fsize\tChange family size default to <fsize> where <fsize> is\n"
            "\t\ta positive float point number.  Default value is %" PRIuHSIZE ".\n"
            "Examples:\n"
            "\t big -fsize 2.1e9 \t# test with file size just under 2GB\n"
            "\t big -fsize 2.2e9 \t# test with file size just above 2GB\n"
            "\t Be sure the file system can support the file size requested\n",
            (hsize_t)FAMILY_SIZE);
}

static int
test_sec2(hid_t fapl)
{
    char     filename[1024];
    fsizes_t testsize;

    testsize = supports_big();
    if (testsize == NO_FILE) {
        fprintf(stdout, "Test for sec2 is skipped because file system does not support big files.\n");
        goto quit;
    }
    /* Test big file with the SEC2 driver */
    puts("Testing big file with the SEC2 Driver ");

    h5_fixname(FILENAME[1], fapl, filename, sizeof filename);

    if (writer(filename, fapl, testsize, WRT_N))
        goto error;
    if (reader(filename, fapl))
        goto error;

    puts("Test passed with the SEC2 Driver.");

quit:
    /* End with normal return code */
    /* Clean up the test file */
    h5_delete_all_test_files(FILENAME, fapl);
    H5Pclose(fapl);
    HDremove(DNAME);
    return 0;

error:
    puts("*** TEST FAILED ***");
    return 1;
} /* end test_sec2() */

static int
test_stdio(hid_t fapl)
{
    char     filename[1024];
    fsizes_t testsize;

    testsize = supports_big();
    if (testsize == NO_FILE) {
        fprintf(stdout, "Test for stdio is skipped because file system does not support big files.\n");
        goto quit;
    }
    puts("\nTesting big file with the STDIO Driver ");

    h5_fixname(FILENAME[2], fapl, filename, sizeof filename);

    if (writer(filename, fapl, testsize, WRT_N))
        goto error;
    if (reader(filename, fapl))
        goto error;
    puts("Test passed with the STDIO Driver.");

    /* Flush stdout at the end of this test routine to ensure later
     * output to stderr will not come out before it.
     */

quit:
    /* End with normal return code */
    /* Clean up the test file */
    h5_delete_all_test_files(FILENAME, fapl);
    H5Pclose(fapl);
    HDremove(DNAME);
    fflush(stdout);
    return 0;

error:
    puts("*** TEST FAILED ***");
    fflush(stdout);
    return 1;
} /* end test_stdio() */

static int
test_family(hid_t fapl)
{
    char filename[1024];

    /* Test huge file with the family driver */
    puts("Testing big file with the Family Driver ");
    if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        goto error;

    if (H5Pset_fapl_family(fapl, family_size_def, H5P_DEFAULT) < 0)
        goto error;

    if (cflag) {
        /*
         * We shouldn't run this test if the file system doesn't support holes
         * because we would generate multi-gigabyte files.
         */
        puts("Checking if file system is adequate for this test...");
        if (!sparse_support) {
            puts("Test skipped because file system does not support holes.");
            usage();
            goto quit;
        }
        if (!enough_room(fapl)) {
            puts("Test skipped because of quota (file size or num open files).");
            usage();
            goto quit;
        }
    }

    /* Do the test with the Family Driver */
    h5_fixname(FILENAME[0], fapl, filename, sizeof filename);

    if (writer(filename, fapl, HUGEFILE, WRT_N))
        goto error;
    if (reader(filename, fapl))
        goto error;

    puts("Test passed with the Family Driver.");

quit:
    /* End with normal return code */
    /* Clean up the test file */
    h5_delete_all_test_files(FILENAME, fapl);
    H5Pclose(fapl);
    HDremove(DNAME);
    return 0;

error:
    puts("*** TEST FAILED ***");
    return 1;
} /* end test_family() */

/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:
 *
 * Return:    Success:
 *
 *            Failure:
 *
 *-------------------------------------------------------------------------
 */
int
main(int ac, char **av)
{
    unsigned long seed   = 0; /* Random # seed */
    hid_t         fapl   = H5I_INVALID_HID;
    hid_t         driver = H5I_INVALID_HID;

    /* parameters setup */

    while (--ac > 0) {
        av++;
        if (strcmp("-fsize", *av) == 0) {
            /* specify a different family file size */
            ac--;
            av++;
            if (ac > 0) {
                family_size_def = (hsize_t)strtoull(*av, NULL, 0);
            }
            else {
                printf("***Missing fsize value***\n");
                usage();
                return 1;
            }
        }
        else if (strcmp("-c", *av) == 0) {
            /* turn off file system check before test */
            cflag = 0;
        }
        else if (strcmp("-h", *av) == 0) {
            usage();
            return 0;
        }
        else {
            usage();
            return 1;
        }
    }

    /* check VFD to see if this is one we test */
    if ((fapl = h5_fileaccess()) < 0)
        goto error;
    if ((driver = H5Pget_driver(fapl)) < 0)
        goto error;

    /* check sparse file support unless cflag is not set. */
    if (cflag)
        sparse_support = is_sparse();

    /* Choose random # seed */
    seed = (unsigned long)time(NULL);
#if 0
    /* seed = (unsigned long)1155438845; */
    fprintf(stderr, "Random # seed was: %lu\n", seed);
#endif
    srand((unsigned)seed);

    /* run VFD-specific test */
    if (H5FD_SEC2 == driver) {
        if (test_sec2(fapl) != 0)
            goto error;
    }
    else if (H5FD_STDIO == driver) {
        if (test_stdio(fapl) != 0)
            goto error;
    }
    else if (H5FD_FAMILY == driver) {
        if (test_family(fapl) != 0)
            goto error;
    }
    else
        puts("This VFD is not supported");

    /* End with normal exit code */
    /* fapls are cleaned up in the vfd test code */
    return 0;

error:
    puts("*** TEST FAILED ***");
    if (fapl > 0)
        H5Pclose(fapl);
    return 1;
}
