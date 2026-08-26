//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file OpenEXRCoreUnity.h

#include "OpenEXRCore/openexr_config.h"

// The internal libdeflate sources are included by OpenEXRCore/compression.c
// itself (guarded by OPENEXR_USE_INTERNAL_DEFLATE) as of OpenEXR 3.4.14, so
// they must not be included here as well -- doing so compiles those
// translation units twice and redefines symbols such as feature_sysctls and
// query_arm_cpu_features in this unity build.

#include "openexr-c.h"

#include "OpenEXRCore/attributes.c"
#include "OpenEXRCore/base.c"
#include "OpenEXRCore/bytes.c"
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
#include "OpenEXRCore/internal_b44_table_init.c"
#include "OpenEXRCore/internal_b44.c"
#include "OpenEXRCore/internal_dwa.c"
#include "OpenEXRCore/internal_dwa_table.c"
#include "OpenEXRCore/internal_dwa_table_init.c"
#include "OpenEXRCore/internal_ht.c"
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
