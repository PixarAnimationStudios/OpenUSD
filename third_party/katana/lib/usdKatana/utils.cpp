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
#include "pxr/pxr.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdRi/statements.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include "usdKatana/utils.h"
#include "usdKatana/lookAPI.h"

#include <FnLogging/FnLogging.h>

FnLogSetup("PxrUsdKatanaUtils::SGG");

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


static std::string
_ResolvePath(const std::string& path)
{
    return ArGetResolver().Resolve(path);
}

static std::string
_ResolveSearchPath(const std::string& searchPath)
{
    std::vector<std::string> splitPath = TfStringSplit(searchPath, "/");
    if (splitPath.size() < 2)
        return "";

    std::string pathToResolve = _ResolvePath(splitPath[0]);
    if (pathToResolve.empty())
        return "";

    std::vector<std::string> pathElemsRemain(++(splitPath.begin()), 
                                             splitPath.end());
    pathToResolve = TfStringCatPaths(pathToResolve,
                                     TfStringJoin(pathElemsRemain, "/"));
    return _ResolvePath(pathToResolve);
}

static std::string
_ResolveAssetPath(const std::string &assetPath, bool asModel)
{
    // TODO: Implement same-model reference behavior (e.g. working on 
    // src/OtterHair, resolving a path like OtterHair/hairman/Foo.ihair). It's 
    // not clear how to handle same-model references with Ar as it's a very 
    // specific Pr feature.
    if (asModel && ArGetResolver().IsSearchPath(assetPath))
    {
        std::string resolvedPath = _ResolveSearchPath(assetPath);
        if (!resolvedPath.empty())
        {
            return resolvedPath;
        }
    }

    std::string modelName, relPath;
    const std::string resolvedPath = _ResolvePath(assetPath);
    if (!resolvedPath.empty())
        return resolvedPath;

    // If we could not resolve the path, return the given input-- i.e., this
    // may be a new asset path to which a DSO is writing.
    return assetPath;
}

double
PxrUsdKatanaUtils::ReverseTimeSample(double sample)
{
    // Only multiply when the sample is not 0 to avoid writing
    // out a motion block containing -0.
    return (sample == 0.0) ? sample : sample * -1;
}

void
PxrUsdKatanaUtils::ConvertNumVertsToStartVerts( const std::vector<int> &numVertsVec,
                              std::vector<int> *startVertsVec )
{
    startVertsVec->resize( numVertsVec.size()+1 );
    int index = 0;
    for (size_t i=0; i<=numVertsVec.size(); ++i) {
        (*startVertsVec)[i] = index;
        if (i < numVertsVec.size()) {
            index += numVertsVec[i];
        }
    }
}

static void
_ConvertArrayToVector(const VtVec2fArray &a, std::vector<float> *r)
{
    r->resize(a.size()*2);
    size_t i=0;
    TF_FOR_ALL(vec, a) {
        (*r)[i++] = (*vec)[0];
        (*r)[i++] = (*vec)[1];
    }
    TF_VERIFY(i == r->size());
}

static void
_ConvertArrayToVector(const VtVec2dArray &a, std::vector<double> *r)
{
    r->resize(a.size()*2);
    size_t i=0;
    TF_FOR_ALL(vec, a) {
        (*r)[i++] = (*vec)[0];
        (*r)[i++] = (*vec)[1];
    }
    TF_VERIFY(i == r->size());
}

void
PxrUsdKatanaUtils::ConvertArrayToVector(const VtVec3fArray &a, std::vector<float> *r)
{
    r->resize(a.size()*3);
    size_t i=0;
    TF_FOR_ALL(vec, a) {
        (*r)[i++] = (*vec)[0];
        (*r)[i++] = (*vec)[1];
        (*r)[i++] = (*vec)[2];
    }
    TF_VERIFY(i == r->size());
}

static void
_ConvertArrayToVector(const VtVec3dArray &a, std::vector<double> *r)
{
    r->resize(a.size()*3);
    size_t i=0;
    TF_FOR_ALL(vec, a) {
        (*r)[i++] = (*vec)[0];
        (*r)[i++] = (*vec)[1];
        (*r)[i++] = (*vec)[2];
    }
    TF_VERIFY(i == r->size());
}

static void
_ConvertArrayToVector(const VtVec4fArray &a, std::vector<float> *r)
{
    r->resize(a.size()*4);
    size_t i=0;
    TF_FOR_ALL(vec, a) {
        (*r)[i++] = (*vec)[0];
        (*r)[i++] = (*vec)[1];
        (*r)[i++] = (*vec)[2];
        (*r)[i++] = (*vec)[3];
    }
    TF_VERIFY(i == r->size());
}

static void
_ConvertArrayToVector(const VtVec4dArray &a, std::vector<double> *r)
{
    r->resize(a.size()*4);
    size_t i=0;
    TF_FOR_ALL(vec, a) {
        (*r)[i++] = (*vec)[0];
        (*r)[i++] = (*vec)[1];
        (*r)[i++] = (*vec)[2];
        (*r)[i++] = (*vec)[3];
    }
    TF_VERIFY(i == r->size());
}

