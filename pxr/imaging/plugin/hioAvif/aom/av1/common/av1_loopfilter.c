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

#include <math.h>

#include "pxr/imaging/plugin/hioAvif/aom/config/aom_config.h"
#include "pxr/imaging/plugin/hioAvif/aom/config/aom_dsp_rtcd.h"

#include "pxr/imaging/plugin/hioAvif/aom/aom_dsp/aom_dsp_common.h"
#include "pxr/imaging/plugin/hioAvif/aom/aom_mem/aom_mem.h"
#include "pxr/imaging/plugin/hioAvif/aom/aom_ports/mem.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/av1_common_int.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/av1_loopfilter.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/reconinter.h"
#include "pxr/imaging/plugin/hioAvif/aom/av1/common/seg_common.h"

static const SEG_LVL_FEATURES seg_lvl_lf_lut[MAX_MB_PLANE][2] = {
  { SEG_LVL_ALT_LF_Y_V, SEG_LVL_ALT_LF_Y_H },
  { SEG_LVL_ALT_LF_U, SEG_LVL_ALT_LF_U },
  { SEG_LVL_ALT_LF_V, SEG_LVL_ALT_LF_V }
};

static const int delta_lf_id_lut[MAX_MB_PLANE][2] = { { 0, 1 },
                                                      { 2, 2 },
                                                      { 3, 3 } };

static const int mode_lf_lut[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // INTRA_MODES
  1, 1, 0, 1,                             // INTER_MODES (GLOBALMV == 0)
  1, 1, 1, 1, 1, 1, 0, 1  // INTER_COMPOUND_MODES (GLOBAL_GLOBALMV == 0)
};

static void update_sharpness(loop_filter_info_n *lfi, int sharpness_lvl) {
  int lvl;

  // For each possible value for the loop filter fill out limits
  for (lvl = 0; lvl <= MAX_LOOP_FILTER; lvl++) {
    // Set loop filter parameters that control sharpness.
    int block_inside_limit = lvl >> ((sharpness_lvl > 0) + (sharpness_lvl > 4));

    if (sharpness_lvl > 0) {
      if (block_inside_limit > (9 - sharpness_lvl))
        block_inside_limit = (9 - sharpness_lvl);
    }

    if (block_inside_limit < 1) block_inside_limit = 1;

    memset(lfi->lfthr[lvl].lim, block_inside_limit, SIMD_WIDTH);
    memset(lfi->lfthr[lvl].mblim, (2 * (lvl + 2) + block_inside_limit),
           SIMD_WIDTH);
  }
}

uint8_t av1_get_filter_level(const AV1_COMMON *cm,
                             const loop_filter_info_n *lfi_n, const int dir_idx,
                             int plane, const MB_MODE_INFO *mbmi) {
  const int segment_id = mbmi->segment_id;
  if (cm->delta_q_info.delta_lf_present_flag) {
    int8_t delta_lf;
    if (cm->delta_q_info.delta_lf_multi) {
      const int delta_lf_idx = delta_lf_id_lut[plane][dir_idx];
      delta_lf = mbmi->delta_lf[delta_lf_idx];
    } else {
      delta_lf = mbmi->delta_lf_from_base;
    }
    int base_level;
    if (plane == 0)
      base_level = cm->lf.filter_level[dir_idx];
    else if (plane == 1)
      base_level = cm->lf.filter_level_u;
    else
      base_level = cm->lf.filter_level_v;
    int lvl_seg = clamp(delta_lf + base_level, 0, MAX_LOOP_FILTER);
    assert(plane >= 0 && plane <= 2);
    const int seg_lf_feature_id = seg_lvl_lf_lut[plane][dir_idx];
    if (segfeature_active(&cm->seg, segment_id, seg_lf_feature_id)) {
      const int data = get_segdata(&cm->seg, segment_id, seg_lf_feature_id);
      lvl_seg = clamp(lvl_seg + data, 0, MAX_LOOP_FILTER);
    }

    if (cm->lf.mode_ref_delta_enabled) {
      const int scale = 1 << (lvl_seg >> 5);
      lvl_seg += cm->lf.ref_deltas[mbmi->ref_frame[0]] * scale;
      if (mbmi->ref_frame[0] > INTRA_FRAME)
        lvl_seg += cm->lf.mode_deltas[mode_lf_lut[mbmi->mode]] * scale;
      lvl_seg = clamp(lvl_seg, 0, MAX_LOOP_FILTER);
    }
    return lvl_seg;
  } else {
    return lfi_n->lvl[plane][segment_id][dir_idx][mbmi->ref_frame[0]]
                     [mode_lf_lut[mbmi->mode]];
  }
}

