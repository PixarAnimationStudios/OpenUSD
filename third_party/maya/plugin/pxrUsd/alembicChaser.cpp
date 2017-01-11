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

#include "pxr/base/tf/staticTokens.h"
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

TF_DEFINE_PRIVATE_TOKENS(
    _tokens, 

    ((abcTypeSuffix, "_AbcType"))
    ((abcGeomScopeSuffix, "_AbcGeomScope"))
    ((abcGeomScopeFaceVarying, "fvr"))
    ((abcGeomScopeUniform, "uni"))
    ((abcGeomScopeVertex, "vtx"))

    ((userProperties, "userProperties:"))
);

struct _Entry
{
    UsdPrim usdPrim;
    // pair.first = Maya attribute name, pair.second = USD attribute name
    std::vector<std::pair<std::string, std::string>> attrNames;
};

static
bool
_EndsWithAbcTag(const std::string& attrName)
{
    // Note: we consume attrName_AbcGeomScope but we use our own type inference
    // instead of attrName_AbcType; however; we want to exclude both for
    // compatibility with existing Alembic-based pipelines that do specify
    // attrName_AbcType.
    const std::string& abcGeomScope = _tokens->abcGeomScopeSuffix.GetString();
    const std::string& abcType = _tokens->abcTypeSuffix.GetString();

    if (attrName.size() > abcGeomScope.size() &&
            attrName.compare(
                    attrName.size() - abcGeomScope.size(),
                    abcGeomScope.size(),
                    abcGeomScope) == 0) {
        return true;
    }

    if (attrName.size() > abcType.size() &&
            attrName.compare(
                    attrName.size() - abcType.size(),
                    abcType.size(),
                    abcType) == 0) {
        return true;
    }

    return false;
}

static
bool
_GetPrimvarInterpolation(const MFnDependencyNode& depFn,
        const std::string attrName,
        TfToken* outInterpolation)
{
    const MPlug scopePlg = depFn.findPlug(MString(attrName.c_str())
            + MString(_tokens->abcGeomScopeSuffix.GetText()), true);
    if (scopePlg.isNull()) {
        return false;
    }

    if (outInterpolation) {
        const char* scopeText = scopePlg.asString().toLowerCase().asChar();
        if (scopeText == _tokens->abcGeomScopeVertex) {
            *outInterpolation = UsdGeomTokens->vertex;
        }
        else if (scopeText == _tokens->abcGeomScopeFaceVarying) {
            *outInterpolation = UsdGeomTokens->faceVarying;
        }
        else if (scopeText == _tokens->abcGeomScopeUniform) {
            *outInterpolation = UsdGeomTokens->uniform;
        } else {
            *outInterpolation = UsdGeomTokens->constant;
        }
    }
    return true;
}

static
void
_AddAttributeNameEntry(
        const std::string mayaAttrName,
        const std::map<std::string, std::string>& mayaToUsdPrefixes,
        std::vector<std::pair<std::string, std::string>>* outAttrNames)
{
    for (const auto& mayaAndUsdPrefix : mayaToUsdPrefixes) {
        const std::string& mayaPrefix = mayaAndUsdPrefix.first;
        const std::string& usdPrefix = mayaAndUsdPrefix.second;
        if (TfStringStartsWith(mayaAttrName, mayaPrefix)) {
            const std::string usdAttrName = usdPrefix
                    + mayaAttrName.substr(mayaPrefix.size());
            outAttrNames->push_back(std::make_pair(mayaAttrName, usdAttrName));
        }
    }
}

static
void
_GatherPrefixedAttrs(
        const std::map<std::string, std::string>& attrPrefixes,
        const std::map<std::string, std::string>& primvarPrefixes,
        const MDagPath &dag, 
        std::vector<std::pair<std::string, std::string>>* attrNames)
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

        // Skip if it's an AbcExport-suffixed hint attribute.
        if (_EndsWithAbcTag(plgName)) {
            continue;
        }

        // If it's a primvar, make the USD name using the primvar lookup map,
        // otherwise, use the normal attribute lookup map.
        bool primvar = _GetPrimvarInterpolation(depFn, plgName, nullptr);
        if (primvar) {
            _AddAttributeNameEntry(plgName, primvarPrefixes, attrNames);
        }
        else {
            _AddAttributeNameEntry(plgName, attrPrefixes, attrNames);
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
    for (const auto& mayaAndUsdNames : entry.attrNames) {
        const std::string& mayaAttrName = mayaAndUsdNames.first;
        const std::string& usdAttrName = mayaAndUsdNames.second;

        MPlug plg = depFn.findPlug(mayaAttrName.c_str(), true);
        UsdAttribute usdAttr;
        TfToken interpolation;

        if (_GetPrimvarInterpolation(depFn, mayaAttrName, &interpolation)) {
            // Treat as custom primvar.
            UsdGeomImageable imageable(entry.usdPrim);
            if (!imageable) {
                MGlobal::displayError(TfStringPrintf(
                        "Cannot create primvar for non-UsdGeomImageable "
                        "USD prim: '%s'",
                        entry.usdPrim.GetPath().GetText()).c_str());
                continue;
            }
            UsdGeomPrimvar primvar = PxrUsdMayaWriteUtil::GetOrCreatePrimvar(
                    plg,
                    imageable,
                    usdAttrName,
                    interpolation,
                    -1,
                    true,
                    false);
            if (primvar) {
                usdAttr = primvar.GetAttr();
            }
        }
        else {
            // Treat as custom attribute.
            usdAttr = PxrUsdMayaWriteUtil::GetOrCreateUsdAttr(
                    plg, entry.usdPrim, usdAttrName, true);
        }

        if (usdAttr) {
            PxrUsdMayaWriteUtil::SetUsdAttr(plg, usdAttr, usdTime);
        }
        else {
            MGlobal::displayError(TfStringPrintf(
                    "Could not create attribute '%s' for "
                    "USD prim: '%s'",
                    usdAttrName.c_str(),
                    entry.usdPrim.GetPath().GetText()).c_str());
            continue;
        }
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
        if (!dag.isValid()) {
            continue;
        }

        MStatus status;
        MFnMesh meshFn(dag, &status);
        if (!status) {
            continue;
        }

        if (UsdGeomMesh usdMesh = UsdGeomMesh::Get(stage, usdPrimPath)) {
            MPlug plug = meshFn.findPlug("SubDivisionMesh");
            bool isSubDivisionMesh = (!plug.isNull() && plug.asBool());

            if (!isSubDivisionMesh) {
                usdMesh.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->none);
            }
        }
    }
}

