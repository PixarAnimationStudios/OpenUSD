//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/vector.h>

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdviewq/hydraObserver.h"
#include "pxr/imaging/hd/dataSource.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/token.h"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static
nb::object
_WrapGetPrim(UsdviewqHydraObserver &self, const SdfPath &primPath)
{
    HdSceneIndexPrim prim = self.GetPrim(primPath);
    nb::list result;
    result.append(prim.primType);

    if (prim.dataSource) {
        result.append(prim.dataSource);
    } else {
        result.append(nb::none());
    }
    return result;
}

static
nb::object _GetPendingNotices(UsdviewqHydraObserver &self)
{
    UsdviewqHydraObserver::NoticeEntryVector noticeEntries = self.GetPendingNotices();

    nb::list result;

    for (const UsdviewqHydraObserver::NoticeEntry &noticeEntry : noticeEntries) {
        if (!noticeEntry.added.empty()) {
            nb::list entries;
            for (const HdSceneIndexObserver::AddedPrimEntry &entry :
                    noticeEntry.added) {
                entries.append(nb::make_tuple(entry.primPath, entry.primType));
            }
            result.append(nb::make_tuple("added", entries));
        }

        if (!noticeEntry.removed.empty()) {
            nb::list entries;
            for (const HdSceneIndexObserver::RemovedPrimEntry &entry :
                    noticeEntry.removed) {
                entries.append(entry.primPath);
            }
            result.append(nb::make_tuple("removed", entries));
        }

        if (!noticeEntry.dirtied.empty()) {
            nb::list entries;
            for (const HdSceneIndexObserver::DirtiedPrimEntry &entry :
                    noticeEntry.dirtied) {
                entries.append(nb::make_tuple(entry.primPath, entry.dirtyLocators));
            }
            result.append(nb::make_tuple("dirtied", entries));
        }
    }

    return result;
}

static
std::string
_WrapLocatorSetAsString(const HdDataSourceLocatorSet &self)
{
    std::ostringstream buffer;
    for (const HdDataSourceLocator &locator : self) {
        buffer << locator.GetString() << ",";
    }

    return buffer.str();
}

static
nb::object
_CastDataSource(const HdDataSourceBaseHandle &ds)
{
    if (auto cds = HdContainerDataSource::Cast(ds)) {
        return nb::cast(cds);
    }

    if (auto sds = HdSampledDataSource::Cast(ds)) {
        return nb::cast(sds);
    }

    if (auto vds = HdVectorDataSource::Cast(ds)) {
        return nb::cast(vds);
    }

    if (ds) {
        return nb::cast(ds);
    }

    return nb::none();
}

static
nb::object
_WrapContainerDataSourceGet(
    const HdContainerDataSourceHandle &self,
    const TfToken &name)
{
    if (!self) {
        return nb::none();
    }
    HdDataSourceBaseHandle ds = self->Get(name);
    return _CastDataSource(ds);
}

static
nb::object
_WrapContainerDataSourceGetGetFromLocator(
    const HdContainerDataSourceHandle &self,
    const HdDataSourceLocator &loc)
{

    HdDataSourceBaseHandle ds = HdContainerDataSource::Get(self, loc);
    return _CastDataSource(ds);
}

size_t
_WrapVectorDataSourceGetNumElements(const HdVectorDataSourceHandle &self)
{
    if (!self) {
        return 0;
    }

    return self->GetNumElements();
}

static
nb::object
_WrapVectorDataSourceGetElement(const HdVectorDataSourceHandle &self, size_t i)
{
    if (!self) {
        return nb::none();
    }
    HdDataSourceBaseHandle ds = self->GetElement(i);
    return _CastDataSource(ds);
}

static
nb::object
_WrapSampledDataSourceGetValue(const HdSampledDataSourceHandle &self,
    HdSampledDataSource::Time shutterOffset)
{
    if (!self) {
        return nb::none();
    }

    VtValue v = self->GetValue(shutterOffset);

    if (v.IsEmpty()) {
        return nb::none();
    }

    try {
        return nb::cast(v);
    } catch (...) {
        // if we cannot convert, clear the python state and produce a string
        // as the value
        PyErr_Clear();
        std::ostringstream buffer;
        buffer << v;
        return nb::cast(buffer.str());
    }
}

static
std::string
_WrapSampledDataSourceGetTypeString(const HdSampledDataSourceHandle &self)
{
    if (!self) {
        return "";
    }

    VtValue v = self->GetValue(0.0f);
    if (v.IsEmpty()) {
        return "";
    }

    return v.GetTypeName();
}

static
TfToken
_WrapLocatorGetElement(const HdDataSourceLocator &self, size_t i)
{
    if (i >= self.GetElementCount()) {
        return TfToken();
    }
    return self.GetElement(i);
}

static
TfToken
_WrapLocatorGetFirstElement(const HdDataSourceLocator &self)
{
    if (self.GetElementCount() == 0) {
        return TfToken();
    }
    return self.GetFirstElement();
}

static
TfToken
_WrapLocatorGetLastElement(const HdDataSourceLocator &self)
{
    if (self.GetElementCount() == 0) {
        return TfToken();
    }
    return self.GetLastElement();
}

static
std::string
_WrapHdDataSourceLocatorStr(const HdDataSourceLocator &self)
{
    std::ostringstream buffer;
    buffer << self;
    return buffer.str();
}

