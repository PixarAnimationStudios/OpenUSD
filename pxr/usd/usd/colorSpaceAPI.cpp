//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/colorSpaceAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdColorSpaceAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdColorSpaceAPI::~UsdColorSpaceAPI()
{
}

/* static */
UsdColorSpaceAPI
UsdColorSpaceAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdColorSpaceAPI();
    }
    return UsdColorSpaceAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdColorSpaceAPI::_GetSchemaKind() const
{
    return UsdColorSpaceAPI::schemaKind;
}

/* static */
bool
UsdColorSpaceAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdColorSpaceAPI>(whyNot);
}

/* static */
UsdColorSpaceAPI
UsdColorSpaceAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdColorSpaceAPI>()) {
        return UsdColorSpaceAPI(prim);
    }
    return UsdColorSpaceAPI();
}

/* static */
const TfType &
UsdColorSpaceAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdColorSpaceAPI>();
    return tfType;
}

/* static */
bool 
UsdColorSpaceAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdColorSpaceAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdColorSpaceAPI::GetColorSpaceNameAttr() const
{
    return GetPrim().GetAttribute(UsdTokens->colorSpaceName);
}

UsdAttribute
UsdColorSpaceAPI::CreateColorSpaceNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTokens->colorSpaceName,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
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
UsdColorSpaceAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTokens->colorSpaceName,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
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

#include "pxr/base/gf/colorSpace.h"
#include "pxr/usd/usd/colorSpaceDefinitionAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

// static
bool UsdColorSpaceAPI::IsValidColorSpaceName(UsdPrim prim, const TfToken& name,
                                             ColorSpaceCache* cache)
{
    if (name.IsEmpty()) {
        return false;
    }

    if (cache && !cache->Find(prim.GetPath()).IsEmpty()) {
        // The color space name has been cached and is therefore known to be valid.
        return true;
    }

    if (GfColorSpace::IsValid(name)) {
        if (cache) {
            cache->Insert(prim.GetPath(), name);
        }
        return true;
    }

    static const std::string colorSpaceDefAPIPrefix =
        UsdTokens->ColorSpaceDefinitionAPI.GetString() +
           UsdObject::GetNamespaceDelimiter();

    // test if the name is defined at the current prim or a parent.
    while (prim) {
        if (prim.HasAPI<UsdColorSpaceDefinitionAPI>()) {
            for (const auto& appliedSchema : prim.GetAppliedSchemas()) {
                // Check if the schema is a UsdColorSpaceDefinitionAPI instance
                if (TfStringStartsWith(appliedSchema, colorSpaceDefAPIPrefix)) {
                    // Extract the instance name
                    const std::string instanceName =
                        appliedSchema.GetString().substr(colorSpaceDefAPIPrefix.size());
                    UsdColorSpaceDefinitionAPI defn(prim, TfToken(instanceName));

                    // Check the color space definition name
                    if (UsdAttribute colorSpaceDefnNameAttr = defn.GetNameAttr()) {
                        TfToken colorSpace;
                        if (colorSpaceDefnNameAttr.Get(&colorSpace)) {
                            if (GfColorSpace::IsValid(colorSpace)) {
                                TF_WARN("Encountered illegal redefinition of standard "
                                        "color space %s at prim %s.",
                                        colorSpace.GetText(),
                                        prim.GetPath().GetText());
                            }
                            else if (name == colorSpace) {
                                if (cache) {
                                    cache->Insert(prim.GetPath(), name);
                                }
                                return true;
                            }
                        }
                    }
                }
            }
        }
        prim = prim.GetParent();
    }
    return false;
}

