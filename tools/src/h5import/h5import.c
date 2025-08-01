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
#include "h5import.h"
#include "h5tools.h"
#include "h5tools_utils.h"

/* Name of tool */
#define PROGRAMNAME "h5import"

#ifdef H5_HAVE_WIN32_API
#define READ_OPEN_FLAGS "rb"
#else
#define READ_OPEN_FLAGS "r"
#endif

/* Local function declarations */
static int             gtoken(char *s);
static int             process(struct Options *opt);
static int             processConfigurationFile(char *infile, struct Input *in);
static int             mapKeywordToIndex(char *key);
static int             parsePathInfo(struct path_info *path, char *strm);
static int             parseDimensions(struct Input *in, char *strm);
static int             getInputSize(struct Input *in, int ival);
static int             getInputClass(struct Input *in, char *strm);
static int             getInputClassType(struct Input *in, char *strm);
static int             getInputByteOrder(struct Input *in, FILE *strm);
static int             InputClassStrToInt(char *temp);
static int             getRank(struct Input *in, FILE *strm);
static int             getDimensionSizes(struct Input *in, FILE *strm);
static int             getOutputSize(struct Input *in, FILE *strm);
static int             getOutputClass(struct Input *in, FILE *strm);
static int             OutputClassStrToInt(char *temp);
static int             getOutputArchitecture(struct Input *in, FILE *strm);
static int             OutputArchStrToInt(const char *temp);
static int             getOutputByteOrder(struct Input *in, FILE *strm);
static int             OutputByteOrderStrToInt(const char *temp);
static int             getChunkedDimensionSizes(struct Input *in, FILE *strm);
static int             getCompressionType(struct Input *in, FILE *strm);
static int             CompressionTypeStrToInt(char *temp);
static int             getCompressionParameter(struct Input *in, FILE *strm);
static int             getExternalFilename(struct Input *in, FILE *strm);
static int             getMaximumDimensionSizes(struct Input *in, FILE *strm);
static int             processDataFile(char *infile, struct Input *in, hid_t file_id);
static int             readIntegerData(FILE *strm, struct Input *in);
static int             readFloatData(FILE *strm, struct Input *in);
static int             allocateIntegerStorage(struct Input *in);
static int             allocateFloatStorage(struct Input *in);
static int             readUIntegerData(FILE *strm, struct Input *in);
static int             allocateUIntegerStorage(struct Input *in);
static int             validateConfigurationParameters(struct Input *in);
static int             processStrData(FILE *strm, struct Input *in, hid_t file_id);
static int             processStrHDFData(FILE *strm, struct Input *in, hid_t file_id);
H5_ATTR_CONST uint16_t swap_uint16(uint16_t val);
H5_ATTR_CONST int16_t  swap_int16(int16_t val);
H5_ATTR_CONST uint32_t swap_uint32(uint32_t val);
H5_ATTR_CONST int32_t  swap_int32(int32_t val);
H5_ATTR_CONST int64_t  swap_int64(int64_t val);
H5_ATTR_CONST uint64_t swap_uint64(uint64_t val);

int
main(int argc, char *argv[])
{
    struct Options *opt;
    int             outfile_named = false;
    int             token;
    int             i;
    int             state = 0;
    struct Input   *in    = NULL;

    const char *err1  = "Invalid number of arguments:  %d.\n";
    const char *err2  = "Error in state table.\n";
    const char *err3  = "No output file given.\n";
    const char *err4  = "Program aborted.\n";
    const char *err5  = "Invalid path %s.\n";
    const char *err6  = "Invalid dimensions - %s.\n";
    const char *err7  = "Invalid type of data - %s.\n";
    const char *err8  = "Invalid size of data - %s.\n";
    const char *err9  = "Cannot specify more than 30 input files in one call to h5import.\n";
    const char *err10 = "Length of output file name limited to 255 chars.\n";

    h5tools_setprogname(PROGRAMNAME);
    h5tools_setstatus(EXIT_SUCCESS);

    /* Initialize h5tools lib */
    h5tools_init();

    (void)HDsetvbuf(rawerrorstream, (char *)NULL, _IOLBF, 0);
    (void)HDsetvbuf(rawoutstream, (char *)NULL, _IOLBF, 0);

    if ((opt = (struct Options *)calloc(1, sizeof(struct Options))) == NULL)
        goto err;

    if (argv[1] && (strcmp("-V", argv[1]) == 0)) {
        print_version(PROGRAMNAME);
        exit(EXIT_SUCCESS);
    }

    /*
     * validate the number of command line arguments
     */
    if (argc < 2) {
        (void)fprintf(rawerrorstream, err1, argc);
        usage(argv[0]);
        goto err;
    }

    /*
     * parse the command line
     */
    for (i = 1; i < argc; i++) {
        if ((token = gtoken(argv[i])) == INVALID_TOKEN) {
            usage(argv[0]);
            goto err;
        }

        state = state_table[state][token];

        switch (state) {

            case 1: /* counting input files */
                if (opt->fcount < 29) {
                    (void)strcpy(opt->infiles[opt->fcount].datafile, argv[i]);
                    in                               = &(opt->infiles[opt->fcount].in);
                    opt->infiles[opt->fcount].config = 0;
                    setDefaultValues(in, opt->fcount);
                    opt->fcount++;
                }
                else {
                    (void)fprintf(rawerrorstream, err9, argv[i]);
                    goto err;
                }

                break;

            case 2: /* -c found; look for configfile */
                break;

            case 3: /* get configfile name */
                (void)strcpy(opt->infiles[opt->fcount - 1].configfile, argv[i]);
                opt->infiles[opt->fcount - 1].config = 1;
                break;

            case 4: /* -o found; look for outfile */
                break;

            case 5: /* get outfile found */
                if (strlen(argv[i]) > MAX_PATH_NAME_LENGTH) {
                    (void)fprintf(rawerrorstream, err10, argv[i]);
                    goto err;
                }
                (void)strcpy(opt->outfile, argv[i]);
                outfile_named = true;
                break;

            case 6: /* -h found; help, then exit */
                help(argv[0]);
                exit(EXIT_SUCCESS);
                break;

            case 7: /* -d found; look for dimensions */
                break;

            case 8: /* read dimensions */
                if (parseDimensions(in, argv[i]) == -1) {
                    (void)fprintf(rawerrorstream, err6, argv[i]);
                    goto err;
                }
                break;

            case 9: /* -p found; look for path name */
                break;

            case 10: /* read path name */
                if (parsePathInfo(&in->path, argv[i]) == -1) {
                    (void)fprintf(rawerrorstream, err5, argv[i]);
                    goto err;
                }
                break;

            case 11: /* -t found; look for data type */
                break;

            case 12: /* read data type */
                if (getInputClass(in, argv[i]) == -1) {
                    (void)fprintf(rawerrorstream, err7, argv[i]);
                    goto err;
                }

                if (in->inputClass == 0 || in->inputClass == 4)
                    in->outputClass = 0;
                if (in->inputClass == 1 || in->inputClass == 2 || in->inputClass == 3)
                    in->outputClass = 1;
                if (in->inputClass == 6 || in->inputClass == 7)
                    in->outputClass = 2;
                break;

            case 13: /* -s found; look for data size */
                break;

            case 14: /* read data size */
                if (getInputSize(in, (int)strtol(argv[i], NULL, BASE_10)) == -1) {
                    (void)fprintf(rawerrorstream, err8, argv[i]);
                    goto err;
                }
                /*set default value for output-size */
                in->outputSize = in->inputSize;
                break;

            case INVALID_TOKEN: /* command syntax error */
            default:
                (void)fprintf(rawerrorstream, "%s", err2);
                usage(argv[0]);
                goto err;
        }
    }

    if (false == outfile_named) {
        (void)fprintf(rawerrorstream, "%s", err3);
        usage(argv[0]);
        goto err;
    }

    if (process(opt) == -1)
        goto err;

    for (i = 0; i < opt->fcount; i++) {
        in = &(opt->infiles[i].in);
        if (in->sizeOfDimension)
            free(in->sizeOfDimension);
        if (in->sizeOfChunk)
            free(in->sizeOfChunk);
        if (in->maxsizeOfDimension)
            free(in->maxsizeOfDimension);
        if (in->externFilename)
            free(in->externFilename);
        if (in->data)
            free(in->data);
    }
    free(opt);

    return EXIT_SUCCESS;
err:
    (void)fprintf(rawerrorstream, "%s", err4);
    for (i = 0; i < opt->fcount; i++) {
        in = &(opt->infiles[i].in);
        if (in->sizeOfDimension)
            free(in->sizeOfDimension);
        if (in->sizeOfChunk)
            free(in->sizeOfChunk);
        if (in->maxsizeOfDimension)
            free(in->maxsizeOfDimension);
        if (in->externFilename)
            free(in->externFilename);
        if (in->data)
            free(in->data);
    }
    free(opt);

    return EXIT_FAILURE;
}

static int
gtoken(char *s)
{
    size_t len;
    int    token = INVALID_TOKEN;

    const char *err1 = "Illegal argument: %s.\n";

    /*
     * identify the token type
     */
    if (s[0] == '-') { /* option name (or negative number) */
        len = strlen(&s[1]);
        switch (s[1]) {
            case 'o':
                if (!strncmp("outfile", &s[1], len))
                    token = OPT_o;
                break;

            case 'c':
                if (!strncmp("config", &s[1], len))
                    token = OPT_c;
                break;

            case 'h':
                if (!strncmp("help", &s[1], len))
                    token = OPT_h;
                break;

            case 'd':
                if (!strncmp("dims", &s[1], len))
                    token = OPT_d;
                break;

            case 'p':
                if (!strncmp("path", &s[1], len))
                    token = OPT_p;
                break;

            case 't':
                if (!strncmp("type", &s[1], len))
                    token = OPT_t;
                break;

            case 's':
                if (!strncmp("size", &s[1], len))
                    token = OPT_s;
                break;
            default:
                token = INVALID_TOKEN; /* not a supported option tag */
                break;
        }

        if (token == INVALID_TOKEN)
            (void)fprintf(rawerrorstream, err1, s);
    }
    else { /* filename */
        token = FILNAME;
    }
    return (token);
}

/*-------------------------------------------------------------------------
 * Function:    processDataFile
 *
 * Purpose:     allocate memory and read data file
 *
 * Return:      0, success, -1, error
 *
 *-------------------------------------------------------------------------
 */

static int
processDataFile(char *infile, struct Input *in, hid_t file_id)
{
    FILE       *strm   = NULL;
    const char *err1   = "Unable to open the input file  %s for reading.\n";
    const char *err2   = "Error in allocating integer data storage.\n";
    const char *err3   = "Error in allocating floating-point data storage.\n";
    const char *err4   = "Error in reading integer data.\n";
    const char *err5   = "Error in reading floating-point data.\n";
    const char *err6   = "Error in allocating unsigned integer data storage.\n";
    const char *err7   = "Error in reading unsigned integer data.\n";
    const char *err10  = "Unrecognized input class type.\n";
    const char *err11  = "Error in reading string data.\n";
    int         retval = -1;

    /*-------------------------------------------------------------------------
     * special case for opening binary classes in H5_HAVE_WIN32_API
     * "FP" denotes a floating point binary file,
     * "IN" denotes a signed integer binary file,
     * "UIN" denotes an unsigned integer binary file,
     *-------------------------------------------------------------------------
     */
    if (in->inputClass == 4 /* "IN" */ || in->inputClass == 3 /* "FP" */ || in->inputClass == 7 /* "UIN" */) {

        if ((strm = fopen(infile, READ_OPEN_FLAGS)) == NULL) {
            (void)fprintf(rawerrorstream, err1, infile);
            goto error;
        }
    }
    /*-------------------------------------------------------------------------
     * if the input class is not binary, just use "r"
     *-------------------------------------------------------------------------
     */
    else {
        if ((strm = fopen(infile, "r")) == NULL) {
            (void)fprintf(rawerrorstream, err1, infile);
            goto error;
        }
    }

    switch (in->inputClass) {
        case 0: /*  TEXTIN */
        case 4: /*  IN  */
            if (allocateIntegerStorage(in) == -1) {
                (void)fprintf(rawerrorstream, err2, infile);
                goto error;
            }

            if (readIntegerData(strm, in) == -1) {
                (void)fprintf(rawerrorstream, err4, infile);
                goto error;
            }
            break;

        case 1: /*  TEXTFP */
        case 2: /*  TEXTFPE  */
        case 3: /*  FP  */
            if (allocateFloatStorage(in) == -1) {
                (void)fprintf(rawerrorstream, err3, infile);
                goto error;
            }

            if (readFloatData(strm, in) == -1) {
                (void)fprintf(rawerrorstream, err5, infile);
                goto error;
            }
            break;

        case 5: /*  STR  */
            if (in->h5dumpInput) {
                if (processStrHDFData(strm, in, file_id) == -1) {
                    (void)fprintf(rawerrorstream, err11, infile);
                    goto error;
                }
            }
            else {
                if (processStrData(strm, in, file_id) == -1) {
                    (void)fprintf(rawerrorstream, err11, infile);
                    goto error;
                }
            }

            break;

        case 6: /* TEXTUIN */
        case 7: /* UIN */
            if (allocateUIntegerStorage(in) == -1) {
                (void)fprintf(rawerrorstream, err6, infile);
                goto error;
            }
            if (readUIntegerData(strm, in) == -1) {
                (void)fprintf(rawerrorstream, err7, infile);
                goto error;
            }
            break;

        default:
            (void)fprintf(rawerrorstream, "%s", err10);
            goto error;
    }

    /* Set success return value */
    retval = 0;

error:
    if (strm)
        fclose(strm);
    return retval;
}

