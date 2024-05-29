//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdImaging/usdviewq/hydraObserver.h"
#include "pxr/imaging/hd/dataSource.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/token.h"


#include <boost/python.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/scope.hpp>

#include <sstream>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static
object
_WrapGetPrim(UsdviewqHydraObserver &self, const SdfPath &primPath)
{
    HdSceneIndexPrim prim = self.GetPrim(primPath);
    list result;
    result.append(prim.primType);

    if (prim.dataSource) {
        result.append(prim.dataSource);
    } else {
        result.append(object());
    }
    return result;
}

static
object _GetPendingNotices(UsdviewqHydraObserver &self)
{
    UsdviewqHydraObserver::NoticeEntryVector noticeEntries = self.GetPendingNotices();

    list result;

    for (const UsdviewqHydraObserver::NoticeEntry &noticeEntry : noticeEntries) {
        if (!noticeEntry.added.empty()) {
            list entries;
            for (const HdSceneIndexObserver::AddedPrimEntry &entry :
                    noticeEntry.added) {
                entries.append(make_tuple(entry.primPath, entry.primType));
            }
            result.append(make_tuple("added", entries));
        }

        if (!noticeEntry.removed.empty()) {
            list entries;
            for (const HdSceneIndexObserver::RemovedPrimEntry &entry :
                    noticeEntry.removed) {
                entries.append(entry.primPath);
            }
            result.append(make_tuple("removed", entries));
        }

        if (!noticeEntry.dirtied.empty()) {
            list entries;
            for (const HdSceneIndexObserver::DirtiedPrimEntry &entry :
                    noticeEntry.dirtied) {
                entries.append(make_tuple(entry.primPath, entry.dirtyLocators));
            }
            result.append(make_tuple("dirtied", entries));
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
list
_WrapContainerDataSourceGetNames(const HdContainerDataSourceHandle &self)
{
    list result;

    if (self) {
        for (const TfToken &name : self->GetNames()) {
            result.append(name);
        }
    }

    return result;
}

static
object
_CastDataSource(const HdDataSourceBaseHandle &ds)
{
    if (auto cds = HdContainerDataSource::Cast(ds)) {
        return object(cds);
    }

    if (auto sds = HdSampledDataSource::Cast(ds)) {
        return object(sds);
    }

    if (auto vds = HdVectorDataSource::Cast(ds)) {
        return object(vds);
    }

    if (ds) {
        return object(ds);
    }

    return object();
}

static
object
_WrapContainerDataSourceGet(
    const HdContainerDataSourceHandle &self,
    const TfToken &name)
{
    if (!self) {
        return object();
    }
    HdDataSourceBaseHandle ds = self->Get(name);
    return _CastDataSource(ds);
}


static
object
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
object
_WrapVectorDataSourceGetElement(const HdVectorDataSourceHandle &self, size_t i)
{
    if (!self) {
        return object();
    }
    HdDataSourceBaseHandle ds = self->GetElement(i);
    return _CastDataSource(ds);
}

static
object
_WrapSampledDataSourceGetValue(const HdSampledDataSourceHandle &self,
    HdSampledDataSource::Time shutterOffset)
{
    if (!self) {
        return object();
    }

    VtValue v = self->GetValue(shutterOffset);

    if (v.IsEmpty()) {
        return object();
    }

    try {
        return object(v);
    } catch (...) {
        // if we cannot convert, clear the python state and produce a string
        // as the value
        PyErr_Clear();
        std::ostringstream buffer;
        buffer << v;
        return object(buffer.str());
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

void wrapHydraObserver()
{
    {
        using This = UsdviewqHydraObserver;

        class_<This> ("HydraObserver", init<>())
            .def("GetRegisteredSceneIndexNames",
                 &This::GetRegisteredSceneIndexNames)
            .staticmethod("GetRegisteredSceneIndexNames")

            .def("TargetToNamedSceneIndex", &This::TargetToNamedSceneIndex)
            .def("TargetToInputSceneIndex", &This::TargetToInputSceneIndex)

            .def("GetDisplayName", &This::GetDisplayName)

            .def("GetInputDisplayNames", &This::GetInputDisplayNames)

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

        class_<This> ("DataSourceLocator", init<>())
            .def( init<const TfToken &>() )
            .def( init<const TfToken &, const TfToken &>() )
            .def( init<const TfToken &, const TfToken &, const TfToken &>() )
            .def( init<const TfToken &, const TfToken &, const TfToken &,
                  const TfToken &>())
            .def( init<const TfToken &, const TfToken &, const TfToken &,
                  const TfToken &, const TfToken &>())
            .def( init<const TfToken &, const TfToken &, const TfToken &,
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
            
            .def(self == self)
            .def(self != self)
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

        class_<This> ("DataSourceLocatorSet", init<>())
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
        using ThisHandle = HdDataSourceBaseHandle;
        class_<ThisHandle> ("DataSourceBase", no_init)
            ;
    }

    {
        using ThisHandle = HdContainerDataSourceHandle;
        class_<ThisHandle> ("ContainerDataSource", no_init)
            .def("GetNames", &_WrapContainerDataSourceGetNames)
            .def("Get", &_WrapContainerDataSourceGet)
            .def("Get", &_WrapContainerDataSourceGetGetFromLocator)
            ;
    }

    {
        using ThisHandle = HdVectorDataSourceHandle;
        class_<ThisHandle> ("VectorDataSource", no_init)
            .def("GetNumElements", &_WrapVectorDataSourceGetNumElements)
            .def("GetElement", &_WrapVectorDataSourceGetElement)
            ;
    }

    {
        using ThisHandle = HdSampledDataSourceHandle;
        class_<ThisHandle> ("SampledDataSource", no_init)
            .def("GetValue", &_WrapSampledDataSourceGetValue)
            .def("GetTypeString", &_WrapSampledDataSourceGetTypeString)
            ;
    }
}
