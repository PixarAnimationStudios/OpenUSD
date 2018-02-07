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
#ifndef PXRUSDMAYA_EDITUTIL_H
#define PXRUSDMAYA_EDITUTIL_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"

#include <maya/MObject.h>
#include <maya/MFnAssembly.h>

#include <string>
#include <vector>
#include <map>


PXR_NAMESPACE_OPEN_SCOPE

/// \brief Utility class for handling edits on Assemblies in Maya.
///
class PxrUsdMayaEditUtil
{
public:
        
    /// \name Enums for inspecting edits
    /// \{
    
    /// \brief Possible operations for a supported edit.
    enum EditOp {
        OP_TRANSLATE,
        OP_ROTATE,
        OP_SCALE
    };
    
    /// \brief Whether the edit affects one component or all components.
    /// The values are explicit, such that X,Y,and Z can be used in []
    /// operators on Vec3s.
    ///
    enum EditSet {
        SET_ALL = -1,
        SET_X = 0,
        SET_Y = 1,
        SET_Z = 2
    };
    
    /// \brief A struct containing the data and associated string for an edit.
    struct RefEdit
    {
        std::string editString;
        
        EditOp op;
        EditSet set;
        VtValue value;
    };
    
    /// \}
    
    /// \brief An ordered list of sequential edits.
    typedef std::vector< RefEdit > RefEditVec;
    
    /// \brief An ordered list of sequential edits for multiple paths, sorted
    /// by path.
    typedef std::map< SdfPath, RefEditVec > PathEditMap;
    
    /// \brief An ordered map of concatenated Avar edits.
    typedef std::map< std::string, double > AvarValueMap;
    
    /// \brief An ordered map of concatenated Avar edits for multiple paths,
    /// sorted by path.
    typedef std::map< SdfPath, AvarValueMap > PathAvarMap;
    
    /// \brief Translates an edit string into a RefEdit structure.
    /// \returns true if translation was successful.
    PXRUSDMAYA_API
    static bool GetEditFromString(
            const MFnAssembly &assemblyFn,
            const std::string &editString,
            SdfPath *outEditPath,
            RefEdit *outEdit );
        
    /// \brief Inspects all edits on \p assemblyObj and returns a parsed
    /// set of proper edits in \p refEdits and invlaid edits in \p invalidEdits.
    PXRUSDMAYA_API
    static void GetEditsForAssembly(
            const MObject &assemblyObj,
            PathEditMap *refEdits,
            std::vector< std::string > *invalidEdits );
    
    /// \brief Apply \p refEdits to a \p stage for an assembly rooted
    /// at \p proxyRootPrim.
    PXRUSDMAYA_API
    static void ApplyEditsToProxy(
            const PathEditMap &refEdits,
            const UsdStagePtr &stage,
            const UsdPrim &proxyRootPrim,
            std::vector< std::string > *failedEdits );
    
    PXRUSDMAYA_API
    static void GetAvarEdits(
            const PathEditMap &refEdits,
            PathAvarMap *avarMap );

private:
    static void _ApplyEditToAvar(
            EditOp op,
            EditSet set,
            double value,
            AvarValueMap *valueMap );

    static void _ApplyEditToAvars(
            const RefEdit &refEdit,
            AvarValueMap *valueMap );

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_EDITUTIL_H
