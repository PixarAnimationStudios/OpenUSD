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
#include "usdKatana/attrMap.h"
#include "usdKatana/readPrim.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"
#include "usdKatana/tokens.h"
#include "usdKatana/blindDataObject.h"


#include "pxr/usd/usd/prim.h"

#include "pxr/base/tf/getenv.h"

#include "pxr/usd/usdUtils/pipeline.h"

#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/collectionAPI.h"

#include "pxr/usd/usdShade/pShaderUtils.h"
#include "pxr/usd/usdShade/look.h"

#include "pxr/usd/usdRi/statements.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h" 
#include "pxr/usd/usd/modelAPI.h"

#include <pystring/pystring.h>
#include <FnLogging/FnLogging.h>

FnLogSetup("PxrUsdKatanaReadPrim");

static FnKat::Attribute
_GetMaterialAssignAttr(
        const UsdPrim& prim,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    if (not prim or prim.GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return FnKat::Attribute();
    }

    UsdRelationship usdRel = UsdShadeLook::GetBindingRel(prim);
    if (usdRel) {
        // USD shading binding
        SdfPathVector targetPaths;
        usdRel.GetForwardedTargets(&targetPaths);
        if (targetPaths.size() > 0) {
            if (not targetPaths[0].IsPrimPath()) {
                FnLogWarn("Target path " << prim.GetPath().GetString() <<
                          " is not a prim");
                return FnKat::Attribute();
            }

            // This is a copy as it could be modified below.
            SdfPath targetPath = targetPaths[0];
            UsdPrim targetPrim = data.GetUsdInArgs()->GetStage()->GetPrimAtPath(targetPath);
            // If the target is inside a master, then it needs to be re-targeted 
            // to the instance.
            // 
            // XXX remove this special awareness once GetMasterWithContext is
            //     is available as the provided prim will automatically
            //     retarget (or provide enough context to retarget without
            //     tracking manually).
            if (targetPrim and targetPrim.IsInMaster()) {
                if (not data.GetInstancePath().IsEmpty() and 
                        not data.GetMasterPath().IsEmpty()) {

                    // Check if the source and the target of the relationship 
                    // belong to the same master.
                    // If they do, we have the context necessary to do the 
                    // re-mapping.
                    if (data.GetMasterPath().GetCommonPrefix(targetPath).
                            GetPathElementCount() > 0) {
                        targetPath = data.GetInstancePath().AppendPath(
                            targetPath.ReplacePrefix(targetPath.GetPrefixes()[0],
                                SdfPath::ReflexiveRelativePath()));
                    } else {
                        // Warn saying the target of relationship isn't within 
                        // the same master as the source.
                        FnLogWarn("Target path " << prim.GetPath().GetString() 
                            << " isn't within the master " << data.GetMasterPath());
                        return FnKat::Attribute();
                    }
                } else {
                    // XXX
                    // When loading beneath a master via an isolatePath
                    // opArg, we can encounter targets which are within masters
                    // but not within the context of a material.
                    // While that would be an error according to the below
                    // warning, it produces the expected results.
                    // This case can occur when expanding pointinstancers as
                    // the sources are made via execution of PxrUsdIn again
                    // at the sub-trees.
                    
                    
                    // Warn saying target of relationship is in a master, 
                    // but the associated instance path is unknown!
                    // FnLogWarn("Target path " << prim.GetPath().GetString() 
                    //         << " is within a master, but the associated "
                    //         "instancePath is unknown.");
                    // return FnKat::Attribute();
                }
            }

            // Convert the target path to the equivalent katana location.
            // XXX: Looks may have an atypical USD->Katana 
            // path mapping
            std::string location =
                PxrUsdKatanaUtils::ConvertUsdMaterialPathToKatLocation(targetPath, data);
                
            // XXX Looks containing only display terminals are causing issues
            //     with katana material manipulation workflows.
            //     For now: exclude any material assign which doesn't include
            //     /Looks/ in the path
            if (location.find(UsdKatanaTokens->katanaLooksScopePathSubstring)
                    == std::string::npos)
            {
                return FnKat::Attribute();
            }
                
                
            // location = TfStringReplace(location, "/Looks/", "/Materials/");
            // XXX handle multiple assignments
            return FnKat::StringAttribute(location);
        }
    }

    return FnKat::Attribute();
}

