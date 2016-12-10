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
#ifndef PXRUSDMAYA_ATTRIBUTECONVERTER_H
#define PXRUSDMAYA_ATTRIBUTECONVERTER_H

/// \file AttributeConverter.h

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MFnDependencyNode.h>

#include <functional>

/// \brief Provides a way to store USD-specific information on a Maya node
/// as an extra Maya attribute (e.g. "USD_hidden" for the hidden state) when no
/// existing Maya attribute maps cleanly.
///
/// This way, we can store information such as the hidden state or the model
/// kind on a Maya node so that the information isn't lost in the USD to Maya to
/// USD roundtrip.
///
/// This class encapsulates:
/// (1) a way to store non-attribute metadata or information (such as the hidden
/// state) on a Maya node when importing, and
/// (2) a way to write out the same metadata or information from a Maya node
/// into a \c UsdPrim when exporting.
class AttributeConverter {
public:
    /// \brief Takes attribute(s) from a Maya node and modifies a
    /// \c UsdPrim accordingly.
    virtual bool MayaToUsd(
            const MFnDependencyNode& depNode,
            UsdPrim& destPrim,
            const UsdTimeCode usdTime) const = 0;
    
    /// \brief Stores information about a source \c UsdPrim in a Maya node.
    virtual bool UsdToMaya(
            const UsdPrim& srcPrim,
            MFnDependencyNode& destNode,
            const UsdTimeCode usdTime) const = 0;
};

/// \brief An implementation of \c AttributeConverter that allows passing in
/// functions or lambdas to handle the conversion between Maya and USD.
class FunctionalAttributeConverter : public AttributeConverter {
public:
    typedef std::function< bool (const MFnDependencyNode&, UsdPrim&,
            UsdTimeCode) > MayaToUsdFn;
    typedef std::function< bool (const UsdPrim&, MFnDependencyNode&,
            UsdTimeCode) > UsdToMayaFn;

    virtual bool MayaToUsd(
            const MFnDependencyNode& depNode,
            UsdPrim& destPrim,
            const UsdTimeCode usdTime) const override;
    virtual bool UsdToMaya(
            const UsdPrim& srcPrim,
            MFnDependencyNode& destNode,
            const UsdTimeCode usdTime) const override;

    /// \brief Constructs a FunctionalAttributeConverter that forwards calls to
    /// MayaToUsd and UsdToMaya to the given functions.
    ///
    /// The functions can be empty functions, in which case MayaToUsd and/or
    /// UsdToMaya will return false.
    FunctionalAttributeConverter(
            MayaToUsdFn mayaToUsdFn,
            UsdToMayaFn usdToMayaFn);

private:
    const MayaToUsdFn _mayaToUsdFn;
    const UsdToMayaFn _usdToMayaFn;
};

#endif // PXRUSDMAYA_ATTRIBUTECONVERTER_H
