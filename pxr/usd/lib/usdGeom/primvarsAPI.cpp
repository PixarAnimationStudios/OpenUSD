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
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomPrimvarsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (PrimvarsAPI)
);

/* virtual */
UsdGeomPrimvarsAPI::~UsdGeomPrimvarsAPI()
{
}

/* static */
UsdGeomPrimvarsAPI
UsdGeomPrimvarsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPrimvarsAPI();
    }
    return UsdGeomPrimvarsAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaType UsdGeomPrimvarsAPI::_GetSchemaType() const {
    return UsdGeomPrimvarsAPI::schemaType;
}

/* static */
const TfType &
UsdGeomPrimvarsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomPrimvarsAPI>();
    return tfType;
}

/* static */
bool 
UsdGeomPrimvarsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomPrimvarsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdGeomPrimvarsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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

UsdGeomPrimvar 
UsdGeomPrimvarsAPI::CreatePrimvar(const TfToken& attrName,
                                const SdfValueTypeName &typeName,
                                const TfToken& interpolation,
                                int elementSize) const
{
    const UsdPrim prim = GetPrim();

    UsdGeomPrimvar primvar(prim, attrName, typeName);

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
UsdGeomPrimvarsAPI::GetPrimvar(const TfToken &name) const
{
    // The getter SHOULD issue an error if 'name' is malformed, which
    // _MakeNamespaced() will do for us.
    return UsdGeomPrimvar(GetPrim().GetAttribute
                           (UsdGeomPrimvar::_MakeNamespaced(name)));
}

std::vector<UsdGeomPrimvar>
UsdGeomPrimvarsAPI::_MakePrimvars(std::vector<UsdProperty> const &props) const
{
    std::vector<UsdGeomPrimvar> primvars;
    primvars.reserve(props.size());
    
    TF_FOR_ALL(prop, props) {
        // All prefixed properties except the ones that contain extra
        // namespaces (eg. the ":indices" attributes belonging to indexed
        // primvars) will be valid primvars.
        if (UsdGeomPrimvar primvar = UsdGeomPrimvar(prop->As<UsdAttribute>()))
            primvars.push_back(primvar);
    }
    return primvars;
}

std::vector<UsdGeomPrimvar>
UsdGeomPrimvarsAPI::GetPrimvars() const
{
    const UsdPrim &prim = GetPrim();
    return _MakePrimvars(prim ? prim.GetPropertiesInNamespace(
                             UsdGeomPrimvar::_GetNamespacePrefix()) :
                         std::vector<UsdProperty>());
}

std::vector<UsdGeomPrimvar>
UsdGeomPrimvarsAPI::GetAuthoredPrimvars() const
{
    const UsdPrim &prim = GetPrim();
    return _MakePrimvars(prim 
                         ? prim.GetAuthoredPropertiesInNamespace(
                             UsdGeomPrimvar::_GetNamespacePrefix())
                         : std::vector<UsdProperty>());
}

std::vector<UsdGeomPrimvar>
UsdGeomPrimvarsAPI::FindInheritedPrimvars() const
{
    TRACE_FUNCTION();
    // Assume the number of primvars is relatively bounded and
    // just use a vector to accumulate primvars up to the root prim.
    std::vector<UsdGeomPrimvar> primvars;
    UsdPrim prim = GetPrim();
    if (!prim) {
        return primvars;
    }
    TfToken const& prefix = UsdGeomPrimvar::_GetNamespacePrefix();
    for (prim = prim.GetParent(); prim && !prim.IsPseudoRoot();
         prim = prim.GetParent()) {
        for (UsdProperty const& prop:
             prim.GetAuthoredPropertiesInNamespace(prefix)) {
            if (UsdGeomPrimvar pv = UsdGeomPrimvar(prop.As<UsdAttribute>())) {
                // If the primvar is defined locally (authored or not),
                // it cannot be inherited.
                if (HasPrimvar(pv.GetPrimvarName())) {
                    continue;
                }
                bool found = false;
                for (UsdGeomPrimvar const& probe: primvars) {
                    if (probe.GetName() == pv.GetName()) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    primvars.push_back(pv);
                }
            }
        }
    }
    // Discard any primvars resolved with non-constant interpolation.
    // We do this as a post-pass to ensure that if a primvar is
    // declared at multiple levels of namespace with different
    // interpolation settings, the most descendant opinion wins.
    for (size_t i=0; i < primvars.size(); ++i) {
        if (primvars[i].GetInterpolation() != UsdGeomTokens->constant) {
            // Swap to the end and truncate the vector.
            // Don't bother to preserve order.
            std::swap(primvars[i], primvars.back());
            primvars.erase(--primvars.end());
            // Revisit same position.
            --i;
        }
    }
    return primvars;
}

UsdGeomPrimvar 
UsdGeomPrimvarsAPI::FindInheritedPrimvar(const TfToken &name) const
{
    TRACE_FUNCTION();
    // If the primvar is defined locally (authored or not), it cannot
    // be inherited.
    if (HasPrimvar(name)) {
        return UsdGeomPrimvar();
    }
    const TfToken attrName = UsdGeomPrimvar::_MakeNamespaced(name);
    UsdPrim prim = GetPrim();
    if (!prim) {
        return UsdGeomPrimvar();
    }
    for (prim = prim.GetParent(); prim && !prim.IsPseudoRoot();
         prim = prim.GetParent()) {
        UsdAttribute attr = prim.GetAttribute(attrName);
        if (attr && attr.IsAuthored()) {
            if (UsdGeomPrimvar pv = UsdGeomPrimvar(attr)) {
                // Only constant primvars can be inherited.
                if (pv.GetInterpolation() == UsdGeomTokens->constant) {
                    return pv;
                } else {
                    // Non-constant interpolation blocks inheritance.
                    return UsdGeomPrimvar();
                }
            }
        }
    }
    return UsdGeomPrimvar();
}

bool
UsdGeomPrimvarsAPI::HasPrimvar(const TfToken &name) const
{
    TfToken primvarName = UsdGeomPrimvar::_MakeNamespaced(name, /* quiet */true);
    return primvarName.IsEmpty() ? false : 
        UsdGeomPrimvar::IsPrimvar(GetPrim().GetAttribute(primvarName));
}

bool
UsdGeomPrimvarsAPI::HasInheritedPrimvar(const TfToken &name) const
{
    TRACE_FUNCTION();
    // If the primvar is defined locally (authored or not), it cannot
    // be inherited.
    if (HasPrimvar(name)) {
        return false;
    }
    const TfToken attrName = UsdGeomPrimvar::_MakeNamespaced(name);
    if (attrName.IsEmpty()) {
        return false;
    }
    UsdPrim prim = GetPrim();
    if (!prim) {
        return false;
    }
    for (prim = prim.GetParent(); prim && !prim.IsPseudoRoot();
         prim = prim.GetParent()) {
        UsdAttribute attr = prim.GetAttribute(attrName);
        if (attr && attr.IsAuthored() && UsdGeomPrimvar::IsPrimvar(attr)) {
            // Only constant primvars can be inherited.
            // Non-constant interpolation blocks inheritance.
            return UsdGeomPrimvar(attr).GetInterpolation()
                == UsdGeomTokens->constant;
        }
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