static bool
_GatherRibAttributes(
        const UsdPrim &prim, 
        double currentTime,
        FnKat::GroupBuilder& attrsBuilder)
{
    bool hasAttrs = false;

    // USD SHADING STYLE ATTRIBUTES
    UsdRiStatements riStatements(prim);
    if (riStatements) {
        const std::vector<UsdProperty> props = 
            riStatements.GetRiAttributes();
        std::string attrName;
        TF_FOR_ALL(propItr, props) {
            UsdProperty prop = *propItr;
            if (!prop) continue;

            std::string nameSpace = 
                riStatements.GetRiAttributeNameSpace(prop).GetString();
            nameSpace = TfStringReplace(nameSpace, ":", ".") + ".";

            attrName = nameSpace +
                riStatements.GetRiAttributeName(prop).GetString();

            VtValue vtValue;
            UsdAttribute usdAttr = prim.GetAttribute(prop.GetName());
            if (usdAttr) {
                if (not usdAttr.Get(&vtValue, currentTime)) 
                    continue;

                // XXX asShaderParam really means:
                // "For arrays, as a single attr vs a type/value pair group"
                // The type/value pair group is meaningful for attrs who don't
                // have a formal type definition -- like a "user" RiAttribute.
                // 
                // However, other array values (such as two-element shadingrate)
                // are not expecting the type/value pair form and will not
                // generate rib correctly. As such, we'll handle the "user"
                // attribute as a special case.
                bool asShaderParam = true;
                
                if (nameSpace == "user.")
                {
                    asShaderParam = false;
                }

                attrsBuilder.set(attrName, PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue,
                    asShaderParam) );
            }
            else {
                UsdRelationship usdRel = prim.GetRelationship(prop.GetName());
                attrsBuilder.set(attrName, PxrUsdKatanaUtils::ConvertRelTargetsToKatAttr(usdRel,
                    /* asShaderParam */ false) );
            }
            hasAttrs = true;
        }
    }

    return hasAttrs;
}

void
PxrUsdKatanaReadPrimPrmanStatements(
        const UsdPrim& prim,
        double currentTime,
        FnKat::GroupBuilder& statements)
{
    if (prim.GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return;
    }

    FnKat::GroupBuilder attrsBuilder;

    // Rib attributes -> attributes.*
    _GatherRibAttributes(prim, currentTime, attrsBuilder);

    //
    // Add gprim-specific prmanStatements.
    //
    
    if (UsdGeomGprim gprim = UsdGeomGprim(prim))
    {
        bool doubleSided = false;
        if (gprim.GetDoubleSidedAttr().Get(&doubleSided) and doubleSided)
        {
            statements.set("sides", FnKat::IntAttribute(2));
        }

        // orientation
        // uses non-literal mapping of lh/rh to better match prman behavior
        // as per: http://bugzilla.pixar.com/show_bug.cgi?id=110542
        TfToken orientation;
        if (gprim.GetOrientationAttr().Get(&orientation))
        {
            statements.set("orientation", FnKat::StringAttribute(
                orientation == UsdGeomTokens->leftHanded ? "inside" : "outside"));
        }
    }

    //
    // Take care of Pixar's conventional model-level shader space.
    //

    if (UsdModelAPI(prim).IsModel()) {
        statements.set("scopedCoordinateSystem",
            FnKat::StringAttribute("ModelSpace"));
    }

    // XXX:
    // Should we have subclasses (e.g., PxrUsdKatanaUtils::HairmanContext), add to or modify
    // this builder instead of setting attributes.NAMESPACE.ATTRNAME for each
    // new attr?  Are there performance implications?
    FnKat::GroupAttribute attributesGroup = attrsBuilder.build();
    if (attributesGroup.getNumberOfChildren()){
        statements.set("attributes", attributesGroup);
    }
}