FnKat::Attribute
PxrUsdKatanaUtils::ConvertVtValueToKatAttr(
        const VtValue & val, 
        bool asShaderParam, bool pathAsModel)
{
    if (val.IsHolding<bool>()) {
        return FnKat::IntAttribute(int(val.Get<bool>()));
    }
    if (val.IsHolding<int>()) {
        return FnKat::IntAttribute(val.Get<int>());
    }
    if (val.IsHolding<float>()) {
        return FnKat::FloatAttribute(val.Get<float>());
    }
    if (val.IsHolding<double>()) {
        return FnKat::DoubleAttribute(val.Get<double>());
    }
    if (val.IsHolding<std::string>()) {
        if (val.Get<std::string>() == "_NO_VALUE_") {
            return FnKat::NullAttribute();
        }
        else {
            return FnKat::StringAttribute(val.Get<std::string>());
        }
    }
    if (val.IsHolding<SdfAssetPath>()) {
        return FnKat::StringAttribute(
            _ResolveAssetPath(val.Get<SdfAssetPath>().GetAssetPath(),
                                    pathAsModel));
    }

    // Compound types require special handling.  Because they do not
    // correspond 1:1 to Fn attribute types, we must describe the
    // type as a separate attribute.
    FnKat::Attribute typeAttr;
    FnKat::Attribute valueAttr;

    if (val.IsHolding<VtArray<std::string> >()) {
        const VtArray<std::string> rawVal = val.Get<VtArray<std::string> >();
        std::vector<std::string> vec(rawVal.begin(), rawVal.end());
        FnKat::StringBuilder builder(/* tupleSize = */ 1);
        builder.set(vec);
        //return builder.build();
        typeAttr = FnKat::StringAttribute(TfStringPrintf("string [%zu]", rawVal.size()));
        valueAttr = builder.build();
    }

    else if (val.IsHolding<VtArray<int> >()) {
        const VtArray<int> rawVal = val.Get<VtArray<int> >();
        std::vector<int> vec(rawVal.begin(), rawVal.end());
        FnKat::IntBuilder builder(/* tupleSize = */ 1);
        builder.set(vec);
        typeAttr = FnKat::StringAttribute(TfStringPrintf("int [%zu]",
                                                         rawVal.size()));
        valueAttr = builder.build();
    }

    else if (val.IsHolding<VtArray<float> >()) {
        const VtArray<float> rawVal = val.Get<VtArray<float> >();
        std::vector<float> vec(rawVal.begin(), rawVal.end());
        FnKat::FloatBuilder builder(/* tupleSize = */ 1);
        builder.set(vec);
        typeAttr = FnKat::StringAttribute(TfStringPrintf("float [%zu]",
                                                         rawVal.size()));
        valueAttr = builder.build();
    }
    else if (val.IsHolding<VtArray<double> >()) {
        const VtArray<double> rawVal = val.Get<VtArray<double> >();
        std::vector<double> vec(rawVal.begin(), rawVal.end());
        FnKat::DoubleBuilder builder(/* tupleSize = */ 1);
        builder.set(vec);
        typeAttr = FnKat::StringAttribute(TfStringPrintf("double [%zu]",
                                                         rawVal.size()));
        valueAttr = builder.build();
    }

    // XXX: Should matrices also be brought in as doubles?
    // What implications does this have? xform.matrix is handled explicitly as
    // a double, and apparently we don't use GfMatrix4f.
    // Shader parameter floats might expect a float matrix?
    if (val.IsHolding<VtArray<GfMatrix4d> >()) {
        const VtArray<GfMatrix4d> rawVal = val.Get<VtArray<GfMatrix4d> >();
        std::vector<float> vec;
        TF_FOR_ALL(mat, rawVal) {
             for (int i=0; i < 4; ++i) {
                 for (int j=0; j < 4; ++j) {
                     vec.push_back( static_cast<float>((*mat)[i][j]) );
                 }
             }
         }
         FnKat::FloatBuilder builder(/* tupleSize = */ 16);
         builder.set(vec);
         valueAttr = builder.build();
         typeAttr = FnKat::StringAttribute(TfStringPrintf("matrix [%zu]",
                                                         rawVal.size()));
    }

    // GfVec2f
    else if (val.IsHolding<GfVec2f>()) {
        const GfVec2f rawVal = val.Get<GfVec2f>();
        FnKat::FloatBuilder builder(/* tupleSize = */ 2);
        std::vector<float> vec;
        vec.resize(2);
        vec[0] = rawVal[0];
        vec[1] = rawVal[1];
        builder.set(vec);
        typeAttr = FnKat::StringAttribute("float [2]");
        valueAttr = builder.build();
    }

    // GfVec2d
    else if (val.IsHolding<GfVec2d>()) {
        const GfVec2d rawVal = val.Get<GfVec2d>();
        FnKat::DoubleBuilder builder(/* tupleSize = */ 2);
        std::vector<double> vec;
        vec.resize(2);
        vec[0] = rawVal[0];
        vec[1] = rawVal[1];
        builder.set(vec);
        typeAttr = FnKat::StringAttribute("double [2]");
        valueAttr = builder.build();
    }

    // GfVec3f
    else if (val.IsHolding<GfVec3f>()) {
        const GfVec3f rawVal = val.Get<GfVec3f>();
        FnKat::FloatBuilder builder(/* tupleSize = */ 3);
        std::vector<float> vec;
        vec.resize(3);
        vec[0] = rawVal[0];
        vec[1] = rawVal[1];
        vec[2] = rawVal[2];
        builder.set(vec);
        typeAttr = FnKat::StringAttribute("float [3]");
        valueAttr = builder.build();
    }

    // GfVec3d
    else if (val.IsHolding<GfVec3d>()) {
        const GfVec3d rawVal = val.Get<GfVec3d>();
        FnKat::DoubleBuilder builder(/* tupleSize = */ 3);
        std::vector<double> vec;
        vec.resize(3);
        vec[0] = rawVal[0];
        vec[1] = rawVal[1];
        vec[2] = rawVal[2];
        builder.set(vec);
        typeAttr = FnKat::StringAttribute("double [3]");
        valueAttr = builder.build();
    }

    // GfVec4f
    else if (val.IsHolding<GfVec4f>()) {
        const GfVec4f rawVal = val.Get<GfVec4f>();
        FnKat::FloatBuilder builder(/* tupleSize = */ 4);
        std::vector<float> vec;
        vec.resize(4);
        vec[0] = rawVal[0];
        vec[1] = rawVal[1];
        vec[2] = rawVal[2];
        vec[3] = rawVal[3];
        builder.set(vec);
        typeAttr = FnKat::StringAttribute("float [4]");
        valueAttr = builder.build();
    }

    // GfVec4d
    else if (val.IsHolding<GfVec4d>()) {
        const GfVec4d rawVal = val.Get<GfVec4d>();
        FnKat::DoubleBuilder builder(/* tupleSize = */ 4);
        std::vector<double> vec;
        vec.resize(4);
        vec[0] = rawVal[0];
        vec[1] = rawVal[1];
        vec[2] = rawVal[2];
        vec[3] = rawVal[3];
        builder.set(vec);
        typeAttr = FnKat::StringAttribute("double [4]");
        valueAttr = builder.build();
    }

    // GfMatrix4d
    // XXX: Should matrices also be brought in as doubles?
    // What implications does this have? xform.matrix is handled explicitly as
    // a double, and apparently we don't use GfMatrix4f.
    // Shader parameter floats might expect a float matrix?
    else if (val.IsHolding<GfMatrix4d>()) {
        const GfMatrix4d rawVal = val.Get<GfMatrix4d>();
        FnKat::FloatBuilder builder(/* tupleSize = */ 16);
        std::vector<float> vec;
        vec.resize(16);
        for (int i=0; i < 4; ++i) {
            for (int j=0; j < 4; ++j) {
                vec[i*4+j] = static_cast<float>(rawVal[i][j]);
            }
        }
        builder.set(vec);
        typeAttr = FnKat::StringAttribute("matrix [1]");
        valueAttr = builder.build();
    }

    // TODO: support complex types such as primvars
    // VtArray<GfVec4f>
    else if (val.IsHolding<VtArray<GfVec4f> >()) {
        const VtArray<GfVec4f> rawVal = val.Get<VtArray<GfVec4f> >();
        std::vector<float> vec;
        _ConvertArrayToVector(rawVal, &vec);
        FnKat::FloatBuilder builder(/* tupleSize = */ 4);
        builder.set(vec);
        valueAttr = builder.build();
    }

    // VtArray<GfVec3f>
    else if (val.IsHolding<VtArray<GfVec3f> >()) {
        const VtArray<GfVec3f> rawVal = val.Get<VtArray<GfVec3f> >();
        std::vector<float> vec;
        PxrUsdKatanaUtils::ConvertArrayToVector(rawVal, &vec);
        FnKat::FloatBuilder builder(/* tupleSize = */ 3);
        builder.set(vec);
        valueAttr = builder.build();
    }

    // VtArray<GfVec2f>
    else if (val.IsHolding<VtArray<GfVec2f> >()) {
        const VtArray<GfVec2f> rawVal = val.Get<VtArray<GfVec2f> >();
        std::vector<float> vec;
        _ConvertArrayToVector(rawVal, &vec);
        FnKat::FloatBuilder builder(/* tupleSize = */ 2);
        builder.set(vec);
        valueAttr = builder.build();
    }

    // VtArray<GfVec4d>
    else if (val.IsHolding<VtArray<GfVec4d> >()) {
        const VtArray<GfVec4d> rawVal = val.Get<VtArray<GfVec4d> >();
        std::vector<double> vec;
        _ConvertArrayToVector(rawVal, &vec);
        FnKat::DoubleBuilder builder(/* tupleSize = */ 4);
        builder.set(vec);
        valueAttr = builder.build();
    }

    // VtArray<GfVec3d>
    else if (val.IsHolding<VtArray<GfVec3d> >()) {
        const VtArray<GfVec3d> rawVal = val.Get<VtArray<GfVec3d> >();
        std::vector<double> vec;
        _ConvertArrayToVector(rawVal, &vec);
        FnKat::DoubleBuilder builder(/* tupleSize = */ 3);
        builder.set(vec);
        valueAttr = builder.build();
    }

    // VtArray<GfVec2d>
    else if (val.IsHolding<VtArray<GfVec2d> >()) {
        const VtArray<GfVec2d> rawVal = val.Get<VtArray<GfVec2d> >();
        std::vector<double> vec;
        _ConvertArrayToVector(rawVal, &vec);
        FnKat::DoubleBuilder builder(/* tupleSize = */ 2);
        builder.set(vec);
        valueAttr = builder.build();
    }

    // VtArray<SdfAssetPath>
    else if (val.IsHolding<VtArray<SdfAssetPath> >()) {
        FnKat::StringBuilder stringBuilder;
        const VtArray<SdfAssetPath> &assetArray = val.Get<VtArray<SdfAssetPath> >();
        TF_FOR_ALL(strItr, assetArray) {
            stringBuilder.push_back(
                _ResolveAssetPath(strItr->GetAssetPath(), pathAsModel));
        }
        FnKat::GroupBuilder attrBuilder;
        attrBuilder.set("type",
                        FnKat::StringAttribute(
                            TfStringPrintf("string [%zu]",assetArray.size())));
        attrBuilder.set("value", stringBuilder.build());
        valueAttr = attrBuilder.build();
    }

    // If being used as a shader param, the type will be provided elsewhere,
    // so simply return the value attribute as-is.
    if (asShaderParam) {
        return valueAttr;
    }
    // Otherwise, return the type & value in a group.
    if (typeAttr.isValid() && valueAttr.isValid()) {
        FnKat::GroupBuilder groupBuilder;
        groupBuilder.set("type", typeAttr);
        groupBuilder.set("value", valueAttr);
        return groupBuilder.build();
    }
    return FnKat::Attribute();
}




