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
#include "pxr/usd/usdNpr/contour.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdNprContour,
        TfType::Bases< UsdGeomMesh > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Contour")
    // to find TfType<UsdNprContour>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdNprContour>("Contour");
}

/* virtual */
UsdNprContour::~UsdNprContour()
{
}

/* static */
UsdNprContour
UsdNprContour::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdNprContour();
    }
    return UsdNprContour(stage->GetPrimAtPath(path));
}

/* static */
UsdNprContour
UsdNprContour::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Contour");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdNprContour();
    }
    return UsdNprContour(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdNprContour::_GetSchemaType() const {
    return UsdNprContour::schemaType;
}

/* static */
const TfType &
UsdNprContour::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdNprContour>();
    return tfType;
}

/* static */
bool 
UsdNprContour::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdNprContour::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdNprContour::GetDrawSilhouetteAttr() const
{
    return GetPrim().GetAttribute(UsdNprTokens->drawSilhouette);
}

UsdAttribute
UsdNprContour::CreateDrawSilhouetteAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdNprTokens->drawSilhouette,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdNprContour::GetDrawBoundaryAttr() const
{
    return GetPrim().GetAttribute(UsdNprTokens->drawBoundary);
}

UsdAttribute
UsdNprContour::CreateDrawBoundaryAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdNprTokens->drawBoundary,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdNprContour::GetDrawCreaseAttr() const
{
    return GetPrim().GetAttribute(UsdNprTokens->drawCrease);
}

UsdAttribute
UsdNprContour::CreateDrawCreaseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdNprTokens->drawCrease,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdNprContour::GetSilhouetteWidthAttr() const
{
    return GetPrim().GetAttribute(UsdNprTokens->silhouetteWidth);
}

UsdAttribute
UsdNprContour::CreateSilhouetteWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdNprTokens->silhouetteWidth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdNprContour::GetBoundaryWidthAttr() const
{
    return GetPrim().GetAttribute(UsdNprTokens->boundaryWidth);
}

UsdAttribute
UsdNprContour::CreateBoundaryWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdNprTokens->boundaryWidth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdNprContour::GetCreaseWidthAttr() const
{
    return GetPrim().GetAttribute(UsdNprTokens->creaseWidth);
}

UsdAttribute
UsdNprContour::CreateCreaseWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdNprTokens->creaseWidth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdNprContour::GetContourViewPointRel() const
{
    return GetPrim().GetRelationship(UsdNprTokens->contourViewPoint);
}

UsdRelationship
UsdNprContour::CreateContourViewPointRel() const
{
    return GetPrim().CreateRelationship(UsdNprTokens->contourViewPoint,
                       /* custom = */ false);
}

UsdRelationship
UsdNprContour::GetContourSurfacesRel() const
{
    return GetPrim().GetRelationship(UsdNprTokens->contourSurfaces);
}

UsdRelationship
UsdNprContour::CreateContourSurfacesRel() const
{
    return GetPrim().CreateRelationship(UsdNprTokens->contourSurfaces,
                       /* custom = */ false);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdNprContour::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdNprTokens->drawSilhouette,
        UsdNprTokens->drawBoundary,
        UsdNprTokens->drawCrease,
        UsdNprTokens->silhouetteWidth,
        UsdNprTokens->boundaryWidth,
        UsdNprTokens->creaseWidth,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomMesh::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
PXR_NAMESPACE_OPEN_SCOPE

std::vector<UsdGeomMesh> 
UsdNprContour::GetContourSurfaces() const
{
  SdfPathVector targets;
  UsdRelationship contourSurfacesRel = GetContourSurfacesRel();
  contourSurfacesRel.GetTargets(&targets);
  std::vector<UsdGeomMesh> contourSurfaces;
  if (targets.size() < 1) return contourSurfaces;
  for(int i=0;i<targets.size();++i)
  {
    SdfPath primPath = targets[i].GetAbsoluteRootOrPrimPath();
    UsdPrim prim = GetPrim().GetStage()->GetPrimAtPath(primPath);
    if(prim.IsA<UsdGeomMesh>())
      contourSurfaces.push_back(UsdGeomMesh(prim));
  }
  return contourSurfaces;
  
}

PXR_NAMESPACE_CLOSE_SCOPE
