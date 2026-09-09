/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_attr.h"

#include "internal_constants.h"
#include "internal_structs.h"

#include <string.h>

/**************************************/

int
exr_attr_opaquedata_init (
    exr_context_t ctxt, exr_attr_opaquedata_t* u, size_t b)
{
    exr_attr_opaquedata_t nil = {0};

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!u)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to opaque data object to initialize");

    if (b > (size_t) INT32_MAX)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid size for opaque data (%" PRIu64
            " bytes, must be <= INT32_MAX)",
            (uint64_t) b);

    *u = nil;
    if (b > 0)
    {

        u->packed_data = ctxt->alloc_fn (b);
        if (!u->packed_data)
            return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
    }
    u->size              = (int32_t) b;
    u->packed_alloc_size = (int32_t) b;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_opaquedata_create (
    exr_context_t ctxt, exr_attr_opaquedata_t* u, size_t b, const void* d)
{
    exr_result_t rv = exr_attr_opaquedata_init (ctxt, u, b);
    if (rv == EXR_ERR_SUCCESS)
    {
        if (d && u->packed_data) memcpy ((void*) u->packed_data, d, b);
    }

    return rv;
}

/**************************************/

exr_result_t
exr_attr_opaquedata_destroy (exr_context_t ctxt, exr_attr_opaquedata_t* ud)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (ud)
    {
        exr_attr_opaquedata_t nil = {0};
        if (ud->packed_data && ud->packed_alloc_size > 0)
            ctxt->free_fn (ud->packed_data);

        if (ud->unpacked_data && ud->destroy_unpacked_func_ptr)
            ud->destroy_unpacked_func_ptr (
                ctxt, ud->unpacked_data, ud->unpacked_size);
        *ud = nil;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_opaquedata_copy (
    exr_context_t                ctxt,
    exr_attr_opaquedata_t*       ud,
    const exr_attr_opaquedata_t* srcud)
{
    exr_result_t rv;
    if (!srcud) return EXR_ERR_INVALID_ARGUMENT;
    if (srcud->packed_data)
        return exr_attr_opaquedata_create (
            ctxt, ud, (size_t) srcud->size, srcud->packed_data);
    rv = exr_attr_opaquedata_init (ctxt, ud, 0);
    if (rv == EXR_ERR_SUCCESS)
        rv = exr_attr_opaquedata_set_unpacked (
            ctxt, ud, srcud->unpacked_data, srcud->unpacked_size);
    return rv;
}

/**************************************/

exr_result_t
exr_attr_opaquedata_unpack (
    exr_context_t ctxt, exr_attr_opaquedata_t* u, int32_t* sz, void** unpacked)
{
    exr_result_t rv;
    int32_t      tmpusz;
    void*        tmpuptr;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (sz) *sz = 0;
    if (unpacked) *unpacked = NULL;

    if (!u)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to opaque data object to initialize");

    if (u->unpacked_data)
    {
        if (sz) *sz = u->unpacked_size;
        if (unpacked) *unpacked = u->unpacked_data;
        return EXR_ERR_SUCCESS;
    }

    if (!u->unpack_func_ptr)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "No unpack provider specified for opaque data");
    rv = u->unpack_func_ptr (
        ctxt, u->packed_data, u->size, &(tmpusz), &(tmpuptr));
    if (rv == EXR_ERR_SUCCESS)
    {
        u->unpacked_size = tmpusz;
        u->unpacked_data = tmpuptr;
        if (sz) *sz = tmpusz;
        if (unpacked) *unpacked = tmpuptr;
    }

    return rv;
}

/**************************************/