FnKat::Attribute
PxrUsdKatanaUtils::ConvertRelTargetsToKatAttr(
        const UsdRelationship &rel, 
        bool asShaderParam)
{
    SdfPathVector targets;
    rel.GetForwardedTargets(&targets);
    FnKat::Attribute valueAttr;
    std::vector<std::string> vec;
    TF_FOR_ALL(targetItr, targets) {
        UsdPrim targetPrim = 
            rel.GetPrim().GetStage()->GetPrimAtPath(*targetItr);
        if (targetPrim) {
            if (targetPrim.IsA<UsdShadeShader>()){
                vec.push_back(PxrUsdKatanaUtils::GenerateShadingNodeHandle(targetPrim));
            }
            else {
                vec.push_back(targetItr->GetString());
            }
        } 
        else if (targetItr->IsPropertyPath()) {
            if (UsdPrim owningPrim = 
                rel.GetPrim().GetStage()->GetPrimAtPath(targetItr->GetPrimPath())) {
                const TfTokenVector &propNames = owningPrim.GetPropertyNames();
                if (std::count(propNames.begin(),
                               propNames.end(),
                               targetItr->GetNameToken())) {
                    vec.push_back(targetItr->GetString());
                }
            }
        }

    }
    FnKat::StringBuilder builder(/* tupleSize = */ 1);
    builder.set(vec);
    valueAttr = builder.build();

    // If being used as a shader param, the type will be provided elsewhere,
    // so simply return the value attribute as-is.
    if (asShaderParam) {
        return valueAttr;
    }

    // Otherwise, return the type & value in a group.
    FnKat::Attribute typeAttr = FnKat::StringAttribute(
        TfStringPrintf("string [%zu]", targets.size()));
    
    if (typeAttr.isValid() && valueAttr.isValid()) {
        FnKat::GroupBuilder groupBuilder;        
        groupBuilder.set("type", typeAttr);
        groupBuilder.set("value", valueAttr);
        return groupBuilder.build();
    }
    return FnKat::Attribute();
}