static bool
_BuildScopedCoordinateSystems(
        const UsdPrim& prim,
        FnKat::GroupBuilder& coordSysBuilder)
{
    // We look at the immediate children of this prim for UsdRi-encoded
    // scopedCooordinateSystems, but emit them as a relative coordinate system
    // on this (parent) so they are applicable to all children of this node.

    if (not prim) {
        return false;
    }

    bool foundCoordSys = false;

    TF_FOR_ALL(childIt, prim.GetChildren()) {

        UsdRiStatements riStmts(*childIt);

        if (not riStmts.HasCoordinateSystem()) {
            continue;
        }

        const std::string gprimName = childIt->GetName();
        std::string usdCoordSysName = riStmts.GetScopedCoordinateSystem();
        if (usdCoordSysName.empty()) {
            usdCoordSysName = gprimName;
        }
        std::string coordSysName = usdCoordSysName;

        coordSysBuilder.set(coordSysName, FnKat::StringAttribute(gprimName));

        // XXX: For backward compatibility we will emit the same coordsys
        // again, prefixed with modelInstanceName.
        //
        // XXX: 20150126: Restoring this backward compatbility
        // shim that the tidscene SGG had.  We're finding we need it
        // to preserve assumptions made internally by the REYES eye
        // shaders.  Possibly we can remove this once we are no longer
        // supporting REYES shows.
        coordSysBuilder.set(
            PxrUsdKatanaUtils::GetModelInstanceName(prim) + "_" + coordSysName,
            FnKat::StringAttribute(gprimName));

        foundCoordSys = true;
    }
    
    return foundCoordSys;
}

static bool
_BuildCollections(
        const UsdPrim& prim,
        FnKat::GroupBuilder& collectionsBuilder)
{
    std::vector<UsdGeomCollectionAPI> collections = 
        UsdGeomCollectionAPI::GetCollections(prim);

    size_t prefixLength = prim.GetPath().GetString().length();
    for (size_t iCollection = 0; iCollection < collections.size(); ++iCollection)
    {
        SdfPathVector targets;
        FnKat::StringBuilder collectionBuilder;
        UsdGeomCollectionAPI &collection = collections[iCollection];
        TfToken name = collection.GetCollectionName();
        collection.GetTargets(&targets, false);
        for (size_t iTarget = 0; iTarget < targets.size(); ++iTarget)
        {
            std::string targetPath = targets[iTarget].GetString();
            
            if (targetPath.size() >= prefixLength)
            {
                std::string relativePath = targetPath.substr(prefixLength);
                // follow katana convention for collections
                // the "self" location relative path is "/". 
                // Absolute paths start with "/root/"
                // relative paths start without the "/" though.
                if (relativePath == "")
                    relativePath = "/";
                collectionBuilder.push_back(relativePath);
            }
        }

        // if empty, no point creating collection
        FnKat::StringAttribute collectionAttr = collectionBuilder.build();
        if (collectionAttr.getNearestSample(0).size() > 0) {
            collectionsBuilder.set(name.GetString() + ".baked",
                collectionAttr);
        }
    }

    return (collections.size() > 0);
}


