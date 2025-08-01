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

#include "H5private.h"
#include "h5diff.h"
#include "ph5diff.h"
#include "h5diff_common.h"
#include "h5tools.h"
#include "h5tools_utils.h"

/* Name of tool */
#define PROGRAMNAME "h5diff"

static void ph5diff_worker(int);

/*-------------------------------------------------------------------------
 * Function: main
 *
 * Purpose: ph5diff main program
 *
 * Return: An exit status of 0 means no differences were found, 1 means some
 *   differences were found.
 *
 * Comments:
 *
 * This function drives the diff process and will do a serial or parallel diff depending
 * on the value of the global variable g_Parallel (default is 0), set to 1 when the program
 * is run as "ph5diff"
 *-------------------------------------------------------------------------
 */

int
main(int argc, char *argv[])
{
    int         nID      = 0;
    const char *fname1   = NULL;
    const char *fname2   = NULL;
    const char *objname1 = NULL;
    const char *objname2 = NULL;
    diff_opt_t  opts;

    MPI_Init(&argc, (char ***)&argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &nID);
    MPI_Comm_size(MPI_COMM_WORLD, &g_nTasks);

    h5tools_setprogname(PROGRAMNAME);
    h5tools_setstatus(EXIT_SUCCESS);

    /* Initialize h5tools lib */
    h5tools_init();

    outBuffOffset = 0;
    g_Parallel    = 1;

    if (g_nTasks == 1) {
        fprintf(rawerrorstream, "Only 1 task available...doing serial diff\n");

        g_Parallel = 0;

        parse_command_line(argc, (const char *const *)argv, &fname1, &fname2, &objname1, &objname2, &opts);

        h5diff(fname1, fname2, objname1, objname2, &opts);

        print_info(&opts);
    }
    /* Parallel h5diff */
    else {

        /* Have the manager process the command-line */
        if (nID == 0) {
            parse_command_line(argc, (const char *const *)argv, &fname1, &fname2, &objname1, &objname2,
                               &opts);

            h5diff(fname1, fname2, objname1, objname2, &opts);

            MPI_Barrier(MPI_COMM_WORLD);

            print_manager_output();

            print_info(&opts);
        }
        /* All other tasks become workers and wait for assignments. */
        else {
            ph5diff_worker(nID);

            MPI_Barrier(MPI_COMM_WORLD);
        } /* end else */

    } /* end else */

    MPI_Finalize();

    return 0;
}

/*-------------------------------------------------------------------------
 * Function: ph5diff_worker
 *
 * Purpose: worker process of ph5diff
 *
 * Return: none
 *
 *-------------------------------------------------------------------------
 */