static bool
_KTypeAndSizeFromUsdVec2(TfToken const &roleName,
                         const char *typeStr,
                         FnKat::Attribute *inputTypeAttr, 
                         FnKat::Attribute *elementSizeAttr)
{
    if (roleName == SdfValueRoleNames->Point) {
        *inputTypeAttr = FnKat::StringAttribute("point2");
    } else if (roleName == SdfValueRoleNames->Vector) {
        *inputTypeAttr = FnKat::StringAttribute("vector2");
    } else if (roleName == SdfValueRoleNames->Normal) {
        *inputTypeAttr = FnKat::StringAttribute("normal2");
    } else if (roleName.IsEmpty()) {
        *inputTypeAttr = FnKat::StringAttribute(typeStr);
        *elementSizeAttr = FnKat::IntAttribute(2);
    } else {
        return false;
    }
    return true;
}

static bool
_KTypeAndSizeFromUsdVec3(TfToken const &roleName,
                         const char *typeStr,
                         FnKat::Attribute *inputTypeAttr, 
                         FnKat::Attribute *elementSizeAttr)
{
    if (roleName == SdfValueRoleNames->Point) {
        *inputTypeAttr = FnKat::StringAttribute("point3");
    } else if (roleName == SdfValueRoleNames->Vector) {
        *inputTypeAttr = FnKat::StringAttribute("vector3");
    } else if (roleName == SdfValueRoleNames->Normal) {
        *inputTypeAttr = FnKat::StringAttribute("normal3");
    } else if (roleName == SdfValueRoleNames->Color) {
        *inputTypeAttr = FnKat::StringAttribute("color3");
    } else if (roleName.IsEmpty()) {
        // Deserves explanation: there is no type in prman
        // (or apparently, katana) that represents 
        // "a 3-vector with no additional behavior/meaning.
        // P-refs fall into this category.  In our pipeline,
        // we have chosen to represent this as float[3] to
        // renderers.
        *inputTypeAttr = FnKat::StringAttribute(typeStr);
        *elementSizeAttr = FnKat::IntAttribute(3);
    } else {
        return false;
    }
    return true;
}

static bool
_KTypeAndSizeFromUsdVec2(TfToken const &roleName,
                         FnKat::Attribute *inputTypeAttr, 
                         FnKat::Attribute *elementSizeAttr)
{
    if (roleName.IsEmpty()) {
        // Deserves explanation: there is no type in prman
        // (or apparently, katana) that represents 
        // "a 2-vector with no additional behavior/meaning.
        // UVs fall into this category.  In our pipeline,
        // we have chosen to represent this as float[2] to
        // renderers.
        *inputTypeAttr = FnKat::StringAttribute("float");
        *elementSizeAttr = FnKat::IntAttribute(2);
    } else {
        return false;
    }
    return true;
}

