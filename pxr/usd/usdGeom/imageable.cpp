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


/* virtual */
UsdSchemaKind UsdGeomImageable::_GetSchemaKind() const {
    return UsdGeomImageable::schemaKind;
}

/* virtual */
UsdSchemaKind UsdGeomImageable::_GetSchemaType() const {
    return UsdGeomImageable::schemaType;
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
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    USD_GEOM_IMAGEABLE_DEPRECATE_PRIMVARS_API, false,
    "Whether UsdGeomImageable's primvars API should issue deprecation warnings");

inline static void
_IssueAPIWarningIfDeprecationFlagEnabled()
{
    if (TfGetEnvSetting(USD_GEOM_IMAGEABLE_DEPRECATE_PRIMVARS_API)) {
        TF_WARN("API deprecation warning: UsdGeomImageable's primvars API "
                "will be removed in the future.  Use UsdGeomPrimvarsAPI "
                "instead.");
    }
}

UsdGeomPrimvar 
UsdGeomImageable::CreatePrimvar(const TfToken& attrName,
                                const SdfValueTypeName &typeName,
                                const TfToken& interpolation,
                                int elementSize) const
{
    _IssueAPIWarningIfDeprecationFlagEnabled();
    return UsdGeomPrimvarsAPI(GetPrim())
        .CreatePrimvar(attrName, typeName, interpolation,
                       elementSize);
}


UsdGeomPrimvar
UsdGeomImageable::GetPrimvar(const TfToken &name) const
{
    _IssueAPIWarningIfDeprecationFlagEnabled();
    return UsdGeomPrimvarsAPI(GetPrim()).GetPrimvar(name);
}

std::vector<UsdGeomPrimvar>
UsdGeomImageable::GetPrimvars() const
{
    _IssueAPIWarningIfDeprecationFlagEnabled();
    return UsdGeomPrimvarsAPI(GetPrim()).GetPrimvars();
}

std::vector<UsdGeomPrimvar>
UsdGeomImageable::GetAuthoredPrimvars() const
{
    _IssueAPIWarningIfDeprecationFlagEnabled();
    return UsdGeomPrimvarsAPI(GetPrim()).GetAuthoredPrimvars();
}

bool
UsdGeomImageable::HasPrimvar(const TfToken &name) const
{
    _IssueAPIWarningIfDeprecationFlagEnabled();
    return UsdGeomPrimvarsAPI(GetPrim()).HasPrimvar(name);
}


/* static */
const TfTokenVector &
UsdGeomImageable::GetOrderedPurposeTokens()
{
    static const TfTokenVector purposeTokens = {
        UsdGeomTokens->default_,
        UsdGeomTokens->render,
        UsdGeomTokens->proxy,
        UsdGeomTokens->guide };

    return purposeTokens;
}

static
TfToken
_ComputeVisibility(UsdPrim const &prim, UsdTimeCode const &time)
{
    TfToken localVis;
    if (UsdGeomImageable ip = UsdGeomImageable(prim)) {
        ip.GetVisibilityAttr().Get(&localVis, time);

        if (localVis == UsdGeomTokens->invisible) {
            return UsdGeomTokens->invisible;
        }
    }

    if (UsdPrim parent = prim.GetParent()) {
        return _ComputeVisibility(parent, time);
    }

    return UsdGeomTokens->inherited;
}

TfToken
UsdGeomImageable::ComputeVisibility(UsdTimeCode const &time) const
{
    return _ComputeVisibility(GetPrim(), time);
}

