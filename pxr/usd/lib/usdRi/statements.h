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
#ifndef USDRI_GENERATED_STATEMENTS_H
#define USDRI_GENERATED_STATEMENTS_H

/// \file usdRi/statements.h

#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdRi/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// STATEMENTS                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdRiStatements
///
/// Container namespace schema for all renderman statements
///
class UsdRiStatements : public UsdSchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdRiStatements on UsdPrim \p prim .
    /// Equivalent to UsdRiStatements::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdRiStatements(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
    {
    }

    /// Construct a UsdRiStatements on the prim held by \p schemaObj .
    /// Should be preferred over UsdRiStatements(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdRiStatements(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
    {
    }

    /// Destructor.
    virtual ~UsdRiStatements();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdRiStatements holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdRiStatements(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    static UsdRiStatements
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // FOCUSREGION 
    // --------------------------------------------------------------------- //
    /// Represents the value of the 'focusregion' option to RiCamera 
    /// in centimeters. Specifies the stretch of space along the camera view 
    /// axis surrounding the focus plane that contains everything that will 
    /// be rendered in perfect focus.  If unauthored, a value of zero should
    /// be assumed. <b>Note:</b> this parameter may not be supportable in
    /// RIS renders in RenderMan 19 and above.
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    UsdAttribute GetFocusRegionAttr() const;

    /// See GetFocusRegionAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateFocusRegionAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class declaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    // --------------------------------------------------------------------- //
    // CreateRiAttribute 
    // --------------------------------------------------------------------- //
    /// Create a rib attribute on the prim to which this schema is attached.
    /// A rib attribute consists of an attribute \em "nameSpace" and an
    /// attribute \em "name".  For example, the namespace "cull" may define
    /// attributes "backfacing" and "hidden", and user-defined attributes
    /// belong to the namespace "user".
    ///
    /// This method makes no attempt to validate that the given \p nameSpace
    /// and \em name are actually meaningful to prman or any other
    /// renderer.
    ///
    /// \param riType should be a known RenderMan type definition, which
    /// can be array-valued.  For instance, both "color" and "float[3]"
    /// are valid values for \p riType.
    UsdAttribute
    CreateRiAttribute(
        const TfToken &name, 
        const std::string &riType,
        const std::string &nameSpace = "user");

    /// Creates an attribute of the given \p tfType.
    /// \overload
    UsdAttribute
    CreateRiAttribute(
        const TfToken &name, 
        const TfType &tfType,
        const std::string &nameSpace = "user");

    // --------------------------------------------------------------------- //
    // CreateRiAttributeAsRel
    // --------------------------------------------------------------------- //
    /// The purpose of this type of rib attribute is to emit an identifier for 
    /// an object in the scenegraph, which might be a prim or a property.  
    /// We identify objects by targetting them with a relationship, which is 
    /// why this method creates a UsdRelationship.  In RenderMan, strings make 
    /// the best identifiers, so clients will likely want to transform the 
    /// target's identity into a string for RenderMan, although it is up to 
    /// your pipeline to choose.
    UsdRelationship
    CreateRiAttributeAsRel(
        const TfToken &name, 
        const std::string &nameSpace = "user");

    // --------------------------------------------------------------------- //
    // GetRiAttributes 
    // --------------------------------------------------------------------- //
    /// Return all rib attributes on this prim, or under a specific 
    /// namespace (e.g.\ "user").
    ///
    /// As noted above, rib attributes can be either UsdAttribute or 
    /// UsdRelationship, and like all UsdProperties, need not have a defined 
    /// value.
    std::vector<UsdProperty>
    GetRiAttributes(const std::string &nameSpace = "") const;

    bool 
    _IsCompatible(const UsdPrim &prim) const;

    // --------------------------------------------------------------------- //
    // GetRiAttributeName 
    // --------------------------------------------------------------------- //
    /// Return the base, most-specific name of the rib attribute.  For example,
    /// the \em name of the rib attribute "cull:backfacing" is "backfacing"
    inline static TfToken GetRiAttributeName(const UsdProperty &prop) {
        return prop.GetBaseName();
    }

    // --------------------------------------------------------------------- //
    // GetRiAttributeNameSpace
    // --------------------------------------------------------------------- //
    /// Return the containing namespace of the rib attribute (e.g.\ "user").
    ///
    static TfToken GetRiAttributeNameSpace(const UsdProperty &prop);

    // --------------------------------------------------------------------- //
    // IsRiAttribute
    // --------------------------------------------------------------------- //
    /// Return true if the property is in the "ri:attributes" namespace.
    ///
    static bool IsRiAttribute(const UsdProperty &prop);

    // --------------------------------------------------------------------- //
    // MakeRiAttributePropertyName
    // --------------------------------------------------------------------- //
    /// Returns the given \p attrName prefixed with the full Ri attribute
    /// namespace, creating a name suitable for an RiAttribute UsdProperty.
    /// This handles conversion of common separator characters used in
    /// other packages, such as periods and underscores.
    ///
    /// Will return empty string if attrName is not a valid property
    /// identifier; otherwise, will return a valid property name
    /// that identifies the property as an RiAttribute, according to the 
    /// following rules:
    /// \li If \p attrName is already a properly constructed RiAttribute 
    ///     property name, return it unchanged.  
    /// \li If \p attrName contains two or more tokens separated by a \em colon,
    ///     consider the first to be the namespace, and the rest the name, 
    ///     joined by underscores
    /// \li If \p attrName contains two or more tokens separated by a \em period,
    ///     consider the first to be the namespace, and the rest the name, 
    ///     joined by underscores
    /// \li If \p attrName contains two or more tokens separated by an,
    ///     \em underscore consider the first to be the namespace, and the
    ///     rest the name, joined by underscores
    /// \li else, assume \p attrName is the name, and "user" is the namespace
    static std::string MakeRiAttributePropertyName(const std::string &attrName);

    // --------------------------------------------------------------------- //
    // SetCoordinateSystem
    // --------------------------------------------------------------------- //
    /// Sets the "ri:coordinateSystem" attribute to the given string value,
    /// creating the attribute if needed. That identifies this prim as providing
    /// a coordinate system, which can be retrieved via
    /// UsdGeomXformable::GetTransformAttr(). Also adds the owning prim to the
    /// ri:modelCoordinateSystems relationship targets on its parent leaf model
    /// prim, if it exists. If this prim is not under a leaf model, no
    /// relationship targets will be authored.
    ///
    void SetCoordinateSystem(const std::string &coordSysName);

    // --------------------------------------------------------------------- //
    // GetCoordinateSystem
    // --------------------------------------------------------------------- //
    /// Returns the value in the "ri:coordinateSystem" attribute if it exists.
    ///
    std::string GetCoordinateSystem() const;

    // --------------------------------------------------------------------- //
    // HasCoordinateSystem
    // --------------------------------------------------------------------- //
    /// Returns true if the underlying prim has a ri:coordinateSystem opinion.
    ///
    bool HasCoordinateSystem() const;

    // --------------------------------------------------------------------- //
    // SetScopedCoordinateSystem
    // --------------------------------------------------------------------- //
    /// Sets the "ri:scopedCoordinateSystem" attribute to the given string
    /// value, creating the attribute if needed. That identifies this prim as
    /// providing a coordinate system, which can be retrieved via
    /// UsdGeomXformable::GetTransformAttr(). Such coordinate systems are
    /// local to the RI attribute stack state, but does get updated properly
    /// for instances when defined inside an object master.  Also adds the
    /// owning prim to the ri:modelScopedCoordinateSystems relationship
    /// targets on its parent leaf model prim, if it exists. If this prim is
    /// not under a leaf model, no relationship targets will be authored.
    ///
    void SetScopedCoordinateSystem(const std::string &coordSysName);

    // --------------------------------------------------------------------- //
    // GetScopedCoordinateSystem
    // --------------------------------------------------------------------- //
    /// Returns the value in the "ri:scopedCoordinateSystem" attribute if it
    /// exists.
    ///
    std::string GetScopedCoordinateSystem() const;

    // --------------------------------------------------------------------- //
    // HasScopedCoordinateSystem
    // --------------------------------------------------------------------- //
    /// Returns true if the underlying prim has a ri:scopedCoordinateSystem
    /// opinion.
    ///
    bool HasScopedCoordinateSystem() const;

    // --------------------------------------------------------------------- //
    // GetModelCoordinateSystems
    // --------------------------------------------------------------------- //
    /// Populates the output \p targets with the authored
    /// ri:modelCoordinateSystems, if any. Returns true if the query was
    /// successful.
    ///
    bool GetModelCoordinateSystems(SdfPathVector *targets) const;

    // --------------------------------------------------------------------- //
    // GetModelScopedCoordinateSystems
    // --------------------------------------------------------------------- //
    /// Populates the output \p targets with the authored
    /// ri:modelScopedCoordinateSystems, if any.  Returns true if the query was
    /// successful.
    ///
    bool GetModelScopedCoordinateSystems(SdfPathVector *targets) const;

};

#endif