exr_result_t
exr_attr_opaquedata_pack (
    exr_context_t ctxt, exr_attr_opaquedata_t* u, int32_t* sz, void** packed)
{
    exr_result_t rv;
    int32_t      nsize  = 0;
    void*        tmpptr = NULL;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (sz) *sz = 0;
    if (packed) *packed = NULL;

    if (!u)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to opaque data object to initialize");

    if (u->packed_data)
    {
        if (sz) *sz = u->size;
        if (packed) *packed = u->packed_data;
        return EXR_ERR_SUCCESS;
    }

    if (!u->pack_func_ptr)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "No pack provider specified for opaque data");

    rv = u->pack_func_ptr (
        ctxt, u->unpacked_data, u->unpacked_size, &nsize, NULL);
    if (rv != EXR_ERR_SUCCESS)
        return ctxt->print_error (
            ctxt,
            rv,
            "Pack function failed finding pack buffer size, unpacked size %d",
            u->unpacked_size);

    if (nsize > 0)
    {
        tmpptr = ctxt->alloc_fn ((size_t) nsize);
        if (tmpptr == NULL)
            return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);

        u->packed_alloc_size = nsize;

        rv = u->pack_func_ptr (
            ctxt, u->unpacked_data, u->unpacked_size, &nsize, tmpptr);
        if (rv != EXR_ERR_SUCCESS)
        {
            ctxt->free_fn (tmpptr);
            nsize                = u->packed_alloc_size;
            u->packed_alloc_size = 0;
            return ctxt->print_error (
                ctxt,
                rv,
                "Pack function failed to pack data, unpacked size %d, packed buffer size %d",
                u->unpacked_size,
                nsize);
        }
        u->size        = nsize;
        u->packed_data = tmpptr;
        if (sz) *sz = nsize;
        if (packed) *packed = tmpptr;

        if (u->destroy_unpacked_func_ptr)
            u->destroy_unpacked_func_ptr (
                ctxt, u->unpacked_data, u->unpacked_size);
        u->unpacked_data = NULL;
        u->unpacked_size = 0;
    }
    return rv;
}

/**************************************/

exr_result_t
exr_attr_opaquedata_set_unpacked (
    exr_context_t ctxt, exr_attr_opaquedata_t* u, void* unpacked, int32_t sz)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!u) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    /* TODO: do we care if the incoming unpacked data is null? */
    if (sz < 0)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Opaque data given invalid negative size (%d)",
            sz);

    if (u->unpacked_data)
    {
        if (u->destroy_unpacked_func_ptr)
            u->destroy_unpacked_func_ptr (
                ctxt, u->unpacked_data, u->unpacked_size);
    }
    u->unpacked_data = unpacked;
    u->unpacked_size = sz;

    if (u->packed_data)
    {
        if (u->packed_alloc_size > 0) ctxt->free_fn (u->packed_data);
        u->packed_data       = NULL;
        u->size              = 0;
        u->packed_alloc_size = 0;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_opaquedata_set_packed (
    exr_context_t          ctxt,
    exr_attr_opaquedata_t* u,
    const void*            packed,
    int32_t                sz)
{
    void* nmem;
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!u) return ctxt->standard_error (ctxt, EXR_ERR_INVALID_ARGUMENT);

    /* TODO: do we care if the incoming unpacked data is null? */
    if (sz < 0)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Opaque data given invalid negative size (%d)",
            sz);

    nmem = ctxt->alloc_fn ((size_t) sz);
    if (!nmem) return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);

    if (u->unpacked_data)
    {
        if (u->destroy_unpacked_func_ptr)
            u->destroy_unpacked_func_ptr (
                ctxt, u->unpacked_data, u->unpacked_size);
    }
    u->unpacked_data = NULL;
    u->unpacked_size = 0;

    if (u->packed_data)
    {
        if (u->packed_alloc_size > 0) ctxt->free_fn (u->packed_data);
        u->packed_data       = NULL;
        u->size              = 0;
        u->packed_alloc_size = 0;
    }

    u->packed_data       = nmem;
    u->size              = sz;
    u->packed_alloc_size = sz;
    if (packed) memcpy ((void*) u->packed_data, packed, (size_t) sz);

    return EXR_ERR_SUCCESS;
}
