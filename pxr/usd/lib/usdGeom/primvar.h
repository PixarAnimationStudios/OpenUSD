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
#ifndef USDGEOM_PRIMVAR_H
#define USDGEOM_PRIMVAR_H

#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <string>
#include <vector>

/// \class UsdGeomPrimvar
///
/// Schema wrapper for UsdAttribute for authoring and introspecting attributes
/// that are primvars.
///
/// UsdGeomPrimvar provides API for authoring and retrieving the
/// additional data required to encode an attribute as a "Primvar",
/// which is a convenient contraction of RenderMan's "Primitive Variable" 
/// concept, which is represented in Alembic as 
/// "arbitrary geometry parameters" (arbGeomParams).
///
/// This includes the attribute's \ref GetInterpolation() "interpolation"
/// across the primitive (which RenderMan refers to as its 
/// \ref Usd_InterpolationVals "class specifier"
/// and Alembic as its <A HREF="http://code.google.com/p/alembic/source/browse/lib/Alembic/AbcGeom/GeometryScope.h#46"> "geometry scope"</A>);
/// it also includes the attribute's \ref GetElementSize() "elementSize",
/// which states how many values in the value array must be aggregated for
/// each element on the primitive.  An attribute's \ref
/// UsdAttribute::GetTypeName() "TypeName" also factors into the encoding of 
/// Primvar.
///
/// \section Usd_What_Is_Primvar What is the Purpose of a Primvar?
///
/// There are two key aspects of Primvar identity:
/// \li Primvars define a value that can vary across the primitive on which
///     they are defined, via prescribed interpolation rules
/// \li Taken collectively on a prim, its Primvars describe the "per-primitive
///     overrides" to the shader(s) to which the prim is bound.  Different
///     renderers may communicate the variables to the shaders using different
///     mechanisms over which Usd has no control; Primvars simply provide the
///     classification that any renderer should use to locate potential
///     overrides.  Do please note that primvars override parameters on
///     UsdShadeShader objects, \em not 
///     \ref UsdShadeLook_Interfaces "Interface Attributes" on UsdShadeLook
///     prims.
///
/// \section Usd_Creating_and_Accessing_Primvars Creating and Accessing Primvars
/// 
/// The <b>only</b> way to create a new Primvar in scene description is by
/// calling UsdGeomImageable::CreatePrimvar().  One cannot "enhance" or
/// "promote" an already existing attribute into a Primvar, because doing so
/// may require a namespace edit to rename the attribute, which cannot, in
/// general, be done within a single UsdEditContext.  Instead, create a new
/// UsdGeomPrimvar using UsdGeomImageable::CreatePrimvar(), and then copy the
/// existing attribute onto the new UsdGeomPrimvar.
///
/// <b>Primvar names cannot contain other namespaces.</b> This is because
/// when Primvars are consumed by a renderer, they are identified by their
/// GetBaseName().  If arbitrary namespaces were allowed in Primvar names,
/// it would be possible to create two otherwise legitimate attributes that
/// would alias to the same property for renderer consumption.  This is
/// enforced in UsdGeomImageable::CreatePrimvar().
///
/// \anchor UsdGeomPrimvar_Using_Primvar
/// If a client wishes to access an already-extant attribute as a Primvar,
/// (which may or may not actually be valid Primvar), they can use the
/// speculative constructor like so:
/// \code
/// if (UsdGeomPrimvar primvar = UsdGeomPrimvar(usdAttr)){
///     TfToken interpolation = primvar.GetInterpolation();
///     int     elementSize = primvar.GetElementSize();
///     ...
/// }
/// \endcode
///
/// (or if you possess UsdProperty objects instead, as the result of
/// a UsdPrim::GetProperties() call...)
/// \code
/// if (UsdGeomPrimvar primvar = UsdGeomPrimvar(usdProp.As<UsdAttribute>())){
/// }
/// \endcode
///
/// Python does not permit the 'assignment in conditional" pattern, so
/// the above example in python would be:
///
/// \code{.py}
/// primvar = Usd.Primvar(usdAttr)
/// if primvar:
///     interpolation = primvar.GetInterpolation()
///     elementSize = primvar.GetElementSize()
///     ...
/// \endcode
///
/// \subsection Usd_Handling_Indexed_Primvars Proper Client Handling of "Indexed" Primvars
///
/// As discussed in greater detail in 
/// \ref UsdGeomPrimvar_Indexed_primvars "Indexed Primvars", primvars can 
/// optionally contain a (possibly time-varying) indexing attribute that 
/// establishes a sharing topology for elements of the primvar.  Consumers
/// can always chose to ignore the possibility of indexed data by exclusively 
/// using the ComputeFlattened() API.  If a client wishes to preserve indexing
/// in their processing of a primvar, we suggest a pattern like the following,
/// which accounts for the fact that a stronger layer can 
/// \ref UsdAttribute::Block() "block" a primvar's indexing from a weaker
/// layer, via UsdGeomPrimvar::BlockIndices():
/// \code
/// VtValue values;
/// VtIntArray indices;
///
/// if (primvar.Get(&values, timeCode)){
///     if (primvar.GetIndices(&indices, timeCode)){
///         // primvar is indexed: validate/process values and indices together
///     }
///     else {
///         // primvar is not indexed: validate/process values as flat array
///     }
/// }
/// \endcode
///
/// \subsection Usd_Primvar_As_Attribute UsdGeomPrimvar and UsdAttribute API
/// 
/// UsdGeomPrimvar presents a small slice of the UsdAttribute API - enough to 
/// extract the data that comprises the "Declaration info", and get/set of
/// the attribute value.  A UsdGeomPrimvar also auto-converts to UsdAttribute, 
/// so you can pass a UsdGeomPrimvar to any function that accepts a UsdAttribute
/// or const-ref thereto.
///
/// \section Usd_Primvar_Types Primvar Allowed Scene Description Types and Plurality
/// There are no limitations imposed on the allowable scene description types
/// for Primvars; it is the responsibility of each consuming client to perform
/// renderer-specific conversions, if need be (the USD distribution will include
/// reference RenderMan conversion utilities).
///
/// A note about type plurality of Primvars: It is legitimate for a Primvar
/// to be of scalar or array type, and again, consuming clients must be
/// prepared to accomodate both.  However, while it is not possible, in all
/// cases, for USD to \em prevent one from \em changing the type of an attribute
/// in different layers or variants of an asset, it is never a good idea to
/// do so.  This is relevant because, except in a few special cases, it is
/// not possible to encode an \em interpolation of any value greater than 
/// \em constant without providing multiple (i.e. array) data values. Therefore,
/// if there is any possibility that downstream clients might need to change
/// a Primvar's interpolation, the Primvar-creator should encode it as an
/// array rather than a scalar.
///
/// Why allow scalar values at all, then?  First, sometimes it brings clarity
/// to (use of) a shader's API to acknowledge that some parameters are meant
/// to be single-valued over a shaded primitive.  Second, many DCC's provide
/// far richer affordances for editing scalars than they do array values, and
/// we feel it is safer to let the content creator make the decision/tradeoff
/// of which kind of flexibility is more relevant, rather than leaving it to
/// an importer/exporter pair to interpret.
///
/// Also, like all attributes, Primvars can be time-sampled, and values can
/// be authored and consumed just as any other attribute.  There is currently
/// no validation that the length of value arrays matches to the size 
/// required by a gprim's topology, interpolation, and elementSize.
///
/// For consumer convenience, we provide GetDeclarationInfo(), which returns
/// all the type information (other than topology) needed to compute the
/// required array size, which is also all the information required to 
/// prepare the Primvar's value for consumption by a renderer.
///
/// \section Usd_UsdGeomPrimvar_Lifetime Lifetime Management and Primvar Validity
///
/// UsdGeomPrimvar has an unspecified-bool-type operator that validates that
/// the attribute IsDefined() and thus valid for querying and authoring
/// values and metadata.  This is a fairly expensive query that we do 
/// <b>not</b> cache, so if client code retains UsdGeomPrimvar objects, it should 
/// manage its object validity closely, for performance.  An ideal pattern
/// is to listen for UsdNotice::StageContentsChanged notifications, and 
/// revalidate/refetch its retained UsdGeomPrimvar s only then, and otherwise use
/// them without validity checking.
///
/// \section Usd_InterpolationVals Interpolation of Geometric Primitive Variables
/// In the following explanation of the meaning of the various kinds/levels
/// of Primvar interpolation, each bolded bullet gives the name of the token
/// in \ref UsdGeomTokens that provides the value.  So to set a Primvar's
/// interpolation to "varying", one would:
/// \code
/// primvar.SetInterpolation(UsdGeomTokens->varying);
/// \endcode
/// 
/// Reprinted and adapted from <a HREF="http://renderman.pixar.com/resources/current/rps/appnote.22.html#classSpecifiers">
/// the RPS documentation</a>, which contains further details, \em interpolation
/// describes how the Primvar will be interpolated over the uv parameter
/// space of a surface primitive (or curve or pointcloud).  The possible
/// values are:
/// \li <b>constant</b> One value remains constant over the entire surface 
///     primitive.
/// \li <b>uniform</b> One value remains constant for each uv patch segment of 
///     the surface primitive (which is a \em face for meshes). 
/// \li <b>varying</b> Four values are interpolated over each uv patch segment 
///     of the surface. Bilinear interpolation is used for interpolation 
///     between the four values.
/// \li <b>vertex</b> Values are interpolated between each vertex in the 
///     surface primitive. The basis function of the surface is used for 
///     interpolation between vertices.
/// \li <b>faceVarying</b> For polygons and subdivision surfaces, four values 
///     are interpolated over each face of the mesh. Bilinear interpolation 
///     is used for interpolation between the four values.
///
/// \section Usd_Extending_UsdObject_Classes UsdGeomPrimvar As Example of Attribute Schema
///
/// Just as UsdSchemaBase and its subclasses provide the pattern for how to
/// layer schema onto the generic UsdPrim object, UsdGeomPrimvar provides an
/// example of how to layer schema onto a generic UsdAttribute object.  In both
/// cases, the schema object wraps and contains the UsdObject.
///
class UsdGeomPrimvar
{
    typedef const UsdAttribute UsdGeomPrimvar::*_UnspecifiedBoolType;

