/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "openexr_config.h"
#include "internal_structs.h"
#include "internal_attr.h"
#include "internal_constants.h"
#include "internal_memory.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
#        include <synchapi.h>
#        include <windows.h>
#    else
#        include <pthread.h>
#    endif
#endif

/**************************************/

static void
default_error_handler (
    exr_const_context_t ctxt, exr_result_t code, const char* msg)
{
#if ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
    static CRITICAL_SECTION sMutex;
    volatile static long    initialized = 0;
    if (InterlockedIncrement (&initialized) == 1)
        InitializeCriticalSection (&sMutex);
    initialized = 1; // avoids overflow on long running programs...
#    else
    static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;
#    endif
#endif

#if ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
    EnterCriticalSection (&sMutex);
#    else
    pthread_mutex_lock (&sMutex);
#    endif
#endif
    if (ctxt)
    {
        if (ctxt->filename.str)
            fprintf (
                stderr,
                "%s: (%s) %s\n",
                ctxt->filename.str,
                exr_get_error_code_as_string (code),
                msg);
        else
            fprintf (
                stderr,
                "Context 0x%p: (%s) %s\n",
                (const void*) ctxt,
                exr_get_error_code_as_string (code),
                msg);
    }
    else
        fprintf (stderr, "<ERROR>: %s\n", msg);
    fflush (stderr);

#if ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
    LeaveCriticalSection (&sMutex);
#    else
    pthread_mutex_unlock (&sMutex);
#    endif
#endif
}

static exr_result_t
dispatch_error (exr_const_context_t ctxt, exr_result_t code, const char* msg)
{
    if (ctxt)
    {
        ctxt->error_handler_fn (ctxt, code, msg);
        return code;
    }

    default_error_handler (ctxt, code, msg);
    return code;
}

/**************************************/

static exr_result_t
dispatch_standard_error (exr_const_context_t ctxt, exr_result_t code)
{
    return dispatch_error (ctxt, code, exr_get_default_error_message (code));
}

/**************************************/

static exr_result_t dispatch_print_error (
    exr_const_context_t ctxt, exr_result_t code, const char* msg, ...)
    EXR_PRINTF_FUNC_ATTRIBUTE;

static exr_result_t
dispatch_print_error (
    exr_const_context_t ctxt, exr_result_t code, const char* msg, ...)
{
    char    stackbuf[256];
    char*   heapbuf = NULL;
    int     nwrit   = 0;
    va_list fmtargs;

    va_start (fmtargs, msg);
    {
        va_list stkargs;

        va_copy (stkargs, fmtargs);
        nwrit = vsnprintf (stackbuf, 256, msg, stkargs);
        va_end (stkargs);
        if (nwrit >= 256)
        {
            heapbuf = ctxt->alloc_fn ((size_t) (nwrit + 1));
            if (heapbuf)
            {
                (void) vsnprintf (heapbuf, (size_t) (nwrit + 1), msg, fmtargs);
                dispatch_error (ctxt, code, heapbuf);
                ctxt->free_fn (heapbuf);
            }
            else
                dispatch_error (
                    ctxt, code, "Unable to allocate temporary memory");
        }
        else
            dispatch_error (ctxt, code, stackbuf);
    }
    va_end (fmtargs);
    return code;
}

/**************************************/

static void
internal_exr_destroy_part (exr_context_t ctxt, exr_priv_part_t cur)
{
    exr_memory_free_func_t dofree = ctxt->free_fn;
    uint64_t*              ctable;

    exr_attr_list_destroy ((exr_context_t) ctxt, &(cur->attributes));

    /* we stack x and y together so only have to free the first */
    if (cur->tile_level_tile_count_x) dofree (cur->tile_level_tile_count_x);

#if defined(_MSC_VER)
    ctable = (uint64_t*) InterlockedOr64 (
        (int64_t volatile*) &(cur->chunk_table), 0);
    cur->chunk_table = 0;
#else
    ctable = (uint64_t*) atomic_load (&(cur->chunk_table));
    atomic_store (&(cur->chunk_table), (uintptr_t) (0));
#endif
    if (ctable && ((uintptr_t) ctable) != UINTPTR_MAX) dofree (ctable);

    /* we avoid malloc on one part files, but need to check */
    if (cur != &(ctxt->first_part)) { dofree (cur); }
    else { memset (cur, 0, sizeof (struct _priv_exr_part_t)); }
}

/**************************************/

static void
internal_exr_destroy_parts (exr_context_t ctxt)
{
    for (int p = 0; p < ctxt->num_parts; ++p)
        internal_exr_destroy_part (ctxt, ctxt->parts[p]);

    if (ctxt->parts != &(ctxt->init_part)) ctxt->free_fn (ctxt->parts);
    ctxt->init_part = NULL;
    ctxt->parts     = NULL;
    ctxt->num_parts = 0;
}