TfToken
UsdGeomImageable::ComputeVisibility(
    const TfToken &parentVisibility, 
    UsdTimeCode const &time) const
{
    if (parentVisibility == UsdGeomTokens->invisible) {
        return UsdGeomTokens->invisible;
    }

    TfToken localVis;
    if (UsdGeomImageable ip = UsdGeomImageable(GetPrim())) {
        if (ip.GetVisibilityAttr().Get(&localVis, time) && 
            localVis == UsdGeomTokens->invisible) {
            return UsdGeomTokens->invisible;
        }
    }

    return UsdGeomTokens->inherited;
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
_SetInheritedIfInvisible(const UsdGeomImageable &imageable,
                         const UsdTimeCode &time)
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
_MakeVisible(const UsdPrim &prim, UsdTimeCode const &time,
             bool *hasInvisibleAncestor)
{
    if (UsdPrim parent = prim.GetParent()) {
        _MakeVisible(parent, time, hasInvisibleAncestor);

        if (UsdGeomImageable imageableParent = UsdGeomImageable(parent)) {

            // Change visibility of parent to inherited if it is invisible.
            if (_SetInheritedIfInvisible(imageableParent, time) ||
                *hasInvisibleAncestor) {

                *hasInvisibleAncestor = true;

                // Invis all siblings of prim.
                for (const UsdPrim &childPrim : parent.GetAllChildren()) {
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

// Helper for computing only the authored purpose token from a valid imageable
// prim. Returns an empty purpose token otherwise.
static TfToken
_ComputeAuthoredPurpose(const UsdGeomImageable &ip)
{
    if (ip) {
        UsdAttribute purposeAttr = ip.GetPurposeAttr();
        if (purposeAttr.HasAuthoredValue()) {
            TfToken purpose;
            purposeAttr.Get(&purpose);
            return purpose;
        }
    }
    return TfToken();
}

// Helper for computing the fallback purpose from a valid imageable prim 
// assuming we didn't find an authored purpose. Returns the "default" purpose
// as the fallback for non-imageable prims.
static TfToken
_ComputeFallbackPurpose(const UsdGeomImageable &ip)
{
    TfToken purpose = UsdGeomTokens->default_;
    if (ip) {
        UsdAttribute purposeAttr = ip.GetPurposeAttr();
        purposeAttr.Get(&purpose);
    }
    return purpose;
}

// Helper for computing the purpose that can be inherited from an ancestor 
// imageable when there is no authored purpose on the prim. Walks up the prim
// hierarchy and returns the first authored purpose opinion found on an 
// imageable prim. Returns an empty token if there's purpose opinion to inherit
// from.
static
TfToken _ComputeInheritableAncestorPurpose(const UsdPrim &prim)
{
    UsdPrim parent = prim.GetParent();
    while (parent) {
        const TfToken purpose = _ComputeAuthoredPurpose(UsdGeomImageable(parent));
        if (!purpose.IsEmpty()) {
            return purpose;
        }
        parent = parent.GetParent();
    }
    return TfToken();
}

TfToken
UsdGeomImageable::ComputePurpose() const
{
    return ComputePurposeInfo().purpose;
}

UsdGeomImageable::PurposeInfo 
UsdGeomImageable::ComputePurposeInfo() const
{
    // Check for an authored purpose opinion (if we're imageable) first. If 
    // none, check for an inheritable ancestor opinion. If still none return 
    // the fallback purpose.
    TfToken authoredPurpose = _ComputeAuthoredPurpose(*this);
    if (authoredPurpose.IsEmpty()) {
        TfToken inheritableParentPurpose = 
            _ComputeInheritableAncestorPurpose(GetPrim());
        if (inheritableParentPurpose.IsEmpty()) {
            return PurposeInfo (_ComputeFallbackPurpose(*this), false);
        } else {
            return PurposeInfo (inheritableParentPurpose, true);
        }
    }
    return PurposeInfo (authoredPurpose, true);
}

UsdGeomImageable::PurposeInfo 
UsdGeomImageable::ComputePurposeInfo(const PurposeInfo &parentPurposeInfo) const
{
    // Check for an authored purpose opinion (if we're imageable) first. If 
    // none, return the passed in parent purpose if its inheritable
    // otherwise return the fallback purpose.
    TfToken authoredPurpose = _ComputeAuthoredPurpose(*this);
    if (authoredPurpose.IsEmpty()) {
        if (parentPurposeInfo.isInheritable) {
            return parentPurposeInfo;
        } else {
            return PurposeInfo (_ComputeFallbackPurpose(*this), false);
        }
    }
    return PurposeInfo (authoredPurpose, true);
}

UsdPrim
UsdGeomImageable::ComputeProxyPrim(UsdPrim *renderPrim) const
{
    UsdPrim  renderRoot, self=GetPrim();

    // XXX: This may not make sense anymore now that computed purpose is no 
    // longer "pruning", i.e. you can't guarantee that all descendant prims will
    // have same purpose as the root of a subtree. Instead we now verify that 
    // this prim has the render purpose and walk up the parent chain until we
    // the last prim that still has the render purpose and treat that as the
    // render root for this prim's proxy.
    TfToken purpose = ComputePurpose();
    UsdPrim prim = GetPrim();
    while (UsdGeomImageable(prim).ComputePurpose() == UsdGeomTokens->render) {
        renderRoot = prim;
        prim = prim.GetParent();
    }

    if (renderRoot){
        SdfPathVector target;
        UsdRelationship  proxyPrimRel = 
            UsdGeomImageable(renderRoot).GetProxyPrimRel();
        if (proxyPrimRel.GetForwardedTargets(&target)){
            if (target.size() == 1){
                if (UsdPrim proxy = self.GetStage()->GetPrimAtPath(target[0])){
                    if (_ComputePurpose(proxy) != UsdGeomTokens->proxy){
                        TF_WARN("Prim <%s>, targeted as proxyPrim of prim "
                                "<%s> does not have purpose 'proxy'",
                                proxy.GetPath().GetText(),
                                renderRoot.GetPath().GetText());
                        return UsdPrim();
                    }
                    if (renderPrim){
                        *renderPrim = renderRoot;
                    }
                    return proxy;
                }
            }
            else if (target.size() > 1){
                TF_WARN("Found multiple targets for proxyPrim rel on "
                        "prim <%s>", renderRoot.GetPath().GetText());
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