void av1_loop_filter_init(AV1_COMMON *cm) {
  assert(MB_MODE_COUNT == NELEMENTS(mode_lf_lut));
  loop_filter_info_n *lfi = &cm->lf_info;
  struct loopfilter *lf = &cm->lf;
  int lvl;

  lf->combine_vert_horz_lf = 1;

  // init limits for given sharpness
  update_sharpness(lfi, lf->sharpness_level);

  // init hev threshold const vectors
  for (lvl = 0; lvl <= MAX_LOOP_FILTER; lvl++)
    memset(lfi->lfthr[lvl].hev_thr, (lvl >> 4), SIMD_WIDTH);
}

// Update the loop filter for the current frame.
// This should be called before loop_filter_rows(),
// av1_loop_filter_frame() calls this function directly.
void av1_loop_filter_frame_init(AV1_COMMON *cm, int plane_start,
                                int plane_end) {
  int filt_lvl[MAX_MB_PLANE], filt_lvl_r[MAX_MB_PLANE];
  int plane;
  int seg_id;
  // n_shift is the multiplier for lf_deltas
  // the multiplier is 1 for when filter_lvl is between 0 and 31;
  // 2 when filter_lvl is between 32 and 63
  loop_filter_info_n *const lfi = &cm->lf_info;
  struct loopfilter *const lf = &cm->lf;
  const struct segmentation *const seg = &cm->seg;

  // update sharpness limits
  update_sharpness(lfi, lf->sharpness_level);

  filt_lvl[0] = cm->lf.filter_level[0];
  filt_lvl[1] = cm->lf.filter_level_u;
  filt_lvl[2] = cm->lf.filter_level_v;

  filt_lvl_r[0] = cm->lf.filter_level[1];
  filt_lvl_r[1] = cm->lf.filter_level_u;
  filt_lvl_r[2] = cm->lf.filter_level_v;

  assert(plane_start >= AOM_PLANE_Y);
  assert(plane_end <= MAX_MB_PLANE);

  for (plane = plane_start; plane < plane_end; plane++) {
    if (plane == 0 && !filt_lvl[0] && !filt_lvl_r[0])
      break;
    else if (plane == 1 && !filt_lvl[1])
      continue;
    else if (plane == 2 && !filt_lvl[2])
      continue;

    for (seg_id = 0; seg_id < MAX_SEGMENTS; seg_id++) {
      for (int dir = 0; dir < 2; ++dir) {
        int lvl_seg = (dir == 0) ? filt_lvl[plane] : filt_lvl_r[plane];
        const int seg_lf_feature_id = seg_lvl_lf_lut[plane][dir];
        if (segfeature_active(seg, seg_id, seg_lf_feature_id)) {
          const int data = get_segdata(&cm->seg, seg_id, seg_lf_feature_id);
          lvl_seg = clamp(lvl_seg + data, 0, MAX_LOOP_FILTER);
        }

        if (!lf->mode_ref_delta_enabled) {
          // we could get rid of this if we assume that deltas are set to
          // zero when not in use; encoder always uses deltas
          memset(lfi->lvl[plane][seg_id][dir], lvl_seg,
                 sizeof(lfi->lvl[plane][seg_id][dir]));
        } else {
          int ref, mode;
          const int scale = 1 << (lvl_seg >> 5);
          const int intra_lvl = lvl_seg + lf->ref_deltas[INTRA_FRAME] * scale;
          lfi->lvl[plane][seg_id][dir][INTRA_FRAME][0] =
              clamp(intra_lvl, 0, MAX_LOOP_FILTER);

          for (ref = LAST_FRAME; ref < REF_FRAMES; ++ref) {
            for (mode = 0; mode < MAX_MODE_LF_DELTAS; ++mode) {
              const int inter_lvl = lvl_seg + lf->ref_deltas[ref] * scale +
                                    lf->mode_deltas[mode] * scale;
              lfi->lvl[plane][seg_id][dir][ref][mode] =
                  clamp(inter_lvl, 0, MAX_LOOP_FILTER);
            }
          }
        }
      }
    }
  }
}

