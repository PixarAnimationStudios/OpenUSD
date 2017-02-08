//
// Copyright 2016 Pixar
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
#ifndef GF_HALF_H
#define GF_HALF_H

/// \file gf/half.h
///
/// This header serves to simply bring in the half float datatype, plus
/// provides other supporting utilities.  For documentation, of the half type,
/// please see the half header.  At the time of this writing the latest
/// version was available at the following link, but note that this may not be
/// the version you are building against.
/// 
/// https://github.com/openexr/openexr/blob/master/IlmBase/Half/half.h

#include <half.h>

/// Overload hash_value for half.
inline size_t hash_value(half h) { return h.bits(); }

#endif // GF_HALF_H
