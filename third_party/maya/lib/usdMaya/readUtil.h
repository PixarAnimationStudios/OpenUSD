//
// Copyright 2018 Pixar
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

/// \file readUtil.h

#ifndef PXRUSDMAYA_READUTIL_H
#define PXRUSDMAYA_READUTIL_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/usd/attribute.h"

#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


/// This struct contains helpers for reading USD (thus writing Maya data).
struct PxrUsdMayaReadUtil
{
    /// \name Helpers for reading USD
    /// \{

    /// Given the \p typeName and \p variability, try to create a Maya attribute
    /// on \p depNode with the name \p attrName.
    /// If the \p typeName isn't supported by this function, raises a coding
    /// error (this function supports the majority of, but not all, type names).
    /// If the attribute already exists and its type information matches, then
    /// its object is returned. If the attribute already exists but its type
    /// information is conflicting, then returns null and posts a runtime error.
    /// If the attribute doesn't exist yet, then creates it and returns the
    /// attribute object.
    PXRUSDMAYA_API
    static MObject FindOrCreateMayaAttr(
            const SdfValueTypeName& typeName,
            const SdfVariability variability,
            MFnDependencyNode& depNode,
            const std::string& attrName,
            const std::string& attrNiceName = std::string());

    /// An overload of FindOrCreateMayaAttr that takes an MDGModifier.
    /// Note that this function will call doIt() on the MDGModifier; thus the
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    static MObject FindOrCreateMayaAttr(
            const SdfValueTypeName& typeName,
            const SdfVariability variability,
            MFnDependencyNode& depNode,
            const std::string& attrName,
            const std::string& attrNiceName,
            MDGModifier& modifier);

    /// Sets a Maya plug using the value on a USD attribute at default time.
    /// If the variability of the USD attribute doesn't match the keyable state
    /// of the Maya plug, then the plug's keyable state will also be updated.
    /// Returns true if the plug was set.
    PXRUSDMAYA_API
    static bool SetMayaAttr(
            MPlug& attrPlug,
            const UsdAttribute& usdAttr);

    /// Sets a Maya plug using the given VtValue. The plug keyable state won't
    /// be affected.
    /// Returns true if the plug was set.
    PXRUSDMAYA_API
    static bool SetMayaAttr(
            MPlug& attrPlug,
            const VtValue& newValue);

    /// An overload of SetMayaAttr that takes an MDGModifier.
    /// Note that this function will call doIt() on the MDGModifier; thus the
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    static bool SetMayaAttr(
            MPlug& attrPlug,
            const VtValue& newValue,
            MDGModifier& modifier);

    /// Sets the plug's keyable state based on whether the variability is
    /// varying or uniform.
    PXRUSDMAYA_API
    static void SetMayaAttrKeyableState(
            MPlug& attrPlug,
            const SdfVariability variability);

    /// An overload of SetMayaAttrKeyableState that takes an MDGModifier.
    /// Note that this function will call doIt() on the MDGModifier; thus the
    /// actions will have been committed when the function returns.
    PXRUSDMAYA_API
    static void SetMayaAttrKeyableState(
            MPlug& attrPlug,
            const SdfVariability variability,
            MDGModifier& modifier);

    /// \}
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_READUTIL_H