static TX_SIZE get_transform_size(const MACROBLOCKD *const xd,
                                  const MB_MODE_INFO *const mbmi,
                                  const EDGE_DIR edge_dir, const int mi_row,
                                  const int mi_col, const int plane,
                                  const struct macroblockd_plane *plane_ptr) {
  assert(mbmi != NULL);
  if (xd && xd->lossless[mbmi->segment_id]) return TX_4X4;

  TX_SIZE tx_size =
      (plane == AOM_PLANE_Y)
          ? mbmi->tx_size
          : av1_get_max_uv_txsize(mbmi->bsize, plane_ptr->subsampling_x,
                                  plane_ptr->subsampling_y);
  assert(tx_size < TX_SIZES_ALL);
  if ((plane == AOM_PLANE_Y) && is_inter_block(mbmi) && !mbmi->skip_txfm) {
    const BLOCK_SIZE sb_type = mbmi->bsize;
    const int blk_row = mi_row & (mi_size_high[sb_type] - 1);
    const int blk_col = mi_col & (mi_size_wide[sb_type] - 1);
    const TX_SIZE mb_tx_size =
        mbmi->inter_tx_size[av1_get_txb_size_index(sb_type, blk_row, blk_col)];
    assert(mb_tx_size < TX_SIZES_ALL);
    tx_size = mb_tx_size;
  }

  // since in case of chrominance or non-square transform need to convert
  // transform size into transform size in particular direction.
  // for vertical edge, filter direction is horizontal, for horizontal
  // edge, filter direction is vertical.
  tx_size = (VERT_EDGE == edge_dir) ? txsize_horz_map[tx_size]
                                    : txsize_vert_map[tx_size];
  return tx_size;
}

typedef struct AV1_DEBLOCKING_PARAMETERS {
  // length of the filter applied to the outer edge
  uint32_t filter_length;
  // deblocking limits
  const uint8_t *lim;
  const uint8_t *mblim;
  const uint8_t *hev_thr;
} AV1_DEBLOCKING_PARAMETERS;

// Return TX_SIZE from get_transform_size(), so it is plane and direction
// aware
static TX_SIZE set_lpf_parameters(
    AV1_DEBLOCKING_PARAMETERS *const params, const ptrdiff_t mode_step,
    const AV1_COMMON *const cm, const MACROBLOCKD *const xd,
    const EDGE_DIR edge_dir, const uint32_t x, const uint32_t y,
    const int plane, const struct macroblockd_plane *const plane_ptr) {
  // reset to initial values
  params->filter_length = 0;

  // no deblocking is required
  const uint32_t width = plane_ptr->dst.width;
  const uint32_t height = plane_ptr->dst.height;
  if ((width <= x) || (height <= y)) {
    // just return the smallest transform unit size
    return TX_4X4;
  }

  const uint32_t scale_horz = plane_ptr->subsampling_x;
  const uint32_t scale_vert = plane_ptr->subsampling_y;
  // for sub8x8 block, chroma prediction mode is obtained from the bottom/right
  // mi structure of the co-located 8x8 luma block. so for chroma plane, mi_row
  // and mi_col should map to the bottom/right mi structure, i.e, both mi_row
  // and mi_col should be odd number for chroma plane.
  const int mi_row = scale_vert | ((y << scale_vert) >> MI_SIZE_LOG2);
  const int mi_col = scale_horz | ((x << scale_horz) >> MI_SIZE_LOG2);
  MB_MODE_INFO **mi =
      cm->mi_params.mi_grid_base + mi_row * cm->mi_params.mi_stride + mi_col;
  const MB_MODE_INFO *mbmi = mi[0];
  // If current mbmi is not correctly setup, return an invalid value to stop
  // filtering. One example is that if this tile is not coded, then its mbmi
  // it not set up.
  if (mbmi == NULL) return TX_INVALID;

  const TX_SIZE ts =
      get_transform_size(xd, mi[0], edge_dir, mi_row, mi_col, plane, plane_ptr);

  {
    const uint32_t coord = (VERT_EDGE == edge_dir) ? (x) : (y);
    const uint32_t transform_masks =
        edge_dir == VERT_EDGE ? tx_size_wide[ts] - 1 : tx_size_high[ts] - 1;
    const int32_t tu_edge = (coord & transform_masks) ? (0) : (1);

    if (!tu_edge) return ts;

    // prepare outer edge parameters. deblock the edge if it's an edge of a TU
    {
      const uint32_t curr_level =
          av1_get_filter_level(cm, &cm->lf_info, edge_dir, plane, mbmi);
      const int curr_skipped = mbmi->skip_txfm && is_inter_block(mbmi);
      uint32_t level = curr_level;
      if (coord) {
        {
          const MB_MODE_INFO *const mi_prev = *(mi - mode_step);
          if (mi_prev == NULL) return TX_INVALID;
          const int pv_row =
              (VERT_EDGE == edge_dir) ? (mi_row) : (mi_row - (1 << scale_vert));
          const int pv_col =
              (VERT_EDGE == edge_dir) ? (mi_col - (1 << scale_horz)) : (mi_col);
          const TX_SIZE pv_ts = get_transform_size(
              xd, mi_prev, edge_dir, pv_row, pv_col, plane, plane_ptr);

          const uint32_t pv_lvl =
              av1_get_filter_level(cm, &cm->lf_info, edge_dir, plane, mi_prev);

          const int pv_skip_txfm =
              mi_prev->skip_txfm && is_inter_block(mi_prev);
          const BLOCK_SIZE bsize = get_plane_block_size(
              mbmi->bsize, plane_ptr->subsampling_x, plane_ptr->subsampling_y);
          assert(bsize < BLOCK_SIZES_ALL);
          const int prediction_masks = edge_dir == VERT_EDGE
                                           ? block_size_wide[bsize] - 1
                                           : block_size_high[bsize] - 1;
          const int32_t pu_edge = !(coord & prediction_masks);
          // if the current and the previous blocks are skipped,
          // deblock the edge if the edge belongs to a PU's edge only.
          if ((curr_level || pv_lvl) &&
              (!pv_skip_txfm || !curr_skipped || pu_edge)) {
            const TX_SIZE min_ts = AOMMIN(ts, pv_ts);
            if (TX_4X4 >= min_ts) {
              params->filter_length = 4;
            } else if (TX_8X8 == min_ts) {
              if (plane != 0)
                params->filter_length = 6;
              else
                params->filter_length = 8;
            } else {
              params->filter_length = 14;
              // No wide filtering for chroma plane
              if (plane != 0) {
                params->filter_length = 6;
              }
            }

            // update the level if the current block is skipped,
            // but the previous one is not
            level = (curr_level) ? (curr_level) : (pv_lvl);
          }
        }
      }
      // prepare common parameters
      if (params->filter_length) {
        const loop_filter_thresh *const limits = cm->lf_info.lfthr + level;
        params->lim = limits->lim;
        params->mblim = limits->mblim;
        params->hev_thr = limits->hev_thr;
      }
    }
  }

  return ts;
}