static void
_AddExtraAttributesOrNamespaces(
        const UsdPrim& prim,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const std::string& rootLocation = 
        data.GetUsdInArgs()->GetRootLocationPath();
    const double currentTime = data.GetUsdInArgs()->GetCurrentTime();

    const PxrUsdKatanaUsdInArgs::StringListMap& extraAttributesOrNamespaces =
        data.GetUsdInArgs()->GetExtraAttributesOrNamespaces();

    PxrUsdKatanaUsdInArgs::StringListMap::const_iterator I = 
        extraAttributesOrNamespaces.begin();
    for (; I != extraAttributesOrNamespaces.end(); ++I)
    {
        const std::string& name = (*I).first;
        const std::vector<std::string>& names = (*I).second;
        
        FnKat::GroupBuilder gb;

        for (std::vector<std::string>::const_iterator I = names.begin(),
                E = names.end(); I != E; ++I)
        {
            const std::string& propOrNamespace = (*I);
            
            std::vector<UsdAttribute> usdAttrs;
            std::vector<UsdRelationship> usdRelationships;
            
            if (UsdAttribute directAttribute =
                    prim.GetAttribute(TfToken(propOrNamespace)))
            {
                usdAttrs.push_back(directAttribute);
            }
            else if (UsdRelationship directRelationship =
                    prim.GetRelationship(TfToken(propOrNamespace)))
            {
                usdRelationships.push_back(directRelationship);
            }
            else
            {
                std::vector<UsdProperty> props = 
                        prim.GetPropertiesInNamespace(propOrNamespace);
                
                for (std::vector<UsdProperty>::const_iterator I =
                        props.begin(), E = props.end(); I != E; ++I)
                {
                    const UsdProperty & prop = (*I);
                    
                    if (UsdAttribute attr = prop.As<UsdAttribute>())
                    {
                        usdAttrs.push_back(attr);
                    }
                    else if (UsdRelationship rel =
                            prop.As<UsdRelationship>())
                    {
                        usdRelationships.push_back(rel);
                    }
                }
            }
            
            for (std::vector<UsdAttribute>::iterator I = usdAttrs.begin(),
                    E = usdAttrs.end(); I != E; ++I)
            {
                UsdAttribute & usdAttr = (*I);
                
                VtValue vtValue;
                if (not usdAttr.Get(&vtValue, currentTime))
                {
                    continue;
                }
                
                FnKat::Attribute attr = 
                    PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue, true);
                
                if (!attr.isValid())
                {
                    continue;
                }
                
                gb.set(pystring::replace(
                    usdAttr.GetName().GetString(), ":", "."), attr);
            }
            
            for (std::vector<UsdRelationship>::iterator I =
                    usdRelationships.begin(), E = usdRelationships.end();
                            I != E; ++I)
            {
                UsdRelationship & usdRelationship = (*I);
                
                FnKat::StringAttribute attr = 
                    PxrUsdKatanaUtils::ConvertRelTargetsToKatAttr(
                        usdRelationship, true);
                if (not attr.isValid())
                {
                    continue;
                }
                
                // Further prefix with the PxrUsdIn root scenegraph
                // location in order to make it a valid katana path.
                // XXX, move this into PxrUsdKatanaUtils::ConvertRelTargetsToKatAttr
                // for future implementations.
                
                FnKat::StringAttribute::array_type values =
                        attr.getNearestSample(0.0f);
                
                std::vector<std::string> prefixedValues;
                prefixedValues.reserve(values.size());
                
                for (size_t i = 0; i < values.size(); ++i)
                {
                    std::ostringstream buffer;
                    buffer << rootLocation;
                    
                    if (values[i][0] && values[i][0] != '/')
                    {
                        buffer << '/';
                    }
                    buffer << values[i];
                    
                    prefixedValues.push_back(buffer.str());
                }
                
                gb.set(pystring::replace(
                        usdRelationship.GetName().GetString(),
                                ":", "."), FnKat::StringAttribute(
                                        prefixedValues));
            }
        }

        FnKat::GroupAttribute result = gb.build();
        attrs.set(name, result.getChildByName(name));
    }
}

