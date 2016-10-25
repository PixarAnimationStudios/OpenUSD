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

/// \file writeUtil.h

#ifndef PXRUSDMAYA_WRITEUTIL_H
#define PXRUSDMAYA_WRITEUTIL_H

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/primvar.h"

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MString.h>

#include <string>


struct PxrUsdMayaWriteUtil
{
    /// \name Helpers for writing USD
    /// \{

    /// Get the SdfValueTypeName that corresponds to the given plug \p attrPlug.
    static SdfValueTypeName GetUsdTypeName(const MPlug& attrPlug);

    /// Given an \p attrPlug, try to create a USD attribute on \p usdPrim with
    /// the name \p attrName. Note, it's value will not be set.
    ///
    /// Attributes that are not part of the primSchema should have \p custom
    /// set to true.
    static UsdAttribute GetOrCreateUsdAttr(
            const MPlug& attrPlug,
            const UsdPrim& usdPrim,
            const std::string& attrName,
            const bool custom = false);

    /// Given an \p attrPlug, try to create a primvar on \p imageable with
    /// the name \p primvarName. Note, it's value will not be set.
    ///
    /// Attributes that are not part of the primSchema should have \p custom
    /// set to true.
    static UsdGeomPrimvar GetOrCreatePrimvar(
            const MPlug& attrPlug,
            UsdGeomImageable& imageable,
            const std::string& primvarName,
            const TfToken& interpolation = TfToken(),
            const int elementSize = -1,
            const bool custom = false);

    /// Given an \p attrPlug, try to create a UsdRi attribute on \p usdPrim with
    /// the name \p attrName. Note, it's value will not be set.
    static UsdAttribute GetOrCreateUsdRiAttribute(
            const MPlug& attrPlug,
            const UsdPrim& usdPrim,
            const std::string& attrName,
            const std::string& nameSpace = "user");

    /// Given an \p attrPlug, determine it's value and set it on \p usdAttr at
    /// \p usdTime.
    static bool SetUsdAttr(
            const MPlug &attrPlug, 
            const UsdAttribute& usdAttr, 
            const UsdTimeCode &usdTime);

    /// Given a Maya node at \p dagPath, inspect it for attributes tagged by
    /// the user for export to USD and write them onto \p usdPrim at time
    /// \p usdTime.
    static bool WriteUserExportedAttributes(
            const MDagPath& dagPath,
            const UsdPrim& usdPrim,
            const UsdTimeCode& usdTime);
    /// \}

    /// \name Helpers for reading Maya data
    /// \{

    /// \brief Reads attribute \p name on \p depNode into \p val.
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name, 
            std::string* val);

    /// \brief Reads attribute \p name on \p depNode into \p val.
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name, 
            VtIntArray* val);

    /// \brief Reads attribute \p name on \p depNode into \p val.
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name, 
            VtFloatArray* val);

    /// \brief Reads attribute \p name on \p depNode into \p val.
    static bool ReadMayaAttribute(
            const MFnDependencyNode& depNode,
            const MString& name, 
            VtVec3fArray* val);
    /// \}

};

#endif // PXRUSDMAYA_WRITEUTIL_H