void av1_filter_block_plane_vert(const AV1_COMMON *const cm,
                                 const MACROBLOCKD *const xd, const int plane,
                                 const MACROBLOCKD_PLANE *const plane_ptr,
                                 const uint32_t mi_row, const uint32_t mi_col) {
  const uint32_t scale_horz = plane_ptr->subsampling_x;
  const uint32_t scale_vert = plane_ptr->subsampling_y;
  uint8_t *const dst_ptr = plane_ptr->dst.buf;
  const int dst_stride = plane_ptr->dst.stride;
  const int y_range = (MAX_MIB_SIZE >> scale_vert);
  const int x_range = (MAX_MIB_SIZE >> scale_horz);
  for (int y = 0; y < y_range; y++) {
    uint8_t *p = dst_ptr + y * MI_SIZE * dst_stride;
    for (int x = 0; x < x_range;) {
      // inner loop always filter vertical edges in a MI block. If MI size
      // is 8x8, it will filter the vertical edge aligned with a 8x8 block.
      // If 4x4 transform is used, it will then filter the internal edge
      //  aligned with a 4x4 block
      const uint32_t curr_x = ((mi_col * MI_SIZE) >> scale_horz) + x * MI_SIZE;
      const uint32_t curr_y = ((mi_row * MI_SIZE) >> scale_vert) + y * MI_SIZE;
      uint32_t advance_units;
      TX_SIZE tx_size;
      AV1_DEBLOCKING_PARAMETERS params;
      memset(&params, 0, sizeof(params));

      tx_size =
          set_lpf_parameters(&params, ((ptrdiff_t)1 << scale_horz), cm, xd,
                             VERT_EDGE, curr_x, curr_y, plane, plane_ptr);
      if (tx_size == TX_INVALID) {
        params.filter_length = 0;
        tx_size = TX_4X4;
      }

#if CONFIG_AV1_HIGHBITDEPTH
      const int use_highbitdepth = cm->seq_params.use_highbitdepth;
      const aom_bit_depth_t bit_depth = cm->seq_params.bit_depth;
      switch (params.filter_length) {
        // apply 4-tap filtering
        case 4:
          if (use_highbitdepth)
            aom_highbd_lpf_vertical_4(CONVERT_TO_SHORTPTR(p), dst_stride,
                                      params.mblim, params.lim, params.hev_thr,
                                      bit_depth);
          else
            aom_lpf_vertical_4(p, dst_stride, params.mblim, params.lim,
                               params.hev_thr);
          break;
        case 6:  // apply 6-tap filter for chroma plane only
          assert(plane != 0);
          if (use_highbitdepth)
            aom_highbd_lpf_vertical_6(CONVERT_TO_SHORTPTR(p), dst_stride,
                                      params.mblim, params.lim, params.hev_thr,
                                      bit_depth);
          else
            aom_lpf_vertical_6(p, dst_stride, params.mblim, params.lim,
                               params.hev_thr);
          break;
        // apply 8-tap filtering
        case 8:
          if (use_highbitdepth)
            aom_highbd_lpf_vertical_8(CONVERT_TO_SHORTPTR(p), dst_stride,
                                      params.mblim, params.lim, params.hev_thr,
                                      bit_depth);
          else
            aom_lpf_vertical_8(p, dst_stride, params.mblim, params.lim,
                               params.hev_thr);
          break;
        // apply 14-tap filtering
        case 14:
          if (use_highbitdepth)
            aom_highbd_lpf_vertical_14(CONVERT_TO_SHORTPTR(p), dst_stride,
                                       params.mblim, params.lim, params.hev_thr,
                                       bit_depth);
          else
            aom_lpf_vertical_14(p, dst_stride, params.mblim, params.lim,
                                params.hev_thr);
          break;
        // no filtering
        default: break;
      }
#else
      switch (params.filter_length) {
        // apply 4-tap filtering
        case 4:
          aom_lpf_vertical_4(p, dst_stride, params.mblim, params.lim,
                             params.hev_thr);
          break;
        case 6:  // apply 6-tap filter for chroma plane only
          assert(plane != 0);
          aom_lpf_vertical_6(p, dst_stride, params.mblim, params.lim,
                             params.hev_thr);
          break;
        // apply 8-tap filtering
        case 8:
          aom_lpf_vertical_8(p, dst_stride, params.mblim, params.lim,
                             params.hev_thr);
          break;
        // apply 14-tap filtering
        case 14:
          aom_lpf_vertical_14(p, dst_stride, params.mblim, params.lim,
                              params.hev_thr);
          break;
        // no filtering
        default: break;
      }
#endif  // CONFIG_AV1_HIGHBITDEPTH
      // advance the destination pointer
      advance_units = tx_size_wide_unit[tx_size];
      x += advance_units;
      p += advance_units * MI_SIZE;
    }
  }
}

