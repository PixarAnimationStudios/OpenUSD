//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDGEOM_GENERATED_PRIMVARSAPI_H
#define USDGEOM_GENERATED_PRIMVARSAPI_H

/// \file usdGeom/primvarsAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdGeom/primvar.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PRIMVARSAPI                                                                //
// -------------------------------------------------------------------------- //

/// \class UsdGeomPrimvarsAPI
///
/// UsdGeomPrimvarsAPI encodes geometric "primitive variables",
/// as UsdGeomPrimvar, which interpolate across a primitive's topology,
/// can override shader inputs, and inherit down namespace.
/// 
/// \section usdGeom_PrimvarFetchingAPI Which Method to Use to Retrieve Primvars
/// 
/// While creating primvars is unambiguous (CreatePrimvar()), there are quite
/// a few methods available for retrieving primvars, making it potentially
/// confusing knowing which one to use.  Here are some guidelines:
/// 
/// \li If you are populating a GUI with the primvars already available for 
/// authoring values on a prim, use GetPrimvars().
/// \li If you want all of the "useful" (e.g. to a renderer) primvars
/// available at a prim, including those inherited from ancestor prims, use
/// FindPrimvarsWithInheritance().  Note that doing so individually for many
/// prims will be inefficient.
/// \li To find a particular primvar defined directly on a prim, which may
/// or may not provide a value, use GetPrimvar().
/// \li To find a particular primvar defined on a prim or inherited from
/// ancestors, which may or may not provide a value, use 
/// FindPrimvarWithInheritance().
/// \li To *efficiently* query for primvars using the overloads of
/// FindPrimvarWithInheritance() and FindPrimvarsWithInheritance(), one
/// must first cache the results of FindIncrementallyInheritablePrimvars() for
/// each non-leaf prim on the stage. 
///
class UsdGeomPrimvarsAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::NonAppliedAPI;

    /// Construct a UsdGeomPrimvarsAPI on UsdPrim \p prim .
    /// Equivalent to UsdGeomPrimvarsAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdGeomPrimvarsAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdGeomPrimvarsAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdGeomPrimvarsAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdGeomPrimvarsAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDGEOM_API
    virtual ~UsdGeomPrimvarsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDGEOM_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdGeomPrimvarsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdGeomPrimvarsAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDGEOM_API
    static UsdGeomPrimvarsAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDGEOM_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDGEOM_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDGEOM_API
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

    /// Author scene description to create an attribute on this prim that
    /// will be recognized as Primvar (i.e. will present as a valid
    /// UsdGeomPrimvar).
    ///
    /// The name of the created attribute may or may not be the specified
    /// \p name, due to the possible need to apply property namespacing
    /// for primvars.  See \ref Usd_Creating_and_Accessing_Primvars
    /// for more information.  Creation may fail and return an invalid
    /// Primvar if \p name contains a reserved keyword, such as the 
    /// "indices" suffix we use for indexed primvars.
    ///
    /// The behavior with respect to the provided \p typeName
    /// is the same as for UsdAttributes::Create(), and
    /// \p interpolation and \p elementSize are as described in
    /// UsdGeomPrimvar::GetInterpolation() and UsdGeomPrimvar::GetElementSize().
    ///
    /// If \p interpolation and/or \p elementSize are left unspecified, we
    /// will author no opinions for them, which means any (strongest) opinion
    /// already authored in any contributing layer for these fields will
    /// become the Primvar's values, or the fallbacks if no opinions
    /// have been authored.
    ///
    /// \return an invalid UsdGeomPrimvar if we failed to create a valid
    /// attribute, a valid UsdGeomPrimvar otherwise.  It is not an
    /// error to create over an existing, compatible attribute.
    ///
    /// \sa UsdPrim::CreateAttribute(), UsdGeomPrimvar::IsPrimvar()
    USDGEOM_API
    UsdGeomPrimvar CreatePrimvar(const TfToken& name,
                                 const SdfValueTypeName &typeName,
                                 const TfToken& interpolation = TfToken(),
                                 int elementSize = -1) const;

    /// Author scene description to create an attribute and authoring a \p value
    /// on this prim that will be recognized as a Primvar (i.e. will present as 
    /// a valid UsdGeomPrimvar). Note that unlike CreatePrimvar using this API 
    /// explicitly authors a block for the indices attr associated with the
    /// primvar, thereby blocking any indices set in any weaker layers.
    ///
    /// \return an invalid UsdGeomPrimvar on error, a valid UsdGeomPrimvar 
    /// otherwise. It is fine to call this method multiple times, and in
    /// different UsdEditTargets, even if there is an existing primvar of the
    /// same name, indexed or not.
    ///
    /// \sa CreatePrimvar(), CreateIndexedPrimvar(), UsdPrim::CreateAttribute(), 
    /// UsdGeomPrimvar::IsPrimvar()
    template <typename T>
    UsdGeomPrimvar CreateNonIndexedPrimvar(
            const TfToken& name,
            const SdfValueTypeName &typeName,
            const T &value,
            const TfToken &interpolation = TfToken(),
            int elementSize = -1,
            UsdTimeCode time = UsdTimeCode::Default()) const
    {
        UsdGeomPrimvar primvar = 
            CreatePrimvar(name, typeName, interpolation, elementSize);

        primvar.GetAttr().Set(value, time);
        primvar.BlockIndices();
        return primvar;
    }

    /// Author scene description to create an attribute and authoring a \p value
    /// on this prim that will be recognized as an indexed Primvar with \p
    /// indices appropriately set (i.e. will present as a valid UsdGeomPrimvar).
    ///
    /// \return an invalid UsdGeomPrimvar on error, a valid UsdGeomPrimvar 
    /// otherwise. It is fine to call this method multiple times, and in
    /// different UsdEditTargets, even if there is an existing primvar of the
    /// same name, indexed or not.
    ///
    /// \sa CreatePrimvar(), CreateNonIndexedPrimvar(), 
    /// UsdPrim::CreateAttribute(), UsdGeomPrimvar::IsPrimvar()
    template <typename T>
    UsdGeomPrimvar CreateIndexedPrimvar(
            const TfToken& name,
            const SdfValueTypeName &typeName,
            const T &value,
            const VtIntArray &indices,
            const TfToken &interpolation = TfToken(),
            int elementSize = -1,
            UsdTimeCode time = UsdTimeCode::Default()) const
    {
        UsdGeomPrimvar primvar = 
            CreatePrimvar(name, typeName, interpolation, elementSize);

        primvar.GetAttr().Set(value, time);
        primvar.SetIndices(indices, time);
        return primvar;
    }

    /// Author scene description to delete an attribute on this prim that
    /// was recognized as Primvar (i.e. will present as a valid UsdGeomPrimvar),
    /// <em>in the current UsdEditTarget</em>.
    ///
    /// Because this method can only remove opinions about the primvar 
    /// from the current EditTarget, you may generally find it more useful to 
    /// use BlockPrimvar() which will ensure that all values from the EditTarget 
    /// and weaker layers for the primvar and its indices will be ignored.
    ///
    /// Removal may fail and return false if \p name contains a reserved 
    /// keyword, such as the "indices" suffix we use for indexed primvars.
    ///
    /// Note this will also remove the indices attribute associated with an
    /// indiced primvar. 
    ///
    /// \return true if UsdGeomPrimvar and indices attribute was successfully 
    /// removed, false otherwise.
    ///
    /// \sa UsdPrim::RemoveProperty())
    USDGEOM_API
    bool RemovePrimvar(const TfToken& name);

    /// Remove all time samples on the primvar and its associated indices attr, 
    /// and author a *block* \c default value. This will cause authored opinions
    /// in weaker layers to be ignored.
    ///
    /// \sa UsdAttribute::Block(), UsdGeomPrimvar::BlockIndices
    USDGEOM_API
    void BlockPrimvar(const TfToken& name);

    /// Return the Primvar object named by \p name, which will
    /// be valid if a Primvar attribute definition already exists.
    ///
    /// Name lookup will account for Primvar namespacing, which means
    /// that this method will succeed in some cases where
    /// \code
    /// UsdGeomPrimvar(prim->GetAttribute(name))
    /// \endcode
    /// will not, unless \p name is properly namespace prefixed.
    ///
    /// \note Just because a Primvar is valid and defined, and *even if* its
    /// underlying UsdAttribute (GetAttr()) answers HasValue() affirmatively,
    /// one must still check the return value of Get(), due to the potential
    /// of time-varying value blocks (see \ref Usd_AttributeBlocking).
    ///
    /// \sa HasPrimvar(), \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    UsdGeomPrimvar GetPrimvar(const TfToken &name) const;
    
    /// Return valid UsdGeomPrimvar objects for all defined Primvars on
    /// this prim, similarly to UsdPrim::GetAttributes().
    ///
    /// The returned primvars may not possess any values, and therefore not
    /// be useful to some clients. For the primvars useful for inheritance
    /// computations, see GetPrimvarsWithAuthoredValues(), and for primvars
    /// useful for direct consumption, see GetPrimvarsWithValues().
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    std::vector<UsdGeomPrimvar> GetPrimvars() const;

    /// Like GetPrimvars(), but include only primvars that have some
    /// authored scene description (though not necessarily a value).
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    std::vector<UsdGeomPrimvar> GetAuthoredPrimvars() const;

    /// Like GetPrimvars(), but include only primvars that have some
    /// value, whether it comes from authored scene description or a schema
    /// fallback.
    ///
    /// For most purposes, this method is more useful than GetPrimvars().
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    std::vector<UsdGeomPrimvar> GetPrimvarsWithValues() const;

    /// Like GetPrimvars(), but include only primvars that have an **authored**
    /// value.
    ///
    /// This is the query used when computing inheritable primvars, and is
    /// generally more useful than GetAuthoredPrimvars().
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    std::vector<UsdGeomPrimvar> GetPrimvarsWithAuthoredValues() const;

    /// Compute the primvars that can be inherited from this prim by its
    /// child prims, including the primvars that **this** prim inherits from
    /// ancestor prims.  Inherited primvars will be bound to attributes on
    /// the corresponding ancestor prims.
    ///
    /// Only primvars with **authored**, **non-blocked**,
    /// **constant interpolation** values are inheritable;
    /// fallback values are not inherited.   The order of the returned
    /// primvars is undefined.
    ///
    /// It is not generally useful to call this method on UsdGeomGprim leaf
    /// prims, and furthermore likely to be expensive since *most* primvars
    /// are defined on Gprims.
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    std::vector<UsdGeomPrimvar> FindInheritablePrimvars() const;

    /// Compute the primvars that can be inherited from this prim by its
    /// child prims, starting from the set of primvars inherited from
    /// this prim's ancestors.  If this method returns an empty vector, then
    /// this prim's children should inherit the same set of primvars available
    /// to this prim, i.e. the input `inheritedFromAncestors` .
    ///
    /// As opposed to FindInheritablePrimvars(), which always recurses up
    /// through all of the prim's ancestors, this method allows more
    /// efficient computation of inheritable primvars by starting with the
    /// list of primvars inherited from this prim's ancestors, and returning
    /// a newly allocated vector only when this prim makes a change to the
    /// set of inherited primvars.  This enables O(n) inherited primvar
    /// computation for all prims on a Stage, with potential to share
    /// computed results that are identical (i.e. when this method returns an
    /// empty vector, its parent's result can (and must!) be reused for all
    /// of the prim's children.
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    std::vector<UsdGeomPrimvar> FindIncrementallyInheritablePrimvars(
        const std::vector<UsdGeomPrimvar> &inheritedFromAncestors) const;

    /// Like GetPrimvar(), but if the named primvar does not exist or has no
    /// authored value on this prim, search for the named, value-producing
    /// primvar on ancestor prims.
    /// 
    /// The returned primvar will be bound to the attribute on the 
    /// corresponding ancestor prim on which it was found (if any).  If neither
    /// this prim nor any ancestor contains a value-producing primvar, then
    /// the returned primvar will be the same as that returned by GetPrimvar().
    ///
    /// This is probably the method you want to call when needing to consume
    /// a primvar of a particular name.
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    UsdGeomPrimvar FindPrimvarWithInheritance(const TfToken &name) const;

    /// \overload
    /// 
    /// This version of FindPrimvarWithInheritance() takes the pre-computed
    /// set of primvars inherited from this prim's ancestors, as computed
    /// by FindInheritablePrimvars() or FindIncrementallyInheritablePrimvars()
    /// on the prim's parent.
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    UsdGeomPrimvar FindPrimvarWithInheritance(const TfToken &name,
        const std::vector<UsdGeomPrimvar> &inheritedFromAncestors) const;

    /// Find all of the value-producing primvars either defined on this prim,
    /// or inherited from ancestor prims.
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    std::vector<UsdGeomPrimvar> FindPrimvarsWithInheritance() const;

    /// \overload
    /// 
    /// This version of FindPrimvarsWithInheritance() takes the pre-computed
    /// set of primvars inherited from this prim's ancestors, as computed
    /// by FindInheritablePrimvars() or FindIncrementallyInheritablePrimvars()
    /// on the prim's parent.
    ///
    /// \sa \ref usdGeom_PrimvarFetchingAPI
    USDGEOM_API
    std::vector<UsdGeomPrimvar> FindPrimvarsWithInheritance(
        const std::vector<UsdGeomPrimvar> &inheritedFromAncestors) const;

    /// Is there a defined Primvar \p name on this prim?
    ///
    /// Name lookup will account for Primvar namespacing.
    ///
    /// Like GetPrimvar(), a return value of `true` for HasPrimvar() does not
    /// guarantee the primvar will produce a value.
    USDGEOM_API
    bool HasPrimvar(const TfToken &name) const;

    /// Is there a Primvar named \p name with an authored value on this
    /// prim or any of its ancestors?
    ///
    /// This is probably the method you want to call when wanting to know
    /// whether or not the prim "has" a primvar of a particular name.
    ///
    /// \sa FindPrimvarWithInheritance()
    USDGEOM_API
    bool HasPossiblyInheritedPrimvar(const TfToken &name) const;

    /// Test whether a given \p name contains the "primvars:" prefix
    ///
    USDGEOM_API
    static bool CanContainPropertyName(const TfToken& name);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
