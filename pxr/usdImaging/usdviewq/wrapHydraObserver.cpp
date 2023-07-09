//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
bool
_WrapLocatorSetIntersectsLocator(HdDataSourceLocatorSet &self,
    const HdDataSourceLocator &loc)
{
    return self.Intersects(loc);
}

static
bool
_WrapLocatorSetIntersectsLocatorSet(HdDataSourceLocatorSet &self,
    const HdDataSourceLocatorSet &locSet)
{
    return self.Intersects(locSet);
}

static
void
_WrapLocatorSetInsertLocator(HdDataSourceLocatorSet &self,
    const HdDataSourceLocator &loc)
{
    self.insert(loc);
}

static
void
_WrapLocatorSetInsertLocatorSet(HdDataSourceLocatorSet &self,
    const HdDataSourceLocatorSet &locSet)
{
    self.insert(locSet);
}

static
std::string
_WrapLocatorSetAsString(HdDataSourceLocatorSet &self)
{
    std::ostringstream buffer;
    for (const HdDataSourceLocator &locator : self) {
        buffer << locator.GetString() << ",";
    }

    return buffer.str();
}

static
list
_WrapContainerDataSourceGetNames(HdContainerDataSourceHandle &self)
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
_WrapContainerDataSourceGet(HdContainerDataSourceHandle &self,
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
_WrapContainerDataSourceGetGetFromLocator(HdContainerDataSourceHandle &self,
    const HdDataSourceLocator &loc)
{

    HdDataSourceBaseHandle ds = HdContainerDataSource::Get(self, loc);
    return _CastDataSource(ds);
}

size_t
_WrapVectorDataSourceGetNumElements(HdVectorDataSourceHandle &self)
{
    if (!self) {
        return 0;
    }

    return self->GetNumElements();
}

static
object
_WrapVectorDataSourceGetElement(HdVectorDataSourceHandle &self, size_t i)
{
    if (!self) {
        return object();
    }
    HdDataSourceBaseHandle ds = self->GetElement(i);
    return _CastDataSource(ds);
}

static
object
_WrapSampledDataSourceGetValue(HdSampledDataSourceHandle &self,
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
_WrapSampledDataSourceGetTypeString(HdSampledDataSourceHandle &self)
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
_WrapLocatorGetElement(HdDataSourceLocator &self, size_t i)
{
    if (i >= self.GetElementCount()) {
        return TfToken();
    }
    return self.GetElement(i);
}

static
TfToken
_WrapLocatorGetFirstElement(HdDataSourceLocator &self)
{
    if (self.GetElementCount() == 0) {
        return TfToken();
    }
    return self.GetFirstElement();
}

static
TfToken
_WrapLocatorGetLastElement(HdDataSourceLocator &self)
{
    if (self.GetElementCount() == 0) {
        return TfToken();
    }
    return self.GetLastElement();
}

static
HdDataSourceLocator
_WrapLocatorAppendToken(HdDataSourceLocator &self, const TfToken &name)
{
    return self.Append(name);
}

static
HdDataSourceLocator
_WrapLocatorAppendLocator(HdDataSourceLocator &self,
    const HdDataSourceLocator &loc)
{
    return self.Append(loc);
}

void wrapHydraObserver()
{
    typedef UsdviewqHydraObserver This;

    class_<This> ("HydraObserver", init<>())
        .def("GetRegisteredSceneIndexNames",
                &This::GetRegisteredSceneIndexNames)
            .staticmethod("GetRegisteredSceneIndexNames")

        .def("TargetToNamedSceneIndex", &This::TargetToNamedSceneIndex)
        .def("TargetToInputSceneIndex", &This::TargetToInputSceneIndex)

        .def("GetDisplayName",
                &This::GetDisplayName)

        .def("GetInputDisplayNames",
                &This::GetInputDisplayNames)

        .def("GetChildPrimPaths",
                &This::GetChildPrimPaths)
        .def("GetPrim",
                &_WrapGetPrim)

        .def("HasPendingNotices", &This::HasPendingNotices)
        .def("GetPendingNotices", &_GetPendingNotices)
        .def("ClearPendingNotices", &This::ClearPendingNotices)
    ;

    typedef HdDataSourceLocator Loc;

    class_<Loc> ("DataSourceLocator", init<>())
        .def( init<const TfToken &>() )
        .def( init<const TfToken &, const TfToken &>() )
        .def( init<const TfToken &, const TfToken &, const TfToken &>() )
        .def( init<const TfToken &, const TfToken &, const TfToken &,
            const TfToken &>())
        .def( init<const TfToken &, const TfToken &, const TfToken &,
            const TfToken &, const TfToken &>())
        .def( init<const TfToken &, const TfToken &, const TfToken &,
            const TfToken &, const TfToken &, const TfToken &>())
        .def("IsEmpty", &Loc::IsEmpty)
        .def("GetElementCount", &Loc::GetElementCount)
        .def("GetElement", &_WrapLocatorGetElement)
        .def("GetFirstElement", &_WrapLocatorGetFirstElement)
        .def("GetLastElement", &_WrapLocatorGetLastElement)
        .def("ReplaceLastElement", &Loc::ReplaceLastElement)
        .def("RemoveLastElement", &Loc::RemoveLastElement)
        .def("RemoveFirstElement", &Loc::RemoveFirstElement)
        .def("Append", &_WrapLocatorAppendToken)
        .def("Append", &_WrapLocatorAppendLocator)
        .def("HasPrefix", &Loc::HasPrefix)
        .def("GetCommonPrefix", &Loc::GetCommonPrefix)
        .def("ReplacePrefix", &Loc::ReplacePrefix)
        .def("Intersects", &Loc::Intersects)
        .def("GetString", &Loc::GetString)
        
        .def(self == self)
        .def(self != self)
        .def("__hash__", &Loc::Hash)

        // TODO, further methods not needed for browser case
    ;

    typedef HdDataSourceLocatorSet LocSet;
    class_<LocSet> ("DataSourceLocatorSet", init<>())
        .def("Intersects", &_WrapLocatorSetIntersectsLocator)
        .def("Intersects", &_WrapLocatorSetIntersectsLocatorSet)
        .def("IsEmpty", &LocSet::IsEmpty)
        .def("Contains", &LocSet::Contains)
        .def("insert", &_WrapLocatorSetInsertLocator)
        .def("insert", &_WrapLocatorSetInsertLocatorSet)
        .def("AsString", &_WrapLocatorSetAsString)
        
        // TODO, further methods not needed for browser case
    ;

    typedef HdDataSourceBaseHandle Ds;
    class_<Ds> ("DataSourceBase", no_init)
    ;

    typedef HdContainerDataSourceHandle Cds;
    class_<Cds> ("ContainerDataSource", no_init)
        .def("GetNames", &_WrapContainerDataSourceGetNames)
        .def("Get", &_WrapContainerDataSourceGet)
        .def("Get", &_WrapContainerDataSourceGetGetFromLocator)
    ;

    typedef HdVectorDataSourceHandle Vds;
    class_<Vds> ("VectorDataSource", no_init)
        .def("GetNumElements", &_WrapVectorDataSourceGetNumElements)
        .def("GetElement", &_WrapVectorDataSourceGetElement)
    ;

    typedef HdSampledDataSourceHandle Sds;
    class_<Sds> ("SampledDataSource", no_init)
        .def("GetValue", &_WrapSampledDataSourceGetValue)
        .def("GetTypeString", &_WrapSampledDataSourceGetTypeString)
    ;
}