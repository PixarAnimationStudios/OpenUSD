//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_API_SCHEMA_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_API_SCHEMA_ADAPTER_H

/// \file usdImaging/apiSchemaAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/version.h"
#include "pxr/usdImaging/usdImaging/types.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/base/tf/type.h"

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingDataSourceStageGlobals;

using UsdImagingAPISchemaAdapterSharedPtr = 
    std::shared_ptr<class UsdImagingAPISchemaAdapter>;

/// \class UsdImagingAPISchemaAdapter
///
/// Base class for all API schema adapters.
///
/// These map behavior of applied API schemas to contributions the hydra prims
/// and data sources generated for a given USD prim.
class UsdImagingAPISchemaAdapter 
            : public std::enable_shared_from_this<UsdImagingAPISchemaAdapter>
{
public:

    USDIMAGING_API
    virtual ~UsdImagingAPISchemaAdapter();

    /// Called to determine whether an API schema defines additional child
    /// hydra prims beyond the primary hydra prim representing the USD prim
    /// on which the API schema is applied. The token values returned are
    /// appended (as property names) to the SdfPath which serves as the hydra
    /// id of the primary prim.
    /// \p appliedInstanceName will be non-empty for multiple-apply schema
    /// instance names.
    USDIMAGING_API
    virtual TfTokenVector GetImagingSubprims(
            UsdPrim const& prim,
            TfToken const& appliedInstanceName);

    /// Called to determine whether an API schema specifies the hydra type of
    /// a given prim previously defined by a call to GetImagingSubprims.
    /// \p subprim corresponds to an element in the result of a previous call
    /// to GetImagingSubprims.
    /// \p appliedInstanceName will be non-empty for multiple-apply schema
    /// instance names.
    USDIMAGING_API
    virtual TfToken GetImagingSubprimType(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName);

    /// Returns an HdContainerDataSourceHandle representing the API schema's
    /// contributions to the primary prim (empty \p subprim value) or a specific
    /// subprim. The non-null results of the prim adapter and each applied API
    /// schema adapter are overlaid (in application order).
    /// 
    /// Ideally, data sources within this container are lazily evaluated to
    /// avoid doing work until some consumes the data.
    USDIMAGING_API
    virtual HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

    /// Given the names of USD properties which have changed, an adapter may
    /// provide a HdDataSourceLocatorSet describing which data sources should
    /// be flagged as dirty.
    USDIMAGING_API
    virtual HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType);
};


class UsdImagingAPISchemaAdapterFactoryBase : public TfType::FactoryBase
{
public:
    virtual UsdImagingAPISchemaAdapterSharedPtr New() const = 0;
};

template <class T>
class UsdImagingAPISchemaAdapterFactory
    : public UsdImagingAPISchemaAdapterFactoryBase
{
public:
    virtual UsdImagingAPISchemaAdapterSharedPtr New() const
    {
        return std::make_shared<T>();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
