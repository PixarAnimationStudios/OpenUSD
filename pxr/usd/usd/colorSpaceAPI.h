//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USD_GENERATED_COLORSPACEAPI_H
#define USD_GENERATED_COLORSPACEAPI_H

/// \file usd/colorSpaceAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/tf/bigRWMutex.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// COLORSPACEAPI                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdColorSpaceAPI
///
/// UsdColorSpaceAPI is an API schema that introduces a `colorSpace`
/// property for authoring scene referred color space opinions. It also provides
/// a mechanism to determine the applicable color space within a scope through
/// inheritance. Accordingly, this schema may be applied to any prim to 
/// introduce a color space at any point in a compositional hierarchy.
/// 
/// Color space resolution involves determining the color space authored on an
/// attribute by first examining the attribute itself for a color space which
/// may have been authored via `UsdAttribute::SetColorSpace()`. If none is found,
/// the attribute's prim is checked for the existence of the `UsdColorSpaceAPI`,
/// and any color space authored there. If none is found on the attribute's
/// prim, the prim's ancestors are examined up the hierarchy until an authored
/// color space is found. If no color space is found, an empty `TfToken` is
/// returned. When no color space is found, the default color space is linear, 
/// with Rec709 primaries and D65 white point, corresponding to the GfColorSpace
/// token `LinearRec709`.
/// 
/// For a list of built in color space token values, see `GfColorSpaceNames`.
/// 
/// Use a pattern like this when determining an attribute's resolved color space:
/// 
/// ```
/// TfToken attrCs = attr.GetColorSpace();
/// if (!attrCs.IsEmpty()) { 
/// return attrCs; 
/// }
/// auto csAPI = UsdColorSpaceAPI(attr.GetPrim());
/// return UsdColorSpaceAPI::ComputeColorSpaceName(attr);
/// ```
/// 
/// `GfColorSpace` and its associated utilities can be used to perform color 
/// transformations; some examples:
/// 
/// ```
/// srcSpace = GfColorSpace(ComputeColorSpaceName(attr))
/// targetSpace = GfColorSpace(targetSpaceName)
/// targetColor = srcSpace.Convert(targetSpace, srcColor)
/// srcSpace.ConvertRGBSpan(targetSpace, colorSpan)
/// ```
/// 
/// It is recommended that in situations where performance is a concern, an 
/// application should perform conversions infrequently and cache results
/// wherever possible.
/// 
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdTokens.
/// So to set an attribute to the value "rightHanded", use UsdTokens->rightHanded
/// as the value.
///
class UsdColorSpaceAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdColorSpaceAPI on UsdPrim \p prim .
    /// Equivalent to UsdColorSpaceAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdColorSpaceAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdColorSpaceAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdColorSpaceAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdColorSpaceAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USD_API
    virtual ~UsdColorSpaceAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USD_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdColorSpaceAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdColorSpaceAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USD_API
    static UsdColorSpaceAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Returns true if this <b>single-apply</b> API schema can be applied to 
    /// the given \p prim. If this schema can not be a applied to the prim, 
    /// this returns false and, if provided, populates \p whyNot with the 
    /// reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USD_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ColorSpaceAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdColorSpaceAPI object is returned upon success. 
    /// An invalid (or empty) UsdColorSpaceAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USD_API
    static UsdColorSpaceAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USD_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USD_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USD_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // COLORSPACENAME 
    // --------------------------------------------------------------------- //
    /// The color space that applies to attributes with
    /// unauthored color spaces on this prim and its descendents.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `uniform token colorSpace:name` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref SdfVariability "Variability" | SdfVariabilityUniform |
    USD_API
    UsdAttribute GetColorSpaceNameAttr() const;

    /// See GetColorSpaceNameAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USD_API
    UsdAttribute CreateColorSpaceNameAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

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

    /// A minimalistic cache for color space lookups.
    /// An application may provide its own cache implementation to avoid
    /// redundant color space lookups. The cache should be cleared or updated
    /// when the color space properties have changed. A typical use pattern
    /// would be to create a temporary cache at the beginning of a traversal, 
    /// and pass it to the color space API methods.
    class ColorSpaceCache {
    public:
        ColorSpaceCache() = default;
        virtual ~ColorSpaceCache() = default;

        virtual TfToken Find(const SdfPath& prim) = 0;
        virtual void Insert(const SdfPath& prim, const TfToken& colorSpace) = 0;
    };

    /// This is a simple example of a color space cache implementation.
    /// An application can use this cache to avoid redundant color space lookups,
    /// but may substitute its own cache implementation if desired. This 
    /// implementation does not track changes to the color space properties, so
    /// it is the application's responsibility to clear or update the cache when
    /// necessary.
    ///
    /// A more sophisticated cache implementation might also  be intelligent
    /// about caching locally defined color spaces and their computed
    /// GfColorSpaces. It might also be aware of changes to the scene hierarchy
    ///  and update the cache accordingly.
    ///
    /// As it is, this simple cache will be efficient in a typical scene using
    /// built in color spaces. It uses `TfBigRWMutex` to enable many-reader
    /// single-writer thread safety; however this example does not provide a
    /// guarantee of thread-efficiency for any given cache access pattern.
    class ColorSpaceHashCache : public ColorSpaceCache {
    protected:
        TfHashMap<SdfPath, TfToken, SdfPath::Hash> _cache;
        mutable TfBigRWMutex _mutex;
    public:
        ColorSpaceHashCache() = default;
        ~ColorSpaceHashCache() = default;

        TfToken Find(const SdfPath& prim) override {
            TfBigRWMutex::ScopedLock lock(_mutex, /*write=*/false);
            auto it = _cache.find(prim);
            if (it != _cache.end()) {
                return it->second;
            }
            return TfToken();
        }

        void Insert(const SdfPath& prim, const TfToken& colorSpace) override {
            TfBigRWMutex::ScopedLock lock(_mutex);
            _cache[prim] = colorSpace;
        }
    };

    /// Computes the color space name for the given attribute.
    /// The attribute is first checked for an authored color space; if one
    /// exists, it's returned. Otherwise, the attribute's prim is consulted,
    /// following the inheritance rules for color space determination on a prim.
    /// If one is found, it's returned. Otherwise, the value on the attribute's
    /// prim definition is returned if there is one. Otherwise, an empty TfToken
    /// is returned.
    ///
    /// This function may be considered a reference implementation for
    /// determining the color space of an attribute. Since the algorithm is
    /// implemented as an exhaustive search performed through the prim 
    /// hierarchy, applications may want to implement a caching mechanism to
    /// avoid redundant searches.
    ///
    /// \sa UsdColorSpaceAPI::Apply
    /// \sa UsdColorSpaceAPI::GetColorSpaceAttr
    /// \param attribute The attribute to compute the color space for.
    /// \param cache An optional cache for accelerating color space lookups.
    USD_API
    static TfToken ComputeColorSpaceName(const UsdAttribute& attribute,
		                                 ColorSpaceCache* cache = nullptr);

    /// Computes the color space for the given attribute on this prim, using 
    /// the same algorithm as `ComputeColorSpaceName`. The same performance
    /// caveat applies.
    /// \param attribute The attribute to compute the color space for.
    /// \param cache A cache object for accelerating color space lookups.
    USD_API
    static GfColorSpace ComputeColorSpace(const UsdAttribute& attribute,
		                                  ColorSpaceCache* cache = nullptr);

    /// Computes the color space name for the given prim.
    /// The color space is determined by checking this prim for a
    /// `colorSpace` property. If no `colorSpace `property is authored,
    /// the search continues up the prim's hierarchy until a `colorSpace`
    /// property is found or the root prim is reached. If no `colorSpace` 
    /// property is found, an empty `TfToken` is returned.
    ///
    /// If a `colorSpace` name is found, but does not match one of the standard 
    /// color spaces or a user defined color space, an empty `TfToken` is 
    /// returned.
    ///
    /// This function should be considered as a reference implementation, and
    /// applications may want to implement a caching mechanism for performance.
    ///
    /// \sa UsdColorSpaceAPI::Apply
    /// \sa UsdColorSpaceAPI::GetColorSpaceAttr
    /// \param prim The prim to compute the color space for.
    /// \param cache A cache object for accelerating color space lookups.
    USD_API
    static TfToken ComputeColorSpaceName(UsdPrim prim,
                                         ColorSpaceCache* cache = nullptr);

    /// Creates a color space object for the named color space if it built in,
    /// defined on the prim or on an ancestor.
    /// \param prim The prim from which a search for a defined color space begins.
    /// \param colorSpace The name of the color space.
    /// \param cache A cache object for accelerating color space lookups.
    USD_API
    static GfColorSpace ComputeColorSpace(UsdPrim prim,
                                          const TfToken& colorSpace,
                                          ColorSpaceCache* cache = nullptr);

    /// Computes the color space for this prim, using the same algorithm as
    /// `ComputeColorSpaceName()`. The same performance caveat applies.
    /// \param prim The prim to check for the color space.
    /// \param cache A cache object for accelerating color space lookups.
    USD_API
    static GfColorSpace ComputeColorSpace(UsdPrim prim,
                                          ColorSpaceCache* cache = nullptr);

    /// Returns true if the named color space is built in, defined on the
    /// supplied prim, or on one of the prim's ancestors.
    /// \param prim The prim from which a search for a defined color space begins.
    /// \param colorSpace The name of the color space to verify.
    /// \param cache A cache object for accelerating color space lookups.
    USD_API
    static bool IsValidColorSpaceName(UsdPrim prim, 
                                      const TfToken& colorSpace,
                                      ColorSpaceCache* cache = nullptr);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
