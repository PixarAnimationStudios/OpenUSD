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

#ifndef AOM_AV1_COMMON_ALLOCCOMMON_H_
#define AOM_AV1_COMMON_ALLOCCOMMON_H_

#define INVALID_IDX -1  // Invalid buffer index.

#include "pxr/imaging/plugin/hioAvif/aom/config/aom_config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct AV1Common;
struct BufferPool;
struct CommonContexts;
struct CommonModeInfoParams;

void av1_remove_common(struct AV1Common *cm);

int av1_alloc_above_context_buffers(struct CommonContexts *above_contexts,
                                    int num_tile_rows, int num_mi_cols,
                                    int num_planes);
void av1_free_above_context_buffers(struct CommonContexts *above_contexts);
int av1_alloc_context_buffers(struct AV1Common *cm, int width, int height);
void av1_init_mi_buffers(struct CommonModeInfoParams *mi_params);
void av1_free_context_buffers(struct AV1Common *cm);

void av1_free_ref_frame_buffers(struct BufferPool *pool);
#if !CONFIG_REALTIME_ONLY
void av1_alloc_restoration_buffers(struct AV1Common *cm);
void av1_free_restoration_buffers(struct AV1Common *cm);
#endif

int av1_alloc_state_buffers(struct AV1Common *cm, int width, int height);
void av1_free_state_buffers(struct AV1Common *cm);

int av1_get_MBs(int width, int height);

#if CONFIG_LPF_MASK
int av1_alloc_loop_filter_mask(struct AV1Common *cm);
void av1_free_loop_filter_mask(struct AV1Common *cm);
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_COMMON_ALLOCCOMMON_H_
