/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_attr.h"

#include "internal_structs.h"

#include <string.h>

/**************************************/

exr_result_t
exr_attr_preview_init (
    exr_context_t ctxt, exr_attr_preview_t* p, uint32_t w, uint32_t h)
{
    exr_attr_preview_t nil   = {0};
    uint64_t           bytes = (uint64_t) w * (uint64_t) h * (uint64_t) 4;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (bytes > (size_t) INT32_MAX)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid very large size for preview image (%u x %u - %" PRIu64
            " bytes)",
            w,
            h,
            (uint64_t) bytes);

    if (!p)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to preview object to initialize");

    *p = nil;
    if (bytes > 0)
    {
        p->rgba = (uint8_t*) ctxt->alloc_fn (bytes);
        if (p->rgba == NULL)
            return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
        p->alloc_size = bytes;
        p->width      = w;
        p->height     = h;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_preview_create (
    exr_context_t       ctxt,
    exr_attr_preview_t* p,
    uint32_t            w,
    uint32_t            h,
    const uint8_t*      d)
{
    size_t copybytes = (size_t) w * (size_t) h * (size_t) 4;

    if (copybytes > 0 && !d)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid NULL preview rgba data for %u x %u preview",
            w,
            h);

    exr_result_t rv = exr_attr_preview_init (ctxt, p, w, h);
    if (rv == EXR_ERR_SUCCESS && copybytes > 0)
        memcpy (EXR_CONST_CAST (uint8_t*, p->rgba), d, copybytes);
    return rv;
}

/**************************************/

exr_result_t
exr_attr_preview_destroy (exr_context_t ctxt, exr_attr_preview_t* p)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (p)
    {
        exr_attr_preview_t nil = {0};
        if (p->rgba && p->alloc_size > 0)
            ctxt->free_fn (EXR_CONST_CAST (uint8_t*, p->rgba));
        *p = nil;
    }
    return EXR_ERR_SUCCESS;
}
