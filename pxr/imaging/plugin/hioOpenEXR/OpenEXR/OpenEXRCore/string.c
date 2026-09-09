/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#include "internal_attr.h"

#include "internal_constants.h"
#include "internal_structs.h"

#include <string.h>

/**************************************/

exr_result_t
exr_attr_string_init (exr_context_t ctxt, exr_attr_string_t* s, int32_t len)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (len < 0)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Received request to allocate negative sized string (%d)",
            len);

    if (!s)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to string object to initialize");

    s->str = (char*) ctxt->alloc_fn ((size_t) (len + 1));
    if (s->str == NULL)
        return ctxt->standard_error (ctxt, EXR_ERR_OUT_OF_MEMORY);

    s->length     = len;
    s->alloc_size = len + 1;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_string_init_static_with_length (
    exr_context_t ctxt, exr_attr_string_t* s, const char* v, int32_t len)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (len < 0)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Received request to allocate negative sized string (%d)",
            len);

    if (!v)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid static string argument to initialize");

    if (!s)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid reference to string object to initialize");

    s->length     = len;
    s->alloc_size = 0;
    s->str        = v;
    return EXR_ERR_SUCCESS;
}

/**************************************/

exr_result_t
exr_attr_string_init_static (
    exr_context_t ctxt, exr_attr_string_t* s, const char* v)
{
    size_t  fulllen = 0;
    int32_t length  = 0;

    if (v)
    {
        fulllen = strlen (v);
        if (fulllen >= (size_t) INT32_MAX)
        {
            if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
            return ctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid string too long for attribute");
        }
        length = (int32_t) fulllen;
    }
    return exr_attr_string_init_static_with_length (ctxt, s, v, length);
}

/**************************************/

exr_result_t
exr_attr_string_create_with_length (
    exr_context_t ctxt, exr_attr_string_t* s, const char* d, int32_t len)
{
    exr_result_t rv;
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!s)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid (NULL) arguments to string create with length");

    rv = exr_attr_string_init (ctxt, s, len);
    if (rv == EXR_ERR_SUCCESS)
    {
        /* we know we own the string memory */
        char* outs = EXR_CONST_CAST (char*, s->str);
        /* someone might pass in a string shorter than requested length (or longer) */
        if (len > 0)
        {
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4996)
#elif defined(__clang__)
// clang: do nothing
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
            if (d)
            {
                size_t slen = strnlen( d, (size_t) len);
                memcpy (outs, d, slen);
                memset (outs + slen, '\0', ((size_t)len - slen) + 1);
            }
            else
                memset (outs, 0, (size_t) len);
#ifdef _MSC_VER
#    pragma warning(pop)
#elif defined(__clang__)
// clang: do nothing
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
        }
        else outs[0] = '\0';
    }
    return rv;
}

/**************************************/

exr_result_t
exr_attr_string_create (exr_context_t ctxt, exr_attr_string_t* s, const char* d)
{
    int32_t len     = 0;
    if (d)
    {
        size_t  fulllen = strlen (d);
        if (fulllen >= (size_t) INT32_MAX)
        {
            if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
            return ctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid string too long for attribute");
        }
        len = (int32_t) fulllen;
    }
    return exr_attr_string_create_with_length (ctxt, s, d, len);
}

/**************************************/

exr_result_t
exr_attr_string_set_with_length (
    exr_context_t ctxt, exr_attr_string_t* s, const char* d, int32_t len)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (!s)
        return ctxt->report_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Invalid string argument to string set");

    if (len < 0)
        return ctxt->print_error (
            ctxt,
            EXR_ERR_INVALID_ARGUMENT,
            "Received request to assign a negative sized string (%d)",
            len);

    if (s->alloc_size > len)
    {
        /* we own the memory */
        char* sstr;

        s->length = len;
        sstr      = EXR_CONST_CAST (char*, s->str);
        if (len > 0)
        {
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4996)
#elif defined(__clang__)
// clang: do nothing
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
            if (d)
            {
                size_t slen = strnlen( d, (size_t) len);
                memcpy (sstr, d, slen);
                memset (sstr + slen, '\0', ((size_t)len - slen) + 1);
            }
            else
                memset (sstr, 0, (size_t) (len + 1));
#ifdef _MSC_VER
#    pragma warning(pop)
#elif defined(__clang__)
// clang: do nothing
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
        }
        else sstr[0] = '\0';
        return EXR_ERR_SUCCESS;
    }
    exr_attr_string_destroy (ctxt, s);
    return exr_attr_string_create_with_length (ctxt, s, d, len);
}

/**************************************/

exr_result_t
exr_attr_string_set (exr_context_t ctxt, exr_attr_string_t* s, const char* d)
{
    size_t  fulllen = 0;
    int32_t len     = 0;
    if (d)
    {
        fulllen = strlen (d);
        if (fulllen >= (size_t) INT32_MAX)
        {
            if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;
            return ctxt->report_error (
                ctxt,
                EXR_ERR_INVALID_ARGUMENT,
                "Invalid string too long for attribute");
        }
        len = (int32_t) fulllen;
    }
    return exr_attr_string_set_with_length (ctxt, s, d, len);
}

/**************************************/

exr_result_t
exr_attr_string_destroy (exr_context_t ctxt, exr_attr_string_t* s)
{
    if (!ctxt) return EXR_ERR_MISSING_CONTEXT_ARG;

    if (s)
    {
        exr_attr_string_t nil = {0};
        if (s->str && s->alloc_size > 0)
            ctxt->free_fn ((char*) (uintptr_t) s->str);
        *s = nil;
    }
    return EXR_ERR_SUCCESS;
}
