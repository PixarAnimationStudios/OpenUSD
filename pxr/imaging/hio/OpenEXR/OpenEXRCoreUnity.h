//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
/// \file OpenEXRCoreUnity.h

#include "OpenEXRCore/openexr_conf.h"

#include "deflate/lib/lib_common.h"
#include "deflate/common_defs.h"
#include "deflate/lib/utils.c"
#include "deflate/lib/arm/cpu_features.c"
#include "deflate/lib/x86/cpu_features.c"
#include "deflate/lib/deflate_compress.c"
#undef BITBUF_NBITS
#include "deflate/lib/deflate_decompress.c"
#include "deflate/lib/adler32.c"
#include "deflate/lib/zlib_compress.c"
#include "deflate/lib/zlib_decompress.c"

#include "openexr-c.h"

#include "OpenEXRCore/attributes.c"
#include "OpenEXRCore/base.c"
#include "OpenEXRCore/channel_list.c"
#include "OpenEXRCore/chunk.c"
#include "OpenEXRCore/coding.c"
#include "OpenEXRCore/compression.c"
#include "OpenEXRCore/context.c"
#include "OpenEXRCore/debug.c"
#include "OpenEXRCore/decoding.c"
#include "OpenEXRCore/encoding.c"
#include "OpenEXRCore/float_vector.c"
#include "OpenEXRCore/internal_b44_table.c"
#include "OpenEXRCore/internal_b44.c"
#include "OpenEXRCore/internal_dwa.c"
#include "OpenEXRCore/internal_huf.c"
#include "OpenEXRCore/internal_piz.c"
#include "OpenEXRCore/internal_pxr24.c"
#include "OpenEXRCore/internal_rle.c"
#include "OpenEXRCore/internal_structs.c"
#include "OpenEXRCore/internal_zip.c"
#include "OpenEXRCore/memory.c"
#include "OpenEXRCore/opaque.c"
#include "OpenEXRCore/pack.c"
#include "OpenEXRCore/parse_header.c"
#include "OpenEXRCore/part_attr.c"
#include "OpenEXRCore/part.c"
#include "OpenEXRCore/preview.c"
#include "OpenEXRCore/std_attr.c"
#include "OpenEXRCore/string_vector.c"
#include "OpenEXRCore/string.c"
#include "OpenEXRCore/unpack.c"
#include "OpenEXRCore/validation.c"
#include "OpenEXRCore/write_header.c"
