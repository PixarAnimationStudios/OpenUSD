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

/// \file usdMaya/writeUtil.h

#ifndef PXRUSDMAYA_WRITEUTIL_H
#define PXRUSDMAYA_WRITEUTIL_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/userTaggedAttribute.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/primvar.h"

#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdUtilsSparseValueWriter;

/// This struct contains helpers for writing USD (thus reading Maya data).
struct UsdMayaWriteUtil
{
    /// \name Helpers for writing USD
    /// \{

    /// Returns whether the environment setting for writing the TexCoord
    /// types is set to true
    PXRUSDMAYA_API
    static bool WriteUVAsFloat2();

    /// Get the SdfValueTypeName that corresponds to the given plug \p attrPlug.
    /// If \p translateMayaDoubleToUsdSinglePrecision is true, Maya plugs that
    /// contain double data will return the appropriate float-based type.
    /// Otherwise, the type returned will be the appropriate double-based type.
    PXRUSDMAYA_API
    static SdfValueTypeName GetUsdTypeName(
            const MPlug& attrPlug,
            const bool translateMayaDoubleToUsdSinglePrecision =
                UsdMayaUserTaggedAttribute::GetFallbackTranslateMayaDoubleToUsdSinglePrecision());

    /// Given an \p attrPlug, try to create a USD attribute on \p usdPrim with
    /// the name \p attrName. Note, it's value will not be set.
    ///
    /// Attributes that are not part of the primSchema should have \p custom
    /// set to true.
    ///
    /// If \p translateMayaDoubleToUsdSinglePrecision is true, Maya plugs that
    /// contain double data will result in USD attributes of the appropriate
    /// float-based type. Otherwise, their type will be double-based.
    PXRUSDMAYA_API
    static UsdAttribute GetOrCreateUsdAttr(
            const MPlug& attrPlug,
            const UsdPrim& usdPrim,
            const std::string& attrName,
            const bool custom = false,
            const bool translateMayaDoubleToUsdSinglePrecision =
                UsdMayaUserTaggedAttribute::GetFallbackTranslateMayaDoubleToUsdSinglePrecision());

    /// Given an \p attrPlug, try to create a primvar on \p imageable with
    /// the name \p primvarName. Note, it's value will not be set.
    ///
    /// If \p translateMayaDoubleToUsdSinglePrecision is true, Maya plugs that
    /// contain double data will result in primvars of the appropriate
    /// float-based type. Otherwise, their type will be double-based.
    PXRUSDMAYA_API
    static UsdGeomPrimvar GetOrCreatePrimvar(
            const MPlug& attrPlug,
            UsdGeomImageable& imageable,
            const std::string& primvarName,
            const TfToken& interpolation = TfToken(),
            const int elementSize = -1,
            const bool translateMayaDoubleToUsdSinglePrecision =
                UsdMayaUserTaggedAttribute::GetFallbackTranslateMayaDoubleToUsdSinglePrecision());

    /// Given an \p attrPlug, try to create a UsdRi attribute on \p usdPrim with
    /// the name \p attrName. Note, it's value will not be set.
    ///
    /// If \p translateMayaDoubleToUsdSinglePrecision is true, Maya plugs that
    /// contain double data will result in UsdRi attributes of the appropriate
    /// float-based type. Otherwise, their type will be double-based.
    PXRUSDMAYA_API
    static UsdAttribute GetOrCreateUsdRiAttribute(
            const MPlug& attrPlug,
            const UsdPrim& usdPrim,
            const std::string& attrName,
            const std::string& nameSpace = "user",
            const bool translateMayaDoubleToUsdSinglePrecision =
                UsdMayaUserTaggedAttribute::GetFallbackTranslateMayaDoubleToUsdSinglePrecision());

    /// Given an \p attrPlug, reads its value and returns it as a wrapped
    /// VtValue. The type of the value is determined by consulting the given
    /// \p typeName. If the value cannot be converted into a \p typeName, then
    /// an empty VtValue is returned.
    ///
    /// For type names with color roles, the value read from Maya will be
    /// converted to a linear color value if \p linearizeColors is true.
    PXRUSDMAYA_API
    static VtValue GetVtValue(
            const MPlug& attrPlug,
            const SdfValueTypeName& typeName,
            const bool linearizeColors = true);

    /// Given an \p attrPlug, reads its value and returns it as a wrapped
    /// VtValue. The type of the value is determined by consulting the given
    /// \p type. If the value cannot be converted into a \p typeName, then an
    /// empty VtValue is returned.
    ///
    /// For types with color roles, the value read from Maya will be converted
    /// to a linear color value if \p linearizeColors is true.
    PXRUSDMAYA_API
    static VtValue GetVtValue(
            const MPlug& attrPlug,
            const TfType& type,
            const TfToken& role,
            const bool linearizeColors = true);

    /// Given an \p attrPlug, determine it's value and set it on \p usdAttr at
    /// \p usdTime.
    ///
    /// Whether to export Maya attributes as single-precision or
    /// double-precision floating point is determined by consulting the type
    /// name of the USD attribute.
    PXRUSDMAYA_API
    static bool SetUsdAttr(
            const MPlug& attrPlug,
            const UsdAttribute& usdAttr,
            const UsdTimeCode& usdTime,
            UsdUtilsSparseValueWriter *valueWriter=nullptr);

    /// Given a Maya node \p mayaNode, inspect it for attributes tagged by
    /// the user for export to USD and write them onto \p usdPrim at time
    /// \p usdTime.
    PXRUSDMAYA_API
    static bool WriteUserExportedAttributes(
            const MObject& mayaNode,
            const UsdPrim& usdPrim,
            const UsdTimeCode& usdTime,
            UsdUtilsSparseValueWriter *valueWriter=nullptr);

