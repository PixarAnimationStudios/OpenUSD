//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDVOL_GENERATED_VOLUME_H
#define USDVOL_GENERATED_VOLUME_H

/// \file usdVol/volume.h

#include "pxr/pxr.h"
#include "pxr/usd/usdVol/api.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// VOLUME                                                                     //
// -------------------------------------------------------------------------- //

/// \class UsdVolVolume
///
/// A renderable volume primitive. A volume is made up of any number
/// of FieldBase primitives bound together in this volume. Each
/// FieldBase primitive is specified as a relationship with a
/// namespace prefix of "field".
/// 
/// The relationship name is used by the renderer to associate
/// individual fields with the named input parameters on the volume
/// shader. Using this indirect approach to connecting fields to
/// shader parameters (rather than using the field prim's name)
/// allows a single field to be reused for different shader inputs, or
/// to be used as different shader parameters when rendering different
/// Volumes. This means that the name of the field prim is not
/// relevant to its contribution to the volume prims which refer to
/// it. Nor does the field prim's location in the scene graph have
/// any relevance, and Volumes may refer to fields anywhere in the
/// scene graph.  **However**, unless Field prims need to be shared
/// by multiple Volumes, a Volume's Field prims should be located
/// under the Volume in namespace, for enhanced organization.
///
class UsdVolVolume : public UsdGeomGprim
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a UsdVolVolume on UsdPrim \p prim .
    /// Equivalent to UsdVolVolume::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdVolVolume(const UsdPrim& prim=UsdPrim())
        : UsdGeomGprim(prim)
    {
    }

    /// Construct a UsdVolVolume on the prim held by \p schemaObj .
    /// Should be preferred over UsdVolVolume(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdVolVolume(const UsdSchemaBase& schemaObj)
        : UsdGeomGprim(schemaObj)
    {
    }

    /// Destructor.
    USDVOL_API
    virtual ~UsdVolVolume();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDVOL_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdVolVolume holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdVolVolume(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDVOL_API
    static UsdVolVolume
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
    USDVOL_API
    static UsdVolVolume
    Define(const UsdStagePtr &stage, const SdfPath &path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDVOL_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDVOL_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDVOL_API
    const TfType &_GetTfType() const override;

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
    /// \name Field Attachment and Inspection
    /// @{
    // --------------------------------------------------------------------- //

    typedef std::map<TfToken, SdfPath> FieldMap;
    
    /// Return a map of field relationship names to the fields themselves,
    /// represented as prim paths.  This map provides all the information
    /// that should be needed to tie fields to shader parameters and render
    /// this volume.
    ///
    /// The field relationship names that server as the map keys will have
    /// the field namespace stripped from them.
    USDVOL_API
    FieldMap GetFieldPaths() const;

    /// Checks if there is an existing field relationship with a given name.
    /// This query will return \c true even for a field relationship that
    /// has been blocked and therefore will not contribute to the map
    /// returned by GetFieldRelationships()
    ///
    /// The name lookup automatically applies the field relationship
    /// namespacing, if it isn't specified in the name token.
    USDVOL_API
    bool HasFieldRelationship(const TfToken &name) const;

    /// Checks if there is an existing field relationship with a given name,
    /// and if so, returns the path to the Field prim it targets, or else
    /// the empty path.
    ///
    /// The name lookup automatically applies the field relationship
    /// namespacing, if it isn't specified in the name token.
    USDVOL_API
    SdfPath GetFieldPath(const TfToken &name) const;

    /// Creates a relationship on this volume that targets the specified field.
    /// If an existing relationship exists with the same name, it is replaced
    /// (since only one target is allowed for each named relationship).
    ///
    /// Returns \c true if the relationship was successfully created and set - 
    /// it is legal to call this method for a field relationship that already
    /// "exists", i.e. already posesses scene description, as this is the
    /// only method we provide for setting a field relatioonship's value, to help
    /// enforce that field relationships can have only a single (or no) target.
    ///
    /// \param fieldPath - can be a prim path, or the path of another 
    ///        relationship, to effect \ref usd_relationship_forwarding
    ///
    /// The name lookup automatically applies the field relationship
    /// namespacing, if it isn't specified in the name token.
    USDVOL_API
    bool CreateFieldRelationship(const TfToken &name,
                                 const SdfPath &fieldPath) const;

    /// Blocks an existing field relationship on this volume, ensuring it
    /// will not be enumerated by GetFieldPaths().
    ///
    /// Returns true if the relationship existed, false if it did not. In other
    /// words the return value indicates whether the volume prim was changed.
    ///
    /// The name lookup automatically applies the field relationship
    /// namespacing, if it isn't specified in the name token.
    USDVOL_API
    bool BlockFieldRelationship(const TfToken &name) const;

private:
    /// Return \p name prepended with the field namespace, if it isn't
    /// already prefixed.
    ///
    /// Does not validate name as a legal relationship identifier.
    static TfToken _MakeNamespaced(const TfToken& name);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
