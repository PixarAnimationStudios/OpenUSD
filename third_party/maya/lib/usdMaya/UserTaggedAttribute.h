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

/// \file UserTaggedAttribute.h

#ifndef PXRUSDMAYA_USERTAGGEDATTRIBUTE_H
#define PXRUSDMAYA_USERTAGGEDATTRIBUTE_H

#include "usdMaya/api.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/staticTokens.h"

#include <maya/MDagPath.h>
#include <maya/MPlug.h>

#include <map>
#include <string>
#include <vector>

#define PXRUSDMAYA_ATTR_TOKENS \
    ((USDAttrTypePrimvar, "primvar")) \
    ((USDAttrTypeUsdRi, "usdRi"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaUserTaggedAttributeTokens, USDMAYA_API,
    PXRUSDMAYA_ATTR_TOKENS);

/// \brief Represents a single attribute tagged for USD export, and describes
/// how it will be exported.
class PxrUsdMayaUserTaggedAttribute {
private:
    MPlug _plug;
    const std::string _name;
    const TfToken _type;
    const TfToken _interpolation;

public:
    USDMAYA_API
    PxrUsdMayaUserTaggedAttribute(
            MPlug plug,
            const std::string& name,
            const TfToken& type,
            const TfToken& interpolation);

    /// \brief Gets all of the exported attributes for the given node.
    USDMAYA_API
    static std::vector<PxrUsdMayaUserTaggedAttribute>
            GetUserTaggedAttributesForNode(const MDagPath& dagPath);

    /// \brief Gets the plug for the Maya attribute to be exported.
    USDMAYA_API
    MPlug GetMayaPlug() const;

    /// \brief Gets the name of the Maya attribute that will be exported;
    /// the name will not contain the name of the node.
    USDMAYA_API
    std::string GetMayaName() const;

    /// \brief Gets the name of the USD attribute to which the Maya attribute
    /// will be exported.
    USDMAYA_API
    std::string GetUsdName() const;

    /// \brief Gets the type of the USD attribute to export: whether it is a
    /// regular attribute, primvar, etc.
    USDMAYA_API
    TfToken GetUsdType() const;

    /// \brief Gets the interpolation for primvars.
    USDMAYA_API
    TfToken GetUsdInterpolation() const;
};

#endif // PXRUSDMAYA_USERTAGGEDATTRIBUTE_H
