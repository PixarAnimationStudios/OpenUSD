//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PXR_TSL_EXT_H
#define PXR_BASE_TF_PXR_TSL_EXT_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pxrTslRobinMap/robin_growth_policy.h"

PXR_NAMESPACE_OPEN_SCOPE

// This file provides extensions for tsl robin_hash, and so follows its
// conventions & style.

/**
 * Grow the hash table by a factor of GrowthFactor keeping the bucket count to a
 * power of two. Maps a hash to a bucket by taking the highest-order bits,
 * rather than the lowest-order bits as power_of_two_growth_policy
 * does. Preferable when the hash function concentrates entropy in the high bits
 * (e.g., multiplicative hashing of addresses).
 *
 * GrowthFactor must be a power of two >= 2.
 */
template <std::size_t GrowthFactor>
class high_bits_power_of_two_growth_policy {
 public:
  /**
   * Called on the hash table creation and on rehash. The number of buckets for
   * the table is passed in parameter. This number is a minimum, the policy may
   * update this value with a higher value if needed (but not lower).
   *
   * If 0 is given, min_bucket_count_in_out must still be 0 after the policy
   * creation and bucket_for_hash must always return 0 in this case.
   */
  explicit high_bits_power_of_two_growth_policy(
      std::size_t& min_bucket_count_in_out) {
    if (min_bucket_count_in_out > max_bucket_count()) {
      PXR_TSL_RH_THROW_OR_TERMINATE(std::length_error,
                                    "The hash table exceeds its maximum size.");
    }

    if (min_bucket_count_in_out > 0) {
      min_bucket_count_in_out =
          round_up_to_power_of_two(min_bucket_count_in_out);
      m_shift_minus_one =
          HASH_BITS - log2_of_power_of_two(min_bucket_count_in_out) - 1;
    } else {
      m_shift_minus_one = HASH_BITS - 1;
    }
  }

  /**
   * Return the bucket [0, bucket_count()) to which the hash belongs.
   * If bucket_count() is 0, it must always return 0.
   */
  std::size_t bucket_for_hash(std::size_t hash) const noexcept {
    // Shifting by HASH_BITS is Undefined Behavior. The double-shift with the
    // stored value pre-decremented by one is UB-free for m_shift_minus_one in
    // [0, HASH_BITS-1] and avoids a subtraction on the hot path. When
    // bucket_count <= 1, the sentinel m_shift_minus_one == HASH_BITS-1 gives
    // (hash >> (HASH_BITS-1)) >> 1 == 0.
    return (hash >> m_shift_minus_one) >> 1;
  }

  /**
   * Return the number of buckets that should be used on next growth.
   */
  std::size_t next_bucket_count() const {
    // Recover bucket count from stored shift. Sentinel m_shift_minus_one ==
    // HASH_BITS-1 means bucket_count is 0 or 1; treat as 1 to produce
    // GrowthFactor on next growth, matching power_of_two_growth_policy's
    // behaviour when m_mask == 0.
    const std::size_t bucket_count = (m_shift_minus_one >= HASH_BITS - 1)
        ? std::size_t{1}
        : (std::size_t{1} << (HASH_BITS - 1 - m_shift_minus_one));

    if (bucket_count > max_bucket_count() / GrowthFactor) {
      PXR_TSL_RH_THROW_OR_TERMINATE(std::length_error,
                                    "The hash table exceeds its maximum size.");
    }

    return bucket_count * GrowthFactor;
  }

  /**
   * Return the maximum number of buckets supported by the policy.
   */
  std::size_t max_bucket_count() const {
    return (std::numeric_limits<std::size_t>::max() / 2) + 1;
  }

  /**
   * Reset the growth policy as if it was created with a bucket count of 0.
   * After a clear, the policy must always return 0 when bucket_for_hash is
   * called.
   */
  void clear() noexcept { m_shift_minus_one = HASH_BITS - 1; }

 private:
  static constexpr std::size_t HASH_BITS = sizeof(std::size_t) * CHAR_BIT;

  // Precondition: value is a power of two > 0.
  static std::size_t log2_of_power_of_two(std::size_t value) noexcept {
    std::size_t k = 0;
    while (value > 1) { value >>= 1; ++k; }
    return k;
  }

  static std::size_t round_up_to_power_of_two(std::size_t value) {
    if (is_power_of_two(value)) { return value; }
    if (value == 0) { return 1; }
    --value;
    for (std::size_t i = 1; i < HASH_BITS; i *= 2) {
      value |= value >> i;
    }
    return value + 1;
  }

  static constexpr bool is_power_of_two(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
  }

 protected:
  static_assert(is_power_of_two(GrowthFactor) && GrowthFactor >= 2,
                "GrowthFactor must be a power of two >= 2.");

  // Stores (HASH_BITS - log2(bucket_count) - 1). bucket_for_hash computes
  // (hash >> m_shift_minus_one) >> 1, which is equivalent to
  // hash >> (HASH_BITS - log2(bucket_count)) but avoids a subtraction on the
  // hot path. Sentinel value HASH_BITS-1 is used when bucket_count <= 1.
  std::size_t m_shift_minus_one;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PXR_TSL_EXT_H