// this plugin is provided as an example and can be updated to more closely
// match what exporting a file from maya to alembic does.  For now, it just
// supports attrprefix and primvarprefix to export custom attributes and
// primvars.
class AlembicChaser : public PxrUsdMayaChaser
{
public:
    AlembicChaser(
            UsdStagePtr stage,
            const PxrUsdMayaChaserRegistry::FactoryContext::DagToUsdMap& dagToUsd,
            const std::map<std::string, std::string>& attrPrefixes,
            const std::map<std::string, std::string>& primvarPrefixes)
    : _stage(stage)
    , _dagToUsd(dagToUsd)
    {
        for (const auto& p: dagToUsd) {
            const MDagPath& dag = p.first;
            const SdfPath& usdPrimPath = p.second;

            UsdPrim usdPrim = stage->GetPrimAtPath(usdPrimPath);
            if (!dag.isValid() || !usdPrim) {
                continue;
            }

            _pathToEntry.push_back( std::make_pair(dag, _Entry()));

            _Entry& currEntry = _pathToEntry.back().second;
            currEntry.usdPrim = usdPrim;

            _GatherPrefixedAttrs(attrPrefixes, primvarPrefixes, dag,
                    &(currEntry.attrNames));
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

static
void
_ParseMapArgument(
        const std::map<std::string, std::string>& myArgs,
        const std::string argName,
        const std::string& defaultValue,
        bool allowNamespaceValues,
        std::map<std::string, std::string>* outMap)
{
    const auto& it = myArgs.find(argName);
    if (it == myArgs.end()) {
        return;
    }

    const std::string& argValue = it->second;
    for (const std::string& token : TfStringSplit(argValue, ",")) {
        std::vector<std::string> keyAndValue = TfStringSplit(
                TfStringTrim(token), "=");

        std::string key;
        std::string value;
        if (keyAndValue.size() == 1) {
            key = keyAndValue[0];
            value = defaultValue;
        }
        else if (keyAndValue.size() == 2) {
            key = keyAndValue[0];
            value = keyAndValue[1];
        }
        else {
            continue;
        }

        // Primvar prefixes can't contain namespaces (i.e. cannot have
        // colons), so we need to sanitize before continuing.
        if (!allowNamespaceValues && TfStringContains(value, ":")) {
            MGlobal::displayError(TfStringPrintf(
                    "Prefix not allowed because it contains a namespace: '%s'",
                    value.c_str()).c_str());
            continue;
        }

        (*outMap)[key] = value;
    }
}

PXRUSDMAYA_DEFINE_CHASER_FACTORY(alembic, ctx)
{
    std::map<std::string, std::string> myArgs;
    TfMapLookup(ctx.GetJobArgs().allChaserArgs, "alembic", &myArgs);

    // The attrprefix and primvarprefix arguments provide the
    // prefixes for attributes/primvars to export from Maya and an optional
    // replacement prefix for the output USD attribute name.
    //
    // The format of the argument is:
    //    mayaPrefix1[=usdPrefix1],mayaPrefix2[=usdPrefix2],...
    // usdPrefix can contain namespaces (i.e. can have colons) for attrprefix
    // but not for primvarprefix (since primvar names can't have namespaces).
    // If [=usdPrefix] is omitted, then these defaults are used:
    //    - for attrprefix, "userProperties:"
    //    - for primvarprefix, "" (empty)
    //
    // Example attrprefix:
    //    ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:
    // ABC_attrName  -> userProperties:attrName
    // ABC2_attrName -> customPrefix_attrName
    // ABC3_attrName -> attrName
    // ABC4_attrName -> customNamespace:attrName
    //
    // Example primvarprefix:
    //    ABC_,ABC2_=customPrefix_,ABC3_=
    // ABC_attrName  -> attrName
    // ABC2_attrName -> customPrefix_attrName
    // ABC3_attrName -> attrName
    std::map<std::string, std::string> attrPrefixes;
    _ParseMapArgument(myArgs, "attrprefix", _tokens->userProperties.GetString(),
            true, &attrPrefixes);
    std::map<std::string, std::string> primvarPrefixes;
    _ParseMapArgument(myArgs, "primvarprefix", std::string(),
            false, &primvarPrefixes);

    return new AlembicChaser(
            ctx.GetStage(),
            ctx.GetDagToUsdMap(),
            attrPrefixes,
            primvarPrefixes);
}