    /// Writes all of the adaptor metadata from \p mayaObject onto the \p prim.
    /// Returns true if successful (even if there was nothing to export).
    PXRUSDMAYA_API
    static bool WriteMetadataToPrim(
            const MObject& mayaObject,
            const UsdPrim& prim);

    /// Writes all of the adaptor API schema attributes from \p mayaObject onto
    /// the \p prim. Only attributes on applied schemas will be written to
    /// \p prim.
    /// Returns true if successful (even if there was nothing to export).
    /// \sa UsdMayaAdaptor::GetAppliedSchemas
    PXRUSDMAYA_API
    static bool WriteAPISchemaAttributesToPrim(
            const MObject& mayaObject,
            const UsdPrim& prim,
            UsdUtilsSparseValueWriter *valueWriter=nullptr);

    template <typename T>
    static size_t WriteSchemaAttributesToPrim(
            const MObject& object,
            const UsdPrim& prim,
            const std::vector<TfToken>& attributeNames,
            const UsdTimeCode& usdTime = UsdTimeCode::Default(),
            UsdUtilsSparseValueWriter *valueWriter=nullptr)
    {
        return WriteSchemaAttributesToPrim(
                object,
                prim,
                TfType::Find<T>(),
                attributeNames,
                usdTime,
                valueWriter);
    }

    /// Writes schema attributes specified by \attributeNames for the schema
    /// with type \p schemaType to the prim \p prim.
    /// Values are read at the current Maya time, and are written into the USD
    /// stage at time \p usdTime. If the optional \p valueWriter is provided,
    /// it will be used to write the values.
    /// Returns the number of attributes actually written to the USD stage.
    static size_t WriteSchemaAttributesToPrim(
            const MObject& object,
            const UsdPrim& prim,
            const TfType& schemaType,
            const std::vector<TfToken>& attributeNames,
            const UsdTimeCode& usdTime = UsdTimeCode::Default(),
            UsdUtilsSparseValueWriter *valueWriter=nullptr);

    /// Authors class inherits on \p usdPrim.  \p inheritClassNames are
    /// specified as names (not paths).  For example, they should be
    /// ["_class_Special", ...].
    PXRUSDMAYA_API
    static bool WriteClassInherits(
            const UsdPrim& usdPrim,
            const std::vector<std::string>& inheritClassNames);

    /// Given \p inputPointsData (native Maya particle data), writes the
    /// arrays as point-instancer attributes on the given \p instancer
    /// schema object.
    /// Returns true if successful.
    PXRUSDMAYA_API
    static bool WriteArrayAttrsToInstancer(
            MFnArrayAttrsData& inputPointsData,
            const UsdGeomPointInstancer& instancer,
            const size_t numPrototypes,
            const UsdTimeCode& usdTime,
            UsdUtilsSparseValueWriter *valueWriter=nullptr);

    /// Get the name of the USD prim under which exported materials are
    /// authored.
    ///
    /// By default, this scope is named "Looks", but it can be configured
    /// in the UsdMaya metadata of a plugInfo.json file like so:
    /// 
    /// "UsdMaya": {
    ///     "UsdExport": {
    ///         "materialsScopeName": "SomeScopeName"
    ///     }
    /// }
    ///
    /// Note that this name can also be specified as a parameter during export
    /// and the value returned by this function will not account for that. In
    /// that case, the value should be read from the export args for that
    /// particular export instead.
    PXRUSDMAYA_API
    static TfToken GetMaterialsScopeName();

    /// \}

    /// \name Helpers for reading Maya data
    /// \{

    /// \brief Reads attribute \p name on \p depNode into \p val.
    PXRUSDMAYA_API
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name,
            std::string* val);

    PXRUSDMAYA_API
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name,
            std::vector<std::string>* val);

    /// \brief Reads attribute \p name on \p depNode into \p val.
    PXRUSDMAYA_API
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name,
            VtIntArray* val);

    /// \brief Reads attribute \p name on \p depNode into \p val.
    PXRUSDMAYA_API
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name,
            VtFloatArray* val);

    /// \brief Reads attribute \p name on \p depNode into \p val.
    PXRUSDMAYA_API
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name,
            VtVec3fArray* val);
    /// \}

    /// \name Frame/time utilities {

    /// Gets an ordered list of frame samples for the given \p frameRange,
    /// advancing the time by \p stride on each iteration, and computing extra
    /// subframe samples using \p subframeOffsets.
    /// \p stride determines how much to increment the "current time" on each
    /// iteration; whenever the current time is incremented past the end of
    /// \p frameRange, iteration will stop.
    /// \p subframeOffsets is treated as a set of offsets from the
    /// "current time"; empty \p subframeOffsets is equivalent to {0.0}, which
    /// means to only add one frame sample per time increment.
    ///
    /// Raises a runtime error and returns an empty list of time samples if
    /// \p stride is not greater than 0.
    /// Warns if any \p subframeOffsets fall outside of the open interval
    /// (-\p stride, +\p stride), but returns a valid result in that case,
    /// ensuring that the returned list is sorted.
    ///
    /// Example: frameRange = [1, 5], subframeOffsets = {0.0, 0.9}, stride = 2.0
    ///     This gives the time samples [1, 1.9, 3, 3.9, 5, 5.9].
    ///     Note that the \p subframeOffsets allows the last frame to go
    ///     _outside_ the specified \p frameRange.
    PXRUSDMAYA_API
    static std::vector<double> GetTimeSamples(
            const GfInterval& frameRange,
            const std::set<double>& subframeOffsets,
            const double stride = 1.0);

    /// \}

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
