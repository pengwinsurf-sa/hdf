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

#include "H5private.h" /* Generic Functions      */
#include "h5tools.h"
#include "h5tools_utils.h"
#include "h5tools_ref.h"
#include "h5trav.h"
#include "hdf5.h"

/* Name of tool */
#define PROGRAMNAME "h5stat"

/* Parameters to control statistics gathered */

/* Default threshold for small groups/datasets/attributes */
#define DEF_SIZE_SMALL_GROUPS 10
#define DEF_SIZE_SMALL_DSETS  10
#define DEF_SIZE_SMALL_ATTRS  10

#define SIZE_SMALL_SECTS 10

#define H5_NFILTERS_IMPL                                                                                     \
    8 /* Number of currently implemented filters + one to                                                    \
         accommodate for user-define filters + one                                                           \
         to accommodate datasets without any filters */

/* File space management strategies: see H5Fpublic.h for declarations */
static const char *FS_STRATEGY_NAME[] = {"H5F_FSPACE_STRATEGY_FSM_AGGR",
                                         "H5F_FSPACE_STRATEGY_PAGE",
                                         "H5F_FSPACE_STRATEGY_AGGR",
                                         "H5F_FSPACE_STRATEGY_NONE",
                                         "unknown",
                                         NULL};

/* Datatype statistics for datasets */
typedef struct dtype_info_t {
    hid_t         tid;   /* ID of datatype */
    unsigned long count; /* Number of types found */
    unsigned long named; /* Number of types that are named */
} dtype_info_t;

typedef struct ohdr_info_t {
    hsize_t total_size; /* Total size of object headers */
    hsize_t free_size;  /* Total free space in object headers */
} ohdr_info_t;

/* Info to pass to the iteration functions */
typedef struct iter_t {
    hid_t         fid;         /* File ID */
    hsize_t       filesize;    /* Size of the file */
    unsigned long uniq_groups; /* Number of unique groups */
    unsigned long uniq_dsets;  /* Number of unique datasets */
    unsigned long uniq_dtypes; /* Number of unique named datatypes */
    unsigned long uniq_links;  /* Number of unique links */
    unsigned long uniq_others; /* Number of other unique objects */

    unsigned long  max_links;        /* Maximum # of links to an object */
    hsize_t        max_fanout;       /* Maximum fanout from a group */
    unsigned long *num_small_groups; /* Size of small groups tracked */
    unsigned       group_nbins;      /* Number of bins for group counts */
    unsigned long *group_bins;       /* Pointer to array of bins for group counts */
    ohdr_info_t    group_ohdr_info;  /* Object header information for groups */

    hsize_t        max_attrs;       /* Maximum attributes from a group */
    unsigned long *num_small_attrs; /* Size of small attributes tracked */
    unsigned       attr_nbins;      /* Number of bins for attribute counts */
    unsigned long *attr_bins;       /* Pointer to array of bins for attribute counts */

    unsigned       max_dset_rank;                   /* Maximum rank of dataset */
    unsigned long  dset_rank_count[H5S_MAX_RANK];   /* Number of datasets of each rank */
    hsize_t        max_dset_dims;                   /* Maximum dimension size of dataset */
    unsigned long *small_dset_dims;                 /* Size of dimensions of small datasets tracked */
    unsigned long  dset_layouts[H5D_NLAYOUTS];      /* Type of storage for each dataset */
    unsigned long  dset_comptype[H5_NFILTERS_IMPL]; /* Number of currently implemented filters */
    unsigned long  dset_ntypes;                     /* Number of diff. dataset datatypes found */
    dtype_info_t  *dset_type_info;                  /* Pointer to dataset datatype information found */
    unsigned       dset_dim_nbins;                  /* Number of bins for dataset dimensions */
    unsigned long *dset_dim_bins;                   /* Pointer to array of bins for dataset dimensions */
    ohdr_info_t    dset_ohdr_info;                  /* Object header information for datasets */
    hsize_t        dset_storage_size;               /* Size of raw data for datasets */
    hsize_t        dset_external_storage_size;      /* Size of raw data for datasets with external storage */
    ohdr_info_t    dtype_ohdr_info;                 /* Object header information for datatypes */
    hsize_t        groups_btree_storage_size;       /* btree size for group */
    hsize_t        groups_heap_storage_size;        /* heap size for group */
    hsize_t        attrs_btree_storage_size;        /* btree size for attributes (1.8) */
    hsize_t        attrs_heap_storage_size;         /* fractal heap size for attributes (1.8) */
    hsize_t        SM_hdr_storage_size;             /* header size for SOHM table (1.8) */
    hsize_t        SM_index_storage_size;           /* index (btree & list) size for SOHM table (1.8) */
    hsize_t        SM_heap_storage_size;            /* fractal heap size for SOHM table (1.8) */
    hsize_t        super_size;                      /* superblock size */
    hsize_t        super_ext_size;                  /* superblock extension size */
    hsize_t        ublk_size;                       /* user block size (if exists) */
    H5F_fspace_strategy_t fs_strategy;              /* File space management strategy */
    bool                  fs_persist;               /* Free-space persist or not */
    hsize_t               fs_threshold;             /* Free-space section threshold */
    hsize_t               fsp_size;                 /* File space page size */
    hsize_t               free_space;               /* Amount of freespace in the file */
    hsize_t               free_hdr;                 /* Size of free space manager metadata in the file */
    unsigned long         num_small_sects[SIZE_SMALL_SECTS]; /* Size of small free-space sections */
    unsigned              sect_nbins;                        /* Number of bins for free-space section sizes */
    unsigned long        *sect_bins; /* Pointer to array of bins for free-space section sizes */
    hsize_t               datasets_index_storage_size; /* meta size for chunked dataset's indexing type */
    hsize_t               datasets_heap_storage_size;  /* heap size for dataset with external storage */
    unsigned long         nexternal;                   /* Number of external files for a dataset */
    int                   local;                       /* Flag to indicate iteration over the object*/
} iter_t;

size_t page_cache = 0;

static bool use_custom_vol_g = false;
static bool use_custom_vfd_g = false;

static h5tools_vol_info_t vol_info_g = {0};
static h5tools_vfd_info_t vfd_info_g = {0};

#ifdef H5_HAVE_ROS3_VFD
static H5FD_ros3_fapl_ext_t *ros3_fa_g = NULL;
#endif
#ifdef H5_HAVE_LIBHDFS
static H5FD_hdfs_fapl_t *hdfs_fa_g = NULL;
#endif

static int display_all = true;

/* Enable the printing of selected statistics */
static int display_file            = false; /* display file information */
static int display_group           = false; /* display groups information */
static int display_dset            = false; /* display datasets information */
static int display_dset_dtype_meta = false; /* display datasets' datatype information */
static int display_attr            = false; /* display attributes information */
static int display_free_sections   = false; /* display free space information */
static int display_summary         = false; /* display summary of file space information */

static int display_file_metadata  = false; /* display file space info for file's metadata */
static int display_group_metadata = false; /* display file space info for groups' metadata */
static int display_dset_metadata  = false; /* display file space info for datasets' metadata */

static int display_object = false; /* not implemented yet */

/* Initialize threshold for small groups/datasets/attributes */
static int sgroups_threshold = DEF_SIZE_SMALL_GROUPS;
static int sdsets_threshold  = DEF_SIZE_SMALL_DSETS;
static int sattrs_threshold  = DEF_SIZE_SMALL_ATTRS;

/* a structure for handling the order command-line parameters come in */
struct handler_t {
    size_t obj_count;
    char **obj;
};

static const char *s_opts = "a:dfghl:m:sw:ADE*FGH:K:O:STV";
/* e.g. "filemetadata" has to precede "file"; "groupmetadata" has to precede "group" etc. */
static struct h5_long_options l_opts[] = {{"help", no_arg, 'h'},
                                          {"filemetadata", no_arg, 'F'},
                                          {"groupmetadata", no_arg, 'G'},
                                          {"links", require_arg, 'l'},
                                          {"dsetmetadata", no_arg, 'D'},
                                          {"dims", require_arg, 'm'},
                                          {"dtypemetadata", no_arg, 'T'},
                                          {"object", require_arg, 'O'},
                                          {"version", no_arg, 'V'},
                                          {"attribute", no_arg, 'A'},
                                          {"enable-error-stack", optional_arg, 'E'},
                                          {"numattrs", require_arg, 'a'},
                                          {"freespace", no_arg, 's'},
                                          {"summary", no_arg, 'S'},
                                          {"page-buffer-size", require_arg, 'K'},
                                          {"s3-cred", require_arg, 'w'},
                                          {"hdfs-attrs", require_arg, 'H'},
                                          {"endpoint-url", require_arg, 'y'},
                                          {"vol-value", require_arg, '1'},
                                          {"vol-name", require_arg, '2'},
                                          {"vol-info", require_arg, '3'},
                                          {"vfd-value", require_arg, '4'},
                                          {"vfd-name", require_arg, '5'},
                                          {"vfd-info", require_arg, '6'},
                                          {NULL, 0, '\0'}};

