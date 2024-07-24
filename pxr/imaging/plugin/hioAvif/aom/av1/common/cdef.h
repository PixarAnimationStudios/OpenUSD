/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */
#ifndef AOM_AV1_COMMON_CDEF_H_
#define AOM_AV1_COMMON_CDEF_H_

#define CDEF_STRENGTH_BITS 6

#define CDEF_PRI_STRENGTHS 16
#define CDEF_SEC_STRENGTHS 4

#include "pxr/imaging/plugin/hioAvif/aom/config/aom_config.h"

#include "pxr/imaging/plugin/hioAvif/aom/aom_integer.h"
#include "pxr/imaging/plugin/hioAvif/aom/aom_ports/mem.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/av1_common_int.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/cdef_block.h"

static INLINE int sign(int i) { return i < 0 ? -1 : 1; }

static INLINE int constrain(int diff, int threshold, int damping) {
  if (!threshold) return 0;

  const int shift = AOMMAX(0, damping - get_msb(threshold));
  return sign(diff) *
         AOMMIN(abs(diff), AOMMAX(0, threshold - (abs(diff) >> shift)));
}

#ifdef __cplusplus
extern "C" {
#endif

int av1_cdef_compute_sb_list(const CommonModeInfoParams *const mi_params,
                             int mi_row, int mi_col, cdef_list *dlist,
                             BLOCK_SIZE bsize);

/*!\brief Function for applying CDEF to a frame
 *
 * \ingroup in_loop_cdef
 * This function applies CDEF to a frame.
 *
 * \param[in, out]  frame       Compressed frame buffer
 * \param[in, out]  cm          Pointer to top level common structure
 * \param[in]       xd          Pointer to common current coding block structure
 *
 * \return Nothing is returned. Instead, the filtered frame is output in
 * \c frame.
 */
void av1_cdef_frame(YV12_BUFFER_CONFIG *frame, AV1_COMMON *cm, MACROBLOCKD *xd);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // AOM_AV1_COMMON_CDEF_H_
