//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DATASOURCE_H
#define PXR_IMAGING_HD_DATASOURCE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/dataSourceLocator.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <iosfwd>
#include <memory>
#include <vector>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// HD_DECLARE_DATASOURCE_ABSTRACT
/// Used for non-instantiable classes, this defines a set of functions
/// for manipulating handles to this type of datasource.
#define HD_DECLARE_DATASOURCE_ABSTRACT(type) \
    using Handle =  std::shared_ptr<type>; \
    using AtomicHandle = Handle; \
    static Handle AtomicLoad(AtomicHandle &ptr) { \
        return std::atomic_load(&ptr); \
    } \
    static void AtomicStore(AtomicHandle &ptr, const Handle &v) { \
        std::atomic_store(&ptr, v); \
    } \
    static bool AtomicCompareExchange(AtomicHandle &ptr, \
                                      AtomicHandle &expected, \
                                      const Handle &desired) { \
        return std::atomic_compare_exchange_strong(&ptr, &expected, desired); \
    } \
    static Handle Cast(const HdDataSourceBase::Handle &v) { \
        return std::dynamic_pointer_cast<type>(v); \
    }

/// HD_DECLARE_DATASOURCE
/// Used for instantiable classes, this defines functions for manipulating
/// and allocating handles to this type of datasource.
/// 
/// Use of this macro in derived classes is important to make sure that
/// core and client code share the same handle type and allocator.
#define HD_DECLARE_DATASOURCE(type) \
    HD_DECLARE_DATASOURCE_ABSTRACT(type) \
    template <typename ... Args> \
    static Handle New(Args&& ... args) { \
        return Handle(new type(std::forward<Args>(args) ... )); \
    }

/// HD_DECLARE_DATASOURCE_INITIALIZER_LIST_NEW
/// Used for declaring a `New` function for datasource types that have a
/// constructor that takes an initializer_list<T>.
#define HD_DECLARE_DATASOURCE_INITIALIZER_LIST_NEW(type, T) \
    static Handle New(std::initializer_list<T> initList) { \
        return Handle(new type(initList)); \
    }

#define HD_DECLARE_DATASOURCE_HANDLES(type) \
    using type##Handle = type::Handle; \
    using type##AtomicHandle = type::AtomicHandle;

/// \class HdDataSourceBase
///
/// Represents an object which can produce scene data.
/// \sa HdContainerDataSource HdVectorDataSource HdSampledDataSource
/// Note that most derived classes will have standard API for allocation 
/// and handle manipulation. Derived classes that don't support instantiation 
/// should use HD_DECLARE_DATASOURCE_ABSTRACT, which omits the 
/// definition of ::New().
///
class HdDataSourceBase
{
public:
    HD_DECLARE_DATASOURCE_ABSTRACT(HdDataSourceBase)
    
    HD_API 
    virtual ~HdDataSourceBase() = 0;
};

HD_DECLARE_DATASOURCE_HANDLES(HdDataSourceBase);

/// \class HdContainerDataSource
///
/// A datasource representing structured (named, hierarchical) data, for
/// example a geometric primitive or a sub-object like a material definition.
/// Note that implementations are responsible for providing cache invalidation,
/// if necessary.
///
class HdContainerDataSource : public HdDataSourceBase
{
public:
    HD_DECLARE_DATASOURCE_ABSTRACT(HdContainerDataSource);

    /// Returns the list of names for which \p Get(...) is expected to return
    /// a non-null value. This call is expected to be threadsafe.
    virtual TfTokenVector GetNames() = 0;

    /// Returns the child datasource of the given name. This call is expected
    /// to be threadsafe.
    virtual HdDataSourceBaseHandle Get(const TfToken &name) = 0;

    /// A convenience function: given \p container, return the descendant
    /// identified by \p locator, which may be at any depth. Returns
    /// \p container itself on an empty locator, or null if \p locator doesn't
    /// identify a valid descendant.
    HD_API 
    static HdDataSourceBaseHandle Get(
        const Handle &container,
        const HdDataSourceLocator &locator);
};

HD_DECLARE_DATASOURCE_HANDLES(HdContainerDataSource);