static
std::string
_WrapHdDataSourceLocatorSetStr(const HdDataSourceLocatorSet &self)
{
    std::ostringstream buffer;
    buffer << self;
    return buffer.str();
}

void wrapHydraObserver(nb::module_ &m)
{
    {
        using This = UsdviewqHydraObserver;

        nb::class_<This>(m, "HydraObserver")
            .def(nb::init<>())
            .def_static("GetRegisteredSceneIndexNames",
                 &This::GetRegisteredSceneIndexNames)

            .def("TargetToNamedSceneIndex", &This::TargetToNamedSceneIndex)
            .def("TargetToInputSceneIndex", &This::TargetToInputSceneIndex)
            .def("TargetToNestedInputSceneIndex",
                 &This::TargetToNestedInputSceneIndex)

            .def("GetDisplayName", &This::GetDisplayName)

            .def("GetInputDisplayNames", &This::GetInputDisplayNames)
            .def("GetNestedInputDisplayNames",
                 &This::GetNestedInputDisplayNames)

            .def("GetChildPrimPaths", &This::GetChildPrimPaths)
            .def("GetPrim", &_WrapGetPrim)

            .def("HasPendingNotices", &This::HasPendingNotices)
            .def("GetPendingNotices", &_GetPendingNotices)
            .def("ClearPendingNotices", &This::ClearPendingNotices)
            ;
    }

    {
        using This = HdDataSourceLocator;

        using Sig1 = This(This::*)(const TfToken &) const;
        using Sig2 = This(This::*)(const This &) const; 

        nb::class_<This>(m, "DataSourceLocator")
            .def( nb::init<>() )
            .def( nb::init<const TfToken &>() )
            .def( nb::init<const TfToken &, const TfToken &>() )
            .def( nb::init<const TfToken &, const TfToken &, const TfToken &>() )
            .def( nb::init<const TfToken &, const TfToken &, const TfToken &,
                  const TfToken &>())
            .def( nb::init<const TfToken &, const TfToken &, const TfToken &,
                  const TfToken &, const TfToken &>())
            .def( nb::init<const TfToken &, const TfToken &, const TfToken &,
                  const TfToken &, const TfToken &, const TfToken &>())
            .def("IsEmpty", &This::IsEmpty)
            .def("GetElementCount", &This::GetElementCount)
            .def("GetElement", &_WrapLocatorGetElement)
            .def("GetFirstElement", &_WrapLocatorGetFirstElement)
            .def("GetLastElement", &_WrapLocatorGetLastElement)
            .def("ReplaceLastElement", &This::ReplaceLastElement)
            .def("RemoveLastElement", &This::RemoveLastElement)
            .def("RemoveFirstElement", &This::RemoveFirstElement)
            .def("Append", (Sig1)&This::Append)
            .def("Append", (Sig2)&This::Append)
            .def("HasPrefix", &This::HasPrefix)
            .def("GetCommonPrefix", &This::GetCommonPrefix)
            .def("ReplacePrefix", &This::ReplacePrefix)
            .def("Intersects", &This::Intersects)
            .def("GetString", &This::GetString)
            
            .def(nb::self == nb::self)
            .def(nb::self != nb::self)
            .def("__hash__", &This::Hash)
            .def("__str__", &_WrapHdDataSourceLocatorStr)
            // TODO, further methods not needed for browser case
            ;
    }

    {
        using This = HdDataSourceLocatorSet;

        using Sig1 = bool(This::*)(const HdDataSourceLocator &) const;
        using Sig2 = bool(This::*)(const This &) const;
        using Sig3 = void(This::*)(const HdDataSourceLocator &);
        using Sig4 = void(This::*)(const This &);

        nb::class_<This>(m, "DataSourceLocatorSet")
            .def(nb::init<>())
            .def("Intersects", (Sig1)&This::Intersects)
            .def("Intersects", (Sig2)&This::Intersects)
            .def("IsEmpty", &This::IsEmpty)
            .def("Contains", &This::Contains)
            .def("insert", (Sig3)&This::insert)
            .def("insert", (Sig4)&This::insert)
            .def("AsString", &_WrapLocatorSetAsString)
            .def("__str__", &_WrapHdDataSourceLocatorSetStr)
            // TODO, further methods not needed for browser case
            ;
    }

    {
        nb::class_<HdDataSourceBase>(m, "DataSourceBase")
            ;
    }

    {
        using ThisHandle = HdContainerDataSourceHandle;
        nb::class_<HdContainerDataSource, HdDataSourceBase>(m, "ContainerDataSource")
            .def("GetNames",
                 +[](const ThisHandle &handle) {
                     return handle->GetNames(); })
            .def("Get", &_WrapContainerDataSourceGet)
            .def("Get", &_WrapContainerDataSourceGetGetFromLocator)
            ;
    }

    {
        nb::class_<HdVectorDataSource, HdDataSourceBase>(m, "VectorDataSource")
            .def("GetNumElements", &_WrapVectorDataSourceGetNumElements)
            .def("GetElement", &_WrapVectorDataSourceGetElement)
            ;
    }

    {
        nb::class_<HdSampledDataSource, HdDataSourceBase>(m, "SampledDataSource")
            .def("GetValue", &_WrapSampledDataSourceGetValue)
            .def("GetTypeString", &_WrapSampledDataSourceGetTypeString)
            ;
    }
}