/*-------------------------------------------------------------------------
 * Function:    leave
 *
 * Purpose:     Shutdown HDF5 and call exit()
 *
 * Return:      Does not return
 *-------------------------------------------------------------------------
 */
static void
leave(int ret)
{
    h5tools_close();

    exit(ret);
}

/*-------------------------------------------------------------------------
 * Function:    usage
 *
 * Purpose:     Print the usage message about stat
 *
 * Return:      void
 *-------------------------------------------------------------------------
 */
static void
usage(const char *prog)
{
    FLUSHSTREAM(rawoutstream);
    PRINTSTREAM(rawoutstream, "usage: %s [OPTIONS] file\n", prog);
    PRINTVALSTREAM(rawoutstream, "  OPTIONS\n");
    PRINTVALSTREAM(rawoutstream, "     -h,   --help         Print a usage message and exit\n");
    PRINTVALSTREAM(rawoutstream, "     -V,   --version      Print version number and exit\n");
    PRINTVALSTREAM(rawoutstream, "--------------- Error Options ---------------\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --enable-error-stack Prints messages from the HDF5 error stack as they occur.\n");
    PRINTVALSTREAM(rawoutstream,
                   "                          Optional value 2 also prints file open errors.\n");
    PRINTVALSTREAM(rawoutstream, "                          Default setting disables any error reporting.\n");
    PRINTVALSTREAM(rawoutstream, "--------------- File Options ---------------\n");
    PRINTVALSTREAM(rawoutstream, "     -f, --file            Print file information\n");
    PRINTVALSTREAM(rawoutstream,
                   "     -F, --filemetadata    Print file space information for file's metadata\n");
    PRINTVALSTREAM(rawoutstream, "     -s, --freespace       Print free space information\n");
    PRINTVALSTREAM(rawoutstream, "     -S, --summary         Print summary of file space information\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --page-buffer-size=N Set the page buffer cache size, N=non-negative integers\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --endpoint-url=P     Supply S3 endpoint url information to \"ros3\" vfd.\n");
    PRINTVALSTREAM(rawoutstream, "                          P is the AWS service endpoint.\n");
    PRINTVALSTREAM(rawoutstream, "                          Has no effect if filedriver is not \"ros3\".\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --s3-cred=<cred>     Supply S3 authentication information to \"ros3\" vfd.\n");
    PRINTVALSTREAM(rawoutstream,
                   "                          <cred> :: \"(<aws-region>,<access-id>,<access-key>)\"\n");
    PRINTVALSTREAM(
        rawoutstream,
        "                          <cred> :: \"(<aws-region>,<access-id>,<access-key>,<session-token>)\"\n");
    PRINTVALSTREAM(rawoutstream, "                          If absent, <cred> -> \"(,,)\" or <cred> -> "
                                 "\"(,,,)\", no authentication.\n");
    PRINTVALSTREAM(rawoutstream, "                          Has no effect if filedriver is not \"ros3\".\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --hdfs-attrs=<attrs> Supply configuration information for HDFS file access.\n");
    PRINTVALSTREAM(rawoutstream, "                          For use with \"--filedriver=hdfs\"\n");
    PRINTVALSTREAM(rawoutstream, "                          <attrs> :: (<namenode name>,<namenode port>,\n");
    PRINTVALSTREAM(rawoutstream, "                                      <kerberos cache path>,<username>,\n");
    PRINTVALSTREAM(rawoutstream, "                                      <buffer size>)\n");
    PRINTVALSTREAM(rawoutstream,
                   "                          Any absent attribute will use a default value.\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --vol-value          Value (ID) of the VOL connector to use for opening the\n");
    PRINTVALSTREAM(rawoutstream, "                          HDF5 file specified\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --vol-name           Name of the VOL connector to use for opening the\n");
    PRINTVALSTREAM(rawoutstream, "                          HDF5 file specified\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --vol-info           VOL-specific info to pass to the VOL connector used for\n");
    PRINTVALSTREAM(rawoutstream, "                          opening the HDF5 file specified\n");
    PRINTVALSTREAM(
        rawoutstream,
        "                          If none of the above options are used to specify a VOL, then\n");
    PRINTVALSTREAM(
        rawoutstream,
        "                          the VOL named by HDF5_VOL_CONNECTOR (or the native VOL connector,\n");
    PRINTVALSTREAM(rawoutstream,
                   "                          if that environment variable is unset) will be used\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --vfd-value          Value (ID) of the VFL driver to use for opening the\n");
    PRINTVALSTREAM(rawoutstream, "                          HDF5 file specified\n");
    PRINTVALSTREAM(rawoutstream, "     --vfd-name           Name of the VFL driver to use for opening the\n");
    PRINTVALSTREAM(rawoutstream, "                          HDF5 file specified\n");
    PRINTVALSTREAM(rawoutstream,
                   "     --vfd-info           VFD-specific info to pass to the VFL driver used for\n");
    PRINTVALSTREAM(rawoutstream, "                          opening the HDF5 file specified\n");
    PRINTVALSTREAM(rawoutstream, "--------------- Object Options ---------------\n");
    PRINTVALSTREAM(rawoutstream, "     -g, --group           Print group information\n");
    PRINTVALSTREAM(rawoutstream,
                   "     -l N, --links=N       Set the threshold for the # of links when printing\n");
    PRINTVALSTREAM(rawoutstream,
                   "                           information for small groups.  N is an integer greater\n");
    PRINTVALSTREAM(rawoutstream, "                           than 0.  The default threshold is 10.\n");
    PRINTVALSTREAM(rawoutstream,
                   "     -G, --groupmetadata   Print file space information for groups' metadata\n");
    PRINTVALSTREAM(rawoutstream, "     -d, --dset            Print dataset information\n");
    PRINTVALSTREAM(rawoutstream,
                   "     -m N, --dims=N        Set the threshold for the dimension sizes when printing\n");
    PRINTVALSTREAM(rawoutstream,
                   "                           information for small datasets.  N is an integer greater\n");
    PRINTVALSTREAM(rawoutstream, "                           than 0.  The default threshold is 10.\n");
    PRINTVALSTREAM(rawoutstream,
                   "     -D, --dsetmetadata    Print file space information for datasets' metadata\n");
    PRINTVALSTREAM(rawoutstream, "     -T, --dtypemetadata   Print datasets' datatype information\n");
    PRINTVALSTREAM(rawoutstream, "     -A, --attribute       Print attribute information\n");
    PRINTVALSTREAM(rawoutstream,
                   "     -a N, --numattrs=N    Set the threshold for the # of attributes when printing\n");
    PRINTVALSTREAM(
        rawoutstream,
        "                           information for small # of attributes.  N is an integer greater\n");
    PRINTVALSTREAM(rawoutstream, "                           than 0.  The default threshold is 10.\n");
}

/*-------------------------------------------------------------------------
 * Function: ceil_log10
 *
 * Purpose: Compute the ceiling of log_10(x)
 *
 * Return: >0 on success, 0 on failure
 *
 *-------------------------------------------------------------------------
 */
H5_ATTR_CONST static unsigned
ceil_log10(unsigned long x)
{
    unsigned long pow10 = 1;
    unsigned      ret   = 0;

    while (x >= pow10) {
        pow10 *= 10;
        ret++;
    }

    return ret;
} /* ceil_log10() */

/*-------------------------------------------------------------------------
 * Function: attribute_stats
 *
 * Purpose: Gather statistics about attributes on an object
 *
 * Return:  Success: 0
 *
 *          Failure: -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
attribute_stats(iter_t *iter, const H5O_info2_t *oi, const H5O_native_info_t *native_oi)
{
    unsigned bin; /* "bin" the number of objects falls in */

    /* Update dataset & attribute metadata info */
    iter->attrs_btree_storage_size += native_oi->meta_size.attr.index_size;
    iter->attrs_heap_storage_size += native_oi->meta_size.attr.heap_size;

    /* Update small # of attribute count & limits */
    if (oi->num_attrs <= (hsize_t)sattrs_threshold)
        (iter->num_small_attrs[(size_t)oi->num_attrs])++;
    if (oi->num_attrs > iter->max_attrs)
        iter->max_attrs = oi->num_attrs;

    /* Add attribute count to proper bin */
    bin = ceil_log10((unsigned long)oi->num_attrs);
    if ((bin + 1) > iter->attr_nbins) {
        iter->attr_bins = (unsigned long *)realloc(iter->attr_bins, (bin + 1) * sizeof(unsigned long));
        assert(iter->attr_bins);

        /* Initialize counts for intermediate bins */
        while (iter->attr_nbins < bin)
            iter->attr_bins[iter->attr_nbins++] = 0;
        iter->attr_nbins++;

        /* Initialize count for new bin */
        iter->attr_bins[bin] = 1;
    }
    else
        (iter->attr_bins[bin])++;

    return 0;
} /* end attribute_stats() */