void av1_filter_block_plane_horz(const AV1_COMMON *const cm,
                                 const MACROBLOCKD *const xd, const int plane,
                                 const MACROBLOCKD_PLANE *const plane_ptr,
                                 const uint32_t mi_row, const uint32_t mi_col) {
  const uint32_t scale_horz = plane_ptr->subsampling_x;
  const uint32_t scale_vert = plane_ptr->subsampling_y;
  uint8_t *const dst_ptr = plane_ptr->dst.buf;
  const int dst_stride = plane_ptr->dst.stride;
  const int y_range = (MAX_MIB_SIZE >> scale_vert);
  const int x_range = (MAX_MIB_SIZE >> scale_horz);
  for (int x = 0; x < x_range; x++) {
    uint8_t *p = dst_ptr + x * MI_SIZE;
    for (int y = 0; y < y_range;) {
      // inner loop always filter vertical edges in a MI block. If MI size
      // is 8x8, it will first filter the vertical edge aligned with a 8x8
      // block. If 4x4 transform is used, it will then filter the internal
      // edge aligned with a 4x4 block
      const uint32_t curr_x = ((mi_col * MI_SIZE) >> scale_horz) + x * MI_SIZE;
      const uint32_t curr_y = ((mi_row * MI_SIZE) >> scale_vert) + y * MI_SIZE;
      uint32_t advance_units;
      TX_SIZE tx_size;
      AV1_DEBLOCKING_PARAMETERS params;
      memset(&params, 0, sizeof(params));

      tx_size = set_lpf_parameters(
          &params, (cm->mi_params.mi_stride << scale_vert), cm, xd, HORZ_EDGE,
          curr_x, curr_y, plane, plane_ptr);
      if (tx_size == TX_INVALID) {
        params.filter_length = 0;
        tx_size = TX_4X4;
      }

#if CONFIG_AV1_HIGHBITDEPTH
      const int use_highbitdepth = cm->seq_params.use_highbitdepth;
      const aom_bit_depth_t bit_depth = cm->seq_params.bit_depth;
      switch (params.filter_length) {
        // apply 4-tap filtering
        case 4:
          if (use_highbitdepth)
            aom_highbd_lpf_horizontal_4(CONVERT_TO_SHORTPTR(p), dst_stride,
                                        params.mblim, params.lim,
                                        params.hev_thr, bit_depth);
          else
            aom_lpf_horizontal_4(p, dst_stride, params.mblim, params.lim,
                                 params.hev_thr);
          break;
        // apply 6-tap filtering
        case 6:
          assert(plane != 0);
          if (use_highbitdepth)
            aom_highbd_lpf_horizontal_6(CONVERT_TO_SHORTPTR(p), dst_stride,
                                        params.mblim, params.lim,
                                        params.hev_thr, bit_depth);
          else
            aom_lpf_horizontal_6(p, dst_stride, params.mblim, params.lim,
                                 params.hev_thr);
          break;
        // apply 8-tap filtering
        case 8:
          if (use_highbitdepth)
            aom_highbd_lpf_horizontal_8(CONVERT_TO_SHORTPTR(p), dst_stride,
                                        params.mblim, params.lim,
                                        params.hev_thr, bit_depth);
          else
            aom_lpf_horizontal_8(p, dst_stride, params.mblim, params.lim,
                                 params.hev_thr);
          break;
        // apply 14-tap filtering
        case 14:
          if (use_highbitdepth)
            aom_highbd_lpf_horizontal_14(CONVERT_TO_SHORTPTR(p), dst_stride,
                                         params.mblim, params.lim,
                                         params.hev_thr, bit_depth);
          else
            aom_lpf_horizontal_14(p, dst_stride, params.mblim, params.lim,
                                  params.hev_thr);
          break;
        // no filtering
        default: break;
      }
#else
      switch (params.filter_length) {
        // apply 4-tap filtering
        case 4:
          aom_lpf_horizontal_4(p, dst_stride, params.mblim, params.lim,
                               params.hev_thr);
          break;
        // apply 6-tap filtering
        case 6:
          assert(plane != 0);
          aom_lpf_horizontal_6(p, dst_stride, params.mblim, params.lim,
                               params.hev_thr);
          break;
        // apply 8-tap filtering
        case 8:
          aom_lpf_horizontal_8(p, dst_stride, params.mblim, params.lim,
                               params.hev_thr);
          break;
        // apply 14-tap filtering
        case 14:
          aom_lpf_horizontal_14(p, dst_stride, params.mblim, params.lim,
                                params.hev_thr);
          break;
        // no filtering
        default: break;
      }
#endif  // CONFIG_AV1_HIGHBITDEPTH

      // advance the destination pointer
      advance_units = tx_size_high_unit[tx_size];
      y += advance_units;
      p += advance_units * dst_stride * MI_SIZE;
    }
  }
}