/**************************************/

exr_result_t
internal_exr_add_part (
    exr_context_t f, exr_priv_part_t* outpart, int* new_index)
{
    int              ncount = f->num_parts + 1;
    exr_priv_part_t  part;
    exr_priv_part_t* nptrs = NULL;

    if (new_index) *new_index = f->num_parts;

    if (ncount == 1)
    {
        /* avoid an initial malloc by embedding a part in the struct... */
        part         = &(f->first_part);
        /* also need a pointer to pointer... */
        f->init_part = part;
        nptrs        = &(f->init_part);
    }
    else
    {
        part = f->alloc_fn (sizeof (struct _priv_exr_part_t));
        if (!part) return f->standard_error (f, EXR_ERR_OUT_OF_MEMORY);

        nptrs = f->alloc_fn (sizeof (exr_priv_part_t) * (size_t) ncount);
        if (!nptrs)
        {
            f->free_fn (part);
            return f->standard_error (f, EXR_ERR_OUT_OF_MEMORY);
        }
    }
    memset (part, 0, sizeof (struct _priv_exr_part_t));

    /* assign appropriately invalid values */
    part->storage_mode         = EXR_STORAGE_LAST_TYPE;
    part->data_window.max.x    = -1;
    part->data_window.max.y    = -1;
    part->display_window.max.x = -1;
    part->display_window.max.y = -1;
    part->chunk_count          = -1;
    part->lines_per_chunk      = -1;

    part->zip_compression_level = f->default_zip_level;
    part->dwa_compression_level = f->default_dwa_quality;

    /* put it into the part table */
    for (int p = 0; p < f->num_parts; ++p)
    {
        nptrs[p] = f->parts[p];
    }
    nptrs[f->num_parts] = part;

    if (f->parts && f->parts != &(f->init_part)) { f->free_fn (f->parts); }
    f->parts     = nptrs;
    f->num_parts = ncount;
    if (outpart) *outpart = part;

    return EXR_ERR_SUCCESS;
}

/**************************************/

void
internal_exr_revert_add_part (
    exr_context_t ctxt, exr_priv_part_t* outpart, int* new_index)
{
    int             ncount = ctxt->num_parts - 1;
    exr_priv_part_t part   = *outpart;

    *outpart   = NULL;
    *new_index = -1;

    internal_exr_destroy_part (ctxt, part);

    if ( ncount > 0 )
    {
        int np = 0;
        for (int p = 0; p < ctxt->num_parts; ++p)
        {
            if (ctxt->parts[p] == part) continue;
            ctxt->parts[np] = ctxt->parts[p];
            ++np;
        }
    }
    else
    {
        if (ctxt->parts && ctxt->parts != &(ctxt->init_part))
            ctxt->free_fn (ctxt->parts);

        ctxt->parts = NULL;
    }
    ctxt->num_parts = ncount;
}

/**************************************/

exr_result_t
internal_exr_context_restore_handlers (exr_context_t ctxt, exr_result_t rv)
{
    ctxt->standard_error = &dispatch_standard_error;
    ctxt->report_error   = &dispatch_error;
    ctxt->print_error    = &dispatch_print_error;
    return rv;
}

/**************************************/

