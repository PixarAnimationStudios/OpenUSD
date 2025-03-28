/*
 *
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "pxr/imaging/plugin/hioAvif/aom/config/aom_config.h"

#include "pxr/imaging/plugin/hioAvif/aom/aom_mem/aom_mem.h"

#include "pxr/imaging/plugin/hioAvif/aom/av1/common/alloccommon.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/av1_common_int.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/blockd.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/entropymode.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/entropymv.h"

int av1_get_MBs(int width, int height) {
  const int aligned_width = ALIGN_POWER_OF_TWO(width, 3);
  const int aligned_height = ALIGN_POWER_OF_TWO(height, 3);
  const int mi_cols = aligned_width >> MI_SIZE_LOG2;
  const int mi_rows = aligned_height >> MI_SIZE_LOG2;

  const int mb_cols = (mi_cols + 2) >> 2;
  const int mb_rows = (mi_rows + 2) >> 2;
  return mb_rows * mb_cols;
}

void av1_free_ref_frame_buffers(BufferPool *pool) {
  int i;

  for (i = 0; i < FRAME_BUFFERS; ++i) {
    if (pool->frame_bufs[i].ref_count > 0 &&
        pool->frame_bufs[i].raw_frame_buffer.data != NULL) {
      pool->release_fb_cb(pool->cb_priv, &pool->frame_bufs[i].raw_frame_buffer);
      pool->frame_bufs[i].raw_frame_buffer.data = NULL;
      pool->frame_bufs[i].raw_frame_buffer.size = 0;
      pool->frame_bufs[i].raw_frame_buffer.priv = NULL;
      pool->frame_bufs[i].ref_count = 0;
    }
    aom_free(pool->frame_bufs[i].mvs);
    pool->frame_bufs[i].mvs = NULL;
    aom_free(pool->frame_bufs[i].seg_map);
    pool->frame_bufs[i].seg_map = NULL;
    aom_free_frame_buffer(&pool->frame_bufs[i].buf);
  }
}

#if !CONFIG_REALTIME_ONLY
// Assumes cm->rst_info[p].restoration_unit_size is already initialized
void av1_alloc_restoration_buffers(AV1_COMMON *cm) {
  const int num_planes = av1_num_planes(cm);
  for (int p = 0; p < num_planes; ++p)
    av1_alloc_restoration_struct(cm, &cm->rst_info[p], p > 0);

  if (cm->rst_tmpbuf == NULL) {
    CHECK_MEM_ERROR(cm, cm->rst_tmpbuf,
                    (int32_t *)aom_memalign(16, RESTORATION_TMPBUF_SIZE));
  }

  if (cm->rlbs == NULL) {
    CHECK_MEM_ERROR(cm, cm->rlbs, aom_malloc(sizeof(RestorationLineBuffers)));
  }

  // For striped loop restoration, we divide each row of tiles into "stripes",
  // of height 64 luma pixels but with an offset by RESTORATION_UNIT_OFFSET
  // luma pixels to match the output from CDEF. We will need to store 2 *
  // RESTORATION_CTX_VERT lines of data for each stripe, and also need to be
  // able to quickly answer the question "Where is the <n>'th stripe for tile
  // row <m>?" To make that efficient, we generate the rst_last_stripe array.
  int num_stripes = 0;
  for (int i = 0; i < cm->tiles.rows; ++i) {
    TileInfo tile_info;
    av1_tile_set_row(&tile_info, cm, i);
    const int mi_h = tile_info.mi_row_end - tile_info.mi_row_start;
    const int ext_h = RESTORATION_UNIT_OFFSET + (mi_h << MI_SIZE_LOG2);
    const int tile_stripes = (ext_h + 63) / 64;
    num_stripes += tile_stripes;
  }

  // Now we need to allocate enough space to store the line buffers for the
  // stripes
  const int frame_w = cm->superres_upscaled_width;
  const int use_highbd = cm->seq_params.use_highbitdepth;

  for (int p = 0; p < num_planes; ++p) {
    const int is_uv = p > 0;
    const int ss_x = is_uv && cm->seq_params.subsampling_x;
    const int plane_w = ((frame_w + ss_x) >> ss_x) + 2 * RESTORATION_EXTRA_HORZ;
    const int stride = ALIGN_POWER_OF_TWO(plane_w, 5);
    const int buf_size = num_stripes * stride * RESTORATION_CTX_VERT
                         << use_highbd;
    RestorationStripeBoundaries *boundaries = &cm->rst_info[p].boundaries;

    if (buf_size != boundaries->stripe_boundary_size ||
        boundaries->stripe_boundary_above == NULL ||
        boundaries->stripe_boundary_below == NULL) {
      aom_free(boundaries->stripe_boundary_above);
      aom_free(boundaries->stripe_boundary_below);

      CHECK_MEM_ERROR(cm, boundaries->stripe_boundary_above,
                      (uint8_t *)aom_memalign(32, buf_size));
      CHECK_MEM_ERROR(cm, boundaries->stripe_boundary_below,
                      (uint8_t *)aom_memalign(32, buf_size));

      boundaries->stripe_boundary_size = buf_size;
    }
    boundaries->stripe_boundary_stride = stride;
  }
}

void av1_free_restoration_buffers(AV1_COMMON *cm) {
  int p;
  for (p = 0; p < MAX_MB_PLANE; ++p)
    av1_free_restoration_struct(&cm->rst_info[p]);
  aom_free(cm->rst_tmpbuf);
  cm->rst_tmpbuf = NULL;
  aom_free(cm->rlbs);
  cm->rlbs = NULL;
  for (p = 0; p < MAX_MB_PLANE; ++p) {
    RestorationStripeBoundaries *boundaries = &cm->rst_info[p].boundaries;
    aom_free(boundaries->stripe_boundary_above);
    aom_free(boundaries->stripe_boundary_below);
    boundaries->stripe_boundary_above = NULL;
    boundaries->stripe_boundary_below = NULL;
  }

  aom_free_frame_buffer(&cm->rst_frame);
}
#endif  // !CONFIG_REALTIME_ONLY

void av1_free_above_context_buffers(CommonContexts *above_contexts) {
  int i;
  const int num_planes = above_contexts->num_planes;

  for (int tile_row = 0; tile_row < above_contexts->num_tile_rows; tile_row++) {
    for (i = 0; i < num_planes; i++) {
      aom_free(above_contexts->entropy[i][tile_row]);
      above_contexts->entropy[i][tile_row] = NULL;
    }
    aom_free(above_contexts->partition[tile_row]);
    above_contexts->partition[tile_row] = NULL;

    aom_free(above_contexts->txfm[tile_row]);
    above_contexts->txfm[tile_row] = NULL;
  }
  for (i = 0; i < num_planes; i++) {
    aom_free(above_contexts->entropy[i]);
    above_contexts->entropy[i] = NULL;
  }
  aom_free(above_contexts->partition);
  above_contexts->partition = NULL;

  aom_free(above_contexts->txfm);
  above_contexts->txfm = NULL;

  above_contexts->num_tile_rows = 0;
  above_contexts->num_mi_cols = 0;
  above_contexts->num_planes = 0;
}

void av1_free_context_buffers(AV1_COMMON *cm) {
  cm->mi_params.free_mi(&cm->mi_params);

  av1_free_above_context_buffers(&cm->above_contexts);

#if CONFIG_LPF_MASK
  av1_free_loop_filter_mask(cm);
#endif
}

int av1_alloc_above_context_buffers(CommonContexts *above_contexts,
                                    int num_tile_rows, int num_mi_cols,
                                    int num_planes) {
  const int aligned_mi_cols =
      ALIGN_POWER_OF_TWO(num_mi_cols, MAX_MIB_SIZE_LOG2);

  // Allocate above context buffers
  above_contexts->num_tile_rows = num_tile_rows;
  above_contexts->num_mi_cols = aligned_mi_cols;
  above_contexts->num_planes = num_planes;
  for (int plane_idx = 0; plane_idx < num_planes; plane_idx++) {
    above_contexts->entropy[plane_idx] = (ENTROPY_CONTEXT **)aom_calloc(
        num_tile_rows, sizeof(above_contexts->entropy[0]));
    if (!above_contexts->entropy[plane_idx]) return 1;
  }

  above_contexts->partition = (PARTITION_CONTEXT **)aom_calloc(
      num_tile_rows, sizeof(above_contexts->partition));
  if (!above_contexts->partition) return 1;

  above_contexts->txfm =
      (TXFM_CONTEXT **)aom_calloc(num_tile_rows, sizeof(above_contexts->txfm));
  if (!above_contexts->txfm) return 1;

  for (int tile_row = 0; tile_row < num_tile_rows; tile_row++) {
    for (int plane_idx = 0; plane_idx < num_planes; plane_idx++) {
      above_contexts->entropy[plane_idx][tile_row] =
          (ENTROPY_CONTEXT *)aom_calloc(
              aligned_mi_cols, sizeof(*above_contexts->entropy[0][tile_row]));
      if (!above_contexts->entropy[plane_idx][tile_row]) return 1;
    }

    above_contexts->partition[tile_row] = (PARTITION_CONTEXT *)aom_calloc(
        aligned_mi_cols, sizeof(*above_contexts->partition[tile_row]));
    if (!above_contexts->partition[tile_row]) return 1;

    above_contexts->txfm[tile_row] = (TXFM_CONTEXT *)aom_calloc(
        aligned_mi_cols, sizeof(*above_contexts->txfm[tile_row]));
    if (!above_contexts->txfm[tile_row]) return 1;
  }

  return 0;
}

// Allocate the dynamically allocated arrays in 'mi_params' assuming
// 'mi_params->set_mb_mi()' was already called earlier to initialize the rest of
// the struct members.
static int alloc_mi(CommonModeInfoParams *mi_params) {
  const int aligned_mi_rows = calc_mi_size(mi_params->mi_rows);
  const int mi_grid_size = mi_params->mi_stride * aligned_mi_rows;
  const int alloc_size_1d = mi_size_wide[mi_params->mi_alloc_bsize];
  const int alloc_mi_size =
      mi_params->mi_alloc_stride * (aligned_mi_rows / alloc_size_1d);

  if (mi_params->mi_alloc_size < alloc_mi_size ||
      mi_params->mi_grid_size < mi_grid_size) {
    mi_params->free_mi(mi_params);

    mi_params->mi_alloc =
        aom_calloc(alloc_mi_size, sizeof(*mi_params->mi_alloc));
    if (!mi_params->mi_alloc) return 1;
    mi_params->mi_alloc_size = alloc_mi_size;

    mi_params->mi_grid_base = (MB_MODE_INFO **)aom_calloc(
        mi_grid_size, sizeof(*mi_params->mi_grid_base));
    if (!mi_params->mi_grid_base) return 1;
    mi_params->mi_grid_size = mi_grid_size;

    mi_params->tx_type_map =
        aom_calloc(mi_grid_size, sizeof(*mi_params->tx_type_map));
    if (!mi_params->tx_type_map) return 1;
  }

  return 0;
}

int av1_alloc_context_buffers(AV1_COMMON *cm, int width, int height) {
  CommonModeInfoParams *const mi_params = &cm->mi_params;
  mi_params->set_mb_mi(mi_params, width, height);
  if (alloc_mi(mi_params)) goto fail;
  return 0;

fail:
  // clear the mi_* values to force a realloc on resync
  mi_params->set_mb_mi(mi_params, 0, 0);
  av1_free_context_buffers(cm);
  return 1;
}

void av1_remove_common(AV1_COMMON *cm) {
  av1_free_context_buffers(cm);

  aom_free(cm->fc);
  cm->fc = NULL;
  aom_free(cm->default_frame_context);
  cm->default_frame_context = NULL;
}

void av1_init_mi_buffers(CommonModeInfoParams *mi_params) {
  mi_params->setup_mi(mi_params);
}

#if CONFIG_LPF_MASK
int av1_alloc_loop_filter_mask(AV1_COMMON *cm) {
  aom_free(cm->lf.lfm);
  cm->lf.lfm = NULL;

  // Each lfm holds bit masks for all the 4x4 blocks in a max
  // 64x64 (128x128 for ext_partitions) region.  The stride
  // and rows are rounded up / truncated to a multiple of 16
  // (32 for ext_partition).
  cm->lf.lfm_stride =
      (cm->mi_params.mi_cols + (MI_SIZE_64X64 - 1)) >> MIN_MIB_SIZE_LOG2;
  cm->lf.lfm_num =
      ((cm->mi_params.mi_rows + (MI_SIZE_64X64 - 1)) >> MIN_MIB_SIZE_LOG2) *
      cm->lf.lfm_stride;
  cm->lf.lfm =
      (LoopFilterMask *)aom_calloc(cm->lf.lfm_num, sizeof(*cm->lf.lfm));
  if (!cm->lf.lfm) return 1;

  unsigned int i;
  for (i = 0; i < cm->lf.lfm_num; ++i) av1_zero(cm->lf.lfm[i]);

  return 0;
}

void av1_free_loop_filter_mask(AV1_COMMON *cm) {
  if (cm->lf.lfm == NULL) return;

  aom_free(cm->lf.lfm);
  cm->lf.lfm = NULL;
  cm->lf.lfm_num = 0;
  cm->lf.lfm_stride = 0;
}
#endif
