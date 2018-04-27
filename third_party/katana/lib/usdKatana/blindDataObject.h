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
#ifndef USDKATANA_GENERATED_BLINDDATAOBJECT_H
#define USDKATANA_GENERATED_BLINDDATAOBJECT_H

/// \file usdKatana/blindDataObject.h

#include "pxr/pxr.h"
#include "usdKatana/api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "usdKatana/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// BLINDDATAOBJECT                                                            //
// -------------------------------------------------------------------------- //

/// \class UsdKatanaBlindDataObject
///
/// Container namespace schema for katana blind data from the klf file
///
class UsdKatanaBlindDataObject : public UsdTyped
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = true;

    /// Construct a UsdKatanaBlindDataObject on UsdPrim \p prim .
    /// Equivalent to UsdKatanaBlindDataObject::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdKatanaBlindDataObject(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a UsdKatanaBlindDataObject on the prim held by \p schemaObj .
    /// Should be preferred over UsdKatanaBlindDataObject(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdKatanaBlindDataObject(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    USDKATANA_API
    virtual ~UsdKatanaBlindDataObject();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDKATANA_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdKatanaBlindDataObject holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdKatanaBlindDataObject(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDKATANA_API
    static UsdKatanaBlindDataObject
    Get(const UsdStagePtr &stage, const SdfPath &path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    USDKATANA_API
    static UsdKatanaBlindDataObject
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDKATANA_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDKATANA_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // TYPE 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: std::string
    /// \n  Usd Type: SdfValueTypeNames->String
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDKATANA_API
    UsdAttribute GetTypeAttr() const;

    /// See GetTypeAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDKATANA_API
    UsdAttribute CreateTypeAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VISIBLE 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDKATANA_API
    UsdAttribute GetVisibleAttr() const;

    /// See GetVisibleAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDKATANA_API
    UsdAttribute CreateVisibleAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SUPPRESSGROUPTOASSEMBLYPROMOTION 
    // --------------------------------------------------------------------- //
    /// If true don't promote a group to an assembly.
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDKATANA_API
    UsdAttribute GetSuppressGroupToAssemblyPromotionAttr() const;

    /// See GetSuppressGroupToAssemblyPromotionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDKATANA_API
    UsdAttribute CreateSuppressGroupToAssemblyPromotionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    // --------------------------------------------------------------------- //
    // CreateKbdAttribute 
    // --------------------------------------------------------------------- //
    /// \brief Create an attribute on the prim to which this schema is attached.
    ///
    /// This will be a blind representation of a Katana attribute from Klf file.
    /// \p katanaFullName should be the full attribute name from katana, i.e. 
    /// "materials.interface.foo".   \p usdType is the typename for the
    /// attribute and will be passed directly to \p UsdPrim::CreateAttribute().
    UsdAttribute
    CreateKbdAttribute(
        const std::string &katanaFullName,
        const SdfValueTypeName &usdType);

    // --------------------------------------------------------------------- //
    // GetKbdAttributes 
    // --------------------------------------------------------------------- //
    /// Return all rib attributes on this prim, or under a specific 
    /// namespace (e.g. "user")
    ///
    /// As noed above, rib attributes can be either UsdAttribute or 
    /// UsdRelationship, and like all UsdProperties, need not have a defined 
    /// value.
    std::vector<UsdProperty>
    GetKbdAttributes(const std::string &nameSpace = "") const;

    // --------------------------------------------------------------------- //
    // GetKbdAttribute
    // --------------------------------------------------------------------- //
    /// Return a specific KBD attribute
    UsdAttribute
    GetKbdAttribute(const std::string &katanaFullName);

    // --------------------------------------------------------------------- //
    // GetKbdAttributeNameSpace
    // --------------------------------------------------------------------- //
    /// Return the containing namespace of the katana attribute (e.g.
    /// "geometry" or "materials").  Can be used with
    /// GetGroupBuilderKeyForProperty()
    ///
    static TfToken GetKbdAttributeNameSpace(const UsdProperty &prop);

    // --------------------------------------------------------------------- //
    // GetGroupBuilderKeyForProperty
    // --------------------------------------------------------------------- //
    /// Return a string that is the attribute name that can be used with a
    /// group builder.  For example, when constructing the GroupAttribute for
    /// the top-level group "geometry", this should be used as follows:
    /// 
    /// FnKat::GroupBuilder gb;
    /// props = UsdKatanaBlindDataObject(prim).GetKbdAttribute("geometry");
    /// gb.set(UsdKatanaBlindDataObject::GetGroupBuilderKeyForProperty(props[0]), ...)
    /// return gb.build();
    ///
    /// For the attribute:
    /// custom int katana:fromKlf:materials:interface:foo = 0
    ///
    /// this returns
    /// "interface.foo"
    ///
    /// To get "materials", use GetKbdAttributeNameSpace()
    static std::string GetGroupBuilderKeyForProperty(const UsdProperty& prop);

    // --------------------------------------------------------------------- //
    // IsKbdAttribute
    // --------------------------------------------------------------------- //
    /// Return true if the property is in the "ri:attributes" namespace.
    ///
    static bool IsKbdAttribute(const UsdProperty &prop);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
