//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDTEXT_GENERATED_COLUMNSTYLEAPI_H
#define USDTEXT_GENERATED_COLUMNSTYLEAPI_H

/// \file usdText/columnStyleAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdText/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdText/tokens.h"

#include "pxr/usd/usdText/columnStyle.h"
#include <tbb/concurrent_unordered_map.h>
        

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// COLUMNSTYLEAPI                                                             //
// -------------------------------------------------------------------------- //

/// \class UsdTextColumnStyleAPI
///
/// "UsdTextColumnStyleAPI is an API schema that provides an interface for binding column style to a text 
/// primitive.
///
class UsdTextColumnStyleAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdTextColumnStyleAPI on UsdPrim \p prim .
    /// Equivalent to UsdTextColumnStyleAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdTextColumnStyleAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdTextColumnStyleAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdTextColumnStyleAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdTextColumnStyleAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDTEXT_API
    virtual ~UsdTextColumnStyleAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDTEXT_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdTextColumnStyleAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdTextColumnStyleAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDTEXT_API
    static UsdTextColumnStyleAPI
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
    USDTEXT_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "ColumnStyleAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdTextColumnStyleAPI object is returned upon success. 
    /// An invalid (or empty) UsdTextColumnStyleAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDTEXT_API
    static UsdTextColumnStyleAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDTEXT_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDTEXT_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDTEXT_API
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
    using ColumnStyleBindingCache =
        tbb::concurrent_unordered_multimap<SdfPath, SdfPath, SdfPath::Hash>;

    USDTEXT_API
    UsdRelationship GetBindingRel() const;

    /// \class ColumnStyleBinding
    /// This class represents binding to a columnStyle.
    class ColumnStyleBinding {
    public:
        /// Default constructor initializes a columnstyleBinding object with 
        /// invalid style and bindingRel data members.
        ColumnStyleBinding()
        {}

        /// Explicit constructor.
        USDTEXT_API
        explicit ColumnStyleBinding(const UsdRelationship &bindingRel,
                SdfPath const& textPrimPath);

        /// Gets the columnstyle prim that this binding binds to.
        USDTEXT_API
        std::vector<UsdTextColumnStyle> GetColumnStyles() const;

        /// Returns the path to the columnstyle that is bound to by this 
        /// binding.
        const std::vector<SdfPath> &GetColumnStylePaths() const {
            return _columnStylePaths;
        }

        /// Returns the binding-relationship that represents this binding.
        const UsdRelationship &GetBindingRel() const {
            return _bindingRel;
        }

    private:
        // The path to the columnstyle that is bound to.
        std::vector<SdfPath> _columnStylePaths;

        // The binding relationship.
        UsdRelationship _bindingRel;
    };

    /// Get the binding.
    USDTEXT_API
    ColumnStyleBinding GetColumnStyleBinding(SdfPath const& prim) const;

    /// Bind column styles.
    USDTEXT_API
    bool Bind(const std::vector<UsdTextColumnStyle> columnStyles) const;

    /// Test whether a given \p name contains the "columnStyle:" prefix
    ///
    USDTEXT_API
    static bool CanContainPropertyName(const TfToken &name);

    /// Add a binding to a columnstyle and the text prim to the cache.
    USDTEXT_API
    static bool AddBindToCache(SdfPath const& columnStylePrimPath,
            SdfPath const& textPrimPath)
    {
        _styleBindingCache.emplace(columnStylePrimPath, textPrimPath);
        return true;
    }

    /// Find the text prims who have the binding to the specified columnstyle.
    USDTEXT_API
    static bool FindBindedText(SdfPath const& columnStylePrimPath,
            std::pair<ColumnStyleBindingCache::iterator,
            ColumnStyleBindingCache::iterator>& pathPair)
    {
        pathPair = _styleBindingCache.equal_range(columnStylePrimPath);
        if (pathPair.first != _styleBindingCache.end())
            return true;
        else
            return false;
    }

private:
    // A cache that save the map between columnstyle and its binded text prim.
    static ColumnStyleBindingCache _styleBindingCache;

    UsdRelationship _CreateBindingRel() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