static bool
_AddCustomProperties(
        const UsdPrim &prim,
        double currentTime,
        FnKat::GroupBuilder& customBuilder)
{
    if (TfGetenv("USD_KATANA_ADD_CUSTOM_PROPERTIES", "0") == "0")
    {
        return false;
    }

    bool foundCustomProperties = false;

    const std::vector<UsdAttribute>& usdAttributes = prim.GetAttributes();
    for (size_t i = 0; i < usdAttributes.size(); ++i)
    {
        const UsdAttribute& usdAttr = usdAttributes[i];
        if (not usdAttr.IsCustom())
        {
            continue;
        }
        
        VtValue vtValue;
        if (not usdAttr.Get(&vtValue, currentTime))
        {
            continue;
        }
        
        FnKat::Attribute attr =
            PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue, true);

        if (not attr.isValid())
        {
            continue;
        }
        
        customBuilder.set(pystring::replace(
            usdAttr.GetName().GetString(), ":", "."), attr);

        foundCustomProperties = true;
    }

    return foundCustomProperties;
}

FnKat::Attribute
PxrUsdKatanaGeomGetPrimvarGroup(
        const UsdGeomImageable& imageable,
        const PxrUsdKatanaUsdInPrivateData& data)
{
    // Usd primvars -> Primvar attributes
    FnKat::GroupBuilder gdBuilder;

    std::vector<UsdGeomPrimvar> primvarAttrs = imageable.GetPrimvars();
    TF_FOR_ALL(primvar, primvarAttrs) {
        // If there is a block from blind data, skip to avoid the cost
        UsdKatanaBlindDataObject kbd(imageable.GetPrim());
        UsdAttribute blindAttr = kbd.GetKbdAttribute("geometry.arbitrary." + 
                                        primvar->GetBaseName().GetString());
        if (blindAttr) {
            VtValue vtValue;
            if (!blindAttr.Get(&vtValue) and blindAttr.HasAuthoredValueOpinion()) {
                continue;
            }
        }
        
        TfToken          name, interpolation;
        SdfValueTypeName typeName;
        int              elementSize;

        primvar->GetDeclarationInfo(&name, &typeName, 
                                    &interpolation, &elementSize);

        // Name: this will eventually want to be GetBaseName() to strip
        // off prefixes
        std::string gdName = name;

        // Convert interpolation -> scope
        FnKat::StringAttribute scopeAttr;
        const bool isCurve = imageable.GetPrim().IsA<UsdGeomCurves>();
        if (isCurve && interpolation == UsdGeomTokens->vertex) {
            // it's a curve, so "vertex" == "vertex"
            scopeAttr = FnKat::StringAttribute("vertex");
        } else {
            scopeAttr = FnKat::StringAttribute(
                (interpolation == UsdGeomTokens->faceVarying)? "vertex" :
                (interpolation == UsdGeomTokens->varying)    ? "point" :
                (interpolation == UsdGeomTokens->vertex)     ? "point" /*see below*/ :
                (interpolation == UsdGeomTokens->uniform)    ? "face" :
                "primitive" );
        }

        // Resolve the value
        VtValue vtValue;
        if (not primvar->ComputeFlattened(
                &vtValue, data.GetUsdInArgs()->GetCurrentTime()))
        {
            continue;
        }

        // Convert value to the required Katana attributes to describe it.
        FnKat::Attribute valueAttr, inputTypeAttr, elementSizeAttr;
        PxrUsdKatanaUtils::ConvertVtValueToKatCustomGeomAttr(
            vtValue, elementSize, typeName.GetRole(),
            &valueAttr, &inputTypeAttr, &elementSizeAttr);

        // Bundle them into a group attribute
        FnKat::GroupBuilder attrBuilder;
        attrBuilder.set("scope", scopeAttr);
        attrBuilder.set("inputType", inputTypeAttr);
        if (elementSizeAttr.isValid()) {
            attrBuilder.set("elementSize", elementSizeAttr);
        }
        attrBuilder.set("value", valueAttr);
        // Note that 'varying' vs 'vertex' require special handling, as in
        // Katana they are both expressed as 'point' scope above. To get
        // 'vertex' interpolation we must set an additional
        // 'interpolationType' attribute.  So we will flag that here.
        const bool vertexInterpolationType = 
            (interpolation == UsdGeomTokens->vertex);
        if (vertexInterpolationType) {
            attrBuilder.set("interpolationType",
                FnKat::StringAttribute("subdiv"));
        }
        gdBuilder.set(gdName, attrBuilder.build());
    }

    return gdBuilder.build();
}