static int
readIntegerData(FILE *strm, struct Input *in)
{
    H5DT_INT8  *in08;
    H5DT_INT16 *in16;
    H5DT_INT16  temp16;
    H5DT_INT32 *in32;
    H5DT_INT32  temp32;
    H5DT_INT64 *in64;
    H5DT_INT64  temp64;
    char        buffer[256];
    hsize_t     len = 1;
    hsize_t     i;
    int         j;

    const char *err1 = "Unable to get integer value from file.\n";
    const char *err2 = "Unrecognized input class type.\n";
    const char *err3 = "Invalid input size.\n";

    for (j = 0; j < in->rank; j++)
        len *= in->sizeOfDimension[j];

    switch (in->inputSize) {
        case 8:
            switch (in->inputClass) {
                case 0: /* TEXTIN */
                    in08 = (H5DT_INT8 *)in->data;
                    for (i = 0; i < len; i++, in08++) {
                        if (fscanf(strm, "%hd", &temp16) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        (*in08) = (H5DT_INT8)temp16;
                    }
                    break;

                case 4: /* IN */
                    in08 = (H5DT_INT8 *)in->data;
                    for (i = 0; i < len; i++, in08++) {
                        if (fread((char *)in08, sizeof(H5DT_INT8), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
#ifdef H5DEBUGIMPORT
                        printf("readIntegerData %d (0x%.8X)\n", *in08, *in08);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        case 16:
            in16 = (H5DT_INT16 *)in->data;
            switch (in->inputClass) {
                case 0: /* TEXTIN */
                    for (i = 0; i < len; i++, in16++) {
                        if (fscanf(strm, "%hd", in16) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }
                    in16 = (H5DT_INT16 *)in->data;
                    break;

                case 4: /* IN */
                    for (i = 0; i < len; i++, in16++) {
                        if (fread((char *)&temp16, sizeof(H5DT_INT16), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        /*
                                        if (in-> h5dumpInput && (in->inputByteOrder != in->outputByteOrder))
                                            *in16 = swap_int16(temp16);
                                        else
                        */
                        *in16 = temp16;
#ifdef H5DEBUGIMPORT
                        printf("readIntegerData %d (0x%.8X)\n", *in16, temp16);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        case 32:
            in32 = (H5DT_INT32 *)in->data;
            switch (in->inputClass) {
                case 0: /* TEXTIN */
                    for (i = 0; i < len; i++, in32++) {
                        if (fscanf(strm, "%d", in32) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }
                    break;

                case 4: /* IN */
                    for (i = 0; i < len; i++, in32++) {
                        if (fread((char *)&temp32, sizeof(H5DT_INT32), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        /*
                                        if (in-> h5dumpInput && (in->inputByteOrder != in->outputByteOrder))
                                            *in32 = swap_int32(temp32);
                                        else
                        */
                        *in32 = temp32;
#ifdef H5DEBUGIMPORT
                        printf("readIntegerData %d (0x%.8X = 0x%.8X)\n", *in32, *in32, temp32);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        case 64:
            in64 = (H5DT_INT64 *)in->data;
            switch (in->inputClass) {
                case 0: /* TEXTIN */
                    for (i = 0; i < len; i++, in64++) {
                        if (fscanf(strm, "%s", buffer) < 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        *in64 = (H5DT_INT64)strtoll(buffer, NULL, 10);
                    }
                    break;

                case 4: /* IN */
                    for (i = 0; i < len; i++, in64++) {
                        if (fread((char *)&temp64, sizeof(H5DT_INT64), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        /*
                                        if (in-> h5dumpInput && (in->inputByteOrder != in->outputByteOrder))
                                            *in64 = swap_int64(temp64);
                                        else
                        */
                        *in64 = temp64;
#ifdef H5DEBUGIMPORT
                        printf("readIntegerData %d (0x%.8X)\n", *in64, temp64);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        default:
            (void)fprintf(rawerrorstream, "%s", err3);
            break;
    }
    return (0);
}

static int
readUIntegerData(FILE *strm, struct Input *in)
{
    H5DT_UINT8  *in08;
    H5DT_UINT16 *in16;
    H5DT_UINT16  temp16;
    H5DT_UINT32 *in32;
    H5DT_UINT32  temp32;
    H5DT_UINT64 *in64;
    H5DT_UINT64  temp64;
    char         buffer[256];
    hsize_t      len = 1;
    hsize_t      i;
    int          j;
    const char  *err1 = "Unable to get unsigned integer value from file.\n";
    const char  *err2 = "Unrecognized input class type.\n";
    const char  *err3 = "Invalid input size.\n";

    for (j = 0; j < in->rank; j++)
        len *= in->sizeOfDimension[j];

    switch (in->inputSize) {
        case 8:
            switch (in->inputClass) {
                case 6: /* TEXTUIN */
                    in08 = (H5DT_UINT8 *)in->data;
                    for (i = 0; i < len; i++, in08++) {
                        if (fscanf(strm, "%hu", &temp16) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        (*in08) = (H5DT_UINT8)temp16;
                    }
                    break;

                case 7: /* UIN */
                    in08 = (H5DT_UINT8 *)in->data;
                    for (i = 0; i < len; i++, in08++) {
                        if (fread((char *)in08, sizeof(H5DT_UINT8), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        case 16:
            in16 = (H5DT_UINT16 *)in->data;
            switch (in->inputClass) {
                case 6: /* TEXTUIN */
                    for (i = 0; i < len; i++, in16++) {
                        if (fscanf(strm, "%hu", in16) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }
                    break;

                case 7: /* UIN */
                    for (i = 0; i < len; i++, in16++) {
                        if (fread((char *)&temp16, sizeof(H5DT_UINT16), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        /*
                                        if (in-> h5dumpInput && (in->inputByteOrder != in->outputByteOrder))
                                            *in16 = swap_uint16(temp16);
                                        else
                        */
                        *in16 = temp16;
#ifdef H5DEBUGIMPORT
                        printf("readUIntegerData %d (0x%.4X = 0x%.4X)\n", *in16, *in16, temp16);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        case 32:
            in32 = (H5DT_UINT32 *)in->data;
            switch (in->inputClass) {
                case 6: /* TEXTUIN */
                    for (i = 0; i < len; i++, in32++) {
                        if (fscanf(strm, "%u", in32) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }
                    break;

                case 7: /* UIN */
                    for (i = 0; i < len; i++, in32++) {
                        if (fread((char *)&temp32, sizeof(H5DT_UINT32), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        /*
                                        if (in-> h5dumpInput && (in->inputByteOrder != in->outputByteOrder))
                                            *in32 = swap_uint32(temp32);
                                        else
                        */
                        *in32 = temp32;
#ifdef H5DEBUGIMPORT
                        printf("readUIntegerData %d (0x%.8X = 0x%.8X)\n", *in32, *in32, temp32);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        case 64:
            in64 = (H5DT_UINT64 *)in->data;
            switch (in->inputClass) {
                case 6: /* TEXTUIN */
                    for (i = 0; i < len; i++, in64++) {
                        if (fscanf(strm, "%s", buffer) < 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        *in64 = (H5DT_UINT64)strtoll(buffer, NULL, 10);
                    }
                    break;

                case 7: /* UIN */
                    for (i = 0; i < len; i++, in64++) {
                        if (fread((char *)&temp64, sizeof(H5DT_UINT64), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        /*
                                        if (in-> h5dumpInput && (in->inputByteOrder != in->outputByteOrder))
                                            *in64 = swap_uint64(temp64);
                                        else
                        */
                        *in64 = temp64;
#ifdef H5DEBUGIMPORT
                        printf("readUIntegerData %ld (0x%.8X = 0x%.8X)\n", *in64, *in64, temp64);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        default:
            (void)fprintf(rawerrorstream, "%s", err3);
            break;
    }
    return (0);
}

static int
readFloatData(FILE *strm, struct Input *in)
{
    H5DT_FLOAT32 *fp32;
    uint32_t     *bfp32;
    uint32_t      temp32;
    H5DT_FLOAT64 *fp64;
    uint64_t     *bfp64;
    uint64_t      temp64;

    hsize_t     len = 1;
    hsize_t     i;
    int         j;
    const char *err1 = "Unable to get float value from file.\n";
    const char *err2 = "Unrecognized input class type.\n";
    const char *err3 = "Invalid input size type.\n";

    for (j = 0; j < in->rank; j++)
        len *= in->sizeOfDimension[j];

    switch (in->inputSize) {
        case 32:
            fp32 = (H5DT_FLOAT32 *)in->data;
            switch (in->inputClass) {
                case 1: /* TEXTFP */
                    for (i = 0; i < len; i++, fp32++) {
                        if (fscanf(strm, "%f", fp32) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }

                    fp32 = (H5DT_FLOAT32 *)in->data;
                    break;

                    /* same as TEXTFP */
                case 2: /*TEXTFPE */

                    for (i = 0; i < len; i++, fp32++) {
                        if (fscanf(strm, "%f", fp32) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }

                    fp32 = (H5DT_FLOAT32 *)in->data;
                    break;

                case 3: /* FP */
                    bfp32 = (uint32_t *)in->data;
                    for (i = 0; i < len; i++, bfp32++) {
                        if (fread((char *)&temp32, sizeof(uint32_t), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        /*
                                        if (in-> h5dumpInput && (in->inputByteOrder != in->outputByteOrder))
                                            *bfp32 = swap_uint32(temp32);
                                        else
                        */
                        *bfp32 = temp32;
#ifdef H5DEBUGIMPORT
                        printf("readFloatData %ld (0x%.8X = 0x%.8X)\n", *bfp32, *bfp32, temp32);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        case 64:
            fp64 = (H5DT_FLOAT64 *)in->data;
            switch (in->inputClass) {
                case 1: /* TEXTFP */
                    for (i = 0; i < len; i++, fp64++) {
                        if (fscanf(strm, "%lf", fp64) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }

                    fp64 = (H5DT_FLOAT64 *)in->data;
                    break;

                    /* same as TEXTFP */
                case 2: /*TEXTFPE */

                    for (i = 0; i < len; i++, fp64++) {
                        if (fscanf(strm, "%lf", fp64) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                    }

                    fp64 = (H5DT_FLOAT64 *)in->data;
                    break;

                case 3: /* FP */
                    bfp64 = (uint64_t *)in->data;
                    for (i = 0; i < len; i++, bfp64++) {
                        if (fread((char *)&temp64, sizeof(uint64_t), 1, strm) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err1);
                            return (-1);
                        }
                        /*
                                        if (in-> h5dumpInput && (in->inputByteOrder != in->outputByteOrder))
                                            *bfp64 = swap_uint64(temp64);
                                        else
                        */
                        *bfp64 = temp64;
#ifdef H5DEBUGIMPORT
                        printf("readFloatData %ld (0x%.16lX)\n", *bfp64, temp64);
#endif
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err2);
                    return (-1);
            }
            break;

        default:
            (void)fprintf(rawerrorstream, "%s", err3);
            break;
    }
    return (0);
}

/*-------------------------------------------------------------------------
 * Function: processStrData
 *
 * Purpose: read an ASCII file with string data and generate an HDF5 dataset
 *  with a variable length type
 *
 * Return: 0, ok, -1 no
 *
 *-------------------------------------------------------------------------
 */
static int
processStrData(FILE *strm, struct Input *in, hid_t file_id)
{
    hid_t   group_id;
    hid_t   dset_id;
    hid_t   space_id;
    hid_t   mspace_id;
    hid_t   type_id;
    hid_t   handle;
    hsize_t dims[1];
    char    str[1024];
    int     c;
    int     i = 0;
    int     j;
    hsize_t nlines = 0;
    hsize_t line;

    /*-------------------------------------------------------------------------
     * get number of lines in the input file
     *-------------------------------------------------------------------------
     */

    while (EOF != (c = fgetc(strm)))
        if (c == 10) /* eol */
            nlines++;

    if (!nlines)
        return 0;

    /* number of records */
    dims[0] = nlines;

    /* rewind */
    HDfseek(strm, 0L, 0);

    /*-------------------------------------------------------------------------
     * read file again and generate an HDF5 dataset
     *-------------------------------------------------------------------------
     */

    if ((type_id = H5Tcopy(H5T_C_S1)) < 0)
        goto out;

    if (H5Tset_size(type_id, H5T_VARIABLE) < 0)
        goto out;

    /* disable error reporting */
    H5E_BEGIN_TRY
    {
        /* create parent groups */
        if (in->path.count > 1) {
            j      = 0;
            handle = file_id;
            while (j < in->path.count - 1) {
                if ((group_id = H5Gopen2(handle, in->path.group[j], H5P_DEFAULT)) < 0) {
                    group_id = H5Gcreate2(handle, in->path.group[j++], H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                    for (; j < in->path.count - 1; j++)
                        group_id =
                            H5Gcreate2(group_id, in->path.group[j], H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                    handle = group_id;
                    break;
                }
                handle = group_id;
                j++;
            }
        }
        else {
            handle = file_id;
            j      = 0;
        }

        /*enable error reporting */
    }
    H5E_END_TRY

    if ((space_id = H5Screate_simple(1, dims, NULL)) < 0)
        goto out;

    if ((mspace_id = H5Screate(H5S_SCALAR)) < 0)
        goto out;

    if ((dset_id = H5Dcreate2(handle, in->path.group[j], type_id, space_id, H5P_DEFAULT, H5P_DEFAULT,
                              H5P_DEFAULT)) < 0)
        goto out;

    line = 0;

    while (EOF != (c = fgetc(strm))) {
        str[i] = (char)c;

        i++;

        if (c == 10) { /* eol */
            char   *str2 = str;
            hid_t   fspace_id;
            hsize_t start[1];
            hsize_t count[1] = {1};

            str[i - 1] = '\0'; /* terminate string */

            if ((fspace_id = H5Dget_space(dset_id)) < 0)
                goto out;

            start[0] = line++;

            if (H5Sselect_hyperslab(fspace_id, H5S_SELECT_SET, start, NULL, count, NULL) < 0)
                goto out;

            if (H5Dwrite(dset_id, type_id, mspace_id, fspace_id, H5P_DEFAULT, &str2) < 0)
                goto out;

            if (H5Sclose(fspace_id) < 0)
                goto out;

            i      = 0;
            str[0] = '\0';
        }
    }

    /* close */
    H5Dclose(dset_id);
    H5Sclose(space_id);
    H5Sclose(mspace_id);
    H5Tclose(type_id);

    return (0);

out:

    return (-1);
}

/*-------------------------------------------------------------------------
 * Function: processStrData
 *
 * Purpose: read an ASCII file with string data and generate an HDF5 dataset
 *  with a variable length type
 *
 * Return: 0, ok, -1 no
 *
 *-------------------------------------------------------------------------
 */
static int
processStrHDFData(FILE *strm, struct Input *in, hid_t file_id)
{
    hid_t   group_id  = H5I_INVALID_HID;
    hid_t   dset_id   = H5I_INVALID_HID;
    hid_t   space_id  = H5I_INVALID_HID;
    hid_t   mspace_id = H5I_INVALID_HID;
    hid_t   type_id   = H5I_INVALID_HID;
    hid_t   handle    = H5I_INVALID_HID;
    char   *str1      = NULL;
    char   *str2      = NULL;
    char   *str3      = NULL;
    char    str[1024] = "";
    int     j;
    hsize_t line;

    /*-------------------------------------------------------------------------
     * read file and generate an HDF5 dataset
     *-------------------------------------------------------------------------
     */
#ifdef H5DEBUGIMPORT
    printf("processStrHDFData DATATYPE STRING\n");
#endif

    if ((type_id = H5Tcopy(H5T_C_S1)) < 0)
        goto out;

    if (H5Tset_size(type_id, H5T_VARIABLE) < 0)
        goto out;

    /* disable error reporting */
    H5E_BEGIN_TRY
    {
        /* create parent groups */
        if (in->path.count > 1) {
            j      = 0;
            handle = file_id;
            while (j < in->path.count - 1) {
                if ((group_id = H5Gopen2(handle, in->path.group[j], H5P_DEFAULT)) < 0) {
                    group_id = H5Gcreate2(handle, in->path.group[j++], H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                    for (; j < in->path.count - 1; j++)
                        group_id =
                            H5Gcreate2(group_id, in->path.group[j], H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                    handle = group_id;
                    break;
                }
                handle = group_id;
                j++;
            }
        }
        else {
            handle = file_id;
            j      = 0;
        }

        /*enable error reporting */
    }
    H5E_END_TRY
#ifdef H5DEBUGIMPORT
    printf("processStrHDFData DATATYPE STRING groups created\n");
#endif

    if ((space_id = H5Screate_simple(in->rank, in->sizeOfDimension, NULL)) < 0)
        goto out;

    if ((mspace_id = H5Screate(H5S_SCALAR)) < 0)
        goto out;

    if ((dset_id = H5Dcreate2(handle, in->path.group[j], type_id, space_id, H5P_DEFAULT, H5P_DEFAULT,
                              H5P_DEFAULT)) < 0)
        goto out;

#ifdef H5DEBUGIMPORT
    printf("processStrHDFData DATATYPE STRING ready to process strings\n");
#endif
    line = 0;
    j    = 0;

    while (fgets(str, sizeof(str), strm)) {
        str1 = str;
        str2 = NULL;
        str3 = NULL;
#ifdef H5DEBUGIMPORT
        printf("processStrHDFData DATATYPE STRING[%llu]={%s}\n", (unsigned long long)line, str1);
#endif
        /* process string to remove the first and last quote char */
        str2 = strchr(str1, '"');
        if (str2 != NULL) {
#ifdef H5DEBUGIMPORT
            printf("processStrHDFData DATATYPE STRING len:%d for {%s}\n", strlen(str2), str2);
#endif
            str2++;
#ifdef H5DEBUGIMPORT
            printf("processStrHDFData DATATYPE STRING len:%d for {%s}\n", strlen(str2), str2);
#endif
            str3 = strrchr(str2, '"');
            if (str3 != NULL) {
#ifdef H5DEBUGIMPORT
                printf("processStrHDFData DATATYPE STRING len:%d for {%s}\n", strlen(str3), str3);
#endif
                *str3 = '\0';

#ifdef H5DEBUGIMPORT
                printf("processStrHDFData DATATYPE STRING len:%d for {%s}\n", strlen(str2), str2);
#endif

                if (strlen(str2) > 0) {
                    hid_t   fspace_id;
                    hsize_t start[1];
                    hsize_t count[1] = {1};

#ifdef H5DEBUGIMPORT
                    printf("processStrHDFData DATATYPE STRING[%llu] store %s\n", (unsigned long long)line,
                           str2);
#endif
                    if ((fspace_id = H5Dget_space(dset_id)) < 0)
                        goto out;

                    start[0] = line++;

                    if (H5Sselect_hyperslab(fspace_id, H5S_SELECT_SET, start, NULL, count, NULL) < 0)
                        goto out;

                    if (H5Dwrite(dset_id, type_id, mspace_id, fspace_id, H5P_DEFAULT, &str2) < 0)
                        goto out;

                    if (H5Sclose(fspace_id) < 0)
                        goto out;
                }
            }
        }
        str[0] = '\0';
        j++;
    }
#ifdef H5DEBUGIMPORT
    printf("processStrHDFData DATATYPE STRING eof reached\n");
#endif

    /* close */
    H5Dclose(dset_id);
    H5Sclose(space_id);
    H5Sclose(mspace_id);
    H5Tclose(type_id);

    return (0);

out:
#ifdef H5DEBUGIMPORT
    printf("processStrHDFData DATATYPE STRING error exit\n");
#endif
    /* disable error reporting */
    H5E_BEGIN_TRY
    {
        /* close */
        H5Dclose(dset_id);
        H5Sclose(space_id);
        H5Sclose(mspace_id);
        H5Tclose(type_id);
    }
    H5E_END_TRY

    return (-1);
}

static int
allocateIntegerStorage(struct Input *in)
{
    hsize_t     len = 1;
    int         j;
    const char *err1 = "Unable to allocate dynamic memory.\n";
    const char *err2 = "Invalid storage size for integer input data.\n";

    for (j = 0; j < in->rank; j++)
        len *= in->sizeOfDimension[j];

    switch (in->inputSize) {
        case 8:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_INT8))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        case 16:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_INT16))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        case 32:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_INT32))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        case 64:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_INT64))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        default:
            (void)fprintf(rawerrorstream, "%s", err2);
            break;
    }
    return (0);
}

static int
allocateUIntegerStorage(struct Input *in)
{
    hsize_t     len  = 1;
    const char *err1 = "Unable to allocate dynamic memory.\n";
    const char *err2 = "Invalid storage size for unsigned integer input data.\n";
    int         j;

    for (j = 0; j < in->rank; j++)
        len *= in->sizeOfDimension[j];

    switch (in->inputSize) {
        case 8:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_UINT8))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        case 16:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_UINT16))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        case 32:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_UINT32))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        case 64:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_UINT64))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        default:
            (void)fprintf(rawerrorstream, "%s", err2);
            break;
    }
    return (0);
}

static int
allocateFloatStorage(struct Input *in)
{
    hsize_t     len = 1;
    int         j;
    const char *err1 = "Unable to allocate dynamic memory.\n";
    const char *err2 = "Invalid storage size for float input data.\n";

    for (j = 0; j < in->rank; j++)
        len *= in->sizeOfDimension[j];

    switch (in->inputSize) {
        case 32:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_FLOAT32))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        case 64:
            if ((in->data = (VOIDP)malloc((size_t)len * sizeof(H5DT_FLOAT64))) == NULL) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }
            break;

        default:
            (void)fprintf(rawerrorstream, "%s", err2);
            break;
    }
    return (0);
}

static int
processConfigurationFile(char *infile, struct Input *in)
{
    FILE *strm = NULL;
    char  key[MAX_PATH_NAME_LENGTH];
    int   kindex;
    char  temp[MAX_PATH_NAME_LENGTH];
    int   ival;
    int   scanret;
    int   retval = -1;

    const char *err1   = "Unable to open the configuration file:  %s for reading.\n";
    const char *err2   = "Unknown keyword: %s in configuration file: %s\n";
    const char *err3a  = "PATH keyword appears twice in %s.\n";
    const char *err3b  = "Error in parsing the path information from %s.\n";
    const char *err4a  = "INPUT-CLASS keyword appears twice in %s.\n";
    const char *err4b  = "Error in retrieving the input class from %s.\n";
    const char *err5a  = "INPUT-SIZE keyword appears twice in %s.\n";
    const char *err5b  = "Error in retrieving the input size from %s.\n";
    const char *err6a  = "RANK keyword appears twice in %s.\n";
    const char *err6b  = "Error in retrieving the rank from %s.\n";
    const char *err7a  = "DIMENSION-SIZES keyword appears twice in %s.\n";
    const char *err7b  = "DIMENSION-SIZES cannot appear before RANK is provided.\n";
    const char *err7c  = "Error in retrieving the dimension sizes from %s.\n";
    const char *err8a  = "OUTPUT-CLASS keyword appears twice in %s.\n";
    const char *err8b  = "Error in retrieving the output class from %s.\n";
    const char *err9a  = "OUTPUT-SIZE keyword appears twice in %s.\n";
    const char *err9b  = "Error in retrieving the output size from %s.\n";
    const char *err10a = "OUTPUT-ARCHITECTURE keyword appears twice in %s.\n";
    const char *err10b = "Error in retrieving the output architecture from %s.\n";
    const char *err11a = "OUTPUT-BYTE-ORDER keyword appears twice in %s.\n";
    const char *err11b = "Error in retrieving the output byte order from %s.\n";
    const char *err11c = "INPUT-BYTE-ORDER keyword appears twice in %s.\n";
    const char *err11d = "Error in retrieving the input byte order from %s.\n";
    const char *err11e = "Invalid value for output byte-order.\n";
    const char *err12a = "CHUNKED-DIMENSION-SIZES keyword appears twice in %s.\n";
    const char *err12b = "CHUNKED-DIMENSION-SIZES cannot appear before DIMENSION-SIZES are provided.\n";
    const char *err12c = "Error in retrieving the chunked dimension sizes from %s.\n";
    const char *err13a = "COMPRESSION-TYPE keyword appears twice in %s.\n";
    const char *err13b = "Error in retrieving the compression type from %s.\n";
    const char *err14a = "COMPRESSION-PARAM keyword appears twice in %s.\n";
    const char *err14b = "Error in retrieving the compression parameter from %s.\n";
    const char *err15a = "EXTERNAL-STORAGE keyword appears twice in %s.\n";
    const char *err15b = "Error in retrieving the external storage parameters from %s.\n";
    const char *err16a = "MAXIMUM-DIMENSIONS keyword appears twice in %s.\n";
    const char *err16b = "MAXIMUM-DIMENSIONS cannot appear before DIMENSION-SIZES are provided.\n";
    const char *err16c = "Error in retrieving the maximum dimension sizes from %s.\n";
    const char *err17  = "Configuration parameters are invalid in %s.\n";
    const char *err18  = "Unable to get string value.\n";
    const char *err19  = "Unable to get integer value.\n";
    const char *err20  = "Unable to get subset values.\n";

    /* - create vector to map which keywords have been found
     * - check vector after each keyword to check for violation
     * - at the end check vector to see if required fields have been provided
     * - process the output file according to the options
     */

    /* Initialize machine endian */
    volatile uint32_t ibyte = 0x01234567;
    /* 0 for big endian, 1 for little endian. */
    if ((*((volatile uint8_t *)(&ibyte))) == 0x67) {
        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err11e);
            return (-1);
        }
    }
    else {
        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err11e);
            return (-1);
        }
    }
    in->inputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
    printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif
    in->inputArchitecture = 0; /* default to NATIVE */

    if ((strm = fopen(infile, "r")) == NULL) {
        (void)fprintf(rawerrorstream, err1, infile);
        goto error;
    }

    scanret = fscanf(strm, "%254s", key);
    if ((scanret == 1) && !strcmp("HDF5", key)) {
#ifdef H5DEBUGIMPORT
        int pndx;
        printf("\nh5dump file\n");
#endif
        in->h5dumpInput = 1;
        scanret         = fscanf(strm, "%254s", temp); /* filename */
        scanret         = fscanf(strm, "%254s", temp); /* start bracket */
        scanret         = fscanf(strm, "%254s", key);  /* DATASET */
        while (scanret == 1) {
            if (!strcmp("DATASET", key)) { /* PATH */
#ifdef H5DEBUGIMPORT
                printf("h5dump DATASET key\n");
#endif
                if (in->configOptionVector[PATH] == 1) {
                    (void)fprintf(rawerrorstream, err3a, infile);
                    goto error;
                }
                if (fscanf(strm, "%254s", temp) != 1) {
                    (void)fprintf(rawerrorstream, "%s", err18);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump DATASET %s found\n", temp);
#endif
                if (parsePathInfo(&in->path, temp) == -1) {
                    (void)fprintf(rawerrorstream, err3b, infile);
                    goto error;
                }
                in->configOptionVector[PATH] = 1;
                scanret                      = fscanf(strm, "%254s", temp); /* start bracket */
#ifdef H5DEBUGIMPORT
                printf("h5dump DATASET %s found\n", temp);
#endif
            }                                    /* if(!strcmp("DATASET", key))  PATH */
            else if (!strcmp("DATATYPE", key)) { /* INPUT-CLASS */
#ifdef H5DEBUGIMPORT
                printf("h5dump DATATYPE key\n");
#endif
                if (in->configOptionVector[INPUT_CLASS] == 1) {
                    (void)fprintf(rawerrorstream, err4a, infile);
                    goto error;
                }

                if (fscanf(strm, "%254s", temp) != 1) {
                    (void)fprintf(rawerrorstream, "%s", err18);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump DATATYPE %s found\n", temp);
#endif
                if ((kindex = getInputClassType(in, temp)) == -1) {
                    (void)fprintf(rawerrorstream, err4b, infile);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump DATATYPE type %d inputClass\n", in->inputClass);
#endif

                in->configOptionVector[INPUT_CLASS] = 1;

                /*set default value for output-class */
                if (in->configOptionVector[OUTPUT_CLASS] == 0) {
                    if (in->inputClass == 0 || in->inputClass == 4)
                        in->outputClass = 0;
                    if (in->inputClass == 1 || in->inputClass == 2 || in->inputClass == 3)
                        in->outputClass = 1;
                    if (in->inputClass == 6 || in->inputClass == 7)
                        in->outputClass = 2;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump DATATYPE type %d outputClass\n", in->outputClass);
#endif

                if (in->inputClass == 5) { /* STRING */
                    int get_next_prop = 1;
                    in->outputClass   = -1;
#ifdef H5DEBUGIMPORT
                    printf("h5dump DATATYPE STRING found\n");
#endif
                    if (fscanf(strm, "%254s", temp) != 1) { /* start bracket */
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump DATATYPE STRING %s found\n", temp);
#endif
                    if (fscanf(strm, "%254s", temp) != 1) { /* string properties */
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
                    while (get_next_prop) {
                        if (!strcmp("STRSIZE", temp)) { /* STRSIZE */
                            if (fscanf(strm, "%254s", temp) != 1) {
                                (void)fprintf(rawerrorstream, "%s", err19);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump DATATYPE STRING STRSIZE %s found\n", temp);
#endif
                            if (strcmp("H5T_VARIABLE;", temp) != 0) {
                                char *more = temp;
                                ival       = (int)strtol(more, &more, 10);
                                if (getInputSize(in, ival) == -1) {
                                    (void)fprintf(rawerrorstream, err5b, infile);
                                    goto error;
                                }
#ifdef H5DEBUGIMPORT
                                printf("h5dump DATATYPE STRING %d InputSize\n", in->inputSize);
#endif
                            }
                        }
                        else if (!strcmp("STRPAD", temp)) {         /* STRPAD */
                            if (fscanf(strm, "%254s", temp) != 1) { /* STRPAD type */
                                (void)fprintf(rawerrorstream, "%s", err18);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump DATATYPE STRING STRPAD %s found\n", temp);
#endif
                        }
                        else if (!strcmp("CSET", key)) {            /* CSET */
                            if (fscanf(strm, "%254s", temp) != 1) { /* CSET type */
                                (void)fprintf(rawerrorstream, "%s", err18);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump DATATYPE STRING CSET %s found\n", temp);
#endif
                        }
                        else if (!strcmp("CTYPE", temp)) {          /* CTYPE */
                            if (fscanf(strm, "%254s", temp) != 1) { /* CTYPE type */
                                (void)fprintf(rawerrorstream, "%s", err18);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump DATATYPE STRING  CTYPE %s found\n", temp);
#endif
                        } /* if(!strcmp("CSET", key)) */
                        if (fscanf(strm, "%254s", temp) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err18);
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump DATATYPE STRING %s found\n", temp);
#endif
                        if (!strcmp("}", temp)) { /* end bracket */
                            get_next_prop = 0;
                        }
                    }                             /* while (get_next_prop) */
                }                                 /* if(kindex == 5)  STRING */
            }                                     /* else if(!strcmp("DATATYPE", key))  INPUT-CLASS */
            else if (!strcmp("DATASPACE", key)) { /* RANK and DIMENSIONS */
                hsize_t temp_dims[MAX_NUM_DIMENSION];

#ifdef H5DEBUGIMPORT
                printf("h5dump DATASPACE key\n");
#endif
                if (fscanf(strm, "%254s", temp) != 1) {
                    (void)fprintf(rawerrorstream, "%s", err18);
                    goto error;
                }
                if (!strcmp("SCALAR", temp)) { /* SCALAR */
                    in->rank = 0;
                }                                 /* if(!strcmp("SCALAR", key)) */
                else if (!strcmp("NULL", temp)) { /* NULL */
                    (void)fprintf(rawerrorstream, err6b, infile);
                    goto error;
                }                                   /* else if(!strcmp("NULL", key)) */
                else if (!strcmp("SIMPLE", temp)) { /* SIMPLE */
                    int icount = 0;
#ifdef H5DEBUGIMPORT
                    printf("h5dump DATASPACE SIMPLE found\n");
#endif
                    if (fscanf(strm, "%254s", temp) != 1) { /* start bracket */
                        (void)fprintf(rawerrorstream, err6b, infile);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                    if (fscanf(strm, "%254s", temp) != 1) { /* start paren */
                        (void)fprintf(rawerrorstream, err6b, infile);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                    if (!strcmp("(", temp)) { /* start paren */
                        int get_next_dim = 1;
                        int i            = 0;

                        if (fscanf(strm, "%254s", temp) != 1) { /* Dimension with optional comma */
                            (void)fprintf(rawerrorstream, err16c, infile);
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                        while (get_next_dim) {
                            char *more        = temp;
                            temp_dims[icount] = strtoull(more, &more, 10);
                            if (fscanf(strm, "%254s", temp) != 1) { /* Dimension or end paren */
                                (void)fprintf(rawerrorstream, err6b, infile);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                            if (!strcmp(")", temp)) { /* end paren */
                                in->rank                     = ++icount;
                                in->configOptionVector[RANK] = 1;
                                get_next_dim                 = 0;
                            }
                            else { /* Dimension */
                                icount++;
                                if (icount > MAX_NUM_DIMENSION) {
                                    (void)fprintf(rawerrorstream, "Invalid value for rank.\n");
                                    goto error;
                                }
                            }
                        } /* while (get_next_dim) */

                        if ((in->sizeOfDimension = (hsize_t *)malloc((size_t)in->rank * sizeof(hsize_t))) ==
                            NULL) {
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump DATASPACE SIMPLE %ld rank\n", in->rank);
#endif
                        for (i = 0; i < in->rank; i++) {
                            in->sizeOfDimension[i] = temp_dims[i];
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump DATASPACE SIMPLE dims[%ld]:", in->rank);
                        for (pndx = 0; pndx < in->rank; pndx++) {
                            printf(" %ld", in->sizeOfDimension[pndx]);
                        }
                        printf("\n");
#endif
                        in->configOptionVector[DIM] = 1;
                    } /* if(!strcmp("(", key))  start paren */
                    else {
                        (void)fprintf(rawerrorstream, err5b, infile);
                        goto error;
                    }
                    if (fscanf(strm, "%254s", temp) != 1) {
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                    if (!strcmp("/", temp)) { /* / max dims */
                        if ((in->maxsizeOfDimension =
                                 (hsize_t *)malloc((size_t)in->rank * sizeof(hsize_t))) == NULL) {
                            goto error;
                        }
                        if (fscanf(strm, "%254s", temp) != 1) { /* start paren */
                            (void)fprintf(rawerrorstream, err6b, infile);
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                        if (!strcmp("(", temp)) { /* start paren */
                            int get_next_dim = 1;
                            int i            = 0;

#ifdef H5DEBUGIMPORT
                            printf("h5dump DATASPACE SIMPLE process max dim values\n");
#endif
                            if (fscanf(strm, "%254s", temp) != 1) { /* max dim with optional comma */
                                (void)fprintf(rawerrorstream, err16c, infile);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                            while (get_next_dim) {
#ifdef H5DEBUGIMPORT
                                printf("h5dump DATASPACE SIMPLE get max dim value\n");
#endif
                                if (!strcmp("H5S_UNLIMITED", temp) ||
                                    !strcmp("H5S_UNLIMITED,", temp)) { /* unlimited */
                                    in->maxsizeOfDimension[i]      = H5S_UNLIMITED;
                                    in->configOptionVector[EXTEND] = 1;
                                }
                                else {
                                    char *more                = temp;
                                    in->maxsizeOfDimension[i] = strtoull(more, &more, 10);
                                }
                                if (fscanf(strm, "%254s", temp) != 1) { /* max dim or end paren */
                                    (void)fprintf(rawerrorstream, err16c, infile);
                                    goto error;
                                }
#ifdef H5DEBUGIMPORT
                                printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                                if (!strcmp(")", temp)) { /* end paren */
                                    get_next_dim = 0;
                                }
                                else { /* comma */
                                    i++;
                                    if (i >= MAX_NUM_DIMENSION) {
                                        (void)fprintf(rawerrorstream, "Invalid value for rank.\n");
                                        goto error;
                                    }
                                }
                            } /* while (get_next_dim) */
#ifdef H5DEBUGIMPORT
                            printf("h5dump DATASPACE SIMPLE maxdims[%ld]:", in->rank);
                            for (pndx = 0; pndx < in->rank; pndx++) {
                                printf(" %ld", in->maxsizeOfDimension[pndx]);
                            }
                            printf("\n");
                            printf("h5dump DATASPACE SIMPLE get max dim finished\n");
#endif
                        } /* if(!strcmp("(", key))  start paren */
                        else {
                            (void)fprintf(rawerrorstream, err16c, infile);
                            goto error;
                        }
                        scanret = fscanf(strm, "%254s", temp); /* end bracket */
#ifdef H5DEBUGIMPORT
                        printf("h5dump DATASPACE SIMPLE %s found\n", temp);
#endif
                    } /* if(!strcmp("/", key)) max dims separator */
                }     /* else if(!strcmp("SIMPLE", key)) */
                else {
                    (void)fprintf(rawerrorstream, err5b, infile);
                    goto error;
                }
            } /* else if(!strcmp("DATASPACE", key))  RANK and DIMENSIONS */
            else if (!strcmp("STORAGE_LAYOUT", key)) { /* CHUNKED-DIMENSION-SIZES */
#ifdef H5DEBUGIMPORT
                printf("h5dump STORAGE_LAYOUT key\n");
#endif
                if (fscanf(strm, "%254s", temp) != 1) { /* start bracket */
                    (void)fprintf(rawerrorstream, err6b, infile);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump STORAGE_LAYOUT %s found\n", temp);
#endif
                if (fscanf(strm, "%254s", temp) != 1) { /* CHUNKED */
                    (void)fprintf(rawerrorstream, err6b, infile);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump STORAGE_LAYOUT %s found\n", temp);
#endif
                if (!strcmp("CHUNKED", temp)) { /* CHUNKED */
                    if ((in->sizeOfChunk = (hsize_t *)malloc((size_t)in->rank * sizeof(hsize_t))) == NULL) {
                        (void)fprintf(rawerrorstream, "Unable to allocate dynamic memory.\n");
                        goto error;
                    }
                    if (fscanf(strm, "%254s", temp) != 1) { /* start paren */
                        (void)fprintf(rawerrorstream, err6b, infile);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump STORAGE_LAYOUT CHUNKED %s found\n", temp);
#endif
                    if (!strcmp("(", temp)) { /* start paren */
                        int get_next_dim = 1;
                        int icount       = 0;

                        if (fscanf(strm, "%254s", temp) != 1) { /* Dimension with optional comma */
                            (void)fprintf(rawerrorstream, err16c, infile);
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump STORAGE_LAYOUT CHUNKED %s found\n", temp);
#endif
                        while (get_next_dim) {
                            char *more              = temp;
                            in->sizeOfChunk[icount] = strtoull(more, &more, 10);
                            if (fscanf(strm, "%254s", temp) != 1) { /* Dimension or end paren */
                                (void)fprintf(rawerrorstream, err6b, infile);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump STORAGE_LAYOUT CHUNKED %s found\n", temp);
#endif
                            if (!strcmp(")", temp)) { /* end paren */
                                in->configOptionVector[RANK] = 1;
                                get_next_dim                 = 0;
                            }
                            else { /* Dimension */
                                icount++;
                                if (icount > MAX_NUM_DIMENSION) {
                                    (void)fprintf(rawerrorstream, "Invalid value for rank.\n");
                                    goto error;
                                }
                            }
                        } /* while (get_next_dim) */
#ifdef H5DEBUGIMPORT
                        printf("h5dump STORAGE_LAYOUT CHUNKED dims [%ld]:", in->rank);
                        for (pndx = 0; pndx < in->rank; pndx++) {
                            printf(" %ld", in->sizeOfChunk[pndx]);
                        }
                        printf("\n");
#endif
                        in->configOptionVector[DIM] = 1;
                    } /* if(!strcmp("(", key))  start paren */
                    else {
                        (void)fprintf(rawerrorstream, err5b, infile);
                        goto error;
                    }
                    if (fscanf(strm, "%254s", temp) != 1) { /* SIZE */
                        (void)fprintf(rawerrorstream, err6b, infile);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump STORAGE_LAYOUT CHUNKED %s found\n", temp);
#endif
                    if (!strcmp("SIZE", temp)) { /* SIZE */
                        if (fscanf(strm, "%d", (&ival)) != 1) {
                            (void)fprintf(rawerrorstream, "%s", err19);
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump STORAGE_LAYOUT CHUNKED SIZE %d found\n", ival);
#endif
                    }
                    while (strcmp("}", temp) != 0) {
                        if (fscanf(strm, "%254s", temp) != 1) { /* end bracket */
                            (void)fprintf(rawerrorstream, "%s", err18);
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump STORAGE_LAYOUT CHUNKED %s found\n", temp);
#endif
                    }
                    in->configOptionVector[CHUNK] = 1;
                } /* if(!strcmp("CHUNKED", key))  CHUNKED */
            }     /* else if(!strcmp("STORAGE_LAYOUT", key))  CHUNKED-DIMENSION-SIZES */
            else if (!strcmp("FILTERS", key)) { /* FILTERS */
#ifdef H5DEBUGIMPORT
                printf("h5dump FILTERS key\n");
#endif
                if (fscanf(strm, "%254s", temp) != 1) { /* start bracket */
                    (void)fprintf(rawerrorstream, err6b, infile);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump FILTERS %s found\n", temp);
#endif
                if (fscanf(strm, "%254s", temp) != 1) {
                    (void)fprintf(rawerrorstream, err6b, infile);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump FILTERS %s found\n", temp);
#endif
                if (!strcmp("COMPRESSION", temp)) { /* COMPRESSION */
#ifdef H5DEBUGIMPORT
                    printf("h5dump FILTERS COMPRESSION found\n");
#endif
                    if (fscanf(strm, "%254s", temp) != 1) { /* DEFLATE */
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump FILTERS COMPRESSION %s found\n", temp);
#endif
                    if (fscanf(strm, "%254s", temp) != 1) { /* bgin bracket */
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump FILTERS COMPRESSION %s found\n", temp);
#endif
                    if (fscanf(strm, "%254s", temp) != 1) { /* LEVEL */
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump FILTERS COMPRESSION %s found\n", temp);
#endif
                    if (fscanf(strm, "%d", (&ival)) != 1) {
                        (void)fprintf(rawerrorstream, "%s", err19);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump FILTERS COMPRESSION LEVEL %d found\n", ival);
#endif
                    in->compressionParam = ival;
                    if (fscanf(strm, "%254s", temp) != 1) { /* end bracket */
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump FILTERS COMPRESSION %s found\n", temp);
#endif
                    in->compressionType              = 0; /* ONLY GZIP supported */
                    in->configOptionVector[COMPRESS] = 1;
                }
                else if (!strcmp("CONTIGUOUS", temp)) { /* CONTIGUOUS */
#ifdef H5DEBUGIMPORT
                    printf("h5dump FILTERS CONTIGUOUS found\n");
#endif
                    in->configOptionVector[COMPRESS] = 0;
                }
                else if (!strcmp("NONE", temp)) { /* NONE */
#ifdef H5DEBUGIMPORT
                    printf("h5dump FILTERS NONE found\n");
#endif
                    in->configOptionVector[COMPRESS] = 0;
                }
                if (fscanf(strm, "%254s", temp) != 1) { /* end bracket */
                    (void)fprintf(rawerrorstream, "%s", err18);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump FILTERS %s found\n", temp);
#endif
            }
            else if (!strcmp("SUBSET", key)) { /* reduce dimensions */
                hsize_t temp_dims[MAX_NUM_DIMENSION];
                int     get_next_prop = 1;
#ifdef H5DEBUGIMPORT
                printf("h5dump SUBSET key\n");
#endif
                if (fscanf(strm, "%254s", temp) != 1) { /* start bracket */
                    (void)fprintf(rawerrorstream, err20, infile);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump SUBSET %s found\n", temp);
#endif
                if (fscanf(strm, "%254s", temp) != 1) { /* SUBSET keyword */
                    (void)fprintf(rawerrorstream, "%s", err18);
                    goto error;
                }
#ifdef H5DEBUGIMPORT
                printf("h5dump SUBSET %s found\n", temp);
#endif
                while (get_next_prop) {
                    if (!strcmp("COUNT", temp)) { /* COUNT */
                        int icount = 0;
                        if (fscanf(strm, "%254s", temp) != 1) { /* start paren */
                            (void)fprintf(rawerrorstream, err6b, infile);
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump SUBSET %s found\n", temp);
#endif
                        if (!strcmp("(", temp)) { /* start paren */
                            int get_next_dim = 1;
                            int i            = 0;

                            if (fscanf(strm, "%254s", temp) != 1) { /* Dimension with optional comma */
                                (void)fprintf(rawerrorstream, err16c, infile);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump SUBSET COUNT [%s] found\n", temp);
#endif
                            while (get_next_dim) {
                                char *more        = temp;
                                temp_dims[icount] = strtoull(more, &more, 10);
                                if (fscanf(strm, "%254s", temp) != 1) { /* Dimension or end paren */
                                    (void)fprintf(rawerrorstream, err6b, infile);
                                    goto error;
                                }
#ifdef H5DEBUGIMPORT
                                printf("h5dump SUBSET COUNT %s found\n", temp);
#endif
                                if (!strcmp(");", temp)) { /* end paren */
                                    in->rank                     = ++icount;
                                    in->configOptionVector[RANK] = 1;
                                    get_next_dim                 = 0;
                                }
                                else { /* Dimension */
                                    icount++;
                                    if (icount >= MAX_NUM_DIMENSION) {
                                        (void)fprintf(rawerrorstream, "Invalid value for rank.\n");
                                        goto error;
                                    }
                                }
                            } /* while (get_next_dim) */
                            for (i = 0; i < in->rank; i++) {
                                in->sizeOfDimension[i] = temp_dims[i];
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump SUBSET COUNT dims: [%d]", in->rank);
                            for (pndx = 0; pndx < in->rank; pndx++) {
                                printf(" %ld", in->sizeOfDimension[pndx]);
                            }
                            printf("\n");
#endif
                            in->configOptionVector[DIM] = 1;
                        }                         /* if(!strcmp("(", key))  start paren */
                    }                             /* if(!strcmp("COUNT", temp))  COUNT */
                    if (!strcmp("BLOCK", temp)) { /* BLOCK */
                        int icount = 0;
                        if (fscanf(strm, "%254s", temp) != 1) { /* start paren */
                            (void)fprintf(rawerrorstream, err6b, infile);
                            goto error;
                        }
#ifdef H5DEBUGIMPORT
                        printf("h5dump SUBSET %s found\n", temp);
#endif
                        if (!strcmp("(", temp)) { /* start paren */
                            int get_next_dim = 1;
                            int i            = 0;

                            if (fscanf(strm, "%254s", temp) != 1) { /* Dimension with optional comma */
                                (void)fprintf(rawerrorstream, err16c, infile);
                                goto error;
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump SUBSET BLOCK [%s] found\n", temp);
#endif
                            while (get_next_dim) {
                                char *more        = temp;
                                temp_dims[icount] = strtoull(more, &more, 10);
                                if (fscanf(strm, "%254s", temp) != 1) { /* Dimension or end paren */
                                    (void)fprintf(rawerrorstream, err6b, infile);
                                    goto error;
                                }
#ifdef H5DEBUGIMPORT
                                printf("h5dump SUBSET BLOCK %s found\n", temp);
#endif
                                if (!strcmp(");", temp)) { /* end paren */
                                    in->rank                     = ++icount;
                                    in->configOptionVector[RANK] = 1;
                                    get_next_dim                 = 0;
                                }
                                else { /* Dimension */
                                    icount++;
                                    if (icount > MAX_NUM_DIMENSION) {
                                        (void)fprintf(rawerrorstream, "Invalid value for rank.\n");
                                        goto error;
                                    }
                                }
                            } /* while (get_next_dim) */
                            for (i = 0; i < in->rank; i++) {
                                in->sizeOfDimension[i] = in->sizeOfDimension[i] * temp_dims[i];
                            }
#ifdef H5DEBUGIMPORT
                            printf("h5dump SUBSET BLOCK dims: [%d]", in->rank);
                            for (pndx = 0; pndx < in->rank; pndx++) {
                                printf(" %ld", in->sizeOfDimension[pndx]);
                            }
                            printf("\n");
#endif
                            in->configOptionVector[DIM] = 1;
                        } /* if(!strcmp("(", key))  start paren */
                    }     /* if(!strcmp("BLOCK", temp))  BLOCK */
                    if (fscanf(strm, "%254s", temp) != 1) {
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
#ifdef H5DEBUGIMPORT
                    printf("h5dump SUBSET %s found\n", temp);
#endif
                    if (!strcmp("}", temp)) { /* end bracket */
                        get_next_prop = 0;
                    }
                }                            /* while (get_next_prop) */
            }                                /* else if(!strcmp("SUBSET", key)) */
            else if (!strcmp("DATA", key)) { /* FINISHED */
#ifdef H5DEBUGIMPORT
                printf("h5dump DATA key\n");
#endif
                scanret = 0;
                break;
            }
            scanret = fscanf(strm, "%254s", key);
        }
#ifdef H5DEBUGIMPORT
        printf("h5dump path");
        for (pndx = 0; pndx < in->path.count; pndx++) {
            printf(" : %s", in->path.group[pndx]);
        }
        printf("\n");
        printf("h5dump inputClass=%d\n", in->inputClass);
        printf("h5dump inputSize=%d\n", in->inputSize);
        printf("h5dump inputArchitecture=%d\n", in->inputArchitecture);
        printf("h5dump inputByteOrder=%d\n", in->inputByteOrder);
        printf("h5dump rank=%d\n", in->rank);
        printf("h5dump outputClass=%d\n", in->outputClass);
        printf("h5dump outputSize=%d\n", in->outputSize);
        printf("h5dump outputArchitecture=%d\n", in->outputArchitecture);
        printf("h5dump outputByteOrder=%d\n", in->outputByteOrder);
        printf("h5dump compressionType=%d\n", in->compressionType);
        printf("h5dump compressionParam=%d\n", in->compressionParam);
        printf("h5dump externFilename=%s\n", in->externFilename);
        printf("h5dump sizeOfDimensions:\n");
        for (pndx = 0; pndx < in->rank; pndx++) {
            printf("  %ld\n", in->sizeOfDimension[pndx]);
        }
#endif
    }
    else {
#ifdef H5DEBUGIMPORT
        printf("original option keyword parsing\n");
#endif
        while (scanret == 1) {
            if ((kindex = mapKeywordToIndex(key)) == -1) {
                (void)fprintf(rawerrorstream, err2, key, infile);
                goto error;
            }
            switch (kindex) {
                case 0: /* PATH */
                    if (in->configOptionVector[PATH] == 1) {
                        (void)fprintf(rawerrorstream, err3a, infile);
                        goto error;
                    }
                    if (fscanf(strm, "%254s", temp) != 1) {
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
                    if (parsePathInfo(&in->path, temp) == -1) {
                        (void)fprintf(rawerrorstream, err3b, infile);
                        goto error;
                    }
                    in->configOptionVector[PATH] = 1;
                    break;

                case 1: /* INPUT-CLASS */
                    if (in->configOptionVector[INPUT_CLASS] == 1) {
                        (void)fprintf(rawerrorstream, err4a, infile);
                        goto error;
                    }

                    if (fscanf(strm, "%254s", temp) != 1) {
                        (void)fprintf(rawerrorstream, "%s", err18);
                        goto error;
                    }
                    if (getInputClass(in, temp) == -1) {
                        (void)fprintf(rawerrorstream, err4b, infile);
                        goto error;
                    }

                    in->configOptionVector[INPUT_CLASS] = 1;

                    /*set default value for output-class */
                    if (in->configOptionVector[OUTPUT_CLASS] == 0) {
                        if (in->inputClass == 0 || in->inputClass == 4)
                            in->outputClass = 0;
                        if (in->inputClass == 1 || in->inputClass == 2 || in->inputClass == 3)
                            in->outputClass = 1;
                        if (in->inputClass == 6 || in->inputClass == 7)
                            in->outputClass = 2;
                    }
                    break;

                case 2: /* INPUT-SIZE */
                    if (in->configOptionVector[INPUT_SIZE] == 1) {
                        (void)fprintf(rawerrorstream, err5a, infile);
                        goto error;
                    }
                    if (fscanf(strm, "%254d", (&ival)) != 1) {
                        (void)fprintf(rawerrorstream, "%s", err19);
                        goto error;
                    }
                    if (getInputSize(in, ival) == -1) {
                        (void)fprintf(rawerrorstream, err5b, infile);
                        goto error;
                    }
                    in->configOptionVector[INPUT_SIZE] = 1;

                    /*set default value for output-size */
                    if (in->configOptionVector[OUTPUT_SIZE] == 0)
                        in->outputSize = in->inputSize;
                    break;

                case 3: /* RANK */
                    if (in->configOptionVector[RANK] == 1) {
                        (void)fprintf(rawerrorstream, err6a, infile);
                        goto error;
                    }

                    if (getRank(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err6b, infile);
                        goto error;
                    }
                    in->configOptionVector[RANK] = 1;
                    break;

                case 4: /* DIMENSION-SIZES */
                    if (in->configOptionVector[DIM] == 1) {
                        (void)fprintf(rawerrorstream, err7a, infile);
                        goto error;
                    }

                    if (in->configOptionVector[RANK] == 0) {
                        (void)fprintf(rawerrorstream, err7b, infile);
                        goto error;
                    }
                    if (getDimensionSizes(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err7c, infile);
                        goto error;
                    }
                    in->configOptionVector[DIM] = 1;
                    break;

                case 5: /* OUTPUT-CLASS */
                    if (in->configOptionVector[OUTPUT_CLASS] == 1) {
                        (void)fprintf(rawerrorstream, err8a, infile);
                        goto error;
                    }

                    if (getOutputClass(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err8b, infile);
                        goto error;
                    }
                    in->configOptionVector[OUTPUT_CLASS] = 1;
                    break;

                case 6: /* OUTPUT-SIZE */
                    if (in->configOptionVector[OUTPUT_SIZE] == 1) {
                        (void)fprintf(rawerrorstream, err9a, infile);
                        goto error;
                    }

                    if (getOutputSize(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err9b, infile);
                        goto error;
                    }
                    in->configOptionVector[OUTPUT_SIZE] = 1;
                    break;

                case 7: /* OUTPUT-ARCHITECTURE */
                    if (in->configOptionVector[OUTPUT_ARCH] == 1) {
                        (void)fprintf(rawerrorstream, err10a, infile);
                        goto error;
                    }

                    if (getOutputArchitecture(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err10b, infile);
                        goto error;
                    }
                    in->configOptionVector[OUTPUT_ARCH] = 1;
                    break;

                case 8: /* OUTPUT-BYTE-ORDER */
                    if (in->configOptionVector[OUTPUT_B_ORDER] == 1) {
                        (void)fprintf(rawerrorstream, err11a, infile);
                        goto error;
                    }

                    if (getOutputByteOrder(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err11b, infile);
                        goto error;
                    }
                    in->configOptionVector[OUTPUT_B_ORDER] = 1;
                    break;

                case 9: /* CHUNKED-DIMENSION-SIZES */
                    if (in->configOptionVector[CHUNK] == 1) {
                        (void)fprintf(rawerrorstream, err12a, infile);
                        goto error;
                    }
                    /* can't appear before dimension sizes have been provided */
                    if (in->configOptionVector[DIM] == 0) {
                        (void)fprintf(rawerrorstream, err12b, infile);
                        goto error;
                    }

                    if (getChunkedDimensionSizes(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err12c, infile);
                        goto error;
                    }
                    in->configOptionVector[CHUNK] = 1;
                    break;

                case 10: /* COMPRESSION-TYPE */
                    if (in->configOptionVector[COMPRESS] == 1) {
                        (void)fprintf(rawerrorstream, err13a, infile);
                        goto error;
                    }

                    if (getCompressionType(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err13b, infile);
                        goto error;
                    }
                    in->configOptionVector[COMPRESS] = 1;

                    if (in->configOptionVector[COMPRESS_PARAM] == 0) {
                        if (in->compressionType == 0)
                            in->compressionParam = 6; /* default value if compressionType is GZIP */
                    }
                    break;

                case 11: /* COMPRESSION-PARAM */
                    if (in->configOptionVector[COMPRESS_PARAM] == 1) {
                        (void)fprintf(rawerrorstream, err14a, infile);
                        goto error;
                    }

                    if (getCompressionParameter(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err14b, infile);
                        goto error;
                    }

                    in->configOptionVector[COMPRESS_PARAM] = 1;

                    if (in->configOptionVector[COMPRESS] == 0)
                        in->compressionType = 0;

                    break;

                case 12: /* EXTERNAL-STORAGE */
                    if (in->configOptionVector[EXTERNALSTORE] == 1) {
                        (void)fprintf(rawerrorstream, err15a, infile);
                        goto error;
                    }

                    if (getExternalFilename(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err15b, infile);
                        goto error;
                    }
                    in->configOptionVector[EXTERNALSTORE] = 1;
                    break;

                case 13: /* MAXIMUM-DIMENSIONS */
                    if (in->configOptionVector[EXTEND] == 1) {
                        (void)fprintf(rawerrorstream, err16a, infile);
                        goto error;
                    }
                    /* can't appear before dimension sizes have been provided */
                    if (in->configOptionVector[DIM] == 0) {
                        (void)fprintf(rawerrorstream, err16b, infile);
                        goto error;
                    }
                    if (getMaximumDimensionSizes(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err16c, infile);
                        goto error;
                    }
                    in->configOptionVector[EXTEND] = 1;
                    break;

                case 14: /* INPUT-BYTE-ORDER */
                    if (in->configOptionVector[INPUT_B_ORDER] == 1) {
                        (void)fprintf(rawerrorstream, err11c, infile);
                        goto error;
                    }

                    if (getInputByteOrder(in, strm) == -1) {
                        (void)fprintf(rawerrorstream, err11d, infile);
                        goto error;
                    }
                    in->configOptionVector[INPUT_B_ORDER] = 1;
                    break;

                default:
                    break;
            }
            scanret = fscanf(strm, "%254s", key);
        }

        /*
            check if keywords obtained are valid
            if yes, return 0 else error
        */

        if (validateConfigurationParameters(in) == -1) {
            (void)fprintf(rawerrorstream, err17, infile);
            goto error;
        }
    }

    /* Set success return value */
    retval = 0;

error:
    if (strm)
        fclose(strm);
    return retval;
}

static int
validateConfigurationParameters(struct Input *in)
{
    const char *err1 = "One or more of the required fields (RANK, DIMENSION-SIZES) missing.\n";
    const char *err2 =
        "Cannot specify chunking or compression or extendible data sets with the external file option.\n";
    const char *err3 =
        "Cannot specify the compression or the extendible data sets without the chunking option.\n";
    const char *err4a = "OUTPUT-ARCHITECTURE cannot be STD if OUTPUT-CLASS is floating point (FP).\n";
    const char *err4b = "OUTPUT-ARCHITECTURE cannot be IEEE if OUTPUT-CLASS is integer (IN).\n";
    const char *err5  = "For OUTPUT-CLASS FP, valid values for OUTPUT-SIZE are (32, 64) .\n";

    /* for class STR other parameters are ignored */
    if (in->inputClass == 5) /* STR */
        return (0);

    if ((in->configOptionVector[DIM] != 1) || (in->configOptionVector[RANK] != 1)) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    if (in->configOptionVector[EXTERNALSTORE] == 1) {
        if ((in->configOptionVector[COMPRESS] == 1) || (in->configOptionVector[CHUNK] == 1) ||
            (in->configOptionVector[EXTEND] == 1)) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
    }

    if ((in->configOptionVector[COMPRESS] == 1) || (in->configOptionVector[EXTEND] == 1)) {
        if (in->configOptionVector[CHUNK] != 1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
    }

    /* Arch can't be STD if O/p class is FP */
    if (in->outputArchitecture == 1)
        if (in->outputClass == 1) {
            (void)fprintf(rawerrorstream, "%s", err4a);
            return (-1);
        }

    /* Arch can't be IEEE if O/p class is IN */
    if (in->outputArchitecture == 2)
        if (in->outputClass == 0) {
            (void)fprintf(rawerrorstream, "%s", err4b);
            return (-1);
        }

    if (in->outputClass == 1)
        if (in->outputSize != 32 && in->outputSize != 64) {
            (void)fprintf(rawerrorstream, "%s", err5);
            return (-1);
        }

    return (0);
}

static int
mapKeywordToIndex(char *key)
{
    int i;

    for (i = 0; i < NUM_KEYS; i++)
        if (!strcmp(keytable[i], key))
            return i;
    return -1;
}

static int
parsePathInfo(struct path_info *path, char *temp)
{
    const char  delimiter[] = "/\"";
    char       *token;
    int         i    = 0;
    const char *err1 = "Path string larger than MAX_PATH_NAME_LENGTH.\n";

    token = strtok(temp, delimiter);
    if (strlen(token) >= MAX_PATH_NAME_LENGTH) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }
    strcpy(path->group[i++], token);

    while (1) {
        token = strtok(NULL, delimiter);
        if (token == NULL)
            break;
        if (strlen(token) >= MAX_PATH_NAME_LENGTH) {
            (void)fprintf(rawerrorstream, "%s", err1);
            return (-1);
        }
        strcpy(path->group[i++], token);
    }
    path->count = i;
    return (0);
}

static int
parseDimensions(struct Input *in, char *strm)
{
    const char  delimiter[] = ",";
    char        temp[255];
    char       *token;
    int         i    = 0;
    const char *err1 = "Unable to allocate dynamic memory.\n";

    strncpy(temp, strm, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';
    strtok(temp, delimiter);

    while (1) {
        token = strtok(NULL, delimiter);
        if (token == NULL)
            break;
        i++;
    }
    in->rank = i + 1;
    if ((in->sizeOfDimension = (hsize_t *)malloc((size_t)in->rank * sizeof(hsize_t))) == NULL) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    i = 0;
    strncpy(temp, strm, sizeof(temp));
    temp[sizeof(temp) - 1]   = '\0';
    in->sizeOfDimension[i++] = strtoull(strtok(temp, delimiter), NULL, BASE_10);

    while (1) {
        token = strtok(NULL, delimiter);
        if (token == NULL)
            break;
        in->sizeOfDimension[i++] = strtoull(token, NULL, BASE_10);
    }
    return (0);
}

static int
getOutputClass(struct Input *in, FILE *strm)
{
    char        temp[255];
    int         kindex;
    const char *err1 = "Unable to get 'string' value.\n";
    const char *err2 = "Invalid value for output class.\n";

    if (fscanf(strm, "%254s", temp) != 1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    if ((kindex = OutputClassStrToInt(temp)) == -1) {
        (void)fprintf(rawerrorstream, "%s", err2);
        return (-1);
    }

    in->outputClass = kindex;
    return (0);
}

static int
OutputClassStrToInt(char *temp)
{
    int  i;
    char classKeywordTable[3][15] = {"IN", "FP", "UIN"};
    for (i = 0; i < 3; i++)
        if (!strcmp(classKeywordTable[i], temp))
            return i;

    return -1;
}
/* same as getInputSize. But defined separately for extensibility */
static int
getOutputSize(struct Input *in, FILE *strm)
{
    int         ival;
    int         i;
    int         outputSizeValidValues[4] = {8, 16, 32, 64};
    const char *err1                     = "Unable to get integer value.\n";
    const char *err2                     = "Invalid value for output size.\n";

    if (fscanf(strm, "%d", (&ival)) != 1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    for (i = 0; i < 4; i++)
        if (outputSizeValidValues[i] == ival) {
            in->outputSize = ival;
            return (0);
        }
    (void)fprintf(rawerrorstream, "%s", err2);
    return (-1);
}

static int
getInputClass(struct Input *in, char *temp)
{
    int         kindex;
    const char *err1 = "Invalid value for input class.\n";

    if ((kindex = InputClassStrToInt(temp)) == -1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    in->inputClass = kindex;
    return (0);
}

static int
getInputClassType(struct Input *in, char *buffer)
{
    int         kindex = -1;
    const char *err1   = "Invalid value for input class.\n";
    const char *err2   = "Invalid value for output architecture.\n";
    const char *err3   = "Invalid value for input byte-order.\n";

    if (!strcmp(buffer, "H5T_STD_I8BE")) {
        in->inputSize                      = 8;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_STD_I8LE")) {
        in->inputSize                      = 8;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_STD_I16BE")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_STD_I16LE")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_STD_I32BE")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_STD_I32LE")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_STD_I64BE")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_STD_I64LE")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_STD_U8BE")) {
        in->inputSize                      = 8;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_STD_U8LE")) {
        in->inputSize                      = 8;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_STD_U16BE")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_STD_U16LE")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_STD_U32BE")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_STD_U32LE")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_STD_U64BE")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_STD_U64LE")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_SCHAR")) {
        in->inputSize                      = 8;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_UCHAR")) {
        in->inputSize                      = 8;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_SHORT")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_USHORT")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_INT")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_UINT")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_LONG")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_ULONG")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_LLONG")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 4;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_ULLONG")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 7;
    }
    else if (!strcmp(buffer, "H5T_IEEE_F16BE")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("IEEE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_IEEE_F16LE")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("IEEE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_IEEE_F32BE")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("IEEE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_IEEE_F32LE")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("IEEE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_IEEE_F64BE")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("IEEE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_IEEE_F64LE")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("IEEE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_VAX_F32")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_VAX_F64")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        kindex = 3;
    }
#ifdef H5_HAVE__FLOAT16
    else if (!strcmp(buffer, "H5T_NATIVE_FLOAT16")) {
        in->inputSize                      = 16;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 3;
    }
#endif
    else if (!strcmp(buffer, "H5T_NATIVE_FLOAT")) {
        in->inputSize                      = 32;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_DOUBLE")) {
        in->inputSize                      = 64;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_NATIVE_LDOUBLE")) {
        in->inputSize                      = H5_SIZEOF_LONG_DOUBLE;
        in->configOptionVector[INPUT_SIZE] = 1;

        if ((kindex = OutputArchStrToInt("NATIVE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        kindex = 3;
    }
    else if (!strcmp(buffer, "H5T_TIME: not yet implemented")) {
        kindex = -1;
    }
    else if (!strcmp(buffer, "H5T_STRING")) {
        kindex = 5;
    }
    /*    case H5T_BITFIELD: */
    else if (!strcmp(buffer, "H5T_STD_B8BE")) {

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = -1;
    }
    else if (!strcmp(buffer, "H5T_STD_B8LE")) {

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = -1;
    }
    else if (!strcmp(buffer, "H5T_STD_B16BE")) {

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = -1;
    }
    else if (!strcmp(buffer, "H5T_STD_B16LE")) {

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = -1;
    }
    else if (!strcmp(buffer, "H5T_STD_B32BE")) {

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = -1;
    }
    else if (!strcmp(buffer, "H5T_STD_B32LE")) {

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = -1;
    }
    else if (!strcmp(buffer, "H5T_STD_B64BE")) {

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("BE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = -1;
    }
    else if (!strcmp(buffer, "H5T_STD_B64LE")) {

        if ((kindex = OutputArchStrToInt("STD")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err2);
            return (-1);
        }
        in->outputArchitecture = kindex;

        if ((kindex = OutputByteOrderStrToInt("LE")) == -1) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
        in->outputByteOrder = kindex;
#ifdef H5DEBUGIMPORT
        printf("h5dump inputByteOrder %d\n", in->inputByteOrder);
#endif

        kindex = -1;
    }
    /*    case H5T_OPAQUE: */
    else if (!strcmp(buffer, "H5T_OPAQUE")) {
        kindex = -1;
    }
    /*    case H5T_COMPOUND: */
    else if (!strcmp(buffer, "H5T_COMPOUND")) {
        kindex = -1;
    }
    /*    case H5T_REFERENCE: */
    else if (!strcmp(buffer, "H5T_REFERENCE")) {
        kindex = -1;
    }
    /*    case H5T_ENUM: */
    else if (!strcmp(buffer, "H5T_ENUM")) {
        kindex = -1;
    }
    /*    case H5T_VLEN: */
    else if (!strcmp(buffer, "H5T_VLEN")) {
        kindex = -1;
    }
    /*    case H5T_ARRAY: */
    else if (!strcmp(buffer, "H5T_ARRAY")) {
        kindex = -1;
    }
    /*    case H5T_COMPLEX: */
    else if (!strcmp(buffer, "H5T_COMPLEX")) {
        kindex = -1;
    }

    if (kindex == -1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    /*set default value for output-size */
    if (in->configOptionVector[OUTPUT_SIZE] == 0)
        in->outputSize = in->inputSize;
#ifdef H5DEBUGIMPORT
    printf("h5dump DATATYPE InClass %d inputSize\n", in->inputSize);
    printf("h5dump DATATYPE InClass %d outputSize\n", in->outputSize);
    printf("h5dump DATATYPE InClass %d outputArchitecture\n", in->outputArchitecture);
#endif

    in->inputClass = kindex;
    return (0);
}

static int
InputClassStrToInt(char *temp)
{
    int  i;
    char classKeywordTable[8][15] = {"TEXTIN", "TEXTFP", "TEXTFPE", "FP", "IN", "STR", "TEXTUIN", "UIN"};
    for (i = 0; i < 8; i++)
        if (!strcmp(classKeywordTable[i], temp))
            return i;
    return -1;
}

/* same as getOutputSize. But defined separately for extensibility */
static int
getInputSize(struct Input *in, int ival)
{
    int         i;
    int         inputSizeValidValues[4] = {8, 16, 32, 64};
    const char *err1                    = "Invalid value for input size.\n";

    for (i = 0; i < 4; i++)
        if (inputSizeValidValues[i] == ival) {
            in->inputSize = ival;
            return (0);
        }
    (void)fprintf(rawerrorstream, "%s", err1);
    return (-1);
}

static int
getInputByteOrder(struct Input *in, FILE *strm)
{
    char        temp[255];
    int         kindex;
    const char *err1 = "Unable to get 'string' value.\n";
    const char *err2 = "Invalid value for input byte-order.\n";

    if (fscanf(strm, "%254s", temp) != 1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    if ((kindex = OutputByteOrderStrToInt(temp)) == -1) {
        (void)fprintf(rawerrorstream, "%s", err2);
        return (-1);
    }

    in->inputByteOrder = kindex;
    return (0);
}

static int
getRank(struct Input *in, FILE *strm)
{
    int ival;

    const char *err1 = "Unable to get integer value.\n";
    const char *err2 = "Invalid value for rank.\n";

    if (fscanf(strm, "%d", (&ival)) != 1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }
    if (ival >= MIN_NUM_DIMENSION && ival <= MAX_NUM_DIMENSION) {
        in->rank = ival;
        return (0);
    }

    (void)fprintf(rawerrorstream, "%s", err2);
    return (-1);
}

/* same as getChunkedDimensionSizes. But defined separately for extensibility */
static int
getDimensionSizes(struct Input *in, FILE *strm)
{
    unsigned long long ullval;
    int                i = 0;

    const char *err1 = "Unable to allocate dynamic memory.\n";
    const char *err2 =
        "No. of dimensions for which dimension sizes provided is not equal to provided rank.\n";

    if ((in->sizeOfDimension = (hsize_t *)malloc((size_t)in->rank * sizeof(hsize_t))) == NULL) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    while (fscanf(strm, "%llu", (&ullval)) == 1)
        in->sizeOfDimension[i++] = ullval;

    if (in->rank != i) {
        (void)fprintf(rawerrorstream, "%s", err2);
        return (-1);
    }
    return (0);
}
/* same as getDimensionSizes. But defined separately for extensibility */
static int
getChunkedDimensionSizes(struct Input *in, FILE *strm)
{
    unsigned long long ullval;
    int                i = 0;

    const char *err1 = "Unable to allocate dynamic memory.\n";
    const char *err2 =
        "No. of dimensions for which chunked dimension sizes provided is not equal to provided rank.\n";
    const char *err3 = "The CHUNKED-DIMENSION-SIZES cannot exceed the sizes of DIMENSION-SIZES\n";

    if ((in->sizeOfChunk = (hsize_t *)malloc((size_t)in->rank * sizeof(hsize_t))) == NULL) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    while (fscanf(strm, "%llu", (&ullval)) == 1)
        in->sizeOfChunk[i++] = ullval;

    if (in->rank != i) {
        (void)fprintf(rawerrorstream, "%s", err2);
        return (-1);
    }

    for (i = 0; i < in->rank; i++)
        if (in->sizeOfChunk[i] > in->sizeOfDimension[i]) {
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
        }
    return (0);
}

static int
getMaximumDimensionSizes(struct Input *in, FILE *strm)
{
    long long llval;
    int       i = 0;

    const char *err1 = "Unable to allocate dynamic memory.\n";
    const char *err2 =
        "No. of dimensions for which maximum dimension sizes provided is not equal to provided rank.\n";
    const char *err3 = "The MAXIMUM-DIMENSIONS cannot be less than the sizes of DIMENSION-SIZES. Exception: "
                       "can be -1 to indicate unlimited size\n";

    if ((in->maxsizeOfDimension = (hsize_t *)malloc((size_t)in->rank * sizeof(hsize_t))) == NULL) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    while (fscanf(strm, "%lld", (&llval)) == 1) {
        if (llval == -1)
            in->maxsizeOfDimension[i++] = H5S_UNLIMITED;
        else
            in->maxsizeOfDimension[i++] = (hsize_t)llval;
    }

    if (in->rank != i) {
        (void)fprintf(rawerrorstream, "%s", err2);
        return (-1);
    }

    for (i = 0; i < in->rank; i++) {
        if (in->maxsizeOfDimension[i] != H5S_UNLIMITED)
            if (in->maxsizeOfDimension[i] < in->sizeOfDimension[i]) {
                (void)fprintf(rawerrorstream, "%s", err3);
                return (-1);
            }
    }
    return (0);
}

static int
getOutputArchitecture(struct Input *in, FILE *strm)
{
    char        temp[255];
    int         kindex;
    const char *err1 = "Unable to get 'string' value.\n";
    const char *err2 = "Invalid value for output architecture.\n";

    if (fscanf(strm, "%254s", temp) != 1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    if ((kindex = OutputArchStrToInt(temp)) == -1) {
        (void)fprintf(rawerrorstream, "%s", err2);
        return (-1);
    }

    in->outputArchitecture = kindex;
    return (0);
}

static int
OutputArchStrToInt(const char *temp)
{
    int  i;
    char outputArchKeywordTable[8][15] = {"NATIVE", "STD", "IEEE", "INTEL", "CRAY", "MIPS", "ALPHA", "UNIX"};
    for (i = 0; i < 8; i++)
        if (!strcmp(outputArchKeywordTable[i], temp))
            return i;
    return -1;
}

static int
getOutputByteOrder(struct Input *in, FILE *strm)
{
    char        temp[255];
    int         kindex;
    const char *err1 = "Unable to get 'string' value.\n";
    const char *err2 = "Invalid value for output byte-order.\n";

    if (fscanf(strm, "%254s", temp) != 1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    if ((kindex = OutputByteOrderStrToInt(temp)) == -1) {
        (void)fprintf(rawerrorstream, "%s", err2);
        return (-1);
    }

    in->outputByteOrder = kindex;
    return (0);
}

static int
OutputByteOrderStrToInt(const char *temp)
{
    int  i;
    char outputByteOrderKeywordTable[2][15] = {"BE", "LE"};
    for (i = 0; i < 2; i++)
        if (!strcmp(outputByteOrderKeywordTable[i], temp))
            return i;
    return -1;
}

static int
getCompressionType(struct Input *in, FILE *strm)
{
    char        temp[255];
    int         kindex;
    const char *err1 = "Unable to get 'string' value.\n";
    const char *err2 = "Invalid value for compression.\n";

    if (fscanf(strm, "%254s", temp) != 1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    if ((kindex = CompressionTypeStrToInt(temp)) == -1) {
        (void)fprintf(rawerrorstream, "%s", err2);
        return (-1);
    }

    in->compressionType = kindex;
    return (0);
}

static int
CompressionTypeStrToInt(char *temp)
{
    /* currently supports only GZIP */
    /* can be extended by adding fields to the table */

    int  i;
    char CompressionTypeKeywordTable[1][15] = {"GZIP"};
    for (i = 0; i < 1; i++)
        if (!strcmp(CompressionTypeKeywordTable[i], temp))
            return i;
    return -1;
}

static int
getCompressionParameter(struct Input *in, FILE *strm)
{
    /*  currently supports only GZIP */
    /*  can be extended by adding more values to COMPRESSION-TYPE and */
    /*  handling the parameters here by adding more cases  */

    int         ival;
    const char *err1 = "Unable to get integer value.\n";
    const char *err2 = "Invalid value for compression parameter.\n";
    const char *err3 = "Unsupported Compression Type.\n";

    switch (in->compressionType) {
        case 0: /* GZIP */
            if (fscanf(strm, "%d", (&ival)) != 1) {
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
            }

            if (ival < 0 || ival > 9) {
                (void)fprintf(rawerrorstream, "%s", err2);
                return (-1);
            }
            in->compressionParam = ival;
            return (0);

        default:
            (void)fprintf(rawerrorstream, "%s", err3);
            return (-1);
    }
}

static int
getExternalFilename(struct Input *in, FILE *strm)
{
    size_t      temp_len;
    char        temp[255];
    const char *err1 = "Unable to get 'string' value.\n";

    if (fscanf(strm, "%254s", temp) != 1) {
        (void)fprintf(rawerrorstream, "%s", err1);
        return (-1);
    }

    temp_len           = strlen(temp);
    in->externFilename = (char *)malloc((temp_len + 1) * sizeof(char));
    (void)strcpy(in->externFilename, temp);
    in->externFilename[temp_len] = '\0';
    return (0);
}

void
setDefaultValues(struct Input *in, int count)
{
    int  i;
    char temp[255];
    char num[255];

    in->h5dumpInput    = 0;
    in->inputClass     = 3; /* FP */
    in->inputSize      = 32;
    in->outputClass    = 1; /* FP */
    in->outputSize     = 32;
    in->inputByteOrder = -1; /* use default    */
    in->rank           = 0;
    in->path.count     = 1;

    strcpy(temp, "dataset");
    snprintf(num, sizeof(num), "%d", count);
    strcat(temp, num);
    strcpy(in->path.group[0], temp);

    in->outputArchitecture = 0;  /* NATIVE */
    in->outputByteOrder    = -1; /* use default    */
    in->compressionType    = 0;  /* GZIP   */
    for (i = 0; i < NUM_KEYS; i++)
        in->configOptionVector[i] = 0;
}

hid_t
createOutputDataType(struct Input *in)
{
    hid_t       new_type = (-1);
    const char *err1     = "Invalid value for output class.\n";
    const char *err2     = "Invalid value for output size.\n";
    const char *err3     = "Invalid value for output byte order.\n";
    const char *err4     = "Invalid value for output architecture.\n";
    const char *err5     = "STD not supported for float.\n";
    const char *err6     = "IEEE not supported for INT.\n";

    switch (in->outputClass) {
        case 0:
            switch (in->outputArchitecture) {
                case 0: /* NATIVE */
                    switch (in->outputSize) {
                        case 8:
                            new_type = H5Tcopy(H5T_NATIVE_CHAR);
                            break;

                        case 16:
                            new_type = H5Tcopy(H5T_NATIVE_SHORT);
                            break;

                        case 32:
                            new_type = H5Tcopy(H5T_NATIVE_INT);
                            break;

                        case 64:
                            new_type = H5Tcopy(H5T_NATIVE_LLONG);
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err2);
                            return (-1);
                    }
                    switch (in->outputByteOrder) {
                        case -1: /* default */
                            break;
                        case 0:
                            H5Tset_order(new_type, H5T_ORDER_BE);
                            break;

                        case 1:
                            H5Tset_order(new_type, H5T_ORDER_LE);
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err3);
                            return (-1);
                    }
                    break;

                case 1: /* STD */
                    switch (in->outputSize) {
                        case 8:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_STD_I8BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_STD_I8LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        case 16:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_STD_I16BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_STD_I16LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        case 32:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_STD_I32BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_STD_I32LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        case 64:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_STD_I64BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_STD_I64LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err2);
                            return (-1);
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err4);
                    return (-1);
            }
            break;

        case 1:
            switch (in->outputArchitecture) {
                case 0:
                    switch (in->outputSize) {
#ifdef H5_HAVE__FLOAT16
                        case 16:
                            new_type = H5Tcopy(H5T_NATIVE_FLOAT16);
                            break;
#endif

                        case 32:
                            new_type = H5Tcopy(H5T_NATIVE_FLOAT);
                            break;

                        case 64:
                            new_type = H5Tcopy(H5T_NATIVE_DOUBLE);
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err2);
                            return (-1);
                    }
                    switch (in->outputByteOrder) {
                        case -1: /* DEFAULT */
                            break;
                        case 0:
                            H5Tset_order(new_type, H5T_ORDER_BE);
                            break;

                        case 1:
                            H5Tset_order(new_type, H5T_ORDER_LE);
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err3);
                            return (-1);
                    }
                    break;

                case 1:
                    (void)fprintf(rawerrorstream, "%s", err5);
                    return (-1);

                case 2:
                    switch (in->outputSize) {
                        case 16:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_IEEE_F16BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_IEEE_F16LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        case 32:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_IEEE_F32BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_IEEE_F32LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        case 64:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_IEEE_F64BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_IEEE_F64LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err2);
                            return (-1);
                    }
                    break;

                default:
                    (void)fprintf(rawerrorstream, "%s", err4);
                    return (-1);
            }
            break;

        case 2:
            switch (in->outputArchitecture) {
                case 0:
                    switch (in->outputSize) {
                        case 8:
                            new_type = H5Tcopy(H5T_NATIVE_UCHAR);
                            break;

                        case 16:
                            new_type = H5Tcopy(H5T_NATIVE_USHORT);
                            break;

                        case 32:
                            new_type = H5Tcopy(H5T_NATIVE_UINT);
                            break;

                        case 64:
                            new_type = H5Tcopy(H5T_NATIVE_ULLONG);
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err2);
                            return (-1);
                    }
                    switch (in->outputByteOrder) {
                        case -1: /* Default */
                            break;
                        case 0:
                            H5Tset_order(new_type, H5T_ORDER_BE);
                            break;

                        case 1:
                            H5Tset_order(new_type, H5T_ORDER_LE);
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err3);
                            return (-1);
                    }
                    break;

                case 1:
                    switch (in->outputSize) {
                        case 8:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_STD_U8BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_STD_U8LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        case 16:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_STD_U16BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_STD_U16LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        case 32:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_STD_U32BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_STD_U32LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        case 64:
                            switch (in->outputByteOrder) {
                                case -1:
                                case 0:
                                    new_type = H5Tcopy(H5T_STD_U64BE);
                                    break;

                                case 1:
                                    new_type = H5Tcopy(H5T_STD_U64LE);
                                    break;

                                default:
                                    (void)fprintf(rawerrorstream, "%s", err3);
                                    return (-1);
                            }
                            break;

                        default:
                            (void)fprintf(rawerrorstream, "%s", err2);
                            return (-1);
                    }
                    break;

                case 2:
                    (void)fprintf(rawerrorstream, "%s", err6);
                    return (-1);

                default:
                    (void)fprintf(rawerrorstream, "%s", err4);
                    return (-1);
            }
            break;

        default:
            (void)fprintf(rawerrorstream, "%s", err1);
            return (-1);
    }
    return new_type;
}

hid_t
createInputDataType(struct Input *in)
{
    hid_t       new_type = (-1);
    const char *err1     = "Invalid value for input class.\n";
    const char *err2     = "Invalid value for input size.\n";
    const char *err3     = "Invalid value for input byte order.\n";
    const char *err4     = "Invalid value for output architecture.\n";
    const char *err5     = "STD not supported for float.\n";
    const char *err6     = "IEEE not supported for INT.\n";

    if (in->h5dumpInput) {
        switch (in->inputClass) {
            case 4:
                switch (in->inputArchitecture) {
                    case 0: /*NATIVE*/
                        switch (in->inputSize) {
                            case 8:
                                new_type = H5Tcopy(H5T_NATIVE_CHAR);
                                break;

                            case 16:
                                new_type = H5Tcopy(H5T_NATIVE_SHORT);
                                break;

                            case 32:
                                new_type = H5Tcopy(H5T_NATIVE_INT);
                                break;

                            case 64:
                                new_type = H5Tcopy(H5T_NATIVE_LLONG);
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err2);
                                return (-1);
                        }
                        switch (in->inputByteOrder) {
                            case -1: /* default */
                                break;
                            case 0:
                                H5Tset_order(new_type, H5T_ORDER_BE);
                                break;

                            case 1:
                                H5Tset_order(new_type, H5T_ORDER_LE);
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err3);
                                return (-1);
                        }
                        break;

                    case 1: /*STD*/
                        switch (in->inputSize) {
                            case 8:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_STD_I8BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_STD_I8LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            case 16:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_STD_I16BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_STD_I16LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            case 32:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_STD_I32BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_STD_I32LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            case 64:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_STD_I64BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_STD_I64LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err2);
                                return (-1);
                        }
                        break;

                    default:
                        (void)fprintf(rawerrorstream, "%s", err4);
                        return (-1);
                }
                break;

            case 3:
                switch (in->inputArchitecture) {
                    case 0:
                        switch (in->inputSize) {
#ifdef H5_HAVE__FLOAT16
                            case 16:
                                new_type = H5Tcopy(H5T_NATIVE_FLOAT16);
                                break;
#endif

                            case 32:
                                new_type = H5Tcopy(H5T_NATIVE_FLOAT);
                                break;

                            case 64:
                                new_type = H5Tcopy(H5T_NATIVE_DOUBLE);
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err2);
                                return (-1);
                        }
                        switch (in->inputByteOrder) {
                            case -1: /* DEFAULT */
                                break;
                            case 0:
                                H5Tset_order(new_type, H5T_ORDER_BE);
                                break;

                            case 1:
                                H5Tset_order(new_type, H5T_ORDER_LE);
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err3);
                                return (-1);
                        }
                        break;

                    case 1:
                        (void)fprintf(rawerrorstream, "%s", err5);
                        return (-1);

                    case 2:
                        switch (in->inputSize) {
                            case 16:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_IEEE_F16BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_IEEE_F16LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            case 32:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_IEEE_F32BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_IEEE_F32LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            case 64:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_IEEE_F64BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_IEEE_F64LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err2);
                                return (-1);
                        }
                        break;

                    default:
                        (void)fprintf(rawerrorstream, "%s", err4);
                        return (-1);
                }
                break;

            case 7:
                switch (in->inputArchitecture) {
                    case 0:
                        switch (in->inputSize) {
                            case 8:
                                new_type = H5Tcopy(H5T_NATIVE_UCHAR);
                                break;

                            case 16:
                                new_type = H5Tcopy(H5T_NATIVE_USHORT);
                                break;

                            case 32:
                                new_type = H5Tcopy(H5T_NATIVE_UINT);
                                break;

                            case 64:
                                new_type = H5Tcopy(H5T_NATIVE_ULLONG);
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err2);
                                return (-1);
                        }
                        switch (in->inputByteOrder) {
                            case -1: /* Default */
                                break;
                            case 0:
                                H5Tset_order(new_type, H5T_ORDER_BE);
                                break;

                            case 1:
                                H5Tset_order(new_type, H5T_ORDER_LE);
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err3);
                                return (-1);
                        }
                        break;

                    case 1:
                        switch (in->inputSize) {
                            case 8:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_STD_U8BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_STD_U8LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            case 16:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_STD_U16BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_STD_U16LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            case 32:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_STD_U32BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_STD_U32LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            case 64:
                                switch (in->inputByteOrder) {
                                    case -1:
                                    case 0:
                                        new_type = H5Tcopy(H5T_STD_U64BE);
                                        break;

                                    case 1:
                                        new_type = H5Tcopy(H5T_STD_U64LE);
                                        break;

                                    default:
                                        (void)fprintf(rawerrorstream, "%s", err3);
                                        return (-1);
                                }
                                break;

                            default:
                                (void)fprintf(rawerrorstream, "%s", err2);
                                return (-1);
                        }
                        break;

                    case 2:
                        (void)fprintf(rawerrorstream, "%s", err6);
                        return (-1);

                    default:
                        (void)fprintf(rawerrorstream, "%s", err4);
                        return (-1);
                }
                break;

            default:
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
        }
    }
    else {
        switch (in->inputClass) {
            case 0:
            case 4:
                switch (in->inputSize) {
                    case 8:
                        new_type = H5Tcopy(H5T_NATIVE_CHAR);
                        break;

                    case 16:
                        new_type = H5Tcopy(H5T_NATIVE_SHORT);
                        break;

                    case 32:
                        new_type = H5Tcopy(H5T_NATIVE_INT);
                        break;

                    case 64:
                        new_type = H5Tcopy(H5T_NATIVE_LLONG);
                        break;

                    default:
                        (void)fprintf(rawerrorstream, "%s", err2);
                        return (-1);
                }
                break;

            case 1:
            case 2:
            case 3:
                switch (in->inputSize) {
#ifdef H5_HAVE__FLOAT16
                    case 16:
                        new_type = H5Tcopy(H5T_NATIVE_FLOAT16);
                        break;
#endif

                    case 32:
                        new_type = H5Tcopy(H5T_NATIVE_FLOAT);
                        break;

                    case 64:
                        new_type = H5Tcopy(H5T_NATIVE_DOUBLE);
                        break;

                    default:
                        (void)fprintf(rawerrorstream, "%s", err2);
                        return (-1);
                }
                break;

            case 5:
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
                break;

            case 6:
            case 7:
                switch (in->inputSize) {
                    case 8:
                        new_type = H5Tcopy(H5T_NATIVE_UCHAR);
                        break;

                    case 16:
                        new_type = H5Tcopy(H5T_NATIVE_USHORT);
                        break;

                    case 32:
                        new_type = H5Tcopy(H5T_NATIVE_UINT);
                        break;

                    case 64:
                        new_type = H5Tcopy(H5T_NATIVE_ULLONG);
                        break;

                    default:
                        (void)fprintf(rawerrorstream, "%s", err2);
                        return (-1);
                }
                break;

            default:
                (void)fprintf(rawerrorstream, "%s", err1);
                return (-1);
        }
    }
    return new_type;
}

static int
process(struct Options *opt)
{
    struct Input *in;
    FILE         *extfile;
    hid_t         file_id;
    hid_t         group_id;
    hid_t         handle;
    hid_t         dataset;
    hid_t         dataspace = (-1);
    hid_t         intype;
    hid_t         outtype;
    hid_t         proplist;
    hsize_t       numOfElements = 1;
    int           j;
    int           k;

    const char *err1 = "Error creating HDF output file: %s.\n";
    const char *err2 = "Error in processing the configuration file: %s.\n";
    const char *err3 = "Error in reading the input file: %s.\n";
    const char *err4 = "Error in creating or opening external file.\n";
    const char *err5 =
        "Error in creating the output data set. Dataset with the same name may exist at the specified path\n";
    const char *err6 = "Error in writing the output data set.\n";

    H5E_BEGIN_TRY
    {
        if ((file_id = H5Fopen(opt->outfile, H5F_ACC_RDWR, H5P_DEFAULT)) < 0) {
            if ((file_id = H5Fcreate(opt->outfile, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) == FAIL) {
                (void)fprintf(rawerrorstream, err1, opt->outfile);
                return (-1);
            }
        }
    }
    H5E_END_TRY

    for (k = 0; k < opt->fcount; k++) {
        in = &(opt->infiles[k].in);
        if (opt->infiles[k].config == 1) {
            if (processConfigurationFile(opt->infiles[k].configfile, in) == -1) {
                (void)fprintf(rawerrorstream, err2, opt->infiles[k].configfile);
                return (-1);
            }
        }

        if (processDataFile(opt->infiles[k].datafile, in, file_id) == -1) {
            (void)fprintf(rawerrorstream, err3, opt->infiles[k].datafile);
            return (-1);
        }

        if (in->inputClass != 5) { /* STR */
            for (j = 0; j < in->rank; j++)
                numOfElements *= in->sizeOfDimension[j];

            /* disable error reporting */
            H5E_BEGIN_TRY
            {
                /* create parent groups */
                if (in->path.count > 1) {
                    j      = 0;
                    handle = file_id;
                    while (j < in->path.count - 1) {
                        if ((group_id = H5Gopen2(handle, in->path.group[j], H5P_DEFAULT)) < 0) {
                            group_id = H5Gcreate2(handle, in->path.group[j++], H5P_DEFAULT, H5P_DEFAULT,
                                                  H5P_DEFAULT);
                            for (; j < in->path.count - 1; j++)
                                group_id = H5Gcreate2(group_id, in->path.group[j], H5P_DEFAULT, H5P_DEFAULT,
                                                      H5P_DEFAULT);
                            handle = group_id;
                            break;
                        }
                        handle = group_id;
                        j++;
                    }
                }
                else {
                    handle = file_id;
                    j      = 0;
                }

                /*enable error reporting */
            }
            H5E_END_TRY

            /*create data type */
            intype  = createInputDataType(in);
            outtype = createOutputDataType(in);
#ifdef H5DEBUGIMPORT
            printf("process intype %ld outtype %ld\n", intype, outtype);
#endif

            /* create property list */
            proplist = H5Pcreate(H5P_DATASET_CREATE);
            if (in->configOptionVector[CHUNK] == 1) {
                H5Pset_layout(proplist, H5D_CHUNKED);
                /* not reqd chunking is implied if set_chunk is used  */
                H5Pset_chunk(proplist, in->rank, in->sizeOfChunk);
            }

            if (in->configOptionVector[COMPRESS] == 1) {
                H5Pset_deflate(proplist, (unsigned)in->compressionParam);
            }

            if (in->configOptionVector[EXTERNALSTORE] == 1) {
                /* creating the external file if it doesn't exist */
                if ((extfile = fopen(in->externFilename, "ab")) == NULL) {
                    (void)fprintf(rawerrorstream, "%s", err4);
                    H5Pclose(proplist);
                    H5Sclose(dataspace);
                    H5Fclose(file_id);
                    return (-1);
                }
                fclose(extfile);
                H5Pset_external(proplist, in->externFilename, 0, numOfElements * (hsize_t)in->inputSize / 8);
            }

            /* create dataspace */
            if (in->configOptionVector[EXTEND] == 1)
                dataspace = H5Screate_simple(in->rank, in->sizeOfDimension, in->maxsizeOfDimension);
            else
                dataspace = H5Screate_simple(in->rank, in->sizeOfDimension, NULL);

            /* disable error reporting */
            H5E_BEGIN_TRY
            {
                /* create data set */
                if ((dataset = H5Dcreate2(handle, in->path.group[j], outtype, dataspace, H5P_DEFAULT,
                                          proplist, H5P_DEFAULT)) < 0) {
                    (void)fprintf(rawerrorstream, "%s", err5);
                    H5Pclose(proplist);
                    H5Sclose(dataspace);
                    H5Fclose(file_id);
                    return (-1);
                }

                /*enable error reporting */
            }
            H5E_END_TRY

            /* write dataset */
            if (H5Dwrite(dataset, intype, H5S_ALL, H5S_ALL, H5P_DEFAULT, (VOIDP)in->data) < 0) {
                (void)fprintf(rawerrorstream, "%s", err6);
                H5Dclose(dataset);
                H5Pclose(proplist);
                H5Sclose(dataspace);
                H5Fclose(file_id);
                return (-1);
            }

            H5Dclose(dataset);
            H5Pclose(proplist);
            H5Sclose(dataspace);
        }

    } /* STR */

    H5Fclose(file_id);
    return 0;
}

uint16_t
swap_uint16(uint16_t val)
{
    return (uint16_t)((val << 8) | (val >> 8));
}

int16_t
swap_int16(int16_t val)
{
    return (int16_t)((val << 8) | ((val >> 8) & 0xFF));
}

uint32_t
swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

int32_t
swap_int32(int32_t val)
{
    val = (int32_t)(((uint32_t)(val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF));
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

int64_t
swap_int64(int64_t val)
{
    val = (int64_t)(((uint64_t)(val << 8) & 0xFF00FF00FF00FF00ULL) |
                    ((uint64_t)(val >> 8) & 0x00FF00FF00FF00FFULL));
    val = (int64_t)(((uint64_t)(val << 16) & 0xFFFF0000FFFF0000ULL) |
                    ((uint64_t)(val >> 16) & 0x0000FFFF0000FFFFULL));
    return (int64_t)((uint64_t)(val << 32) | ((uint64_t)(val >> 32) & 0xFFFFFFFFULL));
}

uint64_t
swap_uint64(uint64_t val)
{
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
}

/*
 * Name:
 *      help
 *
 * Purpose:
 *      Print a helpful summary of command usage and features.
 */

void
help(char *name)
{
    (void)fprintf(rawoutstream, "Name:\n\n");
    (void)fprintf(rawoutstream, "\t%s\n\n", name);
    (void)fprintf(rawoutstream, "\t  TOOL NAME:\n");
    (void)fprintf(rawoutstream, "\t   %s\n", name);
    (void)fprintf(rawoutstream, "\t   SYNTAX:\n");
    (void)fprintf(rawoutstream, "\t   %s -h[elp], OR\n", name);
    (void)fprintf(rawoutstream,
                  "\t   %s <infile> -c[onfig] <configfile> [<infile> -c[config] <configfile>...]", name);
    (void)fprintf(rawoutstream, "\t\t\t\t      -o[utfile] <outfile>\n\n");
    (void)fprintf(rawoutstream, "\t   PURPOSE:\n");
    (void)fprintf(rawoutstream, "\t   To convert data stored in one or more ASCII or binary files\n");
    (void)fprintf(rawoutstream, "\t  into one or more datasets (in accordance with the \n");
    (void)fprintf(rawoutstream, "\t  user-specified type and storage properties) in an existing \n");
    (void)fprintf(rawoutstream, "\t  or new HDF5 file.\n\n");
    (void)fprintf(rawoutstream, "\t   DESCRIPTION:\n");
    (void)fprintf(rawoutstream, "\t  The primary objective of the utility is to convert floating\n");
    (void)fprintf(rawoutstream, "\t  point or integer data stored in ASCII text or binary form \n");
    (void)fprintf(rawoutstream, "\t  into a data-set according to the type and storage properties\n");
    (void)fprintf(rawoutstream, "\t  specified by the user. The utility can also accept ASCII\n");
    (void)fprintf(rawoutstream, "\t  text files and store the contents in a compact form as an\n");
    (void)fprintf(rawoutstream, "\t  array of one-dimensional strings.\n\n");
    (void)fprintf(rawoutstream, "\t  The input data to be written as a data-set can be provided\n");
    (void)fprintf(rawoutstream, "\t  to the utility in one of the following forms:\n");
    (void)fprintf(rawoutstream, "\t  1. ASCII text file with numeric data (floating point or \n");
    (void)fprintf(rawoutstream, "\t  integer data). \n");
    (void)fprintf(rawoutstream, "\t  2. Binary file with native floating point data (32-bit or \n");
    (void)fprintf(rawoutstream, "\t  64-bit) \n");
    (void)fprintf(rawoutstream, "\t  3. Binary file with native integer (signed or unsigned)\n");
    (void)fprintf(rawoutstream, "\t  data (8-bit or 16-bit or 32-bit or 64-bit). \n");
    (void)fprintf(rawoutstream, "\t  4. ASCII text file containing strings (text data).\n");
    (void)fprintf(rawoutstream, "\t    \n");
    (void)fprintf(rawoutstream, "\t  Every input file is associated with a configuration file \n");
    (void)fprintf(rawoutstream, "\t  also provided as an input to the utility. (See Section \n");
    (void)fprintf(rawoutstream, "\t  \"CONFIGURATION FILE\" to know how it is to be organized).\n");
    (void)fprintf(rawoutstream, "\t  The class, size and dimensions of the input data is \n");
    (void)fprintf(rawoutstream, "\t  specified in this configuration file. A point to note is\n");
    (void)fprintf(rawoutstream, "\t  that the floating point data in the ASCII text file may be\n");
    (void)fprintf(rawoutstream, "\t  organized in the fixed floating form (for example 323.56)\n");
    (void)fprintf(rawoutstream, "\t  or in a scientific notation (for example 3.23E+02). A \n");
    (void)fprintf(rawoutstream, "\t  different input-class specification is to be used for both\n");
    (void)fprintf(rawoutstream, "\t  forms.\n\n");
    (void)fprintf(rawoutstream, "\t  The utility extracts the input data from the input file \n");
    (void)fprintf(rawoutstream, "\t  according to the specified parameters and saves it into \n");
    (void)fprintf(rawoutstream, "\t  an H5 dataset. \n\n");
    (void)fprintf(rawoutstream, "\t  The user can specify output type and storage properties in \n");
    (void)fprintf(rawoutstream, "\t  the configuration file. The user is required to specify the \n");
    (void)fprintf(rawoutstream, "\t  path of the dataset. If the groups in the path leading to \n");
    (void)fprintf(rawoutstream, "\t  the data-set do not exist, the groups will be created by the\n");
    (void)fprintf(rawoutstream, "\t  utility. If no group is specified, the dataset will be\n");
    (void)fprintf(rawoutstream, "\t  created under the root group.\n\n");
    (void)fprintf(rawoutstream, "\t  In addition to the name, the user is also required to \n");
    (void)fprintf(rawoutstream, "\t  provide the class and size of output data to be written to \n");
    (void)fprintf(rawoutstream, "\t  the dataset and may optionally specify the output-architecture,\n");
    (void)fprintf(rawoutstream, "\t  and the output-byte-order. If output-architecture is not \n");
    (void)fprintf(rawoutstream, "\t  specified the default is NATIVE. Output-byte-orders are fixed\n");
    (void)fprintf(rawoutstream, "\t  for some architectures and may be specified only if output-\n");
    (void)fprintf(rawoutstream, "\t  architecture is IEEE, UNIX or STD.\n\n");
    (void)fprintf(rawoutstream, "\t   Also, layout and other storage properties such as \n");
    (void)fprintf(rawoutstream, "\t  compression, external storage and extendible data-sets may be\n");
    (void)fprintf(rawoutstream, "\t  optionally specified.  The layout and storage properties \n");
    (void)fprintf(rawoutstream, "\t  denote how raw data is to be organized on the disk. If these \n");
    (void)fprintf(rawoutstream, "\t  options are not specified the default is Contiguous layout \n");
    (void)fprintf(rawoutstream, "\t  and storage.\n\n");
    (void)fprintf(rawoutstream, "\t  The dataset can be organized in any of the following ways:\n");
    (void)fprintf(rawoutstream, "\t  1. Contiguous.\n");
    (void)fprintf(rawoutstream, "\t  2. Chunked.\n");
    (void)fprintf(rawoutstream, "\t  3. External Storage File    (has to be contiguous)\n");
    (void)fprintf(rawoutstream, "\t  4. Extendible data sets     (has to be chunked)\n");
    (void)fprintf(rawoutstream, "\t  5. Compressed.        (has to be chunked)\n");
    (void)fprintf(rawoutstream, "\t  6. Compressed & Extendible  (has to be chunked)\n\n");
    (void)fprintf(rawoutstream, "\t  If the user wants to store raw data in a non-HDF file then \n");
    (void)fprintf(rawoutstream, "\t  the external storage file option is to be used and the name \n");
    (void)fprintf(rawoutstream, "\t  of the file is to be specified. \n\n");
    (void)fprintf(rawoutstream, "\t  If the user wants the dimensions of the data-set to be\n");
    (void)fprintf(rawoutstream, "\t  unlimited, the extendible data set option can be chosen. \n\n");
    (void)fprintf(rawoutstream, "\t  The user may also specify the type of compression and the \n");
    (void)fprintf(rawoutstream, "\t  level to which the data set must be compresses by setting \n");
    (void)fprintf(rawoutstream, "\t  the compressed option.\n\n");
    (void)fprintf(rawoutstream, "\t   SYNOPSIS:\n");
    (void)fprintf(rawoutstream, "\t  h5import -h[elp], OR\n");
    (void)fprintf(rawoutstream, "\t  h5import <infile> -c[onfig] <configfile> \
                    [<infile> -c[config] <confile2>...] -o[utfile] <outfile>\n\n");
    (void)fprintf(rawoutstream, "\t   -h[elp]:\n");
    (void)fprintf(rawoutstream, "\t           Prints this summary of usage, and exits.\n\n");
    (void)fprintf(rawoutstream, "\t   <infile(s)>:\n");
    (void)fprintf(rawoutstream, "\t           Name of the Input file(s), containing a \n");
    (void)fprintf(rawoutstream, "\t    single n-dimensional floating point or integer array \n");
    (void)fprintf(rawoutstream, "\t    in either ASCII text, native floating point(32-bit \n");
    (void)fprintf(rawoutstream, "\t    or 64-bit) or native integer(8-bit or 16-bit or \n");
    (void)fprintf(rawoutstream, "\t    32-bit or 64-bit). Data to be specified in the order\n");
    (void)fprintf(rawoutstream, "\t    of fastest changing dimensions first.\n\n");
    (void)fprintf(rawoutstream, "\t  -c[config] <configfile>:\n");
    (void)fprintf(rawoutstream, "\t    Every input file should be associated with a \n");
    (void)fprintf(rawoutstream, "\t    configuration file and this is done by the -c option.\n");
    (void)fprintf(rawoutstream, "\t    <configfile> is the name of the configuration file.\n");
    (void)fprintf(rawoutstream, "\t    (See Section \"CONFIGURATION FILE\")\n\n");
    (void)fprintf(rawoutstream, "\t   -o[utfile] <outfile>:\n");
    (void)fprintf(rawoutstream, "\t           Name of the HDF5 output file. Data from one or more \n");
    (void)fprintf(rawoutstream, "\t    input files are stored as one or more data sets in \n");
    (void)fprintf(rawoutstream, "\t    <outfile>. The output file may be an existing file or \n");
    (void)fprintf(rawoutstream, "\t    it maybe new in which case it will be created.\n\n\n");
    (void)fprintf(rawoutstream, "\t   CONFIGURATION FILE:\n");
    (void)fprintf(rawoutstream, "\t  The configuration file is an ASCII text file and must be \n");
    (void)fprintf(rawoutstream, "\t  the ddl formatted file (without data values) produced by h5dump \n");
    (void)fprintf(rawoutstream,
                  "\t  when used with the options '-o outfilename -b' of a single dataset (-d) \n");
    (void)fprintf(rawoutstream, "\t  OR organized as \"CONFIG-KEYWORD VALUE\" pairs, one pair on each \n");
    (void)fprintf(rawoutstream, "\t  line.\n\n");
    (void)fprintf(rawoutstream, "\t   The configuration file may have the following keywords each \n");
    (void)fprintf(rawoutstream, "\t   followed by an acceptable value.\n\n");
    (void)fprintf(rawoutstream, "\t  Required KEYWORDS:\n");
    (void)fprintf(rawoutstream, "\t    PATH\n");
    (void)fprintf(rawoutstream, "\t    INPUT-CLASS\n");
    (void)fprintf(rawoutstream, "\t    INPUT-SIZE\n");
    (void)fprintf(rawoutstream, "\t    INPUT-BYTE-ORDER\n");
    (void)fprintf(rawoutstream, "\t    RANK\n");
    (void)fprintf(rawoutstream, "\t    DIMENSION-SIZES\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-CLASS\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-SIZE\n\n");
    (void)fprintf(rawoutstream, "\t  Optional KEYWORDS:\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-ARCHITECTURE\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-BYTE-ORDER\n");
    (void)fprintf(rawoutstream, "\t    CHUNKED-DIMENSION-SIZES\n");
    (void)fprintf(rawoutstream, "\t    COMPRESSION-TYPE\n");
    (void)fprintf(rawoutstream, "\t    COMPRESSION-PARAM\n");
    (void)fprintf(rawoutstream, "\t    EXTERNAL-STORAGE\n");
    (void)fprintf(rawoutstream, "\t    MAXIMUM-DIMENSIONS\n\n\n");
    (void)fprintf(rawoutstream, "\t    Values for keywords:\n");
    (void)fprintf(rawoutstream, "\t    PATH:\n");
    (void)fprintf(rawoutstream, "\t      Strings separated by spaces to represent\n");
    (void)fprintf(rawoutstream, "\t      the path of the data-set. If the groups in\n");
    (void)fprintf(rawoutstream, "\t      the path do not exist, they will be created. \n");
    (void)fprintf(rawoutstream, "\t      For example,\n");
    (void)fprintf(rawoutstream, "\t        PATH grp1/grp2/dataset1\n");
    (void)fprintf(rawoutstream, "\t        PATH: keyword\n");
    (void)fprintf(rawoutstream, "\t        grp1: group under the root. If\n");
    (void)fprintf(rawoutstream, "\t              non-existent will be created.\n");
    (void)fprintf(rawoutstream, "\t        grp2: group under grp1. If \n");
    (void)fprintf(rawoutstream, "\t              non-existent will be created \n");
    (void)fprintf(rawoutstream, "\t              under grp1.\n");
    (void)fprintf(rawoutstream, "\t        dataset1: the name of the data-set \n");
    (void)fprintf(rawoutstream, "\t            to be created.\n\n");
    (void)fprintf(rawoutstream, "\t               INPUT-CLASS:\n");
    (void)fprintf(rawoutstream, "\t      String denoting the type of input data.\n");
    (void)fprintf(rawoutstream, "\t      (\"TEXTIN\", \"TEXTFP\", \"FP\", \"IN\", \n");
    (void)fprintf(rawoutstream, "\t      \"STR\", \"TEXTUIN\", \"UIN\"). \n");
    (void)fprintf(rawoutstream, "\t      INPUT-CLASS \"TEXTIN\" denotes an ASCII text \n");
    (void)fprintf(rawoutstream, "\t      file with signed integer data in ASCII form,\n");
    (void)fprintf(rawoutstream, "\t      INPUT-CLASS \"TEXTUIN\" denotes an ASCII text \n");
    (void)fprintf(rawoutstream, "\t      file with unsigned integer data in ASCII form,\n");
    (void)fprintf(rawoutstream, "\t      \"TEXTFP\" denotes an ASCII text file containing\n");
    (void)fprintf(rawoutstream, "\t      floating point data in the fixed notation\n");
    (void)fprintf(rawoutstream, "\t      (325.34),\n");
    (void)fprintf(rawoutstream, "\t      \"FP\" denotes a floating point binary file,\n");
    (void)fprintf(rawoutstream, "\t      \"IN\" denotes a signed integer binary file,\n");
    (void)fprintf(rawoutstream, "\t      \"UIN\" denotes an unsigned integer binary file,\n");
    (void)fprintf(rawoutstream, "\t       & \"STR\" denotes an ASCII text file the \n");
    (void)fprintf(rawoutstream, "\t      contents of which should be stored as an 1-D \n");
    (void)fprintf(rawoutstream, "\t      array of strings.\n");
    (void)fprintf(rawoutstream, "\t      If INPUT-CLASS is \"STR\", then RANK, \n");
    (void)fprintf(rawoutstream, "\t      DIMENSION-SIZES, OUTPUT-CLASS, OUTPUT-SIZE, \n");
    (void)fprintf(rawoutstream, "\t      OUTPUT-ARCHITECTURE and OUTPUT-BYTE-ORDER \n");
    (void)fprintf(rawoutstream, "\t      will be ignored.\n\n\n");
    (void)fprintf(rawoutstream, "\t    INPUT-SIZE:\n");
    (void)fprintf(rawoutstream, "\t      Integer denoting the size of the input data \n");
    (void)fprintf(rawoutstream, "\t      (8, 16, 32, 64). \n\n");
    (void)fprintf(rawoutstream, "\t      For floating point,\n");
    (void)fprintf(rawoutstream, "\t      INPUT-SIZE can be 32 or 64.\n");
    (void)fprintf(rawoutstream, "\t      For integers (signed and unsigned)\n");
    (void)fprintf(rawoutstream, "\t      INPUT-SIZE can be 8, 16, 32 or 64.\n\n");
    (void)fprintf(rawoutstream, "\t    RANK:\n");
    (void)fprintf(rawoutstream, "\t      Integer denoting the number of dimensions.\n\n");
    (void)fprintf(rawoutstream, "\t    DIMENSION-SIZES:\n");
    (void)fprintf(rawoutstream, "\t            Integers separated by spaces to denote the \n");
    (void)fprintf(rawoutstream, "\t      dimension sizes for the no. of dimensions \n");
    (void)fprintf(rawoutstream, "\t      determined by rank.\n\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-CLASS:\n");
    (void)fprintf(rawoutstream, "\t      String dentoting data type of the dataset to \n");
    (void)fprintf(rawoutstream, "\t      be written (\"IN\",\"FP\", \"UIN\")\n\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-SIZE:\n");
    (void)fprintf(rawoutstream, "\t      Integer denoting the size of the data in the \n");
    (void)fprintf(rawoutstream, "\t      output dataset to be written.\n");
    (void)fprintf(rawoutstream, "\t      If OUTPUT-CLASS is \"FP\", OUTPUT-SIZE can be \n");
    (void)fprintf(rawoutstream, "\t      32 or 64.\n");
    (void)fprintf(rawoutstream, "\t      If OUTPUT-CLASS is \"IN\" or \"UIN\", OUTPUT-SIZE\n");
    (void)fprintf(rawoutstream, "\t      can be 8, 16, 32 or 64.\n\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-ARCHITECTURE:\n");
    (void)fprintf(rawoutstream, "\t      STRING denoting the type of output \n");
    (void)fprintf(rawoutstream, "\t      architecture. Can accept the following values\n");
    (void)fprintf(rawoutstream, "\t      STD\n");
    (void)fprintf(rawoutstream, "\t      IEEE\n");
    (void)fprintf(rawoutstream, "\t      INTEL\n");
    (void)fprintf(rawoutstream, "\t      CRAY\n");
    (void)fprintf(rawoutstream, "\t      MIPS\n");
    (void)fprintf(rawoutstream, "\t      ALPHA\n");
    (void)fprintf(rawoutstream, "\t      NATIVE (default)\n");
    (void)fprintf(rawoutstream, "\t      UNIX\n\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-BYTE-ORDER:\n");
    (void)fprintf(rawoutstream, "\t      String denoting the output-byte-order. Ignored\n");
    (void)fprintf(rawoutstream, "\t      if the OUTPUT-ARCHITECTURE is not specified or\n");
    (void)fprintf(rawoutstream, "\t      if it is IEEE, UNIX or STD. Can accept the \n");
    (void)fprintf(rawoutstream, "\t      following values.\n");
    (void)fprintf(rawoutstream, "\t      BE (default)\n");
    (void)fprintf(rawoutstream, "\t      LE\n\n");
    (void)fprintf(rawoutstream, "\t    CHUNKED-DIMENSION-SIZES:\n");
    (void)fprintf(rawoutstream, "\t      Integers separated by spaces to denote the \n");
    (void)fprintf(rawoutstream, "\t      dimension sizes of the chunk for the no. of \n");
    (void)fprintf(rawoutstream, "\t      dimensions determined by rank. Required field\n");
    (void)fprintf(rawoutstream, "\t      to denote that the dataset will be stored with\n");
    (void)fprintf(rawoutstream, "\t      chunked storage. If this field is absent the\n");
    (void)fprintf(rawoutstream, "\t      dataset will be stored with contiguous storage.\n\n");
    (void)fprintf(rawoutstream, "\t    COMPRESSION-TYPE:\n");
    (void)fprintf(rawoutstream, "\t      String denoting the type of compression to be\n");
    (void)fprintf(rawoutstream, "\t      used with the chunked storage. Requires the\n");
    (void)fprintf(rawoutstream, "\t      CHUNKED-DIMENSION-SIZES to be specified. The only \n");
    (void)fprintf(rawoutstream, "\t      currently supported compression method is GZIP. \n");
    (void)fprintf(rawoutstream, "\t      Will accept the following value\n");
    (void)fprintf(rawoutstream, "\t      GZIP\n\n");
    (void)fprintf(rawoutstream, "\t    COMPRESSION-PARAM:\n");
    (void)fprintf(rawoutstream, "\t      Integer used to denote compression level and \n");
    (void)fprintf(rawoutstream, "\t      this option is to be always specified when \n");
    (void)fprintf(rawoutstream, "\t      the COMPRESSION-TYPE option is specified. The\n");
    (void)fprintf(rawoutstream, "\t      values are applicable only to GZIP \n");
    (void)fprintf(rawoutstream, "\t      compression.\n");
    (void)fprintf(rawoutstream, "\t      Value 1-9: The level of Compression. \n");
    (void)fprintf(rawoutstream, "\t        1 will result in the fastest \n");
    (void)fprintf(rawoutstream, "\t        compression while 9 will result in \n");
    (void)fprintf(rawoutstream, "\t        the best compression ratio. The default\n");
    (void)fprintf(rawoutstream, "\t        level of compression is 6.\n\n");
    (void)fprintf(rawoutstream, "\t    EXTERNAL-STORAGE:\n");
    (void)fprintf(rawoutstream, "\t      String to denote the name of the non-HDF5 file \n");
    (void)fprintf(rawoutstream, "\t      to store data to. Cannot be used if CHUNKED-\n");
    (void)fprintf(rawoutstream, "\t      DIMENSIONS or COMPRESSION-TYPE or EXTENDIBLE-\n");
    (void)fprintf(rawoutstream, "\t      DATASET is specified.\n");
    (void)fprintf(rawoutstream, "\t      Value <external-filename>: the name of the \n");
    (void)fprintf(rawoutstream, "\t      external file as a string to be used.\n\n");
    (void)fprintf(rawoutstream, "\t    MAXIMUM-DIMENSIONS:\n");
    (void)fprintf(rawoutstream, "\t      Integers separated by spaces to denote the \n");
    (void)fprintf(rawoutstream, "\t      maximum dimension sizes of all the \n");
    (void)fprintf(rawoutstream, "\t      dimensions determined by rank. Requires the\n");
    (void)fprintf(rawoutstream, "\t      CHUNKED-DIMENSION-SIZES to be specified. A value of \n");
    (void)fprintf(rawoutstream, "\t      -1 for any dimension implies UNLIMITED \n");
    (void)fprintf(rawoutstream, "\t      DIMENSION size for that particular dimension.\n\n");
    (void)fprintf(rawoutstream, "\t   EXAMPLES:\n");
    (void)fprintf(rawoutstream, "\t  1. Configuration File may look like:\n\n");
    (void)fprintf(rawoutstream, "\t    PATH work h5 pkamat First-set\n");
    (void)fprintf(rawoutstream, "\t    INPUT-CLASS TEXTFP\n");
    (void)fprintf(rawoutstream, "\t    RANK 3\n");
    (void)fprintf(rawoutstream, "\t    DIMENSION-SIZES 5 2 4\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-CLASS FP\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-SIZE 64\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-ARCHITECTURE IEEE\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-BYTE-ORDER LE\n");
    (void)fprintf(rawoutstream, "\t      CHUNKED-DIMENSION-SIZES 2 2 2 \n\n");
    (void)fprintf(rawoutstream, "\t  The above configuration will accept a floating point array \n");
    (void)fprintf(rawoutstream, "\t  (5 x 2 x 4)  in an ASCII file with the rank and dimension sizes \n");
    (void)fprintf(rawoutstream, "\t  specified and will save it in a chunked data-set (of pattern \n");
    (void)fprintf(rawoutstream, "\t  2 X 2 X 2) of 64-bit floating point in the little-endian order \n");
    (void)fprintf(rawoutstream, "\t  and IEEE architecture. The dataset will be stored at\n");
    (void)fprintf(rawoutstream, "\t  \"/work/h5/pkamat/First-set\"\n\n");
    (void)fprintf(rawoutstream, "\t  2. Another configuration could be:\n\n");
    (void)fprintf(rawoutstream, "\t    PATH Second-set\n");
    (void)fprintf(rawoutstream, "\t    INPUT-CLASS IN  \n");
    (void)fprintf(rawoutstream, "\t    RANK 5\n");
    (void)fprintf(rawoutstream, "\t    DIMENSION-SIZES 6 3 5 2 4\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-CLASS IN\n");
    (void)fprintf(rawoutstream, "\t    OUTPUT-SIZE 32\n");
    (void)fprintf(rawoutstream, "\t      CHUNKED-DIMENSION-SIZES 2 2 2 2 2\n");
    (void)fprintf(rawoutstream, "\t    EXTENDIBLE-DATASET 1 3 \n");
    (void)fprintf(rawoutstream, "\t    COMPRESSION-TYPE GZIP\n");
    (void)fprintf(rawoutstream, "\t    COMPRESSION-PARAM 7\n\n\n");
    (void)fprintf(rawoutstream, "\t  The above configuration will accept an integer array \n");
    (void)fprintf(rawoutstream, "\t  (6 X 3 X 5 x 2 x 4)  in a binary file with the rank and \n");
    (void)fprintf(rawoutstream, "\t  dimension sizes specified and will save it in a chunked data-set\n");
    (void)fprintf(rawoutstream, "\t  (of pattern 2 X 2 X 2 X 2 X 2) of 32-bit floating point in \n");
    (void)fprintf(rawoutstream, "\t  native format (as output-architecture is not specified). The \n");
    (void)fprintf(rawoutstream, "\t  first and the third dimension will be defined as unlimited. The \n");
    (void)fprintf(rawoutstream, "\t  data-set will be compressed using GZIP and a compression level \n");
    (void)fprintf(rawoutstream, "\t  of 7.\n");
    (void)fprintf(rawoutstream, "\t  The dataset will be stored at \"/Second-set\"\n\n");
}

void
usage(char *name)
{
    (void)fprintf(rawoutstream, "\nusage:\t%s -h[elp], OR\n", name);
    (void)fprintf(rawoutstream, "\t%s <infile> -c[onfig] <configfile> \
  [<infile> -c[config] <configfile>...] -o[utfile] <outfile> \n\n",
                  name);
}
