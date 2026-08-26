/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_HT_COMMON_H
#define OPENEXR_PRIVATE_HT_COMMON_H

#include <vector>
#include <stdlib.h>
#include "openexr_coding.h"
#include "pxr/base/arch/export.h"

/** Maps a JPEG 2000 codestream component index to the corresponding OpenEXR
 *  file channel index and its byte offset within a packed raster line. */
struct CodestreamChannelInfo
{
    int    file_index;
    size_t raster_line_offset;
    int    scratch; // used when reading from a file
};

/** Build a codestream-to-file channel map for @p channel_count OpenEXR
 *  channels.
 *
 *  When the channels include a matching RGB triplet (e.g. R/G/B, Red/Green/Blue,
 *  or layer-prefixed variants), the first three entries of @p cs_to_file_ch are
 *  assigned to R, G, and B respectively so that the JPEG 2000 Reversible Color
 *  Transform (RCT) can be applied by the encoder.  All remaining channels follow
 *  in their original order.  When no RGB triplet is detected the channels are
 *  mapped in their original order.
 *
 *  @param channel_count  Number of channels in @p channels.
 *  @param channels       OpenEXR per-channel coding descriptors.
 *  @param cs_to_file_ch  Output map from J2K component index to file channel.
 *                        Resized to @p channel_count on return.
 *  @return true  if an RGB triplet was detected (RCT will be applied),
 *          false otherwise.
 */
ARCH_HIDDEN bool make_channel_map (
    int                                 channel_count,
    exr_coding_channel_info_t*          channels,
    std::vector<CodestreamChannelInfo>& cs_to_file_ch);

/** Write an HTJ2K chunk header into @p buffer.
 *
 *  The header encodes the channel map so that a decoder can reconstruct the
 *  original OpenEXR channel ordering from the JPEG 2000 codestream.  It has
 *  the following on-disk layout (all integers big-endian):
 *  @code
 *    uint16_t  MAGIC  = 0x4854 ('H','T')
 *    uint32_t  PLEN          // payload length in bytes
 *    uint16_t  NCH           // number of entries in the channel map
 *    uint16_t  CS_TO_F[NCH]  // OpenEXR channel index for each J2K component
 *    // optional opaque extension bytes up to PLEN
 *    // JPEG 2000 codestream follows immediately after the header
 *  @endcode
 *
 *  @param buffer   Destination buffer; must be at least @p max_sz bytes.
 *  @param max_sz   Capacity of @p buffer in bytes.
 *  @param map      Channel map produced by make_channel_map().
 *  @return Number of bytes written; the JPEG 2000 codestream should be
 *          placed at this offset within @p buffer.
 */
ARCH_HIDDEN size_t write_header (
    uint8_t*                                  buffer,
    size_t                                    max_sz,
    const std::vector<CodestreamChannelInfo>& map);

/** Parse an HTJ2K chunk header from @p buffer and populate the channel map.
 *
 *  Validates the magic number, reads the payload length, and decodes the
 *  per-component OpenEXR channel indices.  Extension bytes inside the payload
 *  (beyond the channel map) are silently skipped.
 *
 *  @param buffer   Chunk data; must be at least @p max_sz bytes.
 *  @param max_sz   Number of readable bytes starting at @p buffer.
 *  @param map      Populated with one entry per J2K component on success.
 *  @return Byte offset of the JPEG 2000 codestream within @p buffer, i.e. the
 *          total size of the header including its payload.
 *  @throws std::runtime_error if the magic number is absent, the header is
 *          larger than @p max_sz.
 */
ARCH_HIDDEN size_t read_header (
    void*                               buffer,
    size_t                              max_sz,
    std::vector<CodestreamChannelInfo>& map);

#endif /* OPENEXR_PRIVATE_HT_COMMON_H */
