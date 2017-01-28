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
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomImageable,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
UsdGeomImageable::~UsdGeomImageable()
{
}

/* static */
UsdGeomImageable
UsdGeomImageable::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomImageable();
    }
    return UsdGeomImageable(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdGeomImageable::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomImageable>();
    return tfType;
}

/* static */
bool 
UsdGeomImageable::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomImageable::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomImageable::GetVisibilityAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->visibility);
}

UsdAttribute
UsdGeomImageable::CreateVisibilityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->visibility,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImageable::GetPurposeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->purpose);
}

UsdAttribute
UsdGeomImageable::CreatePurposeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->purpose,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdGeomImageable::GetProxyPrimRel() const
{
    return GetPrim().GetRelationship(UsdGeomTokens->proxyPrim);
}

UsdRelationship
UsdGeomImageable::CreateProxyPrimRel() const
{
    return GetPrim().CreateRelationship(UsdGeomTokens->proxyPrim,
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
UsdGeomImageable::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->visibility,
        UsdGeomTokens->purpose,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
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
// Just remember to wrap code in the pxr namespace macros:
// PXR_NAMESPACE_OPEN_SCOPE, PXR_NAMESPACE_CLOSE_SCOPE.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include <boost/assign/list_of.hpp>

PXR_NAMESPACE_OPEN_SCOPE

UsdGeomPrimvar 
UsdGeomImageable::CreatePrimvar(const TfToken& attrName,
                                const SdfValueTypeName &typeName,
                                const TfToken& interpolation,
                                int elementSize,
                                bool custom)
{
    const UsdPrim prim = GetPrim();

    UsdGeomPrimvar primvar(prim, attrName, typeName, custom);

    if (primvar){
        if (!interpolation.IsEmpty())
            primvar.SetInterpolation(interpolation);
        if (elementSize > 0)
            primvar.SetElementSize(elementSize);
    }
    // otherwise, errors have already been issued
    return primvar;
}


UsdGeomPrimvar
UsdGeomImageable::GetPrimvar(const TfToken &name) const
{
    // The getter SHOULD issue an error if 'name' is malformed, which
    // _MakeNamespaced() will do for us.
    return UsdGeomPrimvar(GetPrim().GetAttribute
                           (UsdGeomPrimvar::_MakeNamespaced(name)));
}

std::vector<UsdGeomPrimvar>
UsdGeomImageable::_MakePrimvars(std::vector<UsdProperty> const &props) const
{
    std::vector<UsdGeomPrimvar> primvars;
    primvars.reserve(props.size());
    
    TF_FOR_ALL(prop, props) {
        // All prefixed properties except the ones that contain extra namespaces 
        // (eg. the "indices" attributes belonging to indexed primvars) will be 
        // valid primvars.
        if (UsdGeomPrimvar primvar = UsdGeomPrimvar(prop->As<UsdAttribute>()))
            primvars.push_back(primvar);
    }
    return primvars;
}

std::vector<UsdGeomPrimvar>
UsdGeomImageable::GetPrimvars() const
{
    const UsdPrim &prim = GetPrim();
    return _MakePrimvars(prim ? prim.GetPropertiesInNamespace(
                             UsdGeomPrimvar::_GetNamespacePrefix()) :
                         std::vector<UsdProperty>());
}

std::vector<UsdGeomPrimvar>
UsdGeomImageable::GetAuthoredPrimvars() const
{
    const UsdPrim &prim = GetPrim();
    return _MakePrimvars(prim 
                         ? prim.GetAuthoredPropertiesInNamespace(
                             UsdGeomPrimvar::_GetNamespacePrefix())
                         : std::vector<UsdProperty>());
}

bool
UsdGeomImageable::HasPrimvar(const TfToken &name) const
{
    TfToken primvarName = UsdGeomPrimvar::_MakeNamespaced(name, /* quiet */true);
    return primvarName.IsEmpty() ? false : 
        UsdGeomPrimvar::IsPrimvar(GetPrim().GetAttribute(primvarName));
}

/* static */
const TfTokenVector &
UsdGeomImageable::GetOrderedPurposeTokens()
{
    static const TfTokenVector purposeTokens = boost::assign::list_of
        (UsdGeomTokens->default_)
        (UsdGeomTokens->render)
        (UsdGeomTokens->proxy)
        (UsdGeomTokens->guide);

    return purposeTokens;
}

static
TfToken
_ComputeVisibility(UsdPrim const &prim, UsdTimeCode const &time)
{
    if (UsdPrim parent = prim.GetParent()){
        TfToken myVis = _ComputeVisibility(parent, time);
        if (myVis == UsdGeomTokens->invisible)
            return myVis;
        if (UsdGeomImageable ip = UsdGeomImageable(prim)){
            ip.GetVisibilityAttr().Get(&myVis, time);
        }

        return myVis;
    }

    return UsdGeomTokens->inherited;
}

TfToken
UsdGeomImageable::ComputeVisibility(UsdTimeCode const &time) const
{
    return _ComputeVisibility(GetPrim(), time);
}


static
TfToken
_ComputePurpose(UsdPrim const &prim, UsdPrim *root=NULL)
{
    if (UsdPrim parent = prim.GetParent()){
        TfToken myPurpose = _ComputePurpose(parent, root);
        if (myPurpose != UsdGeomTokens->default_)
            return myPurpose;
        if (UsdGeomImageable ip = UsdGeomImageable(prim)){
            ip.GetPurposeAttr().Get(&myPurpose);
            if (root){
                *root = prim;
            }
        }

        return myPurpose;
    }

    return UsdGeomTokens->default_;
}

static void
_SetVisibility(const UsdGeomImageable &imageable, const TfToken &visState, 
               const UsdTimeCode &time)
{
   imageable.CreateVisibilityAttr().Set(visState, time);
}

// Returns true if the imageable has its visibility set to 'invisible' at the 
// given time. It also sets the visibility to inherited before returning.
static 
bool
_SetInheritedIfInvisible(const UsdGeomImageable &imageable, const UsdTimeCode &time)
{
    TfToken vis;
    if (imageable.GetVisibilityAttr().Get(&vis, time)) {
        if (vis == UsdGeomTokens->invisible) {
            _SetVisibility(imageable, UsdGeomTokens->inherited, time);
            return true;
        }
    }
    return false;
}

static
void
_MakeVisible(const UsdPrim &prim, UsdTimeCode const &time, bool *hasInvisibleAncestor)
{
    if (UsdPrim parent = prim.GetParent()) {
        _MakeVisible(parent, time, hasInvisibleAncestor);

        if (UsdGeomImageable imageableParent = UsdGeomImageable(parent)) {

            // Change visibility of parent to inherited if it is invisible.
            if (*hasInvisibleAncestor ||
                _SetInheritedIfInvisible(imageableParent, time))  {

                *hasInvisibleAncestor = true;

                // Invis all siblings of prim.
                UsdPrim::SiblingRange children = parent.GetAllChildren();
                TF_FOR_ALL(childIt, children) {
                    const UsdPrim &childPrim = *childIt;
                    
                    if (childPrim != prim) {
                        UsdGeomImageable imageableChild(childPrim);
                        if (imageableChild) {
                            _SetVisibility(imageableChild, 
                                UsdGeomTokens->invisible, time);
                        }
                    }
                }
            }
        }
    }
}

void 
UsdGeomImageable::MakeVisible(const UsdTimeCode &time) const
{
    bool hasInvisibleAncestor = false;
    _SetInheritedIfInvisible(*this, time);
    _MakeVisible(GetPrim(), time, &hasInvisibleAncestor);
}

void
UsdGeomImageable::MakeInvisible(const UsdTimeCode &time) const
{
    UsdAttribute visAttr = CreateVisibilityAttr();
    TfToken myVis;
    if (!visAttr.Get(&myVis, time) || myVis != UsdGeomTokens->invisible) {
        visAttr.Set(UsdGeomTokens->invisible, time);
    }
}

TfToken
UsdGeomImageable::ComputePurpose() const
{
    return _ComputePurpose(GetPrim());
}

UsdPrim
UsdGeomImageable::ComputeProxyPrim(UsdPrim *renderPrim) const
{
    UsdPrim  purposeRoot, self=GetPrim();
    
    TfToken purpose = _ComputePurpose(self, &purposeRoot);

    if (purpose == UsdGeomTokens->render){
        TF_VERIFY(purposeRoot);
        SdfPathVector target;
        UsdRelationship  proxyPrimRel = 
            UsdGeomImageable(purposeRoot).GetProxyPrimRel();
        if (proxyPrimRel.GetForwardedTargets(&target)){
            if (target.size() == 1){
                if (UsdPrim proxy = self.GetStage()->GetPrimAtPath(target[0])){
                    if (_ComputePurpose(proxy) != UsdGeomTokens->proxy){
                        TF_WARN("Prim <%s>, targeted as proxyPrim of prim "
                                "<%s> does not have purpose 'proxy'",
                                proxy.GetPath().GetText(),
                                purposeRoot.GetPath().GetText());
                        return UsdPrim();
                    }
                    if (renderPrim){
                        *renderPrim = purposeRoot;
                    }
                    return proxy;
                }
            }
            else if (target.size() > 1){
                TF_WARN("Found multiple targets for proxyPrim rel on "
                        "prim <%s>", purposeRoot.GetPath().GetText());
            }
        }
    }

    return UsdPrim();
}

bool
UsdGeomImageable::SetProxyPrim(const UsdPrim &proxy) const
{
    if (proxy){
        SdfPathVector targets {proxy.GetPath()};
        return CreateProxyPrimRel().SetTargets(targets);
    }
    return false;
}

bool
UsdGeomImageable::SetProxyPrim(const UsdSchemaBase &proxy) const
{
    if (proxy){
        SdfPathVector targets {proxy.GetPrim().GetPath()};
        return CreateProxyPrimRel().SetTargets(targets);
    }
    return false;
}


static
TfTokenVector
_MakePurposeVector(TfToken const &purpose1,
                   TfToken const &purpose2,
                   TfToken const &purpose3,
                   TfToken const &purpose4)
{
    TfTokenVector purposes;
    
    if (!purpose1.IsEmpty()) purposes.push_back(purpose1);
    if (!purpose2.IsEmpty()) purposes.push_back(purpose2);
    if (!purpose3.IsEmpty()) purposes.push_back(purpose3);
    if (!purpose4.IsEmpty()) purposes.push_back(purpose4);

    return purposes;
}

GfBBox3d
UsdGeomImageable::ComputeWorldBound(UsdTimeCode const& time,
                                    TfToken const &purpose1,
                                    TfToken const &purpose2,
                                    TfToken const &purpose3,
                                    TfToken const &purpose4) const
{
    TfTokenVector purposes = _MakePurposeVector(purpose1, purpose2,
                                                purpose3, purpose4);

    if (purposes.empty()){
        TF_CODING_ERROR("Must include at least one purpose when computing"
                        " bounds for prim at path <%s>.  See "
                        "UsdGeomImageable::GetPurposeAttr().",
                        GetPrim().GetPath().GetText());
        return GfBBox3d();
    }
    return UsdGeomBBoxCache(time, purposes).ComputeWorldBound(GetPrim());
}

GfBBox3d
UsdGeomImageable::ComputeLocalBound(UsdTimeCode const& time,
                                    TfToken const &purpose1,
                                    TfToken const &purpose2,
                                    TfToken const &purpose3,
                                    TfToken const &purpose4) const
{
    TfTokenVector purposes = _MakePurposeVector(purpose1, purpose2,
                                                purpose3, purpose4);

    if (purposes.empty()){
        TF_CODING_ERROR("Must include at least one purpose when computing"
                        " bounds for prim at path <%s>.  See "
                        "UsdGeomImageable::GetPurposeAttr().",
                        GetPrim().GetPath().GetText());
        return GfBBox3d();
    }
    return UsdGeomBBoxCache(time, purposes).ComputeLocalBound(GetPrim());
}

GfBBox3d
UsdGeomImageable::ComputeUntransformedBound(UsdTimeCode const& time,
                                            TfToken const &purpose1,
                                            TfToken const &purpose2,
                                            TfToken const &purpose3,
                                            TfToken const &purpose4) const
{
    TfTokenVector purposes = _MakePurposeVector(purpose1, purpose2,
                                                purpose3, purpose4);

    if (purposes.empty()){
        TF_CODING_ERROR("Must include at least one purpose when computing"
                        " bounds for prim at path <%s>.  See "
                        "UsdGeomImageable::GetPurposeAttr().",
                        GetPrim().GetPath().GetText());
        return GfBBox3d();
    }
    return
        UsdGeomBBoxCache(time, purposes).ComputeUntransformedBound(GetPrim());
}

GfMatrix4d
UsdGeomImageable::ComputeLocalToWorldTransform(UsdTimeCode const &time) const
{
    return UsdGeomXformCache(time).GetLocalToWorldTransform(GetPrim());
}

GfMatrix4d
UsdGeomImageable::ComputeParentToWorldTransform(UsdTimeCode const &time) const
{
    return UsdGeomXformCache(time).GetParentToWorldTransform(GetPrim());
}

PXR_NAMESPACE_CLOSE_SCOPE
