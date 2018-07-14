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

/// \file userTaggedAttribute.h

#ifndef PXRUSDMAYA_USER_TAGGED_ATTRIBUTE_H
#define PXRUSDMAYA_USER_TAGGED_ATTRIBUTE_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/staticTokens.h"

#include <maya/MDagPath.h>
#include <maya/MPlug.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_ATTR_TOKENS \
    ((USDAttrTypePrimvar, "primvar")) \
    ((USDAttrTypeUsdRi, "usdRi"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaUserTaggedAttributeTokens,
    PXRUSDMAYA_API,
    PXRUSDMAYA_ATTR_TOKENS);

/// \brief Represents a single attribute tagged for translation between Maya
/// and USD, and describes how it will be exported from/imported into Maya.
class PxrUsdMayaUserTaggedAttribute {
private:
    MPlug _plug;
    const std::string _name;
    const TfToken _type;
    const TfToken _interpolation;
    const bool _translateMayaDoubleToUsdSinglePrecision;

public:
    /// \brief Gets the fallback value for whether attribute types should be
    /// mapped from double precision types in Maya to single precision types in
    /// USD.
    /// By default, the fallback value for this property is false so that
    /// double precision data is preserved in the translation back and forth
    /// between Maya and USD.
    static bool GetFallbackTranslateMayaDoubleToUsdSinglePrecision() {
        return false;
    };

    PXRUSDMAYA_API
    PxrUsdMayaUserTaggedAttribute(
            const MPlug& plug,
            const std::string& name,
            const TfToken& type,
            const TfToken& interpolation,
            const bool translateMayaDoubleToUsdSinglePrecision = 
                GetFallbackTranslateMayaDoubleToUsdSinglePrecision());

    /// \brief Gets all of the exported attributes for the given node.
    PXRUSDMAYA_API
    static std::vector<PxrUsdMayaUserTaggedAttribute>
            GetUserTaggedAttributesForNode(const MDagPath& dagPath);

    /// \brief Gets the plug for the Maya attribute to be exported.
    PXRUSDMAYA_API
    MPlug GetMayaPlug() const;

    /// \brief Gets the name of the Maya attribute that will be exported;
    /// the name will not contain the name of the node.
    PXRUSDMAYA_API
    std::string GetMayaName() const;

    /// \brief Gets the name of the USD attribute to which the Maya attribute
    /// will be exported.
    PXRUSDMAYA_API
    std::string GetUsdName() const;

    /// \brief Gets the type of the USD attribute to export: whether it is a
    /// regular attribute, primvar, etc.
    PXRUSDMAYA_API
    TfToken GetUsdType() const;

    /// \brief Gets the interpolation for primvars.
    PXRUSDMAYA_API
    TfToken GetUsdInterpolation() const;

    /// \brief Gets whether the attribute type should be mapped from a double
    /// precision type in Maya to a single precision type in USD.
    /// There is not always a direct mapping between Maya-native types and
    /// USD/Sdf types, and often it's desirable to intentionally use a single
    /// precision type when the extra precision is not needed to reduce size,
    /// I/O bandwidth, etc.
    ///
    /// For example, there is no native Maya attribute type to represent an
    /// array of float triples. To get an attribute with a VtVec3fArray type
    /// in USD, you can create a 'vectorArray' data-typed attribute in Maya
    /// (which stores MVectors, which contain doubles) and set
    /// 'translateMayaDoubleToUsdSinglePrecision' to true so that the data is
    /// cast to single-precision on export. It will be up-cast back to double
    /// on re-import.
    PXRUSDMAYA_API
    bool GetTranslateMayaDoubleToUsdSinglePrecision() const;
};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
