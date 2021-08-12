//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_DIRTY_BITS_TRANSLATOR_H
#define PXR_IMAGING_HD_DIRTY_BITS_TRANSLATOR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/types.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdDirtyBitsTranslator
///
/// A set of optimized functions for translating between dirty bits and
/// datasource locators for different prim types.
class HdDirtyBitsTranslator
{
public:
    // Dirty bits to locators.
    static void RprimDirtyBitsToLocatorSet(TfToken const& primType,
        const HdDirtyBits bits, HdDataSourceLocatorSet *set);
    static void SprimDirtyBitsToLocatorSet(TfToken const& primType,
        const HdDirtyBits bits, HdDataSourceLocatorSet *set);
    static void BprimDirtyBitsToLocatorSet(TfToken const& primType,
        const HdDirtyBits bits, HdDataSourceLocatorSet *set);
    static void InstancerDirtyBitsToLocatorSet(TfToken const& primType,
        const HdDirtyBits bits, HdDataSourceLocatorSet *set);

    // Locators to dirty bits.
    static HdDirtyBits RprimLocatorSetToDirtyBits(TfToken const& primType,
        HdDataSourceLocatorSet const& set);
    static HdDirtyBits SprimLocatorSetToDirtyBits(TfToken const& primType,
        HdDataSourceLocatorSet const& set);
    static HdDirtyBits BprimLocatorSetToDirtyBits(TfToken const& primType,
        HdDataSourceLocatorSet const& set);
    static HdDirtyBits InstancerLocatorSetToDirtyBits(TfToken const& primType,
        HdDataSourceLocatorSet const& set);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_DIRTY_BITS_TRANSLATOR_H