 public:
  
    // Default constructor returns an invalid Primvar.  Exists for 
    // container classes
    UsdGeomPrimvar()
    {
        /* NOTHING */
    }
    
    /// Speculative constructor that will produce a valid UsdGeomPrimvar when
    /// \p attr already represents an attribute that is Primvar, and
    /// produces an \em invalid Primvar otherwise (i.e. 
    /// \ref UsdGeomPrimvar_bool_type "unspecified-bool-type()" will return false).
    ///
    /// Calling \c UsdGeomPrimvar::IsPrimvar(attr) will return the same truth
    /// value as this constructor, but if you plan to subsequently use the
    /// Primvar anyways, just use this constructor, as demonstrated in the 
    /// \ref UsdGeomPrimvar_Using_Primvar "class documentation".
    USDGEOM_API
    explicit UsdGeomPrimvar(const UsdAttribute &attr);

    /// Return the Primvar's interpolation, which is 
    /// \ref Usd_InterpolationVals "UsdGeomTokens->constant" if unauthored
    ///
    /// Interpolation determines how the Primvar interpolates over
    /// a geometric primitive.  See \ref Usd_InterpolationVals
    USDGEOM_API
    TfToken GetInterpolation() const;

    /// Set the Primvar's interpolation.
    ///
    /// Errors and returns false if \p interpolation is out of range as
    /// defined by IsValidInterpolation().  No attempt is made to validate
    /// that the Primvar's value contains the right number of elements
    /// to match its interpolation to its topology.
    ///
    /// \sa GetInterpolation(), \ref Usd_InterpolationVals
    USDGEOM_API
    bool SetInterpolation(const TfToken &interpolation);
    
