/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved.
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "pxr/imaging/plugin/hioAvif/aom/aom_dsp/binary_codes_reader.h"
#include "pxr/imaging/plugin/hioAvif/aom/aom_dsp/recenter.h"

#define read_primitive_quniform(r, n, ACCT_STR_NAME) \
  read_primitive_quniform_(r, n ACCT_STR_ARG(ACCT_STR_NAME))
#define read_primitive_subexpfin(r, n, k, ACCT_STR_NAME) \
  read_primitive_subexpfin_(r, n, k ACCT_STR_ARG(ACCT_STR_NAME))

static uint16_t read_primitive_quniform_(aom_reader *r,
                                         uint16_t n ACCT_STR_PARAM) {
  if (n <= 1) return 0;
  const int l = get_msb(n) + 1;
  const int m = (1 << l) - n;
  const int v = aom_read_literal(r, l - 1, ACCT_STR_NAME);
  return v < m ? v : (v << 1) - m + aom_read_bit(r, ACCT_STR_NAME);
}

// Decode finite subexponential code that for a symbol v in [0, n-1] with
// parameter k
static uint16_t read_primitive_subexpfin_(aom_reader *r, uint16_t n,
                                          uint16_t k ACCT_STR_PARAM) {
  int i = 0;
  int mk = 0;

  while (1) {
    int b = (i ? k + i - 1 : k);
    int a = (1 << b);

    if (n <= mk + 3 * a) {
      return read_primitive_quniform(r, n - mk, ACCT_STR_NAME) + mk;
    }

    if (!aom_read_bit(r, ACCT_STR_NAME)) {
      return aom_read_literal(r, b, ACCT_STR_NAME) + mk;
    }

    i = i + 1;
    mk += a;
  }

  assert(0);
  return 0;
}

uint16_t aom_read_primitive_refsubexpfin_(aom_reader *r, uint16_t n, uint16_t k,
                                          uint16_t ref ACCT_STR_PARAM) {
  return inv_recenter_finite_nonneg(
      n, ref, read_primitive_subexpfin(r, n, k, ACCT_STR_NAME));
}
