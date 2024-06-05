//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDMEDIA_GENERATED_ASSETPREVIEWSAPI_H
#define USDMEDIA_GENERATED_ASSETPREVIEWSAPI_H

/// \file usdMedia/assetPreviewsAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdMedia/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdMedia/tokens.h"

#include "pxr/usd/sdf/types.h"
    

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// ASSETPREVIEWSAPI                                                           //
// -------------------------------------------------------------------------- //

/// \class UsdMediaAssetPreviewsAPI
///
/// AssetPreviewsAPI is the interface for authoring and accessing 
/// precomputed, lightweight previews of assets.  It is an applied schema, 
/// which means that an arbitrary number of prims on a stage can have the schema 
/// applied and therefore can contain previews; however, to access a stage's 
/// "default" previews, one consults 
/// the stage's `defaultPrim`.
/// 
/// AssetPreviewsAPI supports the following kinds of previews:
/// - **thumbnails** : a set of pre-rendered images of the asset.  There is no 
/// prescribed size for thumbnail images, but care should be taken to ensure
/// their inclusion does not substantially increase the overall size of an
/// asset, as, for example, when packaged into USDZ.
/// 
/// Although the UsdMediaAssetPreviewsAPI class can be used to interrogate any
/// prim, no query in the API will succeed unless the schema has been applied
/// to the prim.  This schema deals only with asset paths, and clients wishing
/// to directly consume the returned data must do so by retrieving an ArAsset
/// from the session's ArAssetResolver.
/// 
/// The schema defines no properties or metadata fallback values.  Rather, 
/// Asset Previews are encoded as part of a prim's `assetInfo` metadata.  A 
/// default thumbnail image would look like:
/// ```
/// 1.    assetInfo = {
/// 2.      dictionary previews = {
/// 3.          dictionary thumbnails = {
/// 4.              dictionary default = {
/// 5.                  asset defaultImage = @chair_thumb.jpg@
/// 6.              }
/// 7.          }
/// 8.      }
/// 9.    }
/// ```
/// 
/// 
///
class UsdMediaAssetPreviewsAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdMediaAssetPreviewsAPI on UsdPrim \p prim .
    /// Equivalent to UsdMediaAssetPreviewsAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdMediaAssetPreviewsAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdMediaAssetPreviewsAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdMediaAssetPreviewsAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdMediaAssetPreviewsAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDMEDIA_API
    virtual ~UsdMediaAssetPreviewsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDMEDIA_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdMediaAssetPreviewsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdMediaAssetPreviewsAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDMEDIA_API
    static UsdMediaAssetPreviewsAPI
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
    USDMEDIA_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "AssetPreviewsAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdMediaAssetPreviewsAPI object is returned upon success. 
    /// An invalid (or empty) UsdMediaAssetPreviewsAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDMEDIA_API
    static UsdMediaAssetPreviewsAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDMEDIA_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDMEDIA_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDMEDIA_API
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
    
    /// \struct Thumbnails
    ///
    /// Thumbnails is a value type that serves as schema to aid in serialization
    /// and deserialization of thumbnail images in the assetInfo["thumbnails"]
    /// dictionary
    struct Thumbnails
    {
        USDMEDIA_API
        explicit Thumbnails(const SdfAssetPath &defaultImage=SdfAssetPath());

        SdfAssetPath   defaultImage;
    };
    
    /// Fetch the default Thumbnails data, returning `true` if data was
    /// successfully fetched.
    USDMEDIA_API
    bool GetDefaultThumbnails(Thumbnails *defaultThumbnails) const;

    /// Author the default thumbnails dictionary from the provided Thumbnails
    /// data
    USDMEDIA_API
    void SetDefaultThumbnails(const Thumbnails &defaultThumbnails) const;
    
    /// Remove the entire entry for default Thumbnails in the current
    /// UsdEditTarget
    USDMEDIA_API
    void ClearDefaultThumbnails() const;

    /// Return a schema object that can be used to interrogate previews
    /// for the default prim of the stage constructed from `layerPath`.
    ///
    /// The schema object will create and retain a minimal stage required for
    /// interrogation.  This is equivalent to:
    /// `GetAssetDefaultPreviews(SdfLayer::FindOrOpen(layerPath))`
    USDMEDIA_API
    static UsdMediaAssetPreviewsAPI 
    GetAssetDefaultPreviews(const std::string &layerPath);
    
    /// \overload
    USDMEDIA_API
    static UsdMediaAssetPreviewsAPI 
    GetAssetDefaultPreviews(const SdfLayerHandle &layer);

private:
    UsdStageRefPtr _defaultMaskedStage;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
