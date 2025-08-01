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

#include "h5repack.h"
#include "h5tools.h"
#include "h5tools_utils.h"

/*-------------------------------------------------------------------------
 * Function: init_packobject
 *
 * Purpose: initialize a pack_info_t structure
 *
 * Return: void
 *-------------------------------------------------------------------------
 */

void
init_packobject(pack_info_t *obj)
{
    int j, k;

    strcpy(obj->path, "\0");
    for (j = 0; j < H5_REPACK_MAX_NFILTERS; j++) {
        obj->filter[j].filtn     = -1;
        obj->filter[j].cd_nelmts = DEFAULT_CDELEMTS;
        for (k = 0; k < DEFAULT_CDELEMTS; k++)
            obj->filter[j].cd_values[k] = 0;
    }
    obj->chunk.rank = -1;
    obj->refobj_id  = -1;
    obj->layout     = H5D_LAYOUT_ERROR;
    obj->nfilters   = 0;
}

/*-------------------------------------------------------------------------
 * Function: aux_tblinsert_filter
 *
 * Purpose: auxiliary function, inserts the filter in object OBJS[ idx ]
 *
 * Return: void
 *-------------------------------------------------------------------------
 */

static void
aux_tblinsert_filter(pack_opttbl_t *table, unsigned int idx, filter_info_t filt)
{
    if (table->objs[idx].nfilters < H5_REPACK_MAX_NFILTERS)
        table->objs[idx].filter[table->objs[idx].nfilters++] = filt;
    else
        H5TOOLS_INFO("cannot insert the filter in this object. Maximum capacity exceeded");
}

/*-------------------------------------------------------------------------
 * Function: aux_tblinsert_layout
 *
 * Purpose: auxiliary function, inserts the layout in object OBJS[ idx ]
 *
 * Return: void
 *-------------------------------------------------------------------------
 */
static void
aux_tblinsert_layout(pack_opttbl_t *table, unsigned int idx, pack_info_t *pack)
{
    int k;

    table->objs[idx].layout = pack->layout;
    if (H5D_CHUNKED == pack->layout) {
        /* -2 means the NONE option, remove chunking
        and set the layout to contiguous */
        if (pack->chunk.rank == -2) {
            table->objs[idx].layout     = H5D_CONTIGUOUS;
            table->objs[idx].chunk.rank = -2;
        }
        /* otherwise set the chunking type */
        else {
            table->objs[idx].chunk.rank = pack->chunk.rank;
            for (k = 0; k < pack->chunk.rank; k++)
                table->objs[idx].chunk.chunk_lengths[k] = pack->chunk.chunk_lengths[k];
        }
    }
}

/*-------------------------------------------------------------------------
 * Function: aux_inctable
 *
 * Purpose: auxiliary function, increases the size of the collection by N_OBJS
 *
 * Return: 0, ok, -1, fail
 *-------------------------------------------------------------------------
 */
static int
aux_inctable(pack_opttbl_t *table, unsigned n_objs)
{
    unsigned u;
    int      ret_value = 0;

    table->size += n_objs;
    table->objs = (pack_info_t *)realloc(table->objs, table->size * sizeof(pack_info_t));
    if (table->objs == NULL) {
        H5TOOLS_INFO("not enough memory for options table");
        ret_value = -1;
    }
    else {
        for (u = table->nelems; u < table->size; u++)
            init_packobject(&table->objs[u]);
    }

    return ret_value;
}

/*-------------------------------------------------------------------------
 * Function: options_table_init
 *
 * Purpose: init options table
 *
 * Return: 0, ok, -1, fail
 *-------------------------------------------------------------------------
 */
int
options_table_init(pack_opttbl_t **tbl)
{
    unsigned int   i;
    pack_opttbl_t *table;
    int            ret_value = 0;

    if (NULL == (table = (pack_opttbl_t *)malloc(sizeof(pack_opttbl_t)))) {
        H5TOOLS_GOTO_ERROR((-1), "not enough memory for options table");
    }

    table->size   = 30;
    table->nelems = 0;
    if (NULL == (table->objs = (pack_info_t *)malloc(table->size * sizeof(pack_info_t)))) {
        free(table);
        H5TOOLS_GOTO_ERROR((-1), "not enough memory for options table");
    }

    for (i = 0; i < table->size; i++)
        init_packobject(&table->objs[i]);

    *tbl = table;
done:
    return ret_value;
}

/*-------------------------------------------------------------------------
 * Function: options_table_free
 *
 * Purpose: free table memory
 *
 * Return: 0
 *-------------------------------------------------------------------------
 */

int
options_table_free(pack_opttbl_t *table)
{
    free(table->objs);
    free(table);
    return 0;
}

/*-------------------------------------------------------------------------
 * Function: options_add_layout
 *
 * Purpose: add a layout option to the option list
 *
 * Return: 0, ok, -1, fail
 *-------------------------------------------------------------------------
 */