TfToken UsdColorSpaceAPI::ComputeColorSpaceName(UsdPrim prim,
                                                ColorSpaceCache* cache)
{
    TfToken cachedColorSpace = cache? cache->Find(prim.GetPath()) : TfToken();
    if (!cachedColorSpace.IsEmpty()) {
        return cachedColorSpace;
    }

    UsdPrim startPrim = prim;

    // test this prim, and all of its parents for an assigned colorspace.
    while (prim) {
        if (prim.HasAPI<UsdColorSpaceAPI>()) {
            if (UsdAttribute colorSpaceAttr = 
                        UsdColorSpaceAPI(prim).GetColorSpaceNameAttr()) {
                TfToken colorSpace;
                if (colorSpaceAttr.Get(&colorSpace)) {
                    std::string csname = colorSpace.GetString();

                    // ignore an unauthored value, otherwise check validity.
                    if (csname.length() > 0) {
                        // if this prim, or any of its parents defines the color
                        // space, return it. As a side effect it will be cached.
                        if (IsValidColorSpaceName(prim, colorSpace, cache)) {
                            // cache this prim's color space.
                            if (cache) {
                                cache->Insert(startPrim.GetPath(), colorSpace);
                            }
                            return colorSpace;
                        }

                        // report an unknown color space.
                        TF_WARN("Unknown color space %s encountered. ",
                                csname.c_str());
                        return {};
                    }
                }
            }
        }
        prim = prim.GetParent();
    }

    // a color space is defined nowhere.
    if (cache) {
        cache->Insert(startPrim.GetPath(), TfToken());
    }
    return {};
}

TfToken UsdColorSpaceAPI::ComputeColorSpaceName(const UsdAttribute& attr,
		                                        ColorSpaceCache* cache)
{
    // if the attr has a color space authored, there's no need to do any work. 
    if (attr.HasColorSpace()) {
        return attr.GetColorSpace();
    }

    UsdPrim prim = attr.GetPrim();

    // check if the prim, or prim inheritance provides a color space.
    // As a side effect, the color space will be cached.
    TfToken primCS = UsdColorSpaceAPI::ComputeColorSpaceName(prim, cache);
    if (!primCS.IsEmpty()) {
        return primCS;
    }

    // Default to the attr's prim definition value, if there is one.
    return attr.GetColorSpace();
}

GfColorSpace UsdColorSpaceAPI::ComputeColorSpace(UsdPrim prim, 
                                                 const TfToken& name,
                                                 ColorSpaceCache* cache)
{
    if (name.IsEmpty() || GfColorSpace::IsValid(name)) {
        return GfColorSpace(name);
    }

    // If it wasn't a stock GfColorSpace it might've been defined
    // on the prim, or an ancestor.

    static const std::string colorSpaceDefAPIPrefix =
        UsdTokens->ColorSpaceDefinitionAPI.GetString() +
           UsdObject::GetNamespaceDelimiter();

    // test if the name is defined at the current prim or a parent.
    while (prim) {
        if (prim.HasAPI<UsdColorSpaceDefinitionAPI>()) {
            auto appliedSchemas = prim.GetAppliedSchemas();
            for (const auto& appliedSchema : appliedSchemas) {
                // Check if the schema is a UsdColorSpaceDefinitionAPI instance
                if (TfStringStartsWith(appliedSchema, colorSpaceDefAPIPrefix)) {
                    // Extract the instance name
                    const std::string instanceName =
                        appliedSchema.GetString().substr(colorSpaceDefAPIPrefix.size());
                    UsdColorSpaceDefinitionAPI defn(prim, TfToken(instanceName));

                    // Check the color space definition name
                    if (UsdAttribute colorSpaceDefnNameAttr = defn.GetNameAttr()) {
                        TfToken colorSpace;
                        if (colorSpaceDefnNameAttr.Get(&colorSpace)) {
                            if (colorSpace == name) {
                                if (GfColorSpace::IsValid(colorSpace)) {
                                    TF_WARN("Encountered illegal redefinition of standard "
                                            "color space %s at prim %s.",
                                            colorSpace.GetText(),
                                            prim.GetPath().GetText());
                                }
                                else {
                                    return defn.ComputeColorSpaceFromDefinitionAttributes();
                                }
                            }
                        }
                    }
                }
            }
        }
        prim = prim.GetParent();
    }

    return GfColorSpace(TfToken{});
}

GfColorSpace UsdColorSpaceAPI::ComputeColorSpace(UsdPrim prim,
                                                 ColorSpaceCache* cache)
{
    return ComputeColorSpace(prim, ComputeColorSpaceName(prim, cache), cache);
}

GfColorSpace UsdColorSpaceAPI::ComputeColorSpace(const UsdAttribute& attr,
		                                         ColorSpaceCache* cache)
{
    TfToken colorSpaceName = ComputeColorSpaceName(attr, cache);
    return ComputeColorSpace(attr.GetPrim(), colorSpaceName, cache);
}

PXR_NAMESPACE_CLOSE_SCOPE

