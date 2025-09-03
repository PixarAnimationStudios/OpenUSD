//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_PAGE_CACHE_EXECUTOR_H
#define PXR_EXEC_EF_PAGE_CACHE_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/pageCacheBasedExecutor.h"
#include "pxr/exec/ef/subExecutor.h"

#include "pxr/exec/vdf/executorFactory.h"
#include "pxr/exec/vdf/speculationExecutor.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfPageCacheExecutor
///
/// \brief Executes a VdfNetwork to compute a requested set of values. Caches
///        the computed data in a EfPageCacheStorage container and recalls
///        existing data using a page specified via the currently set value
///        on the key output.
///
///        This executor stores its data in the output-member data manager.
///
template <
    template <typename> class EngineType,
    typename DataManagerType>
class EfPageCacheExecutor :
    public EfPageCacheBasedExecutor<EngineType, DataManagerType>
{
    // Base type definition.
    typedef EfPageCacheBasedExecutor<EngineType, DataManagerType> Base;

    // The speculatino executor engine alias declaration, to be bound as a 
    // template template parameter.
    template <typename T>
    using SpeculationEngineType =
        typename EngineType<T>::SpeculationExecutorEngine;

    // Executor factory.
    typedef
        VdfExecutorFactory<
            EfSubExecutor<EngineType, DataManagerType>,
            VdfSpeculationExecutor<SpeculationEngineType, DataManagerType>>
        _Factory;

public:

    /// Constructor.
    ///
    explicit EfPageCacheExecutor(EfPageCacheStorage *cacheStorage) :
        Base(cacheStorage)
    {}

    /// Destructor.
    ///
    virtual ~EfPageCacheExecutor() {}

    /// Factory construction.
    ///
    virtual const VdfExecutorFactoryBase &GetFactory() const override final {
        return _factory;
    }

private:

    // Clear all data in the local data manager.
    //
    virtual void _ClearData();

    // The factory shared amongst executors of this type.
    //
    static const _Factory _factory;

};

template <template <typename> class EngineType, typename DataManagerType>
const typename EfPageCacheExecutor<EngineType, DataManagerType>::_Factory
    EfPageCacheExecutor<EngineType, DataManagerType>::_factory;

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheExecutor<EngineType, DataManagerType>::_ClearData()
{
    Base::_ClearData();
    Base::_dataManager.Clear();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