    /// Has interpolation been explicitly authored on this Primvar?
    ///
    /// \sa GetInterpolationSize()
    USDGEOM_API
    bool HasAuthoredInterpolation() const;

    /// Return the "element size" for this Primvar, which is 1 if
    /// unauthored.  If this Primvar's type is \em not an array type,
    /// (e.g. "Vec3f[]"), then elementSize is irrelevant.
    ///
    /// ElementSize dictates how many consecutive items in the value array
    /// should be taken as an atomic element to be interpolated over a gprim.
    /// For example, if one is encoding spherical harmonic coefficients in
    /// a Primvar, the typeName would be "float[]", and the elementSize
    /// would be 9.  Changing the elementSize without changing the interpolation
    /// will always necessitate a change to the size of the value array.
    USDGEOM_API
    int GetElementSize() const;
    
    /// Set the elementSize for this Primvar.
    ///
    /// Errors and returns false if \p eltSize less than 1.
    ///
    /// \sa GetElementSize()
    USDGEOM_API
    bool SetElementSize(int eltSize);
    
    /// Has elementSize been explicitly authored on this Primvar?
    ///
    /// \sa GetElementSize()
    USDGEOM_API
    bool HasAuthoredElementSize() const;
    

    /// Test whether a given UsdAttribute represents valid Primvar, which
    /// implies that creating a UsdGeomPrimvar from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    USDGEOM_API
    static bool IsPrimvar(const UsdAttribute &attr);