void
PxrUsdKatanaUtils::ConvertVtValueToKatCustomGeomAttr(
    const VtValue & val, int elementSize,
    const TfToken &roleName,
    FnKat::Attribute *valueAttr,
    FnKat::Attribute *inputTypeAttr,
    FnKat::Attribute *elementSizeAttr )
{
    // The following encoding is taken from Katana's
    // "LOCATIONS AND ATTRIBUTES" doc, which says this about
    // the "geometry.arbitrary.xxx" attributes:
    //
    // > Note: Katana currently supports the following types: float,
    // > double, int, string, color3, color4, normal2, normal3, vector2,
    // > vector3, vector4, point2, point3, point4, matrix9, matrix16.
    // > Depending on the renderer's capabilities, all these nodes might
    // > not be supported.

    // TODO:
    // color4, vector4, point4, matrix9

    if (val.IsHolding<float>()) {
        *valueAttr =  FnKat::FloatAttribute(val.Get<float>());
        *inputTypeAttr = FnKat::StringAttribute("float");
        *elementSizeAttr = FnKat::IntAttribute(elementSize);
        // Leave elementSize empty.
        return;
    }
    if (val.IsHolding<double>()) {
        // XXX(USD) Kat says it supports double here -- should we preserve
        // double-ness?
        *valueAttr =
            FnKat::DoubleAttribute(val.Get<double>());
        *inputTypeAttr = FnKat::StringAttribute("double");
        // Leave elementSize empty.
        return;
    }
    if (val.IsHolding<int>()) {
        *valueAttr = FnKat::IntAttribute(val.Get<int>());
        *inputTypeAttr = FnKat::StringAttribute("int");
        // Leave elementSize empty.
        return;
    }
    if (val.IsHolding<std::string>()) {
        // TODO: support NO_VALUE here?
        // *valueAttr = FnKat::NullAttribute();
        // *inputTypeAttr = FnKat::NullAttribute();
        *valueAttr = FnKat::StringAttribute(val.Get<std::string>());
        *inputTypeAttr = FnKat::StringAttribute("string");
        // Leave elementSize empty.
        return;
    }
    if (val.IsHolding<GfVec2f>()) {
        if (_KTypeAndSizeFromUsdVec2(roleName, "float",
                                     inputTypeAttr, elementSizeAttr)){
            const GfVec2f rawVal = val.Get<GfVec2f>();
            FnKat::FloatBuilder builder(/* tupleSize = */ 2);
            std::vector<float> vec;
            vec.resize(2);
            vec[0] = rawVal[0];
            vec[1] = rawVal[1];
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    if (val.IsHolding<GfVec2d>()) {
        if (_KTypeAndSizeFromUsdVec2(roleName, "double",
                                     inputTypeAttr, elementSizeAttr)){
            const GfVec2d rawVal = val.Get<GfVec2d>();
            FnKat::DoubleBuilder builder(/* tupleSize = */ 2);
            std::vector<double> vec;
            vec.resize(2);
            vec[0] = rawVal[0];
            vec[1] = rawVal[1];
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    if (val.IsHolding<GfVec3f>()) {
        if (_KTypeAndSizeFromUsdVec3(roleName, "float",
                                     inputTypeAttr, elementSizeAttr)){
            const GfVec3f rawVal = val.Get<GfVec3f>();
            FnKat::FloatBuilder builder(/* tupleSize = */ 3);
            std::vector<float> vec;
            vec.resize(3);
            vec[0] = rawVal[0];
            vec[1] = rawVal[1];
            vec[2] = rawVal[2];
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    if (val.IsHolding<GfVec2f>()) {
        if (_KTypeAndSizeFromUsdVec2(roleName, inputTypeAttr, elementSizeAttr)){
            const GfVec2f rawVal = val.Get<GfVec2f>();
            FnKat::FloatBuilder builder(/* tupleSize = */ 2);
            std::vector<float> vec;
            vec.resize(2);
            vec[0] = rawVal[0];
            vec[1] = rawVal[1];
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    if (val.IsHolding<GfVec3d>()) {
        if (_KTypeAndSizeFromUsdVec3(roleName, "double",
                                     inputTypeAttr, elementSizeAttr)){
            const GfVec3d rawVal = val.Get<GfVec3d>();
            FnKat::DoubleBuilder builder(/* tupleSize = */ 3);
            std::vector<double> vec;
            vec.resize(3);
            vec[0] = rawVal[0];
            vec[1] = rawVal[1];
            vec[2] = rawVal[2];
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    // XXX: Should matrices also be brought in as doubles?
    // What implications does this have? xform.matrix is handled explicitly as
    // a double, and apparently we don't use GfMatrix4f.
    // Shader parameter floats might expect a float matrix?
    if (val.IsHolding<GfMatrix4d>()) {
        const GfMatrix4d rawVal = val.Get<GfMatrix4d>();
        FnKat::FloatBuilder builder(/* tupleSize = */ 16);
        std::vector<float> vec;
        vec.resize(16);
        for (int i=0; i < 4; ++i) {
            for (int j=0; j < 4; ++j) {
                vec[i*4+j] = static_cast<float>(rawVal[i][j]);
            }
        }
        builder.set(vec);
        *valueAttr = builder.build();
        *inputTypeAttr = FnKat::StringAttribute("matrix16");
        // Leave elementSize empty.
        return;
    }
    if (val.IsHolding<VtFloatArray>()) {
        const VtFloatArray rawVal = val.Get<VtFloatArray>();
        FnKat::FloatBuilder builder(/* tupleSize = */ 1);
        builder.set( std::vector<float>(rawVal.begin(), rawVal.end()) );
        *valueAttr = builder.build();
        *inputTypeAttr = FnKat::StringAttribute("float");
        if (elementSize > 1) {
            *elementSizeAttr = FnKat::IntAttribute(elementSize);
        }
        return;
    }
    if (val.IsHolding<VtDoubleArray>()) {
        const VtDoubleArray rawVal = val.Get<VtDoubleArray>();
        FnKat::DoubleBuilder builder(/* tupleSize = */ 1);
        builder.set( std::vector<double>(rawVal.begin(), rawVal.end()) );
        *valueAttr = builder.build();
        *inputTypeAttr = FnKat::StringAttribute("double");
        if (elementSize > 1) {
            *elementSizeAttr = FnKat::IntAttribute(elementSize);
        }
        return;
    }
    // XXX: Should matrices also be brought in as doubles?
    // What implications does this have? xform.matrix is handled explicitly as
    // a double, and apparently we don't use GfMatrix4f.
    // Shader parameter floats might expect a float matrix?
    if (val.IsHolding<VtArray<GfMatrix4d> >()) {
        const VtArray<GfMatrix4d> rawVal = val.Get<VtArray<GfMatrix4d> >();
        std::vector<float> vec;
        TF_FOR_ALL(mat, rawVal) {
            for (int i=0; i < 4; ++i) {
                for (int j=0; j < 4; ++j) {
                    vec.push_back( static_cast<float>((*mat)[i][j]) );
                }
            }
        }
        FnKat::FloatBuilder builder(/* tupleSize = */ 16);
        builder.set(vec);
        *valueAttr = builder.build();
        *inputTypeAttr = FnKat::StringAttribute("matrix16");
        if (elementSize > 1) {
            *elementSizeAttr = FnKat::IntAttribute(elementSize);
        }
        return;
    }
    if (val.IsHolding<VtArray<GfVec2f> >()) {
        if (_KTypeAndSizeFromUsdVec2(roleName, "float",
                                     inputTypeAttr, elementSizeAttr)){
            const VtArray<GfVec2f> rawVal = val.Get<VtArray<GfVec2f> >();
            std::vector<float> vec;
            _ConvertArrayToVector(rawVal, &vec);
            FnKat::FloatBuilder builder(/* tupleSize = */ 2);
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    if (val.IsHolding<VtArray<GfVec2d> >()) {
        if (_KTypeAndSizeFromUsdVec2(roleName, "double",
                                     inputTypeAttr, elementSizeAttr)){
            const VtArray<GfVec2d> rawVal = val.Get<VtArray<GfVec2d> >();
            std::vector<double> vec;
            _ConvertArrayToVector(rawVal, &vec);
            FnKat::DoubleBuilder builder(/* tupleSize = */ 2);
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    if (val.IsHolding<VtArray<GfVec3f> >()) {
        if (_KTypeAndSizeFromUsdVec3(roleName, "float",
                                     inputTypeAttr, elementSizeAttr)){
            const VtArray<GfVec3f> rawVal = val.Get<VtArray<GfVec3f> >();
            std::vector<float> vec;
            PxrUsdKatanaUtils::ConvertArrayToVector(rawVal, &vec);
            FnKat::FloatBuilder builder(/* tupleSize = */ 3);
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    if (val.IsHolding<VtArray<GfVec3d> >()) {
        if (_KTypeAndSizeFromUsdVec3(roleName, "double",
                                     inputTypeAttr, elementSizeAttr)){
            const VtArray<GfVec3d> rawVal = val.Get<VtArray<GfVec3d> >();
            std::vector<double> vec;
            _ConvertArrayToVector(rawVal, &vec);
            FnKat::DoubleBuilder builder(/* tupleSize = */ 3);
            builder.set(vec);
            *valueAttr = builder.build();
        }
        return;
    }
    if (val.IsHolding<VtArray<int> >()) {
        const VtArray<int> rawVal = val.Get<VtArray<int> >();
        std::vector<int> vec(rawVal.begin(), rawVal.end());
        FnKat::IntBuilder builder(/* tupleSize = */ 1);
        builder.set(vec);
        *valueAttr = builder.build();
        *inputTypeAttr = FnKat::StringAttribute("int");
        if (elementSize > 1) {
            *elementSizeAttr = FnKat::IntAttribute(elementSize);
        }
        return;
    }
    if (val.IsHolding<VtArray<std::string> >()) {
        const VtArray<std::string> rawVal = val.Get<VtArray<std::string> >();
        std::vector<std::string> vec(rawVal.begin(), rawVal.end());
        FnKat::StringBuilder builder(/* tupleSize = */ 1);
        builder.set(vec);
        *valueAttr = builder.build();
        *inputTypeAttr = FnKat::StringAttribute("string");
        if (elementSize > 1) {
            *elementSizeAttr = FnKat::IntAttribute(elementSize);
        }
        return;
    }

    TF_WARN("Unsupported primvar value type: %s",
            ArchGetDemangled(val.GetTypeid()).c_str());
}

std::string
PxrUsdKatanaUtils::GenerateShadingNodeHandle(const UsdPrim& shadingNode )
{
    std::string name;
    for (UsdPrim curr = shadingNode;
            curr && (
                curr == shadingNode ||
                curr.IsA<UsdGeomScope>());
            curr = curr.GetParent()) {
        name = curr.GetName().GetString() + name;
    }

    return name;
}

void
_FindCameraPaths_Traversal( const UsdPrim &prim, SdfPathVector *result )
{
    // Recursively traverse model hierarchy for camera prims.
    // Note 1: this requires that either prim types be lofted above
    //         payloads for all model references, or that models be loaded.
    // Note 2: Obviously, we will not find cameras embedded within models.
    //         We have made this restriction consciously to reduce the
    //         latency of camera-enumeration
    TF_FOR_ALL(child, prim.GetFilteredChildren(UsdPrimIsModel)) {
        if (child->IsA<UsdGeomCamera>()) {
            result->push_back(child->GetPath());
        }
        _FindCameraPaths_Traversal(*child, result);
    }
}

SdfPathVector
PxrUsdKatanaUtils::FindCameraPaths(const UsdStageRefPtr& stage)
{
    SdfPathVector result;
    _FindCameraPaths_Traversal( stage->GetPseudoRoot(), &result );
    return result;
}

std::string
PxrUsdKatanaUtils::ConvertUsdPathToKatLocation(
        const SdfPath& path,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    if (!TF_VERIFY(path.IsAbsolutePath())) {
        return std::string();
    }

    // If the current prim is in a master for the sake of processing
    // an instance, replace the master path by the instance path before
    // converting to a katana location.
    SdfPath resolvedPath;
    if (data.GetUsdPrim().IsInMaster() && !data.GetInstancePath().IsEmpty())
    {
        resolvedPath = path.ReplacePrefix(data.GetMasterPath(), data.GetInstancePath());
    }
    else
    {
        resolvedPath = path;
    }

    // Convert to the corresponding katana location by stripping
    // off the leading rootPath and prepending rootLocation.
    return TfNormPath(
        data.GetUsdInArgs()->GetRootLocationPath()+"/"+
        resolvedPath.GetString().substr(data.GetUsdInArgs()->GetIsolatePath().size()) );
}

std::string
PxrUsdKatanaUtils::ConvertUsdMaterialPathToKatLocation(
        const SdfPath& path,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    const std::string basePath = ConvertUsdPathToKatLocation(path, data);

    UsdPrim prim = 
        UsdUtilsGetPrimAtPathWithForwarding(
            data.GetUsdInArgs()->GetStage(), path);

    if (!prim.IsValid()) {
        return basePath;
    }

    SdfPath parentPath;

    UsdShadeMaterial materialSchema = UsdShadeMaterial(prim);
    if (materialSchema.HasBaseMaterial()) {
        // This base material is defined as a derivesFrom relationship
        parentPath = materialSchema.GetBaseMaterialPath();
    }

    UsdPrim parentPrim = 
        data.GetUsdInArgs()->GetStage()->GetPrimAtPath(parentPath);


    // Asset sanity check. It is possible the derivesFrom relationship
    // for a Look exists but references a non-existent location. If so,
    // simply return the base path.
    if (!parentPrim)
    {
        return basePath;
    }

    if (parentPrim.IsInMaster())
    {
        // If the prim is inside a master, then attempt to translate the
        // parentPath to the corresponding uninstanced path, assuming that 
        // the given forwarded path and parentPath belong to the same master.
        const SdfPath primPath = prim.GetPath();
        std::pair<SdfPath, SdfPath> prefixPair = 
            primPath.RemoveCommonSuffix(path);
        const SdfPath& masterPath = prefixPair.first;
        const SdfPath& instancePath = prefixPair.second;
        
        // XXX: Assuming that the base look (parent) path belongs to the same
        // master! If it belongs to a different master, we don't have the 
        // context needed to resolve it.
        if (parentPath.HasPrefix(masterPath)) {
            parentPath = instancePath.AppendPath(parentPath.ReplacePrefix(
                masterPath, SdfPath::ReflexiveRelativePath()));
        } else {
            FnLogWarn("Error converting UsdMaterial path <" << path.GetString() <<
                "> to katana location: could not map parent path <" <<
                parentPath.GetString() << "> to uninstanced location.");
            return basePath;
        }
    }

    std::string parentKatLoc = 
        ConvertUsdMaterialPathToKatLocation(parentPath, data);

    std::string primName = prim.GetName();
    UsdAttribute primNameAttr = UsdKatanaLookAPI(prim).GetPrimNameAttr();
    if (primNameAttr.IsValid()) {
        primNameAttr.Get(&primName);
    }
    return parentKatLoc + "/" + primName;
}

bool 
PxrUsdKatanaUtils::ModelGroupIsAssembly(const UsdPrim &prim)
{
    if (!(prim.IsGroup() && prim.GetParent()) || prim.IsInMaster())
        return false;

    // XXX with bug/102670, this test will be trivial: prim.IsAssembly()
    TfToken kind;

    if (!UsdModelAPI(prim).GetKind(&kind)){
        TF_WARN("Expected to find authored kind on prim <%s>",
                prim.GetPath().GetText());
        return false;
    }

    return KindRegistry::IsA(kind, KindTokens->assembly) 
        || PxrUsdKatanaUtils::ModelGroupNeedsProxy(prim);
}

bool 
PxrUsdKatanaUtils::PrimIsSubcomponent(const UsdPrim &prim)
{
    // trying to make this early exit for leaf geometry.
    // unfortunately there's no good IsXXX() method to test
    // for subcomponents -- they aren't Models or Groups --
    // but they do have Payloads.
    if (!(prim.HasPayload() && prim.GetParent()))
        return false;

    // XXX(spiff) with bug/102670, this test will be trivial: prim.IsAssembly()
    TfToken kind;

    if (!UsdModelAPI(prim).GetKind(&kind)){
        TF_WARN("Expected to find authored kind on prim <%s>",
                prim.GetPath().GetText());
        return false;
    }

    return KindRegistry::IsA(kind, KindTokens->subcomponent);
}



bool 
PxrUsdKatanaUtils::ModelGroupNeedsProxy(const UsdPrim &prim)
{
    // Check to see if all children are not group models, if so, we'll make
    // this an assembly as a load/proxy optimization.
    TF_FOR_ALL(childIt, prim.GetChildren()) {
        if (childIt->IsGroup())
            return false; 
    }

    return true;
}

bool 
PxrUsdKatanaUtils::IsModelAssemblyOrComponent(const UsdPrim& prim)
{
    if (!prim.IsModel() || prim.IsInMaster()) {
        return false;
    }

    {
        // handle cameras as they are not "assembly" or "component" to katana.
        if (prim.IsA<UsdGeomCamera>()) {
            return false;
        }

        // XXX: A prim whose kind *equals* "group" should never be
        // considered an assembly or component
        // http://bugzilla.pixar.com/show_bug.cgi?id=106971#c1
        TfToken kind;
        if (!UsdModelAPI(prim).GetKind(&kind)){
            TF_WARN("Expected to find authored kind on prim <%s>",
                    prim.GetPath().GetText());
            return false;
        }
        if (kind == KindTokens->group) {
            return false;
        }
    }

    // XXX: We'll be able to implement all of this in a
    // much more clear way in the future.  for now, just check if it has this
    // authored metadata.
    // XXX: coming with bug/102670
    if (prim.HasAuthoredMetadata(TfToken("references"))) {
        return true;
    }

    return false;
}

bool
PxrUsdKatanaUtils::IsAttributeVarying(const  UsdAttribute& attr, double currentTime) {
    // XXX: Copied from UsdImagingDelegate::_TrackVariability.
    // XXX: This logic is highly sensitive to the underlying quantization of
    //      time. Also, the epsilon value (.000001) may become zero for large
    //      time values.
    double lower, upper, queryTime;
    bool hasSamples;
    queryTime = currentTime + 0.000001;
    // TODO: migrate this logic into UsdAttribute.
    if (attr.GetBracketingTimeSamples(queryTime, &lower, &upper, &hasSamples)
        && hasSamples)
    {
        // The potential results are:
        //    * Requested time was between two time samples
        //    * Requested time was out of the range of time samples (lesser)
        //    * Requested time was out of the range of time samples (greater)
        //    * There was a time sample exactly at the requested time or
        //      there was exactly one time sample.
        // The following logic determines which of these states we are in.

        // Between samples?
        if (lower != upper) {
            return true;
        }

        // Out of range (lower) or exactly on a time sample?
        attr.GetBracketingTimeSamples(lower+.000001,
                                      &lower, &upper, &hasSamples);
        if (lower != upper) {
            return true;
        }

        // Out of range (greater)?
        attr.GetBracketingTimeSamples(lower-.000001,
                                      &lower, &upper, &hasSamples);
        if (lower != upper) {
            return true;
        }
        // Really only one time sample --> not varying for our purposes
    }
    return false;
}

std::string PxrUsdKatanaUtils::GetModelInstanceName(const UsdPrim& prim)
{
    if (!prim) {
        return std::string();
    }

    bool isPseudoRoot = prim.GetPath() == SdfPath::AbsoluteRootPath();

    if (!isPseudoRoot) {
        std::string modelInstanceName;
        if (prim.GetAttribute(TfToken(
                        UsdRiStatements::MakeRiAttributePropertyName(
                            "ModelInstance")))
                .Get(&modelInstanceName)) {
            return modelInstanceName;
        }

        if (PxrUsdKatanaUtils::IsModelAssemblyOrComponent(prim)) {
            FnLogWarn(TfStringPrintf("Could not get modelInstanceName for "
                     "assembly/component '%s'. Using prim.name", 
                     prim.GetPath().GetText()).c_str());
            return prim.GetName();
        }
    }

    // Recurse to namespace parents so we can find the enclosing model
    // instance.  (Note that on the katana side, the modelInstanceName
    // attribute inherits.)
    //
    // XXX tools OM is working on a much more clear future way to handle
    // this, but until then we recurse upwards.
    return PxrUsdKatanaUtils::GetModelInstanceName( prim.GetParent() );
}

std::string 
PxrUsdKatanaUtils::GetAssetName(const UsdPrim& prim)
{
    bool isPseudoRoot = prim.GetPath() == SdfPath::AbsoluteRootPath();

    if (isPseudoRoot)
        return std::string();

    UsdModelAPI model(prim);
    std::string assetName;
    if (model.GetAssetName(&assetName)) {
        if (!assetName.empty())
            return assetName;
    }

    // if we can make it so this function only gets called on assets, we
    // should re-introduce the warning if we were unable to really obtain
    // the model name.  for now, removing the warning because it currently
    // spews for things like cameras, etc.
    if (PxrUsdKatanaUtils::IsModelAssemblyOrComponent(prim)) {
        FnLogWarn(TfStringPrintf("Could not get assetName for "
                    "assembly/component '%s'. Using prim.name", 
                    prim.GetPath().GetText()).c_str());
    }

    return prim.GetName();
}

bool
PxrUsdKatanaUtils::IsBoundable(const UsdPrim& prim)
{
    if (prim.IsModel() &&
        ((!prim.IsGroup()) || PxrUsdKatanaUtils::ModelGroupIsAssembly(prim)))
        return true;

    if (PxrUsdKatanaUtils::PrimIsSubcomponent(prim))
        return true;

    return prim.IsA<UsdGeomBoundable>();
}

FnKat::DoubleAttribute
PxrUsdKatanaUtils::ConvertBoundsToAttribute(
        const std::vector<GfBBox3d>& bounds,
        const std::vector<double>& motionSampleTimes,
        const bool isMotionBackward,
        bool* hasInfiniteBounds)
{
    FnKat::DoubleBuilder boundBuilder(6);

    // There must be one bboxCache per motion sample, for efficiency purposes.
    if (!TF_VERIFY(bounds.size() == motionSampleTimes.size())) {
        return FnKat::DoubleAttribute();
    }

    for (size_t i = 0; i < motionSampleTimes.size(); i++) {
        const GfBBox3d& bbox = bounds[i];

        double relSampleTime = motionSampleTimes[i];

        const GfRange3d range = bbox.ComputeAlignedBox();
        const GfVec3d& min = range.GetMin();
        const GfVec3d& max = range.GetMax();

        // Don't return empty bboxes, Katana/PRMan will not behave well.
        if (range.IsEmpty()) {
            // FnLogWarn(TfStringPrintf(
            //     "Failed to compute bound for <%s>", prim.GetPath().GetText()));
            return FnKat::DoubleAttribute();
        }

        if ( isinf(min[0]) || isinf(min[1]) || isinf(min[2]) ||
            isinf(max[0]) || isinf(max[1]) || isinf(max[2]) ) {
            *hasInfiniteBounds = true;
        }

        std::vector<double> &boundData = boundBuilder.get(
            isMotionBackward ? PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime) : relSampleTime);

        boundData.push_back( min[0] );
        boundData.push_back( max[0] );
        boundData.push_back( min[1] );
        boundData.push_back( max[1] );
        boundData.push_back( min[2] );
        boundData.push_back( max[2] );
    }

    return boundBuilder.build();
}

namespace
{
    typedef std::map<std::string, std::string> StringMap;
    typedef std::set<std::string> StringSet;
    typedef std::map<std::string, StringSet> StringSetMap;
    
    void _walkForMasters(const UsdPrim& prim, StringMap & masterToKey,
            StringSetMap & keyToMasters)
    {
        if (prim.IsInstance())
        {
            const UsdPrim master = prim.GetMaster();
            
            if (master.IsValid())
            {
                std::string masterPath = master.GetPath().GetString();
                
                if (masterToKey.find(masterPath) == masterToKey.end())
                {
                    std::string assetName;
                    UsdModelAPI(prim).GetAssetName(&assetName);
                    if (assetName.empty())
                    {
                        assetName = "master";
                    }
                    
                    std::ostringstream buffer;
                    buffer << assetName << "/variants";
                    
                    UsdVariantSets variantSets = prim.GetVariantSets();
                    
                    std::vector<std::string> names;
                    variantSets.GetNames(&names);
                    TF_FOR_ALL(I, names)
                    {
                        const std::string & variantName = (*I);
                        std::string variantValue =
                                variantSets.GetVariantSet(
                                        variantName).GetVariantSelection();
                        buffer << "__" << variantName << "_" << variantValue;
                    }
                    
                    std::string key = buffer.str();
                    masterToKey[masterPath] = key;
                    keyToMasters[key].insert(masterPath);
                    //TODO, Warn when there are multiple masters with the
                    //      same key.
                    
                    _walkForMasters(master, masterToKey, keyToMasters);
                }
            }
        }
        
        
        TF_FOR_ALL(childIter, prim.GetFilteredChildren(
                UsdPrimIsDefined && UsdPrimIsActive && !UsdPrimIsAbstract))
        {
            const UsdPrim& child = *childIter;
            _walkForMasters(child, masterToKey, keyToMasters);
        }
    }
}

FnKat::GroupAttribute
PxrUsdKatanaUtils::BuildInstanceMasterMapping(
        const UsdStageRefPtr& stage)
{
    StringMap masterToKey;
    StringSetMap keyToMasters;
    _walkForMasters(stage->GetPseudoRoot(), masterToKey, keyToMasters);
    
    
    FnKat::GroupBuilder gb;
    TF_FOR_ALL(I, keyToMasters)
    {
        const std::string & key = (*I).first;
        const StringSet & masters = (*I).second;
        
        size_t i = 0;
        
        TF_FOR_ALL(J, masters)
        {
            const std::string & master = (*J);
            
            std::ostringstream buffer;
            
            buffer << key << "/m" << i;
            gb.set(FnKat::DelimiterEncode(master),
                    FnKat::StringAttribute(buffer.str()));
            
            ++i;
        }
    }
    
    
    return gb.build();
}


PXR_NAMESPACE_CLOSE_SCOPE

