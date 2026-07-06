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
#include "pxr/base/vt/visitValue.h"

#include <iosfwd>
#include <memory>
#include <vector>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

/// HdDataSourceAllocator
///
/// HdDataSourceAllocator simply wraps std::allocator. It exists to allow
/// std::allocate_shared to access the private constructor, by being
/// declared friend.
template <typename U>
struct HdDataSourceAllocator
{
    using value_type = U;
    template <typename V>
    struct rebind { using other = HdDataSourceAllocator<V>; };
    HdDataSourceAllocator() = default;
    template <typename V>
    HdDataSourceAllocator(const HdDataSourceAllocator<V>&) noexcept { }
    U* allocate(size_t n) { return std::allocator<U>{ }.allocate(n); }
    void deallocate(U* p, size_t n) noexcept
    { std::allocator<U>{ }.deallocate(p, n); }
    template <typename... Args>
    void construct(U*p, Args&&... args)
    { ::new(static_cast<void*>(p)) U(std::forward<Args>(args)...); }
    void destroy(U* p) noexcept { p->~U(); }
};

// XXX: clang is strict about qualified friend declarations, and will not
// forward PXR_NS -> PXR_INTERNAL_NS when present. But PXR_INTERNAL_NS is not
// always defined in all builds, so we can't just use that, either.
#if defined(PXR_INTERNAL_NS)
#define _HD_ALLOCATOR_FRIEND \
    template <typename> friend struct PXR_INTERNAL_NS::HdDataSourceAllocator;
#else
#define _HD_ALLOCATOR_FRIEND \
    template <typename> friend struct PXR_NS::HdDataSourceAllocator;
#endif

/// HD_DECLARE_DATASOURCE_ABSTRACT
/// Used for non-instantiable classes, this defines a set of functions
/// for manipulating handles to this type of datasource. This macro should
/// only be used for data source types that are not instantiable! Use
/// HD_DECLARE_DATASOURCE for all instantiable types, including
/// template classes!
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
/// core and client code share the same handle type and allocator. Use this
/// macro for all instantiable data source types, including template classes!
#define HD_DECLARE_DATASOURCE(type) \
    HD_DECLARE_DATASOURCE_ABSTRACT(type) \
    _HD_ALLOCATOR_FRIEND \
    template <typename ... Args> \
    static Handle New(Args&& ... args) { \
        return std::allocate_shared<type>( \
            PXR_NS::HdDataSourceAllocator<type>{ }, \
            std::forward<Args>(args)...); \
    }

/// HD_DECLARE_DATASOURCE_INITIALIZER_LIST_NEW
/// Used for declaring a `New` function for datasource types that have a
/// constructor that takes an initializer_list<...>. This macro allows using
/// braced-initialization of the initializer_list<...> like ::New({foo, bar}),
/// since the standard variadic ::New above cannot resolve such calls.
/// This macro should only be used after HD_DECLARE_DATASOURCE, never before or
/// without it, and only on data sources that have a constructor that takes
/// initializer_list<>
#define HD_DECLARE_DATASOURCE_INITIALIZER_LIST_NEW(type, T) \
    static Handle New(std::initializer_list<T> initList) { \
        return std::allocate_shared<type>( \
            PXR_NS::HdDataSourceAllocator<type>{ }, \
            (initList)); \
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

protected:
    friend class Hd_SampledDataSourceDefaultValueAccessor;

    HD_API
    virtual VtValue _GetDefaultValue();
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

protected:
    virtual VtValue _GetDefaultValue() override {
        if constexpr (VtIsKnownValueType<T>()) {
            return VtValue(T());
        } else {
            return VtValue();
        }
    }
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
protected:
    HdBlockDataSource() = default;
};

HD_DECLARE_DATASOURCE_HANDLES(HdBlockDataSource);

// Utilities //////////////////////////////////////////////////////////////////

// Helper to let HdVisitSampledDataSourceType access a source's value type.
class Hd_SampledDataSourceDefaultValueAccessor
{
public:
    static VtValue
    GetDefaultValue(const HdSampledDataSourceHandle &dataSource) {
        return dataSource ? dataSource->_GetDefaultValue() : VtValue {};
    }
};

/// Helper function to determine the type of a TypeSampledDataSource and perform
/// some operations.
/// Takes a "SampledDataSource", a "Visitor" class (which has a static "Visit"
/// function) and some arguments. The data source's type "T" will be determined
/// and Visitor<T>::Visit will be called with your arguments. If the data
/// source's type cannot be determined or it is an untyped SampledDataSource,
/// Visitor<VtValue>::Visit will be called.
///
/// This is identical to `VtVisitValueType<Visitor, TypeArgs...>(value,
/// args...)` except that instead of a VtValue `value`, the value type is
/// determined by the passed `dataSource`.
template <
    template <class T, class ...> class Visitor,
    typename ...TypeArgs,
    typename ...FnArgs
    >
auto
HdVisitSampledDataSourceType(
    const HdSampledDataSourceHandle& dataSource, FnArgs&&... args)
{
    return VtVisitValueType<Visitor, TypeArgs...>(
        Hd_SampledDataSourceDefaultValueAccessor::GetDefaultValue(dataSource),
        std::forward<FnArgs>(args)...);
}

/// Overload that accepts a leading class template argument that is passed to
/// the visitor as the second template argument.
///
/// This is identical to `VtVisitValueType<Visitor, Tmpl, TypeArgs...>(value,
/// args...)` except that instead of a VtValue `value`, the value type is
/// determined by the passed `dataSource`.
template <
    template <class T, template <class...> class, class ...> class Visitor,
    template <class...> class Tmpl,
    typename ...TypeArgs,
    typename ...FnArgs
    >
auto
HdVisitSampledDataSourceType(
    const HdSampledDataSourceHandle& dataSource, FnArgs&&... args)
{
    return VtVisitValueType<Visitor, Tmpl, TypeArgs...>(
        Hd_SampledDataSourceDefaultValueAccessor::GetDefaultValue(dataSource),
        std::forward<FnArgs>(args)...);
}

/// A VtValue visitor that invokes DataSource<T>::New(args...) if T is one of
/// the "known" Vt value types (see VtVisitValue).  If T is not one of the known
/// types, then if `UntypedDataSource` is `void` return `nullptr`, otherwise
/// return UntypedDataSource::New(args...).
template <
    typename T,
    template <typename...> class DataSource,
    class UntypedDataSource>
struct Hd_CopySampledDataSourceTypeVisitor
{
    template <class ...Args>
    static HdDataSourceBaseHandle Visit(Args&&... args)
    {
        if constexpr (std::is_same_v<T, VtValue>) {
            if constexpr (std::is_void_v<UntypedDataSource>) {
                return nullptr;
            }
            else {
                return UntypedDataSource::New(std::forward<Args>(args)...);
            }
        }
        else {
            return DataSource<T>::New(std::forward<Args>(args)...);
        }
    }
};

/// Helper function to create a new typed data source with the same type as the
/// input sampled data source. DataSource<T>::New will be returned using the
/// provided args. For untyped input data sources nullptr or
/// UntypedDataSource::New will be returned (if UntypedDataSource was provided).
template <
    template <typename...> class DataSource,
    class UntypedDataSource = void,
    typename... Args>
HdDataSourceBaseHandle
HdCopySampledDataSourceType(
    const HdSampledDataSourceHandle& dataSource, Args&&... args)
{
    return HdVisitSampledDataSourceType<
        Hd_CopySampledDataSourceTypeVisitor, DataSource, UntypedDataSource>(
            dataSource, std::forward<Args>(args)...);
}

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