    /// Validate that the provided \p interpolation is a valid setting for 
    /// interpolation as defined by \ref Usd_InterpolationVals.  
    USDGEOM_API
    static bool IsValidInterpolation(const TfToken &interpolation);

    /// Convenience function for fetching all information required to 
    /// properly declare this Primvar.  The \p name returned is the
    /// "client name", stripped of all namespaces, i.e. equivalent to
    /// GetBaseName()
    ///
    /// May also be more efficient than querying key individually.
    USDGEOM_API
    void GetDeclarationInfo(TfToken *name, SdfValueTypeName *typeName,
                            TfToken *interpolation, int *elementSize) const;

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------
    
    /// Allow UsdGeomPrimvar to auto-convert to UsdAttribute, so you can
    /// pass a UsdGeomPrimvar to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute const& () const { return _attr; }

    /// Explicit UsdAttribute extractor
    UsdAttribute const &GetAttr() const { return _attr; }
    
    /// Return true if the wrapped UsdAttribute::IsDefined(), and in
    /// addition the attribute is identified as a Primvar.
    bool IsDefined() const { return IsPrimvar(_attr); }

    /// \anchor UsdGeomPrimvar_bool_type
    /// Return true if this Primvar is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
#ifdef doxygen
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsDefined() ? &UsdGeomPrimvar::_attr : 0;
    }