void
PxrUsdKatanaReadPrim(
        const UsdPrim& prim,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    const double currentTime = data.GetUsdInArgs()->GetCurrentTime();

    //
    // Set the 'kind' attribute to match the model kind.
    //

    TfToken kind;
    if (UsdModelAPI(prim).GetKind(&kind))
    {
        attrs.set("kind", FnKat::StringAttribute(kind.GetString()));
    }

    //
    // Set the 'materialAssign' attribute for locations that have shaders.
    //

    attrs.set("materialAssign", _GetMaterialAssignAttr(prim, data));

    //
    // Set the 'prmanStatements' attribute.
    //

    FnKat::GroupBuilder statementsBuilder;
    PxrUsdKatanaReadPrimPrmanStatements(prim, data.GetUsdInArgs()->GetCurrentTime(), statementsBuilder);
    FnKat::GroupAttribute statements = statementsBuilder.build();
    if (statements.getNumberOfChildren() > 0)
    {
        attrs.set("prmanStatements", statements);
    }

    //
    // Set the 'visible' attribute. Since locations are visible by default
    // only set the attribute if the prim is imageable and invisible.
    //

    TfToken visibility;
    UsdGeomImageable imageable = UsdGeomImageable(prim);
    if (imageable and imageable.GetVisibilityAttr().Get(&visibility, currentTime))
    {
        if (visibility == UsdGeomTokens->invisible)
        {
            attrs.set("visible", FnKat::IntAttribute(0));
        }
    }

    //
    // Set the 'purpose' attribute to exactly match the USD attribute
    // if it is not 'default' (since that is the default value). And,
    // if the 'purpose' happens to be 'proxy', make invisible as well.
    //

    TfToken purpose;
    if (imageable and UsdGeomImageable(prim).GetPurposeAttr().Get(&purpose))
    {
        if (purpose != UsdGeomTokens->default_)
        {
            attrs.set("usd.purpose", FnKat::StringAttribute(purpose.GetString()));
        }

        if (purpose == UsdGeomTokens->proxy)
        {
            attrs.set("visible", FnKat::IntAttribute(0));
        }
    }

    //
    // Set the primvar attributes
    //

    if (imageable)
    {
        FnKat::GroupAttribute primvarGroup = PxrUsdKatanaGeomGetPrimvarGroup(imageable, data);

        if (primvarGroup.isValid())
        {
            FnKat::GroupBuilder arbBuilder;
            arbBuilder.update(primvarGroup);
            attrs.set("geometry.arbitrary", arbBuilder.build());
        }
    }

    //
    // Set the 'relativeScopedCoordinateSystems' attribute if such coordinate
    // systems are found in the children of this prim.
    //

    FnKat::GroupBuilder coordSysBuilder;
    if (_BuildScopedCoordinateSystems(prim, coordSysBuilder))
    {
        attrs.set("relativeScopedCoordinateSystems", coordSysBuilder.build());
    }

    //
    // Set the 'collections' attribute if any found
    //

    FnKat::GroupBuilder collectionsBuilder;
    if (_BuildCollections(prim, collectionsBuilder))
    {
        attrs.set("collections", collectionsBuilder.build());
    }


    //
    // Set the 'customProperties' attribute (if enabled by env variable).
    //

    FnKat::GroupBuilder customBuilder;
    if (_AddCustomProperties(prim, currentTime, customBuilder))
    {
        attrs.set("customProperties", customBuilder.build());
    }

    _AddExtraAttributesOrNamespaces(prim, data, attrs);
}
