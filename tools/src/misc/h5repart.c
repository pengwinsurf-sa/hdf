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
 * Purpose:    Repartitions a file family.  This program can be used to
 *        split a single file into a family of files, join a family of
 *        files into a single file, or copy one family to another while
 *        changing the size of the family members.  It can also be used
 *        to copy a single file to a single file with holes.
 */

/* See H5private.h for how to include system headers */
#include "hdf5.h"
#include "H5private.h"

#define NAMELEN 4096
#define GB      *1024 * 1024 * 1024

/* Make these 2 private properties(defined in H5Fprivate.h) available to h5repart.
 * The first one updates the member file size in the superblock.  The second one
 * change file driver from family to a single file driver.
 */
#define H5F_ACS_FAMILY_NEWSIZE_NAME   "family_newsize"
#define H5F_ACS_FAMILY_TO_SINGLE_NAME "family_to_single"

/*-------------------------------------------------------------------------
 * Function:    usage
 *
 * Purpose:    Prints a usage message.
 *
 * Return:    void
 *
 *-------------------------------------------------------------------------
 */
static void
usage(const char *progname)
{
    fprintf(stderr, "usage: %s [-v] [-V] [-[b|m] N[g|m|k]] [-family_to_sec2|-family_to_single] SRC DST\n",
            progname);
    fprintf(stderr, "   -v     Produce verbose output\n");
    fprintf(stderr, "   -V     Print a version number and exit\n");
    fprintf(stderr, "   -b N   The I/O block size, defaults to 1kB\n");
    fprintf(stderr, "   -m N   The destination member size or 1GB\n");
    fprintf(stderr, "   -family_to_sec2   Deprecated version of -family_to_single (below)\n");
    fprintf(stderr, "   -family_to_single   Change file driver from family to the default single-file VFD "
                    "(windows or sec2)\n");
    fprintf(stderr, "   SRC    The name of the source file\n");
    fprintf(stderr, "   DST    The name of the destination files\n");
    fprintf(stderr, "Sizes may be suffixed with 'g' for GB, 'm' for MB or "
                    "'k' for kB.\n");
    fprintf(stderr, "File family names include an integer printf "
                    "format such as '%%d'\n");
    exit(EXIT_FAILURE);
}

/*-------------------------------------------------------------------------
 * Function:    get_size
 *
 * Purpose:     Reads a size option of the form `-XNS' where `X' is any
 *              letter, `N' is a multi-character positive decimal number, and
 *              `S' is an optional suffix letter in the set [GgMmk].  The
 *              option may also be split among two arguments as: `-X NS'.
 *              The input value of ARGNO is the argument number for the
 *              switch in the ARGV vector and ARGC is the number of entries
 *              in that vector.
 *
 * Return:      Success:    The value N multiplied according to the
 *                          suffix S.  On return ARGNO will be the number
 *                          of the next argument to process.
 *
 *              Failure:    Calls usage() which exits with a non-zero
 *                          status.
 *
 *-------------------------------------------------------------------------
 */
static HDoff_t
get_size(const char *progname, int *argno, int argc, char *argv[])
{
    HDoff_t retval = -1;
    char   *suffix = NULL;

    if (isdigit((int)(argv[*argno][2]))) {
        retval = strtol(argv[*argno] + 2, &suffix, 10);
        (*argno)++;
    }
    else if (argv[*argno][2] || *argno + 1 >= argc) {
        usage(progname);
    }
    else {
        retval = strtol(argv[*argno + 1], &suffix, 0);
        if (suffix == argv[*argno + 1])
            usage(progname);
        *argno += 2;
    }
    if (suffix && suffix[0] && !suffix[1]) {
        switch (*suffix) {
            case 'G':
            case 'g':
                retval *= 1024 * 1024 * 1024;
                break;
            case 'M':
            case 'm':
                retval *= 1024 * 1024;
                break;
            case 'k':
                retval *= 1024;
                break;
            default:
                usage(progname);
        }
    }
    else if (suffix && suffix[0]) {
        usage(progname);
    }
    return retval;
}