/*-------------------------------------------------------------------------
 * Function: group_stats
 *
 * Purpose: Gather statistics about the group
 *
 * Return:  Success: 0
 *          Failure: -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
group_stats(iter_t *iter, const char *name, const H5O_info2_t *oi, const H5O_native_info_t *native_oi)
{
    H5G_info_t ginfo; /* Group information */
    unsigned   bin;   /* "bin" the number of objects falls in */
    herr_t     ret_value = SUCCEED;

    /* Gather statistics about this type of object */
    iter->uniq_groups++;

    /* Get object header information */
    iter->group_ohdr_info.total_size += native_oi->hdr.space.total;
    iter->group_ohdr_info.free_size += native_oi->hdr.space.free;

    /* Get group information */
    if ((ret_value = H5Gget_info_by_name(iter->fid, name, &ginfo, H5P_DEFAULT)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Gget_info_by_name() failed");

    /* Update link stats */
    /* Collect statistics for small groups */
    if (ginfo.nlinks < (hsize_t)sgroups_threshold)
        (iter->num_small_groups[(size_t)ginfo.nlinks])++;
    /* Determine maximum link count */
    if (ginfo.nlinks > iter->max_fanout)
        iter->max_fanout = ginfo.nlinks;

    /* Add group count to proper bin */
    bin = ceil_log10((unsigned long)ginfo.nlinks);
    if ((bin + 1) > iter->group_nbins) {
        /* Allocate more storage for info about dataset's datatype */
        if ((iter->group_bins =
                 (unsigned long *)realloc(iter->group_bins, (bin + 1) * sizeof(unsigned long))) == NULL)
            H5TOOLS_GOTO_ERROR(FAIL, "H5Drealloc() failed");

        /* Initialize counts for intermediate bins */
        while (iter->group_nbins < bin)
            iter->group_bins[iter->group_nbins++] = 0;
        iter->group_nbins++;

        /* Initialize count for new bin */
        iter->group_bins[bin] = 1;
    }
    else
        (iter->group_bins[bin])++;

    /* Update group metadata info */
    iter->groups_btree_storage_size += native_oi->meta_size.obj.index_size;
    iter->groups_heap_storage_size += native_oi->meta_size.obj.heap_size;

    /* Update attribute metadata info */
    if ((ret_value = attribute_stats(iter, oi, native_oi)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "attribute_stats failed");

done:
    return ret_value;
} /* end group_stats() */

/*-------------------------------------------------------------------------
 * Function: dataset_stats
 *
 * Purpose: Gather statistics about the dataset
 *
 * Return:  Success: 0
 *          Failure: -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
dataset_stats(iter_t *iter, const char *name, const H5O_info2_t *oi, const H5O_native_info_t *native_oi)
{
    unsigned     bin;                /* "bin" the number of objects falls in */
    hid_t        did;                /* Dataset ID */
    hid_t        sid;                /* Dataspace ID */
    hid_t        tid;                /* Datatype ID */
    hid_t        dcpl;               /* Dataset creation property list ID */
    hsize_t      dims[H5S_MAX_RANK]; /* Dimensions of dataset */
    H5D_layout_t lout;               /* Layout of dataset */
    unsigned     type_found;         /* Whether the dataset's datatype was */
                                     /* already found */
    int          ndims;              /* Number of dimensions of dataset */
    hsize_t      storage;            /* Size of dataset storage */
    unsigned     u;                  /* Local index variable */
    int          num_ext;            /* Number of external files for a dataset */
    int          nfltr;              /* Number of filters for a dataset */
    H5Z_filter_t fltr;               /* Filter identifier */
    herr_t       ret_value = SUCCEED;

    /* Gather statistics about this type of object */
    iter->uniq_dsets++;

    /* Get object header information */
    iter->dset_ohdr_info.total_size += native_oi->hdr.space.total;
    iter->dset_ohdr_info.free_size += native_oi->hdr.space.free;

    if ((did = H5Dopen2(iter->fid, name, H5P_DEFAULT)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Dopen() failed");

    /* Update dataset metadata info */
    iter->datasets_index_storage_size += native_oi->meta_size.obj.index_size;
    iter->datasets_heap_storage_size += native_oi->meta_size.obj.heap_size;

    /* Update attribute metadata info */
    if ((ret_value = attribute_stats(iter, oi, native_oi)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "attribute_stats() failed");

    /* Get storage info */
    /* Failure 0 indistinguishable from no-data-stored 0 */
    storage = H5Dget_storage_size(did);

    /* Gather layout statistics */
    if ((dcpl = H5Dget_create_plist(did)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Dget_create_plist() failed");

    if ((lout = H5Pget_layout(dcpl)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Pget_layout() failed");

    /* Object header's total size for H5D_COMPACT layout includes raw data size */
    /* "storage" also includes H5D_COMPACT raw data size */
    if (lout == H5D_COMPACT)
        iter->dset_ohdr_info.total_size -= storage;

    /* Track the layout type for dataset */
    (iter->dset_layouts[lout])++;

    /* Get the number of external files for the dataset */
    if ((num_ext = H5Pget_external_count(dcpl)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Pget_external_count() failed");

    /* Accumulate raw data size accordingly */
    if (num_ext) {
        iter->nexternal += (unsigned long)num_ext;
        iter->dset_external_storage_size += (unsigned long)storage;
    }
    else
        iter->dset_storage_size += storage;

    /* Gather dataspace statistics */
    if ((sid = H5Dget_space(did)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Sget_space() failed");

    if ((ndims = H5Sget_simple_extent_dims(sid, dims, NULL)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Sget_simple_extent_dims() failed");

    /* Check for larger rank of dataset */
    if ((unsigned)ndims > iter->max_dset_rank)
        iter->max_dset_rank = (unsigned)ndims;

    /* Track the number of datasets with each rank */
    (iter->dset_rank_count[ndims])++;

    /* Only gather dim size statistics on 1-D datasets */
    if (ndims == 1) {
        /* Determine maximum dimension size */
        if (dims[0] > iter->max_dset_dims)
            iter->max_dset_dims = dims[0];
        /* Collect statistics for small datasets */
        if (dims[0] < (hsize_t)sdsets_threshold)
            (iter->small_dset_dims[(size_t)dims[0]])++;

        /* Add dim count to proper bin */
        bin = ceil_log10((unsigned long)dims[0]);
        if ((bin + 1) > iter->dset_dim_nbins) {
            /* Allocate more storage for info about dataset's datatype */
            if ((iter->dset_dim_bins = (unsigned long *)realloc(iter->dset_dim_bins,
                                                                (bin + 1) * sizeof(unsigned long))) == NULL)
                H5TOOLS_GOTO_ERROR(FAIL, "H5Drealloc() failed");

            /* Initialize counts for intermediate bins */
            while (iter->dset_dim_nbins < bin)
                iter->dset_dim_bins[iter->dset_dim_nbins++] = 0;
            iter->dset_dim_nbins++;

            /* Initialize count for this bin */
            iter->dset_dim_bins[bin] = 1;
        }
        else
            (iter->dset_dim_bins[bin])++;
    }

    if (H5Sclose(sid) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Sclose() failed");

    /* Gather datatype statistics */
    if ((tid = H5Dget_type(did)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Dget_type() failed");

    type_found = false;
    for (u = 0; u < iter->dset_ntypes; u++)
        if (H5Tequal(iter->dset_type_info[u].tid, tid) > 0) {
            type_found = true;
            break;
        }

    if (type_found)
        (iter->dset_type_info[u].count)++;
    else {
        unsigned curr_ntype = (unsigned)iter->dset_ntypes;

        /* Increment # of datatypes seen for datasets */
        iter->dset_ntypes++;

        /* Allocate more storage for info about dataset's datatype */
        if ((iter->dset_type_info = (dtype_info_t *)realloc(
                 iter->dset_type_info, iter->dset_ntypes * sizeof(dtype_info_t))) == NULL)
            H5TOOLS_GOTO_ERROR(FAIL, "H5Drealloc() failed");

        /* Initialize information about datatype */
        if ((iter->dset_type_info[curr_ntype].tid = H5Tcopy(tid)) < 0)
            H5TOOLS_GOTO_ERROR(FAIL, "H5Tcopy() failed");
        iter->dset_type_info[curr_ntype].count = 1;
        iter->dset_type_info[curr_ntype].named = 0;

        /* Set index for later */
        u = curr_ntype;
    }

    /* Check if the datatype is a named datatype */
    if (H5Tcommitted(tid) > 0)
        (iter->dset_type_info[u].named)++;

    if (H5Tclose(tid) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Tclose() failed");

    /* Track different filters */
    if ((nfltr = H5Pget_nfilters(dcpl)) >= 0) {
        if (nfltr == 0)
            iter->dset_comptype[0]++;
        for (u = 0; u < (unsigned)nfltr; u++) {
            fltr = H5Pget_filter2(dcpl, u, 0, 0, 0, 0, 0, NULL);
            if (fltr >= 0) {
                if (fltr < (H5_NFILTERS_IMPL - 1))
                    iter->dset_comptype[fltr]++;
                else
                    iter->dset_comptype[H5_NFILTERS_IMPL - 1]++; /*other filters*/
            }                                                    /* end if */
        }                                                        /* end for */
    }                                                            /* endif nfltr */

    if (H5Pclose(dcpl) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Pclose() failed");

    if (H5Dclose(did) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "H5Dclose() failed");

done:
    return ret_value;
} /* end dataset_stats() */

/*-------------------------------------------------------------------------
 * Function: datatype_stats
 *
 * Purpose: Gather statistics about the datatype
 *
 * Return:  Success: 0
 *          Failure: -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
datatype_stats(iter_t *iter, const H5O_info2_t *oi, const H5O_native_info_t *native_oi)
{
    herr_t ret_value = SUCCEED;

    /* Gather statistics about this type of object */
    iter->uniq_dtypes++;

    /* Get object header information */
    iter->dtype_ohdr_info.total_size += native_oi->hdr.space.total;
    iter->dtype_ohdr_info.free_size += native_oi->hdr.space.free;

    /* Update attribute metadata info */
    if ((ret_value = attribute_stats(iter, oi, native_oi)) < 0)
        H5TOOLS_GOTO_ERROR(FAIL, "attribute_stats() failed");
done:
    return ret_value;
} /* end datatype_stats() */

/*-------------------------------------------------------------------------
 * Function: obj_stats
 *
 * Purpose: Gather statistics about an object
 *
 * Return: Success: 0
 *       Failure: -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
obj_stats(const char *path, const H5O_info2_t *oi, const char *already_visited, void *_iter)
{
    H5O_native_info_t native_info;
    iter_t           *iter      = (iter_t *)_iter;
    herr_t            ret_value = SUCCEED;

    /* If the object has already been seen then just return */
    if (NULL == already_visited) {
        /* Retrieve the native info for the object */
        if (H5Oget_native_info_by_name(iter->fid, path, &native_info, H5O_NATIVE_INFO_ALL, H5P_DEFAULT) < 0)
            H5TOOLS_GOTO_ERROR(FAIL, "H5Oget_native_info_by_name failed");

        /* Gather some general statistics about the object */
        if (oi->rc > iter->max_links)
            iter->max_links = oi->rc;

        switch (oi->type) {
            case H5O_TYPE_GROUP:
                if (group_stats(iter, path, oi, &native_info) < 0)
                    H5TOOLS_GOTO_ERROR(FAIL, "group_stats failed");
                break;

            case H5O_TYPE_DATASET:
                if (dataset_stats(iter, path, oi, &native_info) < 0)
                    H5TOOLS_GOTO_ERROR(FAIL, "dataset_stats failed");
                break;

            case H5O_TYPE_NAMED_DATATYPE:
                if (datatype_stats(iter, oi, &native_info) < 0)
                    H5TOOLS_GOTO_ERROR(FAIL, "datatype_stats failed");
                break;

            case H5O_TYPE_MAP:
            case H5O_TYPE_UNKNOWN:
            case H5O_TYPE_NTYPES:
            default:
                /* Gather statistics about this type of object */
                iter->uniq_others++;
                break;
        }
    }

done:
    return ret_value;
} /* end obj_stats() */

/*-------------------------------------------------------------------------
 * Function: lnk_stats
 *
 * Purpose: Gather statistics about a link
 *
 * Return:  Success: 0
 *          Failure: -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
lnk_stats(const char H5_ATTR_UNUSED *path, const H5L_info2_t *li, void *_iter)
{
    iter_t *iter = (iter_t *)_iter;

    switch (li->type) {
        case H5L_TYPE_SOFT:
        case H5L_TYPE_EXTERNAL:
            /* Gather statistics about links and UD links */
            iter->uniq_links++;
            break;

        case H5L_TYPE_HARD:
        case H5L_TYPE_MAX:
        case H5L_TYPE_ERROR:
        default:
            /* Gather statistics about this type of object */
            iter->uniq_others++;
            break;
    }

    return 0;
} /* end lnk_stats() */

/*-------------------------------------------------------------------------
 * Function: freespace_stats
 *
 * Purpose: Gather statistics for free space sections in the file
 *
 * Return: Success: 0
 *       Failure: -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
freespace_stats(hid_t fid, iter_t *iter)
{
    H5F_sect_info_t *sect_info = NULL; /* Free space sections */
    ssize_t          nsects;           /* Number of free space sections */
    size_t           u;                /* Local index variable */

    /* Query section information */
    if ((nsects = H5Fget_free_sections(fid, H5FD_MEM_DEFAULT, 0, NULL)) < 0)
        return (FAIL);
    else if (nsects) {
        if (NULL == (sect_info = (H5F_sect_info_t *)calloc((size_t)nsects, sizeof(H5F_sect_info_t))))
            return (FAIL);
        nsects = H5Fget_free_sections(fid, H5FD_MEM_DEFAULT, (size_t)nsects, sect_info);
        assert(nsects);
    }

    for (u = 0; u < (size_t)nsects; u++) {
        unsigned bin; /* "bin" the number of objects falls in */

        if (sect_info[u].size < SIZE_SMALL_SECTS)
            (iter->num_small_sects[(size_t)sect_info[u].size])++;

        /* Add section size to proper bin */
        bin = ceil_log10((unsigned long)sect_info[u].size);
        if (bin >= iter->sect_nbins) {
            /* Allocate more storage for section info */
            iter->sect_bins = (unsigned long *)realloc(iter->sect_bins, (bin + 1) * sizeof(unsigned long));
            assert(iter->sect_bins);

            /* Initialize counts for intermediate bins */
            while (iter->sect_nbins < bin)
                iter->sect_bins[iter->sect_nbins++] = 0;
            iter->sect_nbins++;

            /* Initialize count for this bin */
            iter->sect_bins[bin] = 1;
        }
        else
            (iter->sect_bins[bin])++;
    } /* end for */

    if (sect_info)
        free(sect_info);

    return 0;
} /* end freespace_stats() */

/*-------------------------------------------------------------------------
 * Function: hand_free
 *
 * Purpose: Free handler structure
 *
 * Return:      Nothing
 *-------------------------------------------------------------------------
 */
static void
hand_free(struct handler_t *hand)
{
    if (hand) {
        unsigned u;

        for (u = 0; u < hand->obj_count; u++)
            if (hand->obj[u]) {
                free(hand->obj[u]);
                hand->obj[u] = NULL;
            }
        hand->obj_count = 0;
        free(hand->obj);
        free(hand);
    }
} /* end hand_free() */

/*-------------------------------------------------------------------------
 * Function: parse_command_line
 *
 * Purpose: Parses command line and sets up global variable to control output
 *
 * Return: Success: 0
 *
 * Failure: -1
 *
 *-------------------------------------------------------------------------
 */
static int
parse_command_line(int argc, const char *const *argv, struct handler_t **hand_ret)
{
    int               opt;
    unsigned          u;
    struct handler_t *hand = NULL;

    /* no arguments */
    if (argc == 1) {
        usage(h5tools_getprogname());
        goto error;
    }

    /* parse command line options */
    while ((opt = H5_get_option(argc, argv, s_opts, l_opts)) != EOF) {
        switch ((char)opt) {
            case 'h':
                usage(h5tools_getprogname());
                h5tools_setstatus(EXIT_SUCCESS);
                goto done;
                break;

            case 'V':
                print_version(h5tools_getprogname());
                h5tools_setstatus(EXIT_SUCCESS);
                goto done;
                break;

            case 'E':
                if (H5_optarg != NULL)
                    enable_error_stack = atoi(H5_optarg);
                else
                    enable_error_stack = 1;
                break;

            case 'F':
                display_all           = false;
                display_file_metadata = true;
                break;

            case 'f':
                display_all  = false;
                display_file = true;
                break;

            case 'G':
                display_all            = false;
                display_group_metadata = true;
                break;

            case 'g':
                display_all   = false;
                display_group = true;
                break;

            case 'l':
                if (H5_optarg) {
                    sgroups_threshold = atoi(H5_optarg);
                    if (sgroups_threshold < 1) {
                        error_msg("Invalid threshold for small groups\n");
                        goto error;
                    }
                }
                else
                    error_msg("Missing threshold for small groups\n");

                break;

            case 'D':
                display_all           = false;
                display_dset_metadata = true;
                break;

            case 'd':
                display_all  = false;
                display_dset = true;
                break;

            case 'm':
                if (H5_optarg) {
                    sdsets_threshold = atoi(H5_optarg);
                    if (sdsets_threshold < 1) {
                        error_msg("Invalid threshold for small datasets\n");
                        goto error;
                    }
                }
                else
                    error_msg("Missing threshold for small datasets\n");

                break;

            case 'T':
                display_all             = false;
                display_dset_dtype_meta = true;
                break;

            case 'A':
                display_all  = false;
                display_attr = true;
                break;

            case 'a':
                if (H5_optarg) {
                    sattrs_threshold = atoi(H5_optarg);
                    if (sattrs_threshold < 1) {
                        error_msg("Invalid threshold for small # of attributes\n");
                        goto error;
                    }
                }
                else
                    error_msg("Missing threshold for small # of attributes\n");

                break;

            case 's':
                display_all           = false;
                display_free_sections = true;
                break;

            case 'S':
                display_all     = false;
                display_summary = true;
                break;

            case 'O':
                display_all    = false;
                display_object = true;

                /* Allocate space to hold the command line info */
                if (NULL == (hand = (struct handler_t *)calloc((size_t)1, sizeof(struct handler_t)))) {
                    error_msg("unable to allocate memory for object struct\n");
                    goto error;
                }

                /* Allocate space to hold the object strings */
                hand->obj_count = (size_t)argc;
                if (NULL == (hand->obj = (char **)calloc((size_t)argc, sizeof(char *)))) {
                    error_msg("unable to allocate memory for object array\n");
                    goto error;
                }

                /* Store object names */
                for (u = 0; u < hand->obj_count; u++)
                    if (NULL == (hand->obj[u] = strdup(H5_optarg))) {
                        error_msg("unable to allocate memory for object name\n");
                        goto error;
                    }
                break;

            case 'y':
#ifdef H5_HAVE_ROS3_VFD
                snprintf(ros3_fa_g->ep_url, H5FD_ROS3_MAX_ENDPOINT_URL_LEN + 1, "%s", H5_optarg);
#else
                error_msg(
                    "Read-Only S3 VFD is not available unless enabled when HDF5 is configured and built.\n");
                goto error;
#endif
                break;

            case 'w':
#ifdef H5_HAVE_ROS3_VFD
                if (h5tools_parse_ros3_fapl_tuple(H5_optarg, ',', ros3_fa_g) < 0) {
                    error_msg("failed to parse S3 VFD credential info\n");
                    usage(h5tools_getprogname());
                    goto error;
                }

                vfd_info_g.info = ros3_fa_g;
#else
                error_msg(
                    "Read-Only S3 VFD is not available unless enabled when HDF5 is configured and built.\n");
                goto error;
#endif
                break;

            case 'H':
#ifdef H5_HAVE_LIBHDFS
                if (h5tools_parse_hdfs_fapl_tuple(H5_optarg, ',', hdfs_fa_g) < 0) {
                    error_msg("failed to parse HDFS VFD configuration info\n");
                    usage(h5tools_getprogname());
                    goto error;
                }

                vfd_info_g.info = hdfs_fa_g;
#else
                error_msg("HDFS VFD is not available unless enabled when HDF5 is configured and built.\n");
                goto error;
#endif
                break;

            case 'K':
                page_cache = strtoul(H5_optarg, NULL, 0);
                break;

            case '1':
                vol_info_g.type    = VOL_BY_VALUE;
                vol_info_g.u.value = (H5VL_class_value_t)atoi(H5_optarg);
                use_custom_vol_g   = true;
                break;

            case '2':
                vol_info_g.type   = VOL_BY_NAME;
                vol_info_g.u.name = H5_optarg;
                use_custom_vol_g  = true;
                break;

            case '3':
                vol_info_g.info_string = H5_optarg;
                break;

            case '4':
                vfd_info_g.type    = VFD_BY_VALUE;
                vfd_info_g.u.value = (H5FD_class_value_t)atoi(H5_optarg);
                use_custom_vfd_g   = true;
                break;

            case '5':
                vfd_info_g.type   = VFD_BY_NAME;
                vfd_info_g.u.name = H5_optarg;
                use_custom_vfd_g  = true;
                break;

            case '6':
                vfd_info_g.info = (const void *)H5_optarg;
                break;

            default:
                usage(h5tools_getprogname());
                goto error;
        }
    }

#ifdef H5_HAVE_ROS3_VFD
    if (use_custom_vfd_g && !vfd_info_g.info) {
        if (vfd_info_g.type == VFD_BY_NAME && 0 == strcmp(vfd_info_g.u.name, drivernames[ROS3_VFD_IDX]))
            vfd_info_g.info = ros3_fa_g;
    }
#endif
#ifdef H5_HAVE_LIBHDFS
    if (use_custom_vfd_g && !vfd_info_g.info) {
        if (vfd_info_g.type == VFD_BY_NAME && 0 == strcmp(vfd_info_g.u.name, drivernames[HDFS_VFD_IDX]))
            vfd_info_g.info = hdfs_fa_g;
    }
#endif

    /* check for file name to be processed */
    if (argc <= H5_optind) {
        error_msg("missing file name\n");
        usage(h5tools_getprogname());
        goto error;
    }

    /* Set handler structure */
    *hand_ret = hand;

done:
    return 0;

error:
    hand_free(hand);
    h5tools_setstatus(EXIT_FAILURE);

    return -1;
}

/*-------------------------------------------------------------------------
 * Function: iter_free
 *
 * Purpose: Free iter structure
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static void
iter_free(iter_t *iter)
{

    /* Clear array of bins for group counts */
    if (iter->group_bins) {
        free(iter->group_bins);
        iter->group_bins = NULL;
    } /* end if */

    /* Clear array for tracking small groups */
    if (iter->num_small_groups) {
        free(iter->num_small_groups);
        iter->num_small_groups = NULL;
    } /* end if */

    /* Clear array of bins for attribute counts */
    if (iter->attr_bins) {
        free(iter->attr_bins);
        iter->attr_bins = NULL;
    } /* end if */

    /* Clear array for tracking small attributes */
    if (iter->num_small_attrs) {
        free(iter->num_small_attrs);
        iter->num_small_attrs = NULL;
    } /* end if */

    /* Clear dataset datatype information found */
    if (iter->dset_type_info) {
        free(iter->dset_type_info);
        iter->dset_type_info = NULL;
    } /* end if */

    /* Clear array of bins for dataset dimensions */
    if (iter->dset_dim_bins) {
        free(iter->dset_dim_bins);
        iter->dset_dim_bins = NULL;
    } /* end if */

    /* Clear array of tracking 1-D small datasets */
    if (iter->small_dset_dims) {
        free(iter->small_dset_dims);
        iter->small_dset_dims = NULL;
    } /* end if */

    /* Clear array of bins for free-space section sizes */
    if (iter->sect_bins) {
        free(iter->sect_bins);
        iter->sect_bins = NULL;
    } /* end if */
} /* end iter_free() */

/*-------------------------------------------------------------------------
 * Function: print_file_info
 *
 * Purpose: Prints information about file
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_file_info(const iter_t *iter)
{
    fprintf(rawoutstream, "File information\n");
    fprintf(rawoutstream, "\t# of unique groups: %lu\n", iter->uniq_groups);
    fprintf(rawoutstream, "\t# of unique datasets: %lu\n", iter->uniq_dsets);
    fprintf(rawoutstream, "\t# of unique named datatypes: %lu\n", iter->uniq_dtypes);
    fprintf(rawoutstream, "\t# of unique links: %lu\n", iter->uniq_links);
    fprintf(rawoutstream, "\t# of unique other: %lu\n", iter->uniq_others);
    fprintf(rawoutstream, "\tMax. # of links to object: %lu\n", iter->max_links);
    fprintf(rawoutstream, "\tMax. # of objects in group: %" PRIuHSIZE "\n", iter->max_fanout);

    return 0;
} /* print_file_info() */

/*-------------------------------------------------------------------------
 * Function: print_file_metadata
 *
 * Purpose: Prints file space information for file's metadata
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_file_metadata(const iter_t *iter)
{
    fprintf(rawoutstream, "File space information for file metadata (in bytes):\n");
    fprintf(rawoutstream, "\tSuperblock: %" PRIuHSIZE "\n", iter->super_size);
    fprintf(rawoutstream, "\tSuperblock extension: %" PRIuHSIZE "\n", iter->super_ext_size);
    fprintf(rawoutstream, "\tUser block: %" PRIuHSIZE "\n", iter->ublk_size);

    fprintf(rawoutstream, "\tObject headers: (total/unused)\n");
    fprintf(rawoutstream, "\t\tGroups: %" PRIuHSIZE "/%" PRIuHSIZE "\n", iter->group_ohdr_info.total_size,
            iter->group_ohdr_info.free_size);
    fprintf(rawoutstream, "\t\tDatasets(exclude compact data): %" PRIuHSIZE "/%" PRIuHSIZE "\n",
            iter->dset_ohdr_info.total_size, iter->dset_ohdr_info.free_size);
    fprintf(rawoutstream, "\t\tDatatypes: %" PRIuHSIZE "/%" PRIuHSIZE "\n", iter->dtype_ohdr_info.total_size,
            iter->dtype_ohdr_info.free_size);

    fprintf(rawoutstream, "\tGroups:\n");
    fprintf(rawoutstream, "\t\tB-tree/List: %" PRIuHSIZE "\n", iter->groups_btree_storage_size);
    fprintf(rawoutstream, "\t\tHeap: %" PRIuHSIZE "\n", iter->groups_heap_storage_size);

    fprintf(rawoutstream, "\tAttributes:\n");
    fprintf(rawoutstream, "\t\tB-tree/List: %" PRIuHSIZE "\n", iter->attrs_btree_storage_size);
    fprintf(rawoutstream, "\t\tHeap: %" PRIuHSIZE "\n", iter->attrs_heap_storage_size);

    fprintf(rawoutstream, "\tChunked datasets:\n");
    fprintf(rawoutstream, "\t\tIndex: %" PRIuHSIZE "\n", iter->datasets_index_storage_size);

    fprintf(rawoutstream, "\tDatasets:\n");
    fprintf(rawoutstream, "\t\tHeap: %" PRIuHSIZE "\n", iter->datasets_heap_storage_size);

    fprintf(rawoutstream, "\tShared Messages:\n");
    fprintf(rawoutstream, "\t\tHeader: %" PRIuHSIZE "\n", iter->SM_hdr_storage_size);
    fprintf(rawoutstream, "\t\tB-tree/List: %" PRIuHSIZE "\n", iter->SM_index_storage_size);
    fprintf(rawoutstream, "\t\tHeap: %" PRIuHSIZE "\n", iter->SM_heap_storage_size);

    fprintf(rawoutstream, "\tFree-space managers:\n");
    fprintf(rawoutstream, "\t\tHeader: %" PRIuHSIZE "\n", iter->free_hdr);
    fprintf(rawoutstream, "\t\tAmount of free space: %" PRIuHSIZE "\n", iter->free_space);

    return 0;
} /* print_file_metadata() */

/*-------------------------------------------------------------------------
 * Function: print_group_info
 *
 * Purpose: Prints information about groups in the file
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_group_info(const iter_t *iter)
{
    unsigned long power; /* Temporary "power" for bins */
    unsigned long total; /* Total count for various statistics */
    unsigned      u;     /* Local index variable */

    fprintf(rawoutstream, "Small groups (with 0 to %u links):\n", sgroups_threshold - 1);
    total = 0;
    for (u = 0; u < (unsigned)sgroups_threshold; u++) {
        if (iter->num_small_groups[u] > 0) {
            fprintf(rawoutstream, "\t# of groups with %u link(s): %lu\n", u, iter->num_small_groups[u]);
            total += iter->num_small_groups[u];
        } /* end if */
    }     /* end for */
    fprintf(rawoutstream, "\tTotal # of small groups: %lu\n", total);

    fprintf(rawoutstream, "Group bins:\n");
    total = 0;
    if ((iter->group_nbins > 0) && (iter->group_bins[0] > 0)) {
        fprintf(rawoutstream, "\t# of groups with 0 link: %lu\n", iter->group_bins[0]);
        total = iter->group_bins[0];
    } /* end if */
    power = 1;
    for (u = 1; u < iter->group_nbins; u++) {
        if (iter->group_bins[u] > 0) {
            fprintf(rawoutstream, "\t# of groups with %lu - %lu links: %lu\n", power, (power * 10) - 1,
                    iter->group_bins[u]);
            total += iter->group_bins[u];
        } /* end if */
        power *= 10;
    } /* end for */
    fprintf(rawoutstream, "\tTotal # of groups: %lu\n", total);

    return 0;
} /* print_group_info() */

/*-------------------------------------------------------------------------
 * Function: print_group_metadata
 *
 * Purpose: Prints file space information for groups' metadata
 *
 * Return:  Success: 0
 *          Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_group_metadata(const iter_t *iter)
{
    fprintf(rawoutstream, "File space information for groups' metadata (in bytes):\n");

    fprintf(rawoutstream, "\tObject headers (total/unused): %" PRIuHSIZE "/%" PRIuHSIZE "\n",
            iter->group_ohdr_info.total_size, iter->group_ohdr_info.free_size);

    fprintf(rawoutstream, "\tB-tree/List: %" PRIuHSIZE "\n", iter->groups_btree_storage_size);
    fprintf(rawoutstream, "\tHeap: %" PRIuHSIZE "\n", iter->groups_heap_storage_size);

    return 0;
} /* print_group_metadata() */

/*-------------------------------------------------------------------------
 * Function: print_dataset_info
 *
 * Purpose: Prints information about datasets in the file
 *
 * Return:  Success: 0
 *          Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_dataset_info(const iter_t *iter)
{
    unsigned long power; /* Temporary "power" for bins */
    unsigned long total; /* Total count for various statistics */
    unsigned      u;     /* Local index variable */

    if (iter->uniq_dsets > 0) {
        fprintf(rawoutstream, "Dataset dimension information:\n");
        fprintf(rawoutstream, "\tMax. rank of datasets: %u\n", iter->max_dset_rank);
        fprintf(rawoutstream, "\tDataset ranks:\n");
        for (u = 0; u < H5S_MAX_RANK; u++)
            if (iter->dset_rank_count[u] > 0)
                fprintf(rawoutstream, "\t\t# of dataset with rank %u: %lu\n", u, iter->dset_rank_count[u]);

        fprintf(rawoutstream, "1-D Dataset information:\n");
        fprintf(rawoutstream, "\tMax. dimension size of 1-D datasets: %" PRIuHSIZE "\n", iter->max_dset_dims);
        fprintf(rawoutstream, "\tSmall 1-D datasets (with dimension sizes 0 to %u):\n", sdsets_threshold - 1);
        total = 0;
        for (u = 0; u < (unsigned)sdsets_threshold; u++) {
            if (iter->small_dset_dims[u] > 0) {
                fprintf(rawoutstream, "\t\t# of datasets with dimension sizes %u: %lu\n", u,
                        iter->small_dset_dims[u]);
                total += iter->small_dset_dims[u];
            } /* end if */
        }     /* end for */
        fprintf(rawoutstream, "\t\tTotal # of small datasets: %lu\n", total);

        /* Protect against no datasets in file */
        if (iter->dset_dim_nbins > 0) {
            fprintf(rawoutstream, "\t1-D Dataset dimension bins:\n");
            total = 0;
            if (iter->dset_dim_bins[0] > 0) {
                fprintf(rawoutstream, "\t\t# of datasets with dimension size 0: %lu\n",
                        iter->dset_dim_bins[0]);
                total = iter->dset_dim_bins[0];
            } /* end if */
            power = 1;
            for (u = 1; u < iter->dset_dim_nbins; u++) {
                if (iter->dset_dim_bins[u] > 0) {
                    fprintf(rawoutstream, "\t\t# of datasets with dimension size %lu - %lu: %lu\n", power,
                            (power * 10) - 1, iter->dset_dim_bins[u]);
                    total += iter->dset_dim_bins[u];
                } /* end if */
                power *= 10;
            } /* end for */
            fprintf(rawoutstream, "\t\tTotal # of datasets: %lu\n", total);
        } /* end if */

        fprintf(rawoutstream, "Dataset storage information:\n");
        fprintf(rawoutstream, "\tTotal raw data size: %" PRIuHSIZE "\n", iter->dset_storage_size);
        fprintf(rawoutstream, "\tTotal external raw data size: %" PRIuHSIZE "\n",
                iter->dset_external_storage_size);

        fprintf(rawoutstream, "Dataset layout information:\n");
        for (u = 0; u < H5D_NLAYOUTS; u++)
            fprintf(rawoutstream, "\tDataset layout counts[%s]: %lu\n",
                    (u == H5D_COMPACT
                         ? "COMPACT"
                         : (u == H5D_CONTIGUOUS ? "CONTIG" : (u == H5D_CHUNKED ? "CHUNKED" : "VIRTUAL"))),
                    iter->dset_layouts[u]);
        fprintf(rawoutstream, "\tNumber of external files : %lu\n", iter->nexternal);

        fprintf(rawoutstream, "Dataset filters information:\n");
        fprintf(rawoutstream, "\tNumber of datasets with:\n");
        fprintf(rawoutstream, "\t\tNO filter: %lu\n", iter->dset_comptype[H5Z_FILTER_ERROR + 1]);
        fprintf(rawoutstream, "\t\tGZIP filter: %lu\n", iter->dset_comptype[H5Z_FILTER_DEFLATE]);
        fprintf(rawoutstream, "\t\tSHUFFLE filter: %lu\n", iter->dset_comptype[H5Z_FILTER_SHUFFLE]);
        fprintf(rawoutstream, "\t\tFLETCHER32 filter: %lu\n", iter->dset_comptype[H5Z_FILTER_FLETCHER32]);
        fprintf(rawoutstream, "\t\tSZIP filter: %lu\n", iter->dset_comptype[H5Z_FILTER_SZIP]);
        fprintf(rawoutstream, "\t\tNBIT filter: %lu\n", iter->dset_comptype[H5Z_FILTER_NBIT]);
        fprintf(rawoutstream, "\t\tSCALEOFFSET filter: %lu\n", iter->dset_comptype[H5Z_FILTER_SCALEOFFSET]);
        fprintf(rawoutstream, "\t\tUSER-DEFINED filter: %lu\n", iter->dset_comptype[H5_NFILTERS_IMPL - 1]);
    } /* end if */

    return 0;
} /* print_dataset_info() */

/*-------------------------------------------------------------------------
 * Function: print_dataset_metadata
 *
 * Purpose: Prints file space information for datasets' metadata
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_dset_metadata(const iter_t *iter)
{
    fprintf(rawoutstream, "File space information for datasets' metadata (in bytes):\n");

    fprintf(rawoutstream, "\tObject headers (total/unused): %" PRIuHSIZE "/%" PRIuHSIZE "\n",
            iter->dset_ohdr_info.total_size, iter->dset_ohdr_info.free_size);

    fprintf(rawoutstream, "\tIndex for Chunked datasets: %" PRIuHSIZE "\n",
            iter->datasets_index_storage_size);
    fprintf(rawoutstream, "\tHeap: %" PRIuHSIZE "\n", iter->datasets_heap_storage_size);

    return 0;
} /* print_dset_metadata() */

/*-------------------------------------------------------------------------
 * Function: print_dset_dtype_meta
 *
 * Purpose: Prints datasets' datatype information
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_dset_dtype_meta(const iter_t *iter)
{
    unsigned long total;      /* Total count for various statistics */
    size_t        dtype_size; /* Size of encoded datatype */
    unsigned      u;          /* Local index variable */

    if (iter->dset_ntypes) {
        fprintf(rawoutstream, "Dataset datatype information:\n");
        fprintf(rawoutstream, "\t# of unique datatypes used by datasets: %lu\n", iter->dset_ntypes);
        total = 0;
        for (u = 0; u < iter->dset_ntypes; u++) {
            H5Tencode(iter->dset_type_info[u].tid, NULL, &dtype_size);
            fprintf(rawoutstream, "\tDataset datatype #%u:\n", u);
            fprintf(rawoutstream, "\t\tCount (total/named) = (%lu/%lu)\n", iter->dset_type_info[u].count,
                    iter->dset_type_info[u].named);
            fprintf(rawoutstream, "\t\tSize (desc./elmt) = (%lu/%lu)\n", (unsigned long)dtype_size,
                    (unsigned long)H5Tget_size(iter->dset_type_info[u].tid));
            H5Tclose(iter->dset_type_info[u].tid);
            total += iter->dset_type_info[u].count;
        } /* end for */
        fprintf(rawoutstream, "\tTotal dataset datatype count: %lu\n", total);
    } /* end if */

    return 0;
} /* print_dset_dtype_meta() */

/*-------------------------------------------------------------------------
 * Function: print_attr_info
 *
 * Purpose: Prints information about attributes in the file
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_attr_info(const iter_t *iter)
{
    unsigned long power; /* Temporary "power" for bins */
    unsigned long total; /* Total count for various statistics */
    unsigned      u;     /* Local index variable */

    fprintf(rawoutstream, "Small # of attributes (objects with 1 to %u attributes):\n", sattrs_threshold);
    total = 0;
    for (u = 1; u <= (unsigned)sattrs_threshold; u++) {
        if (iter->num_small_attrs[u] > 0) {
            fprintf(rawoutstream, "\t# of objects with %u attributes: %lu\n", u, iter->num_small_attrs[u]);
            total += iter->num_small_attrs[u];
        } /* end if */
    }     /* end for */
    fprintf(rawoutstream, "\tTotal # of objects with small # of attributes: %lu\n", total);

    fprintf(rawoutstream, "Attribute bins:\n");
    total = 0;
    power = 1;
    for (u = 1; u < iter->attr_nbins; u++) {
        if (iter->attr_bins[u] > 0) {
            fprintf(rawoutstream, "\t# of objects with %lu - %lu attributes: %lu\n", power, (power * 10) - 1,
                    iter->attr_bins[u]);
            total += iter->attr_bins[u];
        } /* end if */
        power *= 10;
    } /* end for */
    fprintf(rawoutstream, "\tTotal # of objects with attributes: %lu\n", total);
    fprintf(rawoutstream, "\tMax. # of attributes to objects: %lu\n", (unsigned long)iter->max_attrs);

    return 0;
} /* print_attr_info() */

/*-------------------------------------------------------------------------
 * Function: print_freespace_info
 *
 * Purpose: Prints information about free space in the file
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_freespace_info(const iter_t *iter)
{
    unsigned long power; /* Temporary "power" for bins */
    unsigned long total; /* Total count for various statistics */
    unsigned      u;     /* Local index variable */

    fprintf(rawoutstream, "Free-space persist: %s\n", iter->fs_persist ? "TRUE" : "FALSE");
    fprintf(rawoutstream, "Free-space section threshold: %" PRIuHSIZE " bytes\n", iter->fs_threshold);
    fprintf(rawoutstream, "Small size free-space sections (< %u bytes):\n", (unsigned)SIZE_SMALL_SECTS);
    total = 0;
    for (u = 0; u < SIZE_SMALL_SECTS; u++) {
        if (iter->num_small_sects[u] > 0) {
            fprintf(rawoutstream, "\t# of sections of size %u: %lu\n", u, iter->num_small_sects[u]);
            total += iter->num_small_sects[u];
        } /* end if */
    }     /* end for */
    fprintf(rawoutstream, "\tTotal # of small size sections: %lu\n", total);

    fprintf(rawoutstream, "Free-space section bins:\n");

    total = 0;
    power = 1;
    for (u = 1; u < iter->sect_nbins; u++) {
        if (iter->sect_bins[u] > 0) {
            fprintf(rawoutstream, "\t# of sections of size %lu - %lu: %lu\n", power, (power * 10) - 1,
                    iter->sect_bins[u]);
            total += iter->sect_bins[u];
        } /* end if */
        power *= 10;
    } /* end for */
    fprintf(rawoutstream, "\tTotal # of sections: %lu\n", total);

    return 0;
} /* print_freespace_info() */

/*-------------------------------------------------------------------------
 * Function: print_storage_summary
 *
 * Purpose: Prints file space information for the file
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static herr_t
print_storage_summary(const iter_t *iter)
{
    hsize_t total_meta = 0;
    hsize_t unaccount  = 0;
    double  percent    = 0.0;

    fprintf(rawoutstream, "File space management strategy: %s\n", FS_STRATEGY_NAME[iter->fs_strategy]);
    fprintf(rawoutstream, "File space page size: %" PRIuHSIZE " bytes\n", iter->fsp_size);
    fprintf(rawoutstream, "Summary of file space information:\n");
    total_meta =
        iter->super_size + iter->super_ext_size + iter->ublk_size + iter->group_ohdr_info.total_size +
        iter->dset_ohdr_info.total_size + iter->dtype_ohdr_info.total_size + iter->groups_btree_storage_size +
        iter->groups_heap_storage_size + iter->attrs_btree_storage_size + iter->attrs_heap_storage_size +
        iter->datasets_index_storage_size + iter->datasets_heap_storage_size + iter->SM_hdr_storage_size +
        iter->SM_index_storage_size + iter->SM_heap_storage_size + iter->free_hdr;

    fprintf(rawoutstream, "  File metadata: %" PRIuHSIZE " bytes\n", total_meta);
    fprintf(rawoutstream, "  Raw data: %" PRIuHSIZE " bytes\n", iter->dset_storage_size);

    percent = ((double)iter->free_space / (double)iter->filesize) * 100.0;
    fprintf(rawoutstream, "  Amount/Percent of tracked free space: %" PRIuHSIZE " bytes/%3.1f%%\n",
            iter->free_space, percent);

    if (iter->filesize < (total_meta + iter->dset_storage_size + iter->free_space)) {
        unaccount = (total_meta + iter->dset_storage_size + iter->free_space) - iter->filesize;
        fprintf(rawoutstream, "  ??? File has %" PRIuHSIZE " more bytes accounted for than its size! ???\n",
                unaccount);
    }
    else {
        unaccount = iter->filesize - (total_meta + iter->dset_storage_size + iter->free_space);
        fprintf(rawoutstream, "  Unaccounted space: %" PRIuHSIZE " bytes\n", unaccount);
    }

    fprintf(rawoutstream, "Total space: %" PRIuHSIZE " bytes\n",
            total_meta + iter->dset_storage_size + iter->free_space + unaccount);

    if (iter->nexternal)
        fprintf(rawoutstream, "External raw data: %" PRIuHSIZE " bytes\n", iter->dset_external_storage_size);

    return 0;
} /* print_storage_summary() */

/*-------------------------------------------------------------------------
 * Function: print_file_statistics
 *
 * Purpose: Prints file statistics
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static void
print_file_statistics(const iter_t *iter)
{
    if (display_all) {
        display_file            = true;
        display_group           = true;
        display_dset            = true;
        display_dset_dtype_meta = true;
        display_attr            = true;
        display_free_sections   = true;
        display_summary         = true;

        display_file_metadata  = true;
        display_group_metadata = true;
        display_dset_metadata  = true;
    }

    if (display_file)
        print_file_info(iter);
    if (display_file_metadata)
        print_file_metadata(iter);

    if (display_group)
        print_group_info(iter);
    if (!display_all && display_group_metadata)
        print_group_metadata(iter);

    if (display_dset)
        print_dataset_info(iter);
    if (display_dset_dtype_meta)
        print_dset_dtype_meta(iter);
    if (!display_all && display_dset_metadata)
        print_dset_metadata(iter);

    if (display_attr)
        print_attr_info(iter);
    if (display_free_sections)
        print_freespace_info(iter);
    if (display_summary)
        print_storage_summary(iter);
} /* print_file_statistics() */

/*-------------------------------------------------------------------------
 * Function: print_object_statistics
 *
 * Purpose: Prints object statistics
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static void
print_object_statistics(const char *name)
{
    fprintf(rawoutstream, "Object name %s\n", name);
} /* print_object_statistics() */

/*-------------------------------------------------------------------------
 * Function: print_statistics
 *
 * Purpose: Prints statistics
 *
 * Return: Success: 0
 *
 * Failure: Never fails
 *
 *-------------------------------------------------------------------------
 */
static void
print_statistics(const char *name, const iter_t *iter)
{
    if (display_object)
        print_object_statistics(name);
    else
        print_file_statistics(iter);
} /* print_statistics() */

/*-------------------------------------------------------------------------
 * Function: main
 *
 *-------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    iter_t            iter;
    const char       *fname   = NULL;
    hid_t             fid     = H5I_INVALID_HID;
    struct handler_t *hand    = NULL;
    hid_t             fapl_id = H5P_DEFAULT;

    h5tools_setprogname(PROGRAMNAME);
    h5tools_setstatus(EXIT_SUCCESS);

    /* Initialize h5tools lib */
    h5tools_init();

    memset(&iter, 0, sizeof(iter));

    /* Initialize VFD-specific structures */
#ifdef H5_HAVE_ROS3_VFD
    if (NULL == (ros3_fa_g = calloc(1, sizeof(*ros3_fa_g)))) {
        error_msg("unable to allocate space for configuration structure\n");
        h5tools_setstatus(EXIT_FAILURE);
        goto done;
    }

    /* Default "anonymous" S3 configuration */
    ros3_fa_g->fa.version      = H5FD_CURR_ROS3_FAPL_T_VERSION;
    ros3_fa_g->fa.authenticate = false;
#endif
#ifdef H5_HAVE_LIBHDFS
    if (NULL == (hdfs_fa_g = calloc(1, sizeof(*hdfs_fa_g)))) {
        error_msg("unable to allocate space for configuration structure\n");
        h5tools_setstatus(EXIT_FAILURE);
        goto done;
    }

    /* "Default" HDFS configuration */
    hdfs_fa_g->version            = H5FD__CURR_HDFS_FAPL_T_VERSION;
    hdfs_fa_g->stream_buffer_size = 2048;
    strcpy(hdfs_fa_g->namenode_name, "localhost");
#endif

    if (parse_command_line(argc, (const char *const *)argv, &hand) < 0)
        goto done;

    /* Enable error reporting if command line option */
    h5tools_error_report();

    if ((fapl_id = h5tools_get_new_fapl(H5P_DEFAULT)) < 0) {
        error_msg("unable to create FAPL for file access\n");
        h5tools_setstatus(EXIT_FAILURE);
        goto done;
    }
    /* Set non-default VOL connector, if requested */
    if (use_custom_vol_g) {
        if (h5tools_set_fapl_vol(fapl_id, &vol_info_g) < 0) {
            error_msg("unable to set VOL on fapl for file\n");
            h5tools_setstatus(EXIT_FAILURE);
            goto done;
        }
    }
    /* Set non-default virtual file driver, if requested */
    if (use_custom_vfd_g) {
        if (h5tools_set_fapl_vfd(fapl_id, &vfd_info_g) < 0) {
            error_msg("unable to set VFD on fapl for file\n");
            h5tools_setstatus(EXIT_FAILURE);
            goto done;
        }
    }
    if (page_cache > 0) {
        if (H5Pset_page_buffer_size(fapl_id, page_cache, 0, 0) < 0) {
            error_msg("unable to set page buffer cache size for file access\n");
            h5tools_setstatus(EXIT_FAILURE);
            goto done;
        }
    }

    fname = argv[H5_optind];

    /* Check for filename given */
    if (fname) {
        hid_t       fcpl;
        H5F_info2_t finfo;

        fprintf(rawoutstream, "Filename: %s\n", fname);

        fid = h5tools_fopen(fname, H5F_ACC_RDONLY, fapl_id, (use_custom_vol_g || use_custom_vfd_g), NULL, 0);

        if (fid < 0) {
            error_msg("unable to open file \"%s\"\n", fname);
            h5tools_setstatus(EXIT_FAILURE);
            goto done;
        }

        /* Initialize iter structure */
        iter.fid = fid;

        if (H5Fget_filesize(fid, &iter.filesize) < 0)
            warn_msg("Unable to retrieve file size\n");
        assert(iter.filesize != 0);

        /* Get storage info for file-level structures */
        if (H5Fget_info2(fid, &finfo) < 0)
            warn_msg("Unable to retrieve file info\n");
        else {
            iter.super_size            = finfo.super.super_size;
            iter.super_ext_size        = finfo.super.super_ext_size;
            iter.SM_hdr_storage_size   = finfo.sohm.hdr_size;
            iter.SM_index_storage_size = finfo.sohm.msgs_info.index_size;
            iter.SM_heap_storage_size  = finfo.sohm.msgs_info.heap_size;
            iter.free_space            = finfo.free.tot_space;
            iter.free_hdr              = finfo.free.meta_size;
        } /* end else */

        iter.num_small_groups = (unsigned long *)calloc((size_t)sgroups_threshold, sizeof(unsigned long));
        iter.num_small_attrs = (unsigned long *)calloc((size_t)(sattrs_threshold + 1), sizeof(unsigned long));
        iter.small_dset_dims = (unsigned long *)calloc((size_t)sdsets_threshold, sizeof(unsigned long));

        if (iter.num_small_groups == NULL || iter.num_small_attrs == NULL || iter.small_dset_dims == NULL) {
            error_msg("Unable to allocate memory for tracking small groups/datasets/attributes\n");
            h5tools_setstatus(EXIT_FAILURE);
            goto done;
        }

        if ((fcpl = H5Fget_create_plist(fid)) < 0)
            warn_msg("Unable to retrieve file creation property\n");

        if (H5Pget_userblock(fcpl, &iter.ublk_size) < 0)
            warn_msg("Unable to retrieve userblock size\n");

        if (H5Pget_file_space_strategy(fcpl, &iter.fs_strategy, &iter.fs_persist, &iter.fs_threshold) < 0)
            warn_msg("Unable to retrieve file space information\n");
        assert(iter.fs_strategy >= 0 && iter.fs_strategy < H5F_FSPACE_STRATEGY_NTYPES);

        if (H5Pget_file_space_page_size(fcpl, &iter.fsp_size) < 0)
            warn_msg("Unable to retrieve file space page size\n");

        /* get information for free-space sections */
        if (freespace_stats(fid, &iter) < 0)
            warn_msg("Unable to retrieve freespace info\n");

        /* Walk the objects or all file */
        if (display_object) {
            unsigned u;

            for (u = 0; u < hand->obj_count; u++) {
                if (h5trav_visit(fid, hand->obj[u], true, true, obj_stats, lnk_stats, &iter, H5O_INFO_ALL) <
                    0) {
                    error_msg("unable to traverse object \"%s\"\n", hand->obj[u]);
                    h5tools_setstatus(EXIT_FAILURE);
                }
                else
                    print_statistics(hand->obj[u], &iter);
            } /* end for */
        }     /* end if */
        else {
            if (h5trav_visit(fid, "/", true, true, obj_stats, lnk_stats, &iter, H5O_INFO_ALL) < 0) {
                error_msg("unable to traverse objects/links in file \"%s\"\n", fname);
                h5tools_setstatus(EXIT_FAILURE);
            }
            else
                print_statistics("/", &iter);
        } /* end else */
    }     /* end if */

done:
    hand_free(hand);

    /* Free iter structure */
    iter_free(&iter);

#ifdef H5_HAVE_ROS3_VFD
    free(ros3_fa_g);
#endif
#ifdef H5_HAVE_LIBHDFS
    free(hdfs_fa_g);
#endif

    if (fapl_id != H5P_DEFAULT && H5Pclose(fapl_id) < 0) {
        error_msg("unable to close fapl entry\n");
        h5tools_setstatus(EXIT_FAILURE);
    }

    if (fid >= 0)
        if (H5Fclose(fid) < 0)
            h5tools_setstatus(EXIT_FAILURE);

    leave(h5tools_getstatus());
} /* end main() */