static void
ph5diff_worker(int nID)
{
    hid_t file1_id = H5I_INVALID_HID;
    hid_t file2_id = H5I_INVALID_HID;

    while (1) {
        MPI_Status Status;

        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &Status);

        /* Check for filenames */
        if (Status.MPI_TAG == MPI_TAG_PARALLEL) {
            char filenames[2][MAX_FILENAME];

            /* Retrieve filenames */
            MPI_Recv(filenames, MAX_FILENAME * 2, MPI_CHAR, 0, MPI_TAG_PARALLEL, MPI_COMM_WORLD, &Status);

            /* disable error reporting */
            H5E_BEGIN_TRY
            {
                /* Open the files */
                if ((file1_id = H5Fopen(filenames[0], H5F_ACC_RDONLY, H5P_DEFAULT)) < 0) {
                    printf("h5diff Task [%d]: <%s>: unable to open file\n", nID, filenames[0]);
                    MPI_Abort(MPI_COMM_WORLD, 0);
                }
                if ((file2_id = H5Fopen(filenames[1], H5F_ACC_RDONLY, H5P_DEFAULT)) < 0) {
                    printf("h5diff Task [%d]: <%s>: unable to open file\n", nID, filenames[1]);
                    MPI_Abort(MPI_COMM_WORLD, 0);
                }
                /* enable error reporting */
            }
            H5E_END_TRY
        }
        /* Check for work */
        else if (Status.MPI_TAG == MPI_TAG_ARGS) {
            struct diff_mpi_args args;
            struct diffs_found   diffs;
            unsigned             i;

            /* Make certain we've received the filenames and opened the files already */
            if (file1_id < 0 || file2_id < 0) {
                printf("ph5diff_worker: ERROR: work received before/without filenames\n");
                MPI_Abort(MPI_COMM_WORLD, 0);
                break;
            }

            /* Recv parameters for diff from manager task */
            MPI_Recv(&args, sizeof(args), MPI_BYTE, 0, MPI_TAG_ARGS, MPI_COMM_WORLD, &Status);

            /* Do the diff */
            diffs.nfound  = diff(file1_id, args.name1, file2_id, args.name2, &(args.opts), &(args.argdata));
            diffs.not_cmp = args.opts.not_cmp;

            if ((outBuffOffset == 0) && !overflow_file)
                /* Nothing to print. Send diffs to manager */
                MPI_Send(&diffs, sizeof(diffs), MPI_BYTE, 0, MPI_TAG_DONE, MPI_COMM_WORLD);
            else {
                /*
                 * If print buffer or overflow file have something in
                 * them, request print token.
                 */
                MPI_Send(NULL, 0, MPI_BYTE, 0, MPI_TAG_TOK_REQUEST, MPI_COMM_WORLD);

                /* Wait for print token. */
                MPI_Recv(NULL, 0, MPI_BYTE, 0, MPI_TAG_PRINT_TOK, MPI_COMM_WORLD, &Status);

                if (outBuffOffset > 0) {
                    /* When get token, send all of our output to the manager task and then return the token */
                    for (i = 0; i < outBuffOffset; i += PRINT_DATA_MAX_SIZE)
                        MPI_Send(outBuff + i, PRINT_DATA_MAX_SIZE, MPI_CHAR, 0, MPI_TAG_PRINT_DATA,
                                 MPI_COMM_WORLD);
                }

                /* An overflow file exists, so we send its output to
                 * the manager too and then delete it.
                 */
                if (overflow_file) {
                    char out_data[PRINT_DATA_MAX_SIZE + 1];
                    int  tmp;

                    memset(out_data, 0, PRINT_DATA_MAX_SIZE + 1);
                    i = 0;

                    rewind(overflow_file);
                    while ((tmp = getc(overflow_file)) != EOF) {
                        *(out_data + i++) = (char)tmp;
                        if (i == PRINT_DATA_MAX_SIZE) {
                            MPI_Send(out_data, PRINT_DATA_MAX_SIZE, MPI_CHAR, 0, MPI_TAG_PRINT_DATA,
                                     MPI_COMM_WORLD);
                            i = 0;
                            memset(out_data, 0, PRINT_DATA_MAX_SIZE + 1);
                        }
                    }

                    if (i > 0)
                        MPI_Send(out_data, PRINT_DATA_MAX_SIZE, MPI_CHAR, 0, MPI_TAG_PRINT_DATA,
                                 MPI_COMM_WORLD);

                    fclose(overflow_file);
                    overflow_file = NULL;
                }

                fflush(stdout);
                memset(outBuff, 0, OUTBUFF_SIZE);
                outBuffOffset = 0;

                MPI_Send(&diffs, sizeof(diffs), MPI_BYTE, 0, MPI_TAG_TOK_RETURN, MPI_COMM_WORLD);
            }
        }
        /* Check for leaving */
        else if (Status.MPI_TAG == MPI_TAG_END) {
            MPI_Recv(NULL, 0, MPI_BYTE, 0, MPI_TAG_END, MPI_COMM_WORLD, &Status);
            break;
        }
        else {
            printf("ph5diff_worker: ERROR: invalid tag (%d) received\n", Status.MPI_TAG);
            MPI_Abort(MPI_COMM_WORLD, 0);
            break;
        }
    }

    H5Fclose(file1_id);
    H5Fclose(file2_id);

    return;
}

/*-------------------------------------------------------------------------
 * Function: print_manager_output
 *
 * Purpose: special function that prints any output accumulated by the
 *      manager task.
 *
 * Return: none
 *
 *-------------------------------------------------------------------------
 */
void
print_manager_output(void)
{
    /* If there was something we buffered, let's print it now */
    if (g_Parallel) {
        if (outBuffOffset > 0)
            printf("%s", outBuff);

        if (overflow_file) {
            int tmp;
            rewind(overflow_file);
            while ((tmp = getc(overflow_file)) != EOF)
                putchar(tmp);
            fclose(overflow_file);
            overflow_file = NULL;
        }

        fflush(stdout);
        memset(outBuff, 0, OUTBUFF_SIZE);
        outBuffOffset = 0;
    }
    else if (outBuffOffset > 0) {
        fprintf(rawerrorstream, "h5diff error: outBuffOffset > 0, but we're not in parallel!\n");
    }
}

/*-------------------------------------------------------------------------
 * Function: h5diff_exit
 *
 * Purpose: dismiss phdiff worker processes and exit
 *
 * Return: none
 *
 *-------------------------------------------------------------------------
 */
void
h5diff_exit(int status)
{
    /* if in parallel mode, dismiss workers, close down MPI, then exit */
    if (g_Parallel) {
        if (g_nTasks > 1) {
            phdiff_dismiss_workers();
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();

    status =
        EXIT_SUCCESS; /* Reset exit status, since some mpiexec commands generate output on failure status */

    h5tools_close();

    /* Always exit(0), since MPI implementations do weird stuff when they
     *  receive a non-zero exit value. - QAK
     */
    exit(status);
}