exr_result_t
internal_exr_alloc_context (
    exr_context_t*                   out,
    const exr_context_initializer_t* initializers,
    enum _INTERNAL_EXR_CONTEXT_MODE  mode,
    size_t                           default_size)
{
    void*         memptr;
    exr_result_t  rv;
    exr_context_t ret;
    int           gmaxw, gmaxh;
    size_t        extra_data;

    *out = NULL;
    if (initializers->read_fn || initializers->write_fn)
        extra_data = 0;
    else
        extra_data = default_size;

    memptr = (initializers->alloc_fn) (
        sizeof (struct _priv_exr_context_t) + extra_data);
    if (memptr)
    {
        memset (memptr, 0, sizeof (struct _priv_exr_context_t));

        ret       = memptr;
        ret->mode = (uint8_t) mode;
        /* stash this separately so when a user queries they don't see
         * any of our internal hijinx */
        ret->real_user_data = initializers->user_data;
        if (initializers->read_fn || initializers->write_fn)
            ret->user_data = initializers->user_data;
        else if (extra_data > 0)
            ret->user_data =
                (((uint8_t*) memptr) + sizeof (struct _priv_exr_context_t));

        ret->standard_error   = &dispatch_standard_error;
        ret->report_error     = &dispatch_error;
        ret->print_error      = &dispatch_print_error;
        ret->error_handler_fn = initializers->error_handler_fn;
        ret->alloc_fn         = initializers->alloc_fn;
        ret->free_fn          = initializers->free_fn;

        exr_get_default_maximum_image_size (&gmaxw, &gmaxh);
        if (initializers->max_image_width <= 0)
            ret->max_image_w = gmaxw;
        else
            ret->max_image_w = initializers->max_image_width;
        if (ret->max_image_w > 0 && gmaxw > 0 && ret->max_image_w &&
            ret->max_image_w > gmaxw)
            ret->max_image_w = gmaxw;

        if (initializers->max_image_height <= 0)
            ret->max_image_h = gmaxh;
        else
            ret->max_image_h = initializers->max_image_height;
        if (ret->max_image_h > 0 && gmaxh > 0 && ret->max_image_h &&
            ret->max_image_h > gmaxh)
            ret->max_image_h = gmaxh;

        exr_get_default_maximum_tile_size (&gmaxw, &gmaxh);
        if (initializers->max_tile_width <= 0)
            ret->max_tile_w = gmaxw;
        else
            ret->max_tile_w = initializers->max_tile_width;
        if (ret->max_tile_w > 0 && gmaxw > 0 && ret->max_tile_w &&
            ret->max_tile_w > gmaxw)
            ret->max_tile_w = gmaxw;

        if (initializers->max_tile_height <= 0)
            ret->max_tile_h = gmaxh;
        else
            ret->max_tile_h = initializers->max_tile_height;
        if (ret->max_tile_h > 0 && gmaxh > 0 && ret->max_tile_h &&
            ret->max_tile_h > gmaxh)
            ret->max_tile_h = gmaxh;

        exr_get_default_zip_compression_level (&ret->default_zip_level);
        exr_get_default_dwa_compression_quality (&ret->default_dwa_quality);
        if (initializers->zip_level >= 0)
            ret->default_zip_level = initializers->zip_level;
        if (initializers->dwa_quality >= 0.f)
            ret->default_dwa_quality = initializers->dwa_quality;

        if (initializers->flags & EXR_CONTEXT_FLAG_STRICT_HEADER)
            ret->strict_header = 1;
        if (initializers->flags & EXR_CONTEXT_FLAG_SILENT_HEADER_PARSE)
            ret->silent_header = 1;
        ret->disable_chunk_reconstruct =
            (initializers->flags &
             EXR_CONTEXT_FLAG_DISABLE_CHUNK_RECONSTRUCTION);
        ret->legacy_header =
            (initializers->flags & EXR_CONTEXT_FLAG_WRITE_LEGACY_HEADER);

        ret->file_size       = -1;
        ret->max_name_length = EXR_SHORTNAME_MAXLEN;

        ret->destroy_fn = initializers->destroy_fn;
        ret->read_fn    = initializers->read_fn;
        ret->write_fn   = initializers->write_fn;

#if ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
        InitializeCriticalSection (&(ret->mutex));
#    else
        rv = pthread_mutex_init (&(ret->mutex), NULL);
        if (rv != 0)
        {
            /* fairly unlikely... */
            (initializers->free_fn) (memptr);
            *out = NULL;
            return EXR_ERR_OUT_OF_MEMORY;
        }
#    endif
#endif

        *out = ret;
        rv   = EXR_ERR_SUCCESS;

        /* if we are reading the file, go ahead and set up the first
         * part to make parsing logic easier */
        if (mode != EXR_CONTEXT_WRITE)
        {
            exr_priv_part_t part;
            rv = internal_exr_add_part (ret, &part, NULL);
            if (rv != EXR_ERR_SUCCESS)
            {
                /* this should never happen since we reserve space for
                 * one in the struct, but maybe we changed
                 * something */
                (initializers->free_fn) (memptr);
                *out = NULL;
            }
        }
    }
    else
    {
        (initializers->error_handler_fn) (
            NULL,
            EXR_ERR_OUT_OF_MEMORY,
            exr_get_default_error_message (EXR_ERR_OUT_OF_MEMORY));
        rv = EXR_ERR_OUT_OF_MEMORY;
    }

    return rv;
}

/**************************************/

void
internal_exr_destroy_context (exr_context_t ctxt)
{
    exr_memory_free_func_t dofree = ctxt->free_fn;

    exr_attr_string_destroy (ctxt, &(ctxt->filename));
    exr_attr_string_destroy (ctxt, &(ctxt->tmp_filename));
    exr_attr_list_destroy (ctxt, &(ctxt->custom_handlers));
    internal_exr_destroy_parts (ctxt);
#if ILMTHREAD_THREADING_ENABLED
#    ifdef _WIN32
    DeleteCriticalSection (&(ctxt->mutex));
#    else
    pthread_mutex_destroy (&(ctxt->mutex));
#    endif
#endif

    dofree (ctxt);
}

/**************************************/

void
internal_exr_update_default_handlers (exr_context_initializer_t* inits)
{
    if (!inits->error_handler_fn)
        inits->error_handler_fn = &default_error_handler;

    if (!inits->alloc_fn) inits->alloc_fn = &internal_exr_alloc;
    if (!inits->free_fn) inits->free_fn = &internal_exr_free;
}
