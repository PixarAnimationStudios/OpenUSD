/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_attr.h"

#include "internal_structs.h"

#include <string.h>

/**************************************/

/* allocates ram, but does not fill any data */
exr_result_t
exr_attr_float_vector_init (
    exr_context_t ctxt, exr_attr_float_vector_t* fv, int32_t nent)
{
    exr_attr_float_vector_t nil   = {0};
    size_t                  bytes = (size_t) (nent) * sizeof (float);

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!fv)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to float vector object to initialize");

    *fv = nil;

    if (nent < 0)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Received request to allocate negative sized float vector (%d entries)",
            nent);

    if (bytes > (size_t) INT32_MAX)
    {
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid too large size for float vector (%d entries)",
            nent);
    }
    else if (bytes > 0)
    {
        fv->arr = (float*) ctxt->alloc_fn (bytes);
        if (fv->arr == NULL)
            return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);
        fv->length     = nent;
        fv->alloc_size = nent;
    }

    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_float_vector_init_static (
    exr_context_t            ctxt,
    exr_attr_float_vector_t* fv,
    const float*             arr,
    int32_t                  nent)
{
    exr_attr_float_vector_t nil = {0};

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (nent < 0)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Received request to allocate negative sized float vector (%d entries)",
            nent);
    if (!fv)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to float vector object to initialize");
    if (!arr)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to float array object to initialize");

    *fv            = nil;
    fv->arr        = arr;
    fv->length     = nent;
    fv->alloc_size = 0;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_float_vector_create (
    exr_context_t            ctxt,
    exr_attr_float_vector_t* fv,
    const float*             arr,
    int32_t                  nent)
{
    exr_result_t rv = EXR_ERR_UNKNOWN;

    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!fv || !arr)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid (NULL) arguments to float vector create");

    rv = exr_attr_float_vector_init (ctxt, fv, nent);
    if (rv == EXR_ERR_SUCCESS && nent > 0)
    {
        size_t bytes = (size_t) (nent) * sizeof (float);
        if (bytes <= (size_t) INT32_MAX)
        {
            float* outp = EXR_CONST_CAST (float*, fv->arr);
            memcpy (outp, arr, bytes);
        }
    }

    return rv;
}

/**************************************/

exr_result_t
exr_attr_float_vector_destroy (exr_context_t ctxt, exr_attr_float_vector_t* fv)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
    if (fv)
    {
        exr_attr_float_vector_t nil = {0};
        if (fv->arr && fv->alloc_size > 0)
            ctxt->free_fn (EXR_CONST_CAST (void*, fv->arr));
        *fv = nil;
    }
    return EXR_ERR_SUCCESS;
}
