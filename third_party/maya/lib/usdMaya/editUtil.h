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
#ifndef PXRUSDMAYA_EDIT_UTIL_H
#define PXRUSDMAYA_EDIT_UTIL_H

/// \file usdMaya/editUtil.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include <maya/MFnAssembly.h>
#include <maya/MObject.h>

#include <map>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


/// Utility class for handling edits on Assemblies in Maya.
///
class UsdMayaEditUtil
{
public:

    /// \name Enums for inspecting edits
    /// \{

    /// Possible operations for a supported edit.
    enum EditOp {
        OP_TRANSLATE,
        OP_ROTATE,
        OP_SCALE
    };

    /// Whether the edit affects one component or all components.
    /// The values are explicit, such that X,Y,and Z can be used in []
    /// operators on Vec3s.
    ///
    enum EditSet {
        SET_ALL = -1,
        SET_X = 0,
        SET_Y = 1,
        SET_Z = 2
    };

    /// A struct containing the data and associated string for an edit.
    struct AssemblyEdit
    {
        std::string editString;

        EditOp op;
        EditSet set;
        VtValue value;
    };

    /// \}

    /// An ordered list of sequential edits.
    using AssemblyEditVec = std::vector<AssemblyEdit>;

    /// An ordered list of sequential edits for multiple paths, sorted by path.
    using PathEditMap = std::map<SdfPath, AssemblyEditVec>;

    /// An ordered map of concatenated Avar edits.
    using AvarValueMap = std::map<std::string, double>;

    /// An ordered map of concatenated Avar edits for multiple paths, sorted by
    /// path.
    using PathAvarMap = std::map<SdfPath, AvarValueMap>;

    /// Translates an edit string into a AssemblyEdit structure.
    /// The output edit path is relative to the root of the assembly.
    /// \returns true if translation was successful.
    PXRUSDMAYA_API
    static bool GetEditFromString(
            const MFnAssembly& assemblyFn,
            const std::string& editString,
            SdfPath* outEditPath,
            AssemblyEdit* outEdit);

    /// Inspects all edits on \p assemblyObj and returns a parsed set of proper
    /// edits in \p assemEdits and invalid edits in \p invalidEdits.
    /// The proper edits are keyed by relative path to the root of the
    /// assembly.
    PXRUSDMAYA_API
    static void GetEditsForAssembly(
            const MObject& assemblyObj,
            PathEditMap* assemEdits,
            std::vector<std::string>* invalidEdits);

    /// Apply the assembly edits in \p assemEdits to the USD prim
    /// \p proxyRootPrim, which is the root prim for the assembly.
    ///
    /// If \p failedEdits is not nullptr, it will contain any edits that could
    /// not be applied to \p proxyRootPrim.
    PXRUSDMAYA_API
    static void ApplyEditsToProxy(
            const PathEditMap& assemEdits,
            const UsdPrim& proxyRootPrim,
            std::vector<std::string>* failedEdits);

    PXRUSDMAYA_API
    static void GetAvarEdits(
            const PathEditMap& assemEdits,
            PathAvarMap* avarMap);

private:
    static void _ApplyEditToAvar(
            const EditOp op,
            const EditSet set,
            const double value,
            AvarValueMap* valueMap);

    static void _ApplyEditToAvars(
            const AssemblyEdit& assemEdit,
            AvarValueMap* valueMap);

};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