/// \class HdVectorDataSource
///
/// A datasource representing indexed data. This should be used when a scene
/// index is expected to manipulate the indexing; for array-valued data, a
/// \p HdSampledDataSource can be used instead. Note that implementations are
/// responsible for providing cache invalidation, if necessary.
///
class HdVectorDataSource : public HdDataSourceBase
{ 
public:
    HD_DECLARE_DATASOURCE_ABSTRACT(HdVectorDataSource);

    /// Return the number of elements in this datasource. This call is
    /// expected to be threadsafe.
    virtual size_t GetNumElements() = 0;

    /// Return the element at position \p element in this datasource. This
    /// is expected to return non-null for the range [0, \p numElements).
    /// This call is expected to be threadsafe.
    virtual HdDataSourceBaseHandle GetElement(size_t element) = 0;
};

HD_DECLARE_DATASOURCE_HANDLES(HdVectorDataSource);

/// \class HdSampledDataSource
///
/// A datasource representing time-sampled values. Note that implementations
/// are responsible for providing cache invalidation, if necessary.
///
class HdSampledDataSource : public HdDataSourceBase
{
public:
    HD_DECLARE_DATASOURCE_ABSTRACT(HdSampledDataSource);
    using Time = float;

    /// Returns the value of this data source at frame-relative time
    /// \p shutterOffset. The caller does not track the frame; the scene
    /// index producing this datasource is responsible for that, if applicable.
    /// Note that, although this call returns a VtValue for each shutter
    /// offset, the type of the held value is expected to be the same across
    /// all shutter offsets. This call is expected to be threadsafe.
    virtual VtValue GetValue(Time shutterOffset) = 0;

    /// Given a shutter window of interest (\p startTime and \p endTime
    /// relative to the current frame), return a list of sample times for the
    /// caller to query with GetValue such that the caller can reconstruct the
    /// signal over the shutter window. For a sample-based attribute, this
    /// might be a list of times when samples are defined. For a procedural
    /// scene, this might be a generated distribution. Note that the returned
    /// samples don't need to be within \p startTime and \p endTime; if
    /// a boundary sample is outside of the window, implementers can return it,
    /// and callers should expect it and interpolate to \p startTime or
    /// \p endTime accordingly. If this call returns \p true, the caller is
    /// expected to pass the list of \p outSampleTimes to \p GetValue. If this
    /// call returns \p false, this value is uniform across the shutter window
    /// and the caller should call \p GetValue(0) to get that uniform value.
    virtual bool GetContributingSampleTimesForInterval(
        Time startTime, 
        Time endTime,
        std::vector<Time> * outSampleTimes) = 0;
};

HD_DECLARE_DATASOURCE_HANDLES(HdSampledDataSource);

/// \class HdTypedSampledDataSource
///
/// A datasource representing a concretely-typed sampled value.
///
template <typename T>
class HdTypedSampledDataSource : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE_ABSTRACT(HdTypedSampledDataSource<T>);
    using Type = T;

    /// Returns the value of this data source at frame-relative time
    /// \p shutterOffset, as type \p T.
    virtual T GetTypedValue(Time shutterOffset) = 0;
};


/// \class HdBlockDataSource
///
/// A datasource representing the absence of a datasource. If a container has
/// a child datasource which is a block datasource, that's equivalent to that
/// child being null. This type is useful when composing containers, where a
/// block might shadow sampled data, and sampled data might shadow nullptr.
///
class HdBlockDataSource : public HdDataSourceBase
{
public:
    HD_DECLARE_DATASOURCE(HdBlockDataSource);

    HdBlockDataSource(){}
};

HD_DECLARE_DATASOURCE_HANDLES(HdBlockDataSource);

// Utilities //////////////////////////////////////////////////////////////////

/// Merges contributing sample times from several data sources.
HD_API
bool
HdGetMergedContributingSampleTimesForInterval(
    size_t count,
    const HdSampledDataSourceHandle *inputDataSources,
    HdSampledDataSource::Time startTime,
    HdSampledDataSource::Time endTime,
    std::vector<HdSampledDataSource::Time> * outSampleTimes);

/// Print a datasource to a stream, for debugging/testing.
HD_API
void
HdDebugPrintDataSource(
    std::ostream &,
    HdDataSourceBaseHandle,
    int indentLevel = 0);

/// Print a datasource to stdout, for debugging/testing
HD_API
void
HdDebugPrintDataSource(HdDataSourceBaseHandle, int indentLevel = 0);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_DATASOURCE_H
