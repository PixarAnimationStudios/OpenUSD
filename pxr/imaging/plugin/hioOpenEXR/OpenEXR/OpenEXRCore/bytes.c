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
exr_attr_bytes_init (
    exr_context_t ctxt, exr_attr_bytes_t* u, uint32_t hint_length, size_t bytes_length)
{
    exr_attr_bytes_t nil = {0};

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!u)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to bytes data object to initialize");

    // Note the design choice to disallow lengths > INT32_MAX is borrowed
    // from other files and not rooted in any deep analysis.
    if (hint_length > (size_t) INT32_MAX)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid size for type hint (%" PRIu64
            " chars, must be <= INT32_MAX)",
            (uint64_t) hint_length);

    if (bytes_length > (size_t) INT32_MAX)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid size for bytes data (%" PRIu64
            " bytes, must be <= INT32_MAX)",
            (uint64_t) bytes_length);

    *u = nil;
    // Even though the type hint is supposed to be written and read using the
    // hint length, for people expecting to use C apis that rely on null bytes,
    // we add one now for safety.
    char* tmp = (char*) ctxt->alloc_fn (hint_length + 1);
    if (!tmp)
        return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
    tmp[hint_length] = '\0';
    u->type_hint = (const char *)tmp;
    u->hint_length = (size_t) hint_length;

    if (bytes_length > 0)
    {
        u->data = (uint8_t*) ctxt->alloc_fn (bytes_length);
        if (!u->data)
            return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
    }
    u->size = (size_t) bytes_length;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_bytes_create (
    exr_context_t ctxt,
    exr_attr_bytes_t* u,
    uint32_t h,
    size_t b,
    const void* t,
    const void* d)
{
    if (h > 0 && !t)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid NULL type hint for bytes attribute with hint length %u",
            h);

    if (b > 0 && !d)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid NULL bytes data for bytes attribute with size %zu",
            b);

    exr_result_t rv = exr_attr_bytes_init (ctxt, u, h, b);
    if (rv == EXR_ERR_SUCCESS)
    {
        if (b > 0 && u->data) memcpy ((void*) u->data, d, b);
        if (h > 0 && u->type_hint) memcpy ((void*) u->type_hint, t, h);
    }

    return rv;
}

/**************************************/

exr_result_t
exr_attr_bytes_destroy (exr_context_t ctxt, exr_attr_bytes_t* ud)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (ud)
    {
        exr_attr_bytes_t nil = {0};
        if (ud->data && ud->size > 0)
            ctxt->free_fn (ud->data);
        if (ud->type_hint)
            ctxt->free_fn ((void *)ud->type_hint);

        *ud = nil;
    }
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_bytes_copy (
    exr_context_t           ctxt,
    exr_attr_bytes_t*       ud,
    const exr_attr_bytes_t* src_ud)
{
    if (!src_ud) return EXR_ERR_INVALID_ARGUMENT;

    return exr_attr_bytes_create (
        ctxt,
        ud,
        src_ud->hint_length,
        src_ud->size,
        src_ud->type_hint,
        src_ud->data);
}
