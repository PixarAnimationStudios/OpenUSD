/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_ATTR_BYTES_H
#define OPENEXR_ATTR_BYTES_H

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @addtogroup InternalAttributeFunctions
 * @{
 */

exr_result_t
exr_attr_bytes_init (
    exr_context_t ctxt, exr_attr_bytes_t* odata, uint32_t hl, size_t dl);

exr_result_t
exr_attr_bytes_create (
    exr_context_t ctxt, exr_attr_bytes_t* odata, uint32_t hl, size_t dl,
    const void* th, const void* bd);

exr_result_t
exr_attr_bytes_destroy (exr_context_t ctxt, exr_attr_bytes_t* ud);

exr_result_t
exr_attr_bytes_copy (
    exr_context_t ctxt, exr_attr_bytes_t* ud, const exr_attr_bytes_t* srcud);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* OPENEXR_ATTR_BYTES_H */