void av1_filter_block_plane_vert_test(const AV1_COMMON *const cm,
                                      const MACROBLOCKD *const xd,
                                      const int plane,
                                      const MACROBLOCKD_PLANE *const plane_ptr,
                                      const uint32_t mi_row,
                                      const uint32_t mi_col) {
  const uint32_t scale_horz = plane_ptr->subsampling_x;
  const uint32_t scale_vert = plane_ptr->subsampling_y;
  const int y_range = cm->mi_params.mi_rows >> scale_vert;
  const int x_range = cm->mi_params.mi_cols >> scale_horz;
  for (int y = 0; y < y_range; y++) {
    for (int x = 0; x < x_range;) {
      // inner loop always filter vertical edges in a MI block. If MI size
      // is 8x8, it will filter the vertical edge aligned with a 8x8 block.
      // If 4x4 transform is used, it will then filter the internal edge
      //  aligned with a 4x4 block
      const uint32_t curr_x = ((mi_col * MI_SIZE) >> scale_horz) + x * MI_SIZE;
      const uint32_t curr_y = ((mi_row * MI_SIZE) >> scale_vert) + y * MI_SIZE;
      uint32_t advance_units;
      TX_SIZE tx_size;
      AV1_DEBLOCKING_PARAMETERS params;
      memset(&params, 0, sizeof(params));

      tx_size =
          set_lpf_parameters(&params, ((ptrdiff_t)1 << scale_horz), cm, xd,
                             VERT_EDGE, curr_x, curr_y, plane, plane_ptr);
      if (tx_size == TX_INVALID) {
        params.filter_length = 0;
        tx_size = TX_4X4;
      }

      // advance the destination pointer
      advance_units = tx_size_wide_unit[tx_size];
      x += advance_units;
    }
  }
}

