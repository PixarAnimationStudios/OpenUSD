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

#include "usdMaya/ChaserRegistry.h"
#include "usdMaya/writeUtil.h"

#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <string>
#include <vector>


struct _Entry
{
    UsdPrim usdPrim;
    std::vector<std::string> mayaAttrNames;
};

static
void
_GatherPrefixedAttrs(
        const std::vector<std::string>& attrPrefixes,
        const MDagPath &dag, 
        std::vector<std::string>* mayaAttrNames)
{
    MStatus status;
    MFnDependencyNode depFn(dag.node());

    unsigned int numAttrs = depFn.attributeCount();
    for (unsigned int i=0; i < numAttrs; ++i) {
        MObject attrObj = depFn.attribute(i);
        MPlug plg = depFn.findPlug(attrObj, true);
        if (plg.isNull()) {
            continue;
        }

        // Skip if not dynamic attr (user attribute)
        if (!plg.isDynamic()) {
            continue;
        }

        MString mayaPlgName = plg.partialName(false, false, false, false, false, true, &status);
        std::string plgName(mayaPlgName.asChar());

        // gather all things maya attributes with the attr prefix
        for (const std::string& attrPrefix: attrPrefixes) {
            if (TfStringStartsWith(plgName, attrPrefix)) {
                mayaAttrNames->push_back(plgName);
            }
        }
    }
}

void 
_WritePrefixedAttrs(
        const MDagPath &dag, 
        const UsdTimeCode &usdTime, 
        const _Entry& entry)
{
    MStatus status;
    MFnDependencyNode depFn(dag.node());
    for (const std::string& mayaAttrName: entry.mayaAttrNames) {
        MPlug plg = depFn.findPlug(MString(mayaAttrName.c_str()), true);
        UsdAttribute usdAttr = PxrUsdMayaWriteUtil::GetOrCreateUsdAttr(
                plg, entry.usdPrim, mayaAttrName, true);
        PxrUsdMayaWriteUtil::SetUsdAttr(plg, usdAttr, usdTime);
    }
}

// alembic by default sets meshes to be poly unless it's explicitly set to be
// subdivision.  UsdExport makes meshes catmullClark by default.  Here, we
// implement logic to get set the subdivision scheme so that it matches.
static
void
_SetMeshesSubDivisionScheme(
        UsdStagePtr stage,
        const PxrUsdMayaChaserRegistry::FactoryContext::DagToUsdMap& dagToUsd)
{

    for (const auto& p: dagToUsd) {
        const MDagPath& dag = p.first;
        const SdfPath& usdPrimPath = p.second;
        if (not dag.isValid()) {
            continue;
        }

        MStatus status;
        MFnMesh meshFn(dag, &status);
        if (not status) {
            continue;
        }

        if (UsdGeomMesh usdMesh = UsdGeomMesh::Get(stage, usdPrimPath)) {
            MPlug plug = meshFn.findPlug("SubDivisionMesh");
            bool isSubDivisionMesh = (not plug.isNull() and plug.asBool());

            if (not isSubDivisionMesh) {
                usdMesh.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->none);
            }
        }
    }
}

// this plugin is provided as an example and can be updated to more closely
// match what exporting a file from maya to alembic does.  For now, it just
// supports "attrprefix" 's to get exported as attributes.
class AlembicChaser : public PxrUsdMayaChaser
{
public:
    AlembicChaser(
            UsdStagePtr stage,
            const PxrUsdMayaChaserRegistry::FactoryContext::DagToUsdMap& dagToUsd,
            const std::vector<std::string>& attrPrefixes)
    : _stage(stage)
    , _dagToUsd(dagToUsd)
    {
        for (const auto& p: dagToUsd) {
            const MDagPath& dag = p.first;
            const SdfPath& usdPrimPath = p.second;

            UsdPrim usdPrim = stage->GetPrimAtPath(usdPrimPath);
            if (not dag.isValid() or not usdPrim) {
                continue;
            }

            _pathToEntry.push_back( std::make_pair(dag, _Entry()));

            _Entry& currEntry = _pathToEntry.back().second;
            currEntry.usdPrim = usdPrim;

            _GatherPrefixedAttrs(attrPrefixes, dag, &(currEntry.mayaAttrNames));

        }
    }

    virtual ~AlembicChaser() 
    { 
    }

    virtual bool ExportDefault() override 
    {
        // we fix the meshes once, not per frame.
        _SetMeshesSubDivisionScheme(_stage, _dagToUsd);

        return ExportFrame(UsdTimeCode::Default());
    }

    virtual bool ExportFrame(const UsdTimeCode& frame) override 
    {
        for (const auto& p: _pathToEntry) {
            const MDagPath& dag = p.first;
            const _Entry& entry = p.second;
            _WritePrefixedAttrs(dag, frame, entry);
        }
        return true;
    }

private:
    std::vector<std::pair<MDagPath, _Entry> > _pathToEntry;
    UsdStagePtr _stage;
    const PxrUsdMayaChaserRegistry::FactoryContext::DagToUsdMap& _dagToUsd;
};

PXRUSDMAYA_DEFINE_CHASER_FACTORY(alembic, ctx)
{
    std::map<std::string, std::string> myArgs;
    TfMapLookup(ctx.GetJobArgs().allChaserArgs, "alembic", &myArgs);

    return new AlembicChaser(
            ctx.GetStage(),
            ctx.GetDagToUsdMap(),
            TfStringSplit(myArgs["attrprefix"], ","));
}