#endif // doxygen

    /// \sa UsdAttribute::GetName()
    TfToken const &GetName() const { return _attr.GetName(); }

    /// \sa UsdAttribute::GetBaseName()
    TfToken GetBaseName() const { return _attr.GetBaseName(); }

    /// \sa UsdAttribute::GetNamespace()
    TfToken GetNamespace() const { return _attr.GetNamespace(); }

    /// \sa UsdAttribute::SplitName()
    std::vector<std::string> SplitName() const { return _attr.SplitName(); };

    /// \sa UsdAttribute::GetTypeName()
    SdfValueTypeName GetTypeName() const { return _attr.GetTypeName(); }

    /// Get the attribute value of the Primvar at \p time .
    ///
    /// \sa Usd_Handling_Indexed_Primvars for proper handling of 
    /// \ref Usd_Handling_Indexed_Primvars "indexed primvars"
    template <typename T>
    bool Get(T* value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return _attr.Get(value, time);
    }

    /// Set the attribute value of the Primvar at \p time 
    template <typename T>
    bool Set(const T& value, UsdTimeCode time = UsdTimeCode::Default()) const {
        return _attr.Set(value, time);
    }

    // ---------------------------------------------------------------
    /// @{
    /// \anchor UsdGeomPrimvar_Indexed_primvars
    /// \name Indexed primvars API
    /// 
    /// For non-constant values of interpolation, it is often the case that the 
    /// same value is repeated many times in the array value of a primvar. An 
    /// indexed primvar can be used in such cases to optimize for data storage.
    /// For example, on faceVarying primvars over quad meshes this could 
    /// lead to roughly 4x compression ratio. Certain renderers can 
    /// be smart about it too and achieve similar memory savings in renders.
    /// Additionally, it provides a way of declaring topology, i.e. that the 
    /// faces meeting at a vertex identically share a value and therefore 
    /// form a continuous dataset, rather than (as prman does) trying to infer 
    /// that with dicey floating-point value comparison.
    /// 
    /// To create an indexed primvar, the value of the attribute associated with 
    /// the primvar is set to an array consisting of all the unique values that 
    /// appear in the primvar array. A separate namespaced "indices" attribute 
    /// is set to an integer array containing indices into the array with all 
    /// the unique elements. The final value of the primvar is computed using 
    /// the indices array and the attribute value array.
    /// 
    /// See also \ref Usd_Handling_Indexed_Primvars

    /// Sets the indices value of the indexed primvar at \p time.
    /// 
    /// The values in the indices array must be valid indices into the authored
    /// array returned by Get(). The element numerality of the primvar's 
    /// 'interpolation' metadata applies to the "indices" array, not the attribute
    /// value array (returned by Get()).
    USDGEOM_API
    bool SetIndices(const VtIntArray &indices, 
                    UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Returns the value of the indices array associated with the indexed 
    /// primvar at \p time.
    /// 
    /// \sa SetIndices(), \ref Usd_Handling_Indexed_Primvars
    USDGEOM_API
    bool GetIndices(VtIntArray *indices,
                    UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Block the indices that were previously set.  This effectively makes an
    /// indexed primvar no longer indexed.  This is useful when overriding an
    /// existing primvar.
    USDGEOM_API
    void BlockIndices() const;

    /// Returns true if the primvar is indexed, i.e., if it has an associated
    /// "indices" attribute.
    ///
    /// If you are going to query the indices anyways, prefer to simply 
    /// consult the return-value of GetIndices(), which will be more efficient.
    USDGEOM_API
    bool IsIndexed() const;


    /// Set the index that represents unauthored values in the indices array.
    /// 
    /// Some apps (like Maya) allow you to author primvars sparsely over a 
    /// surface. Since most apps can't handle sparse primvars, Maya needs to 
    /// provide a value even for the elements it didn't author. This metadatum 
    /// provides a way to recover the information in apps that do support 
    /// sparse authoring / representation of primvars. 
    /// 
    /// The fallback value of unauthoredValuesIndex is -1, which indicates that 
    /// there are no unauthored values.
    /// 
    /// \sa GetUnauthoredValuesIndex()
    USDGEOM_API
    bool SetUnauthoredValuesIndex(int unauthoredValuesIndex) const;

    /// Returns the index that represents unauthored values in the indices array.
    /// 
    /// \sa SetUnauthoredValuesIndex()
    USDGEOM_API
    int GetUnauthoredValuesIndex() const;
    
    /// Computes the flattened value of the primvar at \p time. 
    /// 
    /// If the primvar is not indexed or if the value type of this primvar is 
    /// a scalar, this returns the authored value, which is the same as 
    /// \ref Get(). Hence, it's safe to call ComputeFlattened() on non-indexed 
    /// primvars.
    template <typename ScalarType>
    bool ComputeFlattened(VtArray<ScalarType> * value, 
                          UsdTimeCode time = UsdTimeCode::Default()) const;

    /// \overload
    /// Computes the flattened value of the primvar at \p time as a VtValue. 
    /// 
    /// If the primvar is not indexed or if the value type of this primvar is 
    /// a scalar, this returns the authored value, which is the same as 
    /// \ref Get(). Hence, it's safe to call ComputeFlattened() on non-indexed 
    /// primvars.
    USDGEOM_API
    bool ComputeFlattened(VtValue *value, 
                          UsdTimeCode time=UsdTimeCode::Default()) const;

    /// @}

    // ---------------------------------------------------------------
    /// @{
    /// \anchor UsdGeomPrimvar_Id_primvars
    /// \name Id attribute API
    ///
    /// Often there is the need to identify a prim within a scene (e.g. a mesh
    /// on which a procedural should operate, or a shader to inherit).  A
    /// string or string[] -typed primvar can be turned into an "Id Path" primvar by
    /// calling SetIdTarget() with the path of any object on the current stage.  When
    /// the primvar is subsequently queried via Get(), the returned value will
    /// be the stringified value of the targeted object's path in whatever
    /// namespace is defined by the querying stage's root layer.  In other
    /// words, authoring an Id primvar into a published model will return the
    /// path-to-target in the model, but when the model is referenced into a
    /// larger scene, it will return the complete scene path.
    /// 
    /// This works by adding a paired UsdRelationship in a ":idFrom" namespace
    /// "below" the string primvar.  Get() evaluates
    /// UsdRelationship::GetForwardedTargets() to determine the id-string.
    /// Thus, this mechanism will always produce a unique identifier for every
    /// object in a scene, regardless of how many times an asset is referenced
    /// into a scene.  Providing a mesh with a unique identifier primvar can be
    /// especially useful for renderers that allow plugins/shaders access to
    /// processed scene data based on user-provided string identifiers.
    ///
    /// If an Id primvar has both an \em authored string value and a SetIdTarget()'d
    /// target, the target path takes precedence.
    /// 
    /// Currently Id primvars can have only a single target, so the only useful
    /// interpolation is constant.
    // ---------------------------------------------------------------
    
    /// Returns true if the primvar is an Id primvar.
    ///
    /// \sa \ref UsdGeomPrimvar_Id_primvars
    USDGEOM_API
    bool IsIdTarget() const;

    /// This primvar must be of String or StringArray type for this method to
    /// succeed.  If not, a coding error is raised.
    ///
    /// \sa \ref UsdGeomPrimvar_Id_primvars
    USDGEOM_API
    bool SetIdTarget(const SdfPath& path) const;
    
    /// @}
private:
    friend class UsdGeomImageable;
    
    /// Validate that the given \p name contains the primvars namespace.
    /// Does not validate name as a legal property identifier
    static bool _IsNamespaced(const TfToken& name);

    /// Return a Primvar name whose baseName is the baseName of 
    /// \p name, and is prepended with the proper primvars namespace.
    ///
    /// Does not validate name as a legal property identifier, but will
    /// verify that \p name contains no other namespaces, and will return
    /// an empty TfToken if it does. If \p quiet is true, the verification
    /// will be silent
    static TfToken _MakeNamespaced(const TfToken& name, bool quiet=false);

    static TfToken const &_GetNamespacePrefix();

    /// Factory for UsdGeomImageable's use, so that we can encapsulate the
    /// logic of what discriminates Primvar in this class, while
    /// preserving the pattern that attributes can only be created
    /// via their container objects.
    ///
    /// The name of the created attribute may or may not be the specified
    /// \p attrName, due to the possible need to apply property namespacing
    /// for Primvar.
    ///
    /// The behavior with respect to the provided \p typeName,
    /// and \p custom is the same as for UsdAttributes::Create().
    ///
    /// \return an invalid UsdGeomPrimvar if we failed to create a valid
    /// attribute, a valid UsdGeomPrimvar otherwise.  It is not an
    /// error to create over an existing, compatible attribute.
    ///
    /// It is a failed verification for \p prim to be invalid/expired
    ///
    /// \sa UsdPrim::CreateAttribute()
    UsdGeomPrimvar(const UsdPrim& prim, const TfToken& attrName,
                   const SdfValueTypeName &typeName, bool custom);

    UsdAttribute _attr;

    // upon construction, we'll take note of the attrType.  If we're a type
    // that could possibly have an Id associated with it, we'll store that name
    // so we don't have to pay the cost of constructing that token per-Get().
    void _SetIdTargetRelName();

    // Gets or creates the indices attribute corresponding to the primvar.
    UsdAttribute _GetIndicesAttr(bool create) const;

    // Helper method for computing the flattened value of an indexed primvar.
    template<typename ScalarType>
    static bool _ComputeFlattenedHelper(const VtArray<ScalarType> &authored,
                                  const VtIntArray &indices,
                                  VtArray<ScalarType> *value);
    
    // Helper function to evaluate the flattened array value of a primvar given
    // the attribute value and the indices array.
    template <typename ArrayType>
    static bool _ComputeFlattenedArray(const VtValue &attrVal,
                                        const VtIntArray &indices,
                                        VtValue *value);

    // Should only be called if _idTargetRelName is set
    UsdRelationship _GetIdTargetRel(bool create) const;
    TfToken _idTargetRelName;
};

// We instantiate the following so we can check and provide the correct value
// for Id attributes.
template <>
USDGEOM_API bool UsdGeomPrimvar::Get(std::string* value, UsdTimeCode time) const;

template <>
USDGEOM_API bool UsdGeomPrimvar::Get(VtStringArray* value, UsdTimeCode time) const;

template <>
USDGEOM_API bool UsdGeomPrimvar::Get(VtValue* value, UsdTimeCode time) const;

template <typename ScalarType>
bool 
UsdGeomPrimvar::ComputeFlattened(VtArray<ScalarType> *value, UsdTimeCode time) const
{
    VtArray<ScalarType> authored;
    if (not Get(&authored, time))
        return false;

    if (not IsIndexed()) {
        *value = authored;
        return true;
    }

    VtIntArray indices;
    if (not GetIndices(&indices, time)) {
        TF_WARN("No indices authored for indexed primvar <%s>.", 
                _attr.GetPath().GetText());
        return false;
    }

    // If the authored array is empty, there's nothing to do.
    if (authored.empty())
        return false;

    return _ComputeFlattenedHelper(authored, indices, value);
}

template<typename ScalarType>
/* static */
bool
UsdGeomPrimvar::_ComputeFlattenedHelper(const VtArray<ScalarType> &authored,
                                  const VtIntArray &indices,
                                  VtArray<ScalarType> *value)
{
    value->resize(indices.size());
    bool success = true;
    for(size_t i=0; i < indices.size(); i++) {
        int index = indices[i];
        if (index >= 0 and index < authored.size()) {
            (*value)[i] = authored[index];
        } else {
            TF_WARN("Index %d is out of range [0,%ld)", index, 
                authored.size());
            success = false;
        }
    }
    return success;
}

#endif // USD_PRIMVAR_H