void av1_filter_block_plane_horz_test(const AV1_COMMON *const cm,
                                      const MACROBLOCKD *const xd,
                                      const int plane,
                                      const MACROBLOCKD_PLANE *const plane_ptr,
                                      const uint32_t mi_row,
                                      const uint32_t mi_col) {
  const uint32_t scale_horz = plane_ptr->subsampling_x;
  const uint32_t scale_vert = plane_ptr->subsampling_y;
  const int y_range = cm->mi_params.mi_rows >> scale_vert;
  const int x_range = cm->mi_params.mi_cols >> scale_horz;
  for (int x = 0; x < x_range; x++) {
    for (int y = 0; y < y_range;) {
      // inner loop always filter vertical edges in a MI block. If MI size
      // is 8x8, it will first filter the vertical edge aligned with a 8x8
      // block. If 4x4 transform is used, it will then filter the internal
      // edge aligned with a 4x4 block
      const uint32_t curr_x = ((mi_col * MI_SIZE) >> scale_horz) + x * MI_SIZE;
      const uint32_t curr_y = ((mi_row * MI_SIZE) >> scale_vert) + y * MI_SIZE;
      uint32_t advance_units;
      TX_SIZE tx_size;
      AV1_DEBLOCKING_PARAMETERS params;
      memset(&params, 0, sizeof(params));

      tx_size = set_lpf_parameters(
          &params, (cm->mi_params.mi_stride << scale_vert), cm, xd, HORZ_EDGE,
          curr_x, curr_y, plane, plane_ptr);
      if (tx_size == TX_INVALID) {
        params.filter_length = 0;
        tx_size = TX_4X4;
      }

      // advance the destination pointer
      advance_units = tx_size_high_unit[tx_size];
      y += advance_units;
    }
  }
}