int
options_add_layout(obj_list_t *obj_list, unsigned n_objs, pack_info_t *pack, pack_opttbl_t *table)
{
    unsigned i, j, idx;
    unsigned added     = 0;
    bool     found     = false;
    int      ret_value = 0;

    /* increase the size of the collection by N_OBJS if necessary */
    if (table->nelems + n_objs >= table->size)
        if (aux_inctable(table, n_objs) < 0)
            return -1;

    /* search if this object is already in the table; "path" is the key */
    if (table->nelems > 0) {
        /* go through the supplied list of names */
        for (j = 0; j < n_objs; j++) {
            /* linear table search */
            for (i = 0; i < table->nelems; i++) {
                /*already on the table */
                if (strcmp(obj_list[j].obj, table->objs[i].path) == 0) {
                    /* already chunk info inserted for this one; exit */
                    if (table->objs[i].chunk.rank > 0) {
                        H5TOOLS_INFO("chunk information already inserted for <%s>\n", obj_list[j].obj);
                        exit(EXIT_FAILURE);
                    }
                    /* insert the layout info */
                    else {
                        aux_tblinsert_layout(table, i, pack);
                        found = true;
                        break;
                    }
                } /* if */
            }     /* i */

            if (!found) {
                /* keep the grow in a temp var */
                idx = table->nelems + added;
                added++;
                strcpy(table->objs[idx].path, obj_list[j].obj);
                aux_tblinsert_layout(table, idx, pack);
            }
            /* cases where we have an already inserted name but there is a new name also
             example:
             -f dset1:GZIP=1 -l dset1,dset2:CHUNK=20x20
             dset1 is already inserted, but dset2 must also be
             */
            else if (found && strcmp(obj_list[j].obj, table->objs[i].path) != 0) {
                /* keep the grow in a temp var */
                idx = table->nelems + added;
                added++;
                strcpy(table->objs[idx].path, obj_list[j].obj);
                aux_tblinsert_layout(table, idx, pack);
            }
        } /* j */
    }
    /* first time insertion */
    else {
        /* go through the supplied list of names */
        for (j = 0; j < n_objs; j++) {
            idx = table->nelems + added;
            added++;
            strcpy(table->objs[idx].path, obj_list[j].obj);
            aux_tblinsert_layout(table, idx, pack);
        }
    }

    table->nelems += added;

    return ret_value;
}

/*-------------------------------------------------------------------------
 * Function: options_add_filter
 *
 * Purpose: add a compression -f option to the option list
 *
 * Return: 0, ok, -1, fail
 *-------------------------------------------------------------------------
 */
int
options_add_filter(obj_list_t *obj_list, unsigned n_objs, filter_info_t filt, pack_opttbl_t *table)
{
    unsigned int i, j, idx;
    unsigned     added = 0;
    bool         found = false;

    /* increase the size of the collection by N_OBJS if necessary */
    if (table->nelems + n_objs >= table->size)
        if (aux_inctable(table, n_objs) < 0)
            return -1;

    /* search if this object is already in the table; "path" is the key */
    if (table->nelems > 0) {
        /* go through the supplied list of names */
        for (j = 0; j < n_objs; j++) {
            /* linear table search */
            for (i = 0; i < table->nelems; i++) {
                /*already on the table */
                if (strcmp(obj_list[j].obj, table->objs[i].path) == 0) {
                    /* insert */
                    aux_tblinsert_filter(table, i, filt);
                    found = true;
                    break;
                } /* if */
            }     /* i */

            if (!found) {
                /* keep the grow in a temp var */
                idx = table->nelems + added;
                added++;
                strcpy(table->objs[idx].path, obj_list[j].obj);
                aux_tblinsert_filter(table, idx, filt);
            }
            /* cases where we have an already inserted name but there is a new name also
             example:
             -l dset1:CHUNK=20x20 -f dset1,dset2:GZIP=1
             dset1 is already inserted, but dset2 must also be
             */
            else if (found && strcmp(obj_list[j].obj, table->objs[i].path) != 0) {
                /* keep the grow in a temp var */
                idx = table->nelems + added;
                added++;
                strcpy(table->objs[idx].path, obj_list[j].obj);
                aux_tblinsert_filter(table, idx, filt);
            }
        } /* j */
    }

    /* first time insertion */
    else {
        /* go through the supplied list of names */
        for (j = 0; j < n_objs; j++) {
            idx = table->nelems + added;
            added++;
            strcpy(table->objs[idx].path, obj_list[j].obj);
            aux_tblinsert_filter(table, idx, filt);
        }
    }

    table->nelems += added;

    return 0;
}

/*-------------------------------------------------------------------------
 * Function: options_get_object
 *
 * Purpose: get object from table; "path" is the key
 *
 * Return: pack_info_t* OBJECT or NULL if not found; PATH is the key
 *-------------------------------------------------------------------------
 */

pack_info_t *
options_get_object(const char *path, pack_opttbl_t *table)
{
    unsigned int i;
    char         tbl_path[MAX_NC_NAME + 1]; /* +1 for start with "/" case */

    for (i = 0; i < table->nelems; i++) {
        /* make full path (start with "/") to compare correctly  */
        if (strncmp(table->objs[i].path, "/", 1) != 0) {
            strcpy(tbl_path, "/");
            strcat(tbl_path, table->objs[i].path);
        }
        else
            strcpy(tbl_path, table->objs[i].path);

        /* found it */
        if (strcmp(tbl_path, path) == 0) {
            return (&table->objs[i]);
        }
    }
    return NULL;
}
