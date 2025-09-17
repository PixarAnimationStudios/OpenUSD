//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_PAGE_CACHE_SUB_EXECUTOR_H
#define PXR_EXEC_EF_PAGE_CACHE_SUB_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/pageCacheBasedExecutor.h"
#include "pxr/exec/ef/subExecutor.h"

#include "pxr/exec/vdf/executorFactory.h"
#include "pxr/exec/vdf/speculationExecutor.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfPageCacheSubExecutor
///
/// \brief Executes a VdfNetwork to compute a requested set of values. Caches
///        the computed data in an EfPageCacheStorage container and recalls
///        existing data using a page specified via the currently set value
///        on the key output.
///
///        Contrary to the EfPageCacheExecutor, this executor stores its data
///        in the hash-table data manager and supports looking up output values
///        on a parent executor.
///
template <
    template <typename> class EngineType,
    typename DataManagerType>
class EfPageCacheSubExecutor :
    public EfPageCacheBasedExecutor<EngineType, DataManagerType>
{
    // Base type definition.
    typedef EfPageCacheBasedExecutor<EngineType, DataManagerType> Base;

    // The speculation executor engine alias declaration, to be bound as a 
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
    explicit EfPageCacheSubExecutor(EfPageCacheStorage *cacheStorage) :
        Base(cacheStorage)
    {}

    /// Construct with parent executor.
    ///
    EfPageCacheSubExecutor(
        EfPageCacheStorage *cacheStorage,
        const VdfExecutorInterface *parentExecutor);

    /// Destructor.
    ///
    virtual ~EfPageCacheSubExecutor() {}

    /// Factory construction.
    ///
    virtual const VdfExecutorFactoryBase &GetFactory() const override final {
        return _factory;
    }

private:

    // Returns a value for the cache that flows across \p connection.
    //
    virtual const VdfVector *_GetInputValue(
        const VdfConnection &connection,
        const VdfMask &mask) const override;

    // Returns an output value for reading.
    //
    virtual const VdfVector *_GetOutputValueForReading(
        const VdfOutput &output,
        const VdfMask &mask) const override;

    // Get an output value from the parent executor.
    //
    const VdfVector *_GetParentExecutorValue(
        const VdfOutput &output,
        const VdfMask &mask) const;

    // Clear all data in the local data manager.
    //
    virtual void _ClearData();

    // The factory shared amongst executors of this type.
    //
    static const _Factory _factory;

};

///////////////////////////////////////////////////////////////////////////////

template <template <typename> class EngineType, typename DataManagerType>
const typename EfPageCacheSubExecutor<EngineType, DataManagerType>::_Factory
    EfPageCacheSubExecutor<EngineType, DataManagerType>::_factory;

template <template <typename> class EngineType, typename DataManagerType>
EfPageCacheSubExecutor<EngineType, DataManagerType>::EfPageCacheSubExecutor(
    EfPageCacheStorage *cacheStorage,
    const VdfExecutorInterface *parentExecutor) :
    Base(cacheStorage)
{
    // Set the parent executor
    Base::SetParentExecutor(parentExecutor);
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
const VdfVector *
EfPageCacheSubExecutor<EngineType, DataManagerType>::_GetInputValue(
    const VdfConnection &connection,
    const VdfMask &mask) const
{
    // Lookup the output value in the local data manager and page cache, first!
    if (const VdfVector *value =
            Base::_GetInputValue(connection, mask)) {
        return value;
    }

    // If available, also check for the value in the parent executor.
    return _GetParentExecutorValue(connection.GetSourceOutput(), mask);
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
const VdfVector *
EfPageCacheSubExecutor<EngineType, DataManagerType>::_GetOutputValueForReading(
    const VdfOutput &output,
    const VdfMask &mask) const
{
    // Lookup the output value in the local data manager and page cache, first!
    if (const VdfVector *value =
            Base::_GetOutputValueForReading(output, mask)) {
        return value;
    }

    // If available, also check for the value in the parent executor.
    return _GetParentExecutorValue(output, mask);
}

template <template <typename> class EngineType, typename DataManagerType>
const VdfVector *
EfPageCacheSubExecutor<EngineType, DataManagerType>::_GetParentExecutorValue(
    const VdfOutput &output,
    const VdfMask &mask) const
{
    const VdfExecutorInterface *parentExecutor = Base::GetParentExecutor();
    return parentExecutor
        ? parentExecutor->GetOutputValue(output, mask)
        : nullptr;
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheSubExecutor<EngineType, DataManagerType>::_ClearData()
{
    // Clear all the relevant data from the parent class.
    Base::_ClearData();

    // If the data manager remains empty we can bail out.
    if (!Base::_dataManager.IsEmpty()) {
        Base::_dataManager.Clear();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