static void loop_filter_rows(YV12_BUFFER_CONFIG *frame_buffer, AV1_COMMON *cm,
                             MACROBLOCKD *xd, int start, int stop,
#if CONFIG_LPF_MASK
                             int is_decoding,
#endif
                             int plane_start, int plane_end) {
  struct macroblockd_plane *pd = xd->plane;
  const int col_start = 0;
  const int col_end = cm->mi_params.mi_cols;
  int mi_row, mi_col;
  int plane;

#if CONFIG_LPF_MASK
  if (is_decoding) {
    cm->is_decoding = is_decoding;
    for (plane = plane_start; plane < plane_end; plane++) {
      if (plane == 0 && !(cm->lf.filter_level[0]) && !(cm->lf.filter_level[1]))
        break;
      else if (plane == 1 && !(cm->lf.filter_level_u))
        continue;
      else if (plane == 2 && !(cm->lf.filter_level_v))
        continue;

      av1_setup_dst_planes(pd, cm->seq_params.sb_size, frame_buffer, 0, 0,
                           plane, plane + 1);

      av1_build_bitmask_vert_info(cm, &pd[plane], plane);
      av1_build_bitmask_horz_info(cm, &pd[plane], plane);

      // apply loop filtering which only goes through buffer once
      for (mi_row = start; mi_row < stop; mi_row += MI_SIZE_64X64) {
        for (mi_col = col_start; mi_col < col_end; mi_col += MI_SIZE_64X64) {
          av1_setup_dst_planes(pd, BLOCK_64X64, frame_buffer, mi_row, mi_col,
                               plane, plane + 1);
          av1_filter_block_plane_bitmask_vert(cm, &pd[plane], plane, mi_row,
                                              mi_col);
          if (mi_col - MI_SIZE_64X64 >= 0) {
            av1_setup_dst_planes(pd, BLOCK_64X64, frame_buffer, mi_row,
                                 mi_col - MI_SIZE_64X64, plane, plane + 1);
            av1_filter_block_plane_bitmask_horz(cm, &pd[plane], plane, mi_row,
                                                mi_col - MI_SIZE_64X64);
          }
        }
        av1_setup_dst_planes(pd, BLOCK_64X64, frame_buffer, mi_row,
                             mi_col - MI_SIZE_64X64, plane, plane + 1);
        av1_filter_block_plane_bitmask_horz(cm, &pd[plane], plane, mi_row,
                                            mi_col - MI_SIZE_64X64);
      }
    }
    return;
  }
#endif

  for (plane = plane_start; plane < plane_end; plane++) {
    if (plane == 0 && !(cm->lf.filter_level[0]) && !(cm->lf.filter_level[1]))
      break;
    else if (plane == 1 && !(cm->lf.filter_level_u))
      continue;
    else if (plane == 2 && !(cm->lf.filter_level_v))
      continue;

    if (cm->lf.combine_vert_horz_lf) {
      // filter all vertical and horizontal edges in every 128x128 super block
      for (mi_row = start; mi_row < stop; mi_row += MAX_MIB_SIZE) {
        for (mi_col = col_start; mi_col < col_end; mi_col += MAX_MIB_SIZE) {
          // filter vertical edges
          av1_setup_dst_planes(pd, cm->seq_params.sb_size, frame_buffer, mi_row,
                               mi_col, plane, plane + 1);
          av1_filter_block_plane_vert(cm, xd, plane, &pd[plane], mi_row,
                                      mi_col);
          // filter horizontal edges
          if (mi_col - MAX_MIB_SIZE >= 0) {
            av1_setup_dst_planes(pd, cm->seq_params.sb_size, frame_buffer,
                                 mi_row, mi_col - MAX_MIB_SIZE, plane,
                                 plane + 1);
            av1_filter_block_plane_horz(cm, xd, plane, &pd[plane], mi_row,
                                        mi_col - MAX_MIB_SIZE);
          }
        }
        // filter horizontal edges
        av1_setup_dst_planes(pd, cm->seq_params.sb_size, frame_buffer, mi_row,
                             mi_col - MAX_MIB_SIZE, plane, plane + 1);
        av1_filter_block_plane_horz(cm, xd, plane, &pd[plane], mi_row,
                                    mi_col - MAX_MIB_SIZE);
      }
    } else {
      // filter all vertical edges in every 128x128 super block
      for (mi_row = start; mi_row < stop; mi_row += MAX_MIB_SIZE) {
        for (mi_col = col_start; mi_col < col_end; mi_col += MAX_MIB_SIZE) {
          av1_setup_dst_planes(pd, cm->seq_params.sb_size, frame_buffer, mi_row,
                               mi_col, plane, plane + 1);
          av1_filter_block_plane_vert(cm, xd, plane, &pd[plane], mi_row,
                                      mi_col);
        }
      }

      // filter all horizontal edges in every 128x128 super block
      for (mi_row = start; mi_row < stop; mi_row += MAX_MIB_SIZE) {
        for (mi_col = col_start; mi_col < col_end; mi_col += MAX_MIB_SIZE) {
          av1_setup_dst_planes(pd, cm->seq_params.sb_size, frame_buffer, mi_row,
                               mi_col, plane, plane + 1);
          av1_filter_block_plane_horz(cm, xd, plane, &pd[plane], mi_row,
                                      mi_col);
        }
      }
    }
  }
}

void av1_loop_filter_frame(YV12_BUFFER_CONFIG *frame, AV1_COMMON *cm,
                           MACROBLOCKD *xd,
#if CONFIG_LPF_MASK
                           int is_decoding,
#endif
                           int plane_start, int plane_end, int partial_frame) {
  int start_mi_row, end_mi_row, mi_rows_to_filter;

  start_mi_row = 0;
  mi_rows_to_filter = cm->mi_params.mi_rows;
  if (partial_frame && cm->mi_params.mi_rows > 8) {
    start_mi_row = cm->mi_params.mi_rows >> 1;
    start_mi_row &= 0xfffffff8;
    mi_rows_to_filter = AOMMAX(cm->mi_params.mi_rows / 8, 8);
  }
  end_mi_row = start_mi_row + mi_rows_to_filter;
  av1_loop_filter_frame_init(cm, plane_start, plane_end);
  loop_filter_rows(frame, cm, xd, start_mi_row, end_mi_row,
#if CONFIG_LPF_MASK
                   is_decoding,
#endif
                   plane_start, plane_end);
}