/*-------------------------------------------------------------------------
 * Function:    main
 *-------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    const char *prog_name;         /*program name            */
    size_t      blk_size = 1024;   /*size of each I/O block    */
    char       *buf      = NULL;   /*I/O block buffer        */
    size_t      n, i;              /*counters            */
    ssize_t     nio;               /*I/O return value        */
    int         argno = 1;         /*program argument number    */
    int         src, dst = -1;     /*source & destination files    */
    int         need_seek = false; /*destination needs to seek?    */
    int         need_write;        /*data needs to be written?    */
    h5_stat_t   sb;                /*temporary file stat buffer    */

    int verbose = false; /*display file names?        */

    const char *src_gen_name;    /*general source name        */
    char       *src_name = NULL; /*source member name        */

    int src_is_family;  /*is source name a family name?    */
    int src_membno = 0; /*source member number        */

    const char *dst_gen_name;    /*general destination name    */
    char       *dst_name = NULL; /*destination member name    */
    int         dst_is_family;   /*is dst name a family name?    */
    int         dst_membno = 0;  /*destination member number    */

    HDoff_t left_overs = 0;  /*amount of zeros left over    */
    HDoff_t src_offset = 0;  /*offset in source member    */
    HDoff_t dst_offset = 0;  /*offset in destination member    */
    HDoff_t src_size;        /*source logical member size    */
    HDoff_t src_act_size;    /*source actual member size    */
    HDoff_t dst_size = 1 GB; /*destination logical memb size    */
    hid_t   fapl;            /*file access property list     */
    hid_t   file;
    hsize_t hdsize;                   /*destination logical memb size */
    bool    family_to_single = false; /*change family to single file driver? */

    /*
     * Get the program name from argv[0]. Use only the last component.
     */
    if ((prog_name = strrchr(argv[0], '/')))
        prog_name++;
    else
        prog_name = argv[0];

    /*
     * Parse switches.
     */
    while (argno < argc && '-' == argv[argno][0]) {
        if (!strcmp(argv[argno], "-v")) {
            verbose = true;
            argno++;
        }
        else if (!strcmp(argv[argno], "-V")) {
            printf("This is %s version %u.%u release %u\n", prog_name, H5_VERS_MAJOR, H5_VERS_MINOR,
                   H5_VERS_RELEASE);
            exit(EXIT_SUCCESS);
        }
        else if (!strcmp(argv[argno], "-family_to_sec2")) {
            family_to_single = true;
            argno++;
        }
        else if (!strcmp(argv[argno], "-family_to_single")) {
            family_to_single = true;
            argno++;
        }
        else if ('b' == argv[argno][1]) {
            blk_size = (size_t)get_size(prog_name, &argno, argc, argv);
        }
        else if ('m' == argv[argno][1]) {
            dst_size = get_size(prog_name, &argno, argc, argv);
        }
        else {
            usage(prog_name);
        } /* end if */
    }     /* end while */

    /* allocate names */
    if (NULL == (src_name = (char *)calloc((size_t)NAMELEN, sizeof(char))))
        exit(EXIT_FAILURE);
    if (NULL == (dst_name = (char *)calloc((size_t)NAMELEN, sizeof(char))))
        exit(EXIT_FAILURE);

    /*
     * Get the name for the source file and open the first member.  The size
     * of the first member determines the logical size of all the members.
     */
    if (argno >= argc)
        usage(prog_name);
    src_gen_name = argv[argno++];
    if (!src_gen_name) {
        fprintf(stderr, "invalid source file name pointer");
        exit(EXIT_FAILURE);
    }
    H5_WARN_FORMAT_NONLITERAL_OFF
    snprintf(src_name, NAMELEN, src_gen_name, src_membno);
    H5_WARN_FORMAT_NONLITERAL_ON
    src_is_family = strcmp(src_name, src_gen_name);

    if ((src = HDopen(src_name, O_RDONLY)) < 0) {
        perror(src_name);
        exit(EXIT_FAILURE);
    }

    memset(&sb, 0, sizeof(h5_stat_t));
    if (HDfstat(src, &sb) < 0) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    src_size = src_act_size = sb.st_size;
    if (verbose)
        fprintf(stderr, "< %s\n", src_name);

    /*
     * Get the name for the destination file and open the first member.
     */
    if (argno >= argc)
        usage(prog_name);
    dst_gen_name = argv[argno++];
    if (!dst_gen_name) {
        fprintf(stderr, "invalid destination file name pointer");
        exit(EXIT_FAILURE);
    }
    H5_WARN_FORMAT_NONLITERAL_OFF
    snprintf(dst_name, NAMELEN, dst_gen_name, dst_membno);
    H5_WARN_FORMAT_NONLITERAL_ON
    dst_is_family = strcmp(dst_name, dst_gen_name);

    if ((dst = HDopen(dst_name, O_RDWR | O_CREAT | O_TRUNC, H5_POSIX_CREATE_MODE_RW)) < 0) {
        perror(dst_name);
        exit(EXIT_FAILURE);
    }
    if (verbose)
        fprintf(stderr, "> %s\n", dst_name);

    /* No more arguments */
    if (argno < argc)
        usage(prog_name);

    /* Now the real work, split the file */
    buf = (char *)malloc(blk_size);
    while (src_offset < src_size) {

        /* Read a block.  The amount to read is the minimum of:
         *    1. The I/O block size
         *    2. What's left to write in the destination member
         *    3. Left over zeros or what's left in the source member.
         */
        n = blk_size;
        if (dst_is_family)
            n = (size_t)MIN((HDoff_t)n, dst_size - dst_offset);
        if (left_overs) {
            n          = (size_t)MIN((HDoff_t)n, left_overs);
            left_overs = left_overs - (HDoff_t)n;
            need_write = false;
        }
        else if (src_offset < src_act_size) {
            n = (size_t)MIN((HDoff_t)n, src_act_size - src_offset);
            if ((nio = HDread(src, buf, n)) < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            else if ((size_t)nio != n) {
                fprintf(stderr, "%s: short read\n", src_name);
                exit(EXIT_FAILURE);
            }
            for (i = 0; i < n; i++) {
                if (buf[i])
                    break;
            }
            need_write = (i < n);
        }
        else {
            n          = 0;
            left_overs = src_size - src_act_size;
            need_write = false;
        }

        /*
         * If the block contains non-zero data then write it to the
         * destination, otherwise just remember that we'll have to do a seek
         * later in the destination when we finally get non-zero data.
         */
        if (need_write) {
            if (need_seek && HDlseek(dst, dst_offset, SEEK_SET) < 0) {
                perror("HDlseek");
                exit(EXIT_FAILURE);
            }
            if ((nio = HDwrite(dst, buf, n)) < 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            else if ((size_t)nio != n) {
                fprintf(stderr, "%s: short write\n", dst_name);
                exit(EXIT_FAILURE);
            }
            need_seek = false;
        }
        else {
            need_seek = true;
        }

        /*
         * Update the source offset and open the next source family member if
         * necessary.  The source stream ends at the first member which
         * cannot be opened because it doesn't exist.  At the end of the
         * source stream, update the destination offset and break out of the
         * loop.   The destination offset must be updated so we can fix
         * trailing holes.
         */
        src_offset = src_offset + (HDoff_t)n;
        if (src_offset == src_act_size) {
            HDclose(src);
            if (!src_is_family) {
                dst_offset = dst_offset + (HDoff_t)n;
                break;
            }
            H5_WARN_FORMAT_NONLITERAL_OFF
            snprintf(src_name, NAMELEN, src_gen_name, ++src_membno);
            H5_WARN_FORMAT_NONLITERAL_ON
            if ((src = HDopen(src_name, O_RDONLY)) < 0 && ENOENT == errno) {
                dst_offset = dst_offset + (HDoff_t)n;
                break;
            }
            else if (src < 0) {
                perror(src_name);
                exit(EXIT_FAILURE);
            }
            memset(&sb, 0, sizeof(h5_stat_t));
            if (HDfstat(src, &sb) < 0) {
                perror("fstat");
                exit(EXIT_FAILURE);
            }
            src_act_size = sb.st_size;
            if (src_act_size > src_size) {
                fprintf(stderr, "%s: member truncated to %lu bytes\n", src_name, (unsigned long)src_size);
            }
            src_offset = 0;
            if (verbose)
                fprintf(stderr, "< %s\n", src_name);
        }

        /*
         * Update the destination offset, opening a new member if one will be
         * needed. The first member is extended to the logical member size
         * but other members might be smaller if they end with a hole.
         */
        dst_offset = dst_offset + (off_t)n;
        if (dst_is_family && dst_offset == dst_size) {
            if (0 == dst_membno) {
                if (HDlseek(dst, dst_size - 1, SEEK_SET) < 0) {
                    perror("HDHDlseek");
                    exit(EXIT_FAILURE);
                }
                if (HDread(dst, buf, 1) < 0) {
                    perror("read");
                    exit(EXIT_FAILURE);
                }
                if (HDlseek(dst, dst_size - 1, SEEK_SET) < 0) {
                    perror("HDlseek");
                    exit(EXIT_FAILURE);
                }
                if (HDwrite(dst, buf, 1) < 0) {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
            }
            HDclose(dst);
            H5_WARN_FORMAT_NONLITERAL_OFF
            snprintf(dst_name, NAMELEN, dst_gen_name, ++dst_membno);
            H5_WARN_FORMAT_NONLITERAL_ON
            if ((dst = HDopen(dst_name, O_RDWR | O_CREAT | O_TRUNC, H5_POSIX_CREATE_MODE_RW)) < 0) {
                perror(dst_name);
                exit(EXIT_FAILURE);
            }
            dst_offset = 0;
            need_seek  = false;
            if (verbose)
                fprintf(stderr, "> %s\n", dst_name);
        }
    }

    /*
     * Make sure the last family member is the right size and then close it.
     * The last member can't end with a hole or hdf5 will think that the
     * family has been truncated.
     */
    if (need_seek) {
        if (HDlseek(dst, dst_offset - 1, SEEK_SET) < 0) {
            perror("HDlseek");
            exit(EXIT_FAILURE);
        }
        if (HDread(dst, buf, 1) < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (HDlseek(dst, dst_offset - 1, SEEK_SET) < 0) {
            perror("HDlseek");
            exit(EXIT_FAILURE);
        }
        if (HDwrite(dst, buf, 1) < 0) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    HDclose(dst);

    /* Modify family driver information saved in superblock through private property.
     * These private properties are for this tool only. */
    if ((fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0) {
        perror("H5Pcreate");
        exit(EXIT_FAILURE);
    }

    if (family_to_single) {
        /* The user wants to change file driver from family to a single-file VFD.
         * Open the file with the sec2, windows, etc. driver. This property signals
         * the library to ignore the family driver information saved in the superblock.
         */
        if (H5Pset(fapl, H5F_ACS_FAMILY_TO_SINGLE_NAME, &family_to_single) < 0) {
            perror("H5Pset");
            exit(EXIT_FAILURE);
        }
    }
    else {
        /* Modify family size saved in superblock through private property. It signals
         * library to save the new member size(specified in command line) in superblock.
         * This private property is for this tool only. */
        if (H5Pset_fapl_family(fapl, H5F_FAMILY_DEFAULT, H5P_DEFAULT) < 0) {
            perror("H5Pset_fapl_family");
            exit(EXIT_FAILURE);
        }

        /* Set the property of the new member size as hsize_t */
        hdsize = (hsize_t)dst_size;
        if (H5Pset(fapl, H5F_ACS_FAMILY_NEWSIZE_NAME, &hdsize) < 0) {
            perror("H5Pset");
            exit(EXIT_FAILURE);
        }
    }

    /* If the new file is a family file, try to open file for "read and write" to
     * flush metadata. Flushing metadata will update the superblock to the new
     * member size.  If the original file is a family file and the new file is a single
     * file, the property FAMILY_TO_SINGLE will signal the library to switch to default
     * single-file driver when the new file is opened.  If the original file is a single
     * file and the new file can only be a single file, reopen the new file should fail.
     * There's nothing to do in this case.
     */
    H5E_BEGIN_TRY
    {
        file = H5Fopen(dst_gen_name, H5F_ACC_RDWR, fapl);
    }
    H5E_END_TRY

    if (file >= 0) {
        if (H5Fclose(file) < 0) {
            perror("H5Fclose");
            exit(EXIT_FAILURE);
        } /* end if */
    }     /* end if */

    if (H5Pclose(fapl) < 0) {
        perror("H5Pclose");
        exit(EXIT_FAILURE);
    } /* end if */

    /* Free resources and return */
    free(src_name);
    free(dst_name);
    free(buf);
    return EXIT_SUCCESS;
} /* end main */
