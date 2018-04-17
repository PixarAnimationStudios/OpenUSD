//
// Copyright 2018 Pixar
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
#include "gusd/stageCache.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

#include <UT/UT_StringSet.h>


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


UT_StringSet _ListToStringSet(const list& list)
{
    UT_StringSet set;
    const size_t size = len(list);
    for(size_t i = 0; i < size; ++i)
        set.insert(UT_StringHolder(extract<std::string>(list[i])));
    return set;
}


/// Helper for extracting a pointer from \p obj, which may hold
/// a correct pointer type, or None.
/// This is a hack to help handle possibly null intrusive pointers,
/// which boost python otherwise doesn't know how to convert by default.
template <typename T>
T _ExtractPtr(const object& obj)
{   
    if(obj) {
        T val = extract<T>(obj);
        return val;
    }
    return nullptr;
}


/// Helper for creating a python object holding a stage ref ptr.
/// Note that normal object creation produces a weak ptr.
object
_StageRefToObj(const UsdStageRefPtr& stage)
{
    using RefPtrFactory =
        Tf_MakePyConstructor::RefPtrFactory<>::apply<UsdStageRefPtr>::type;
    return object(handle<>(RefPtrFactory()(stage)));
}


UsdStageRefPtr
_Find(GusdStageCache& self,
      const std::string& path,
      const GusdStageOpts& opts,
      const object& edit)
{
    return GusdStageCacheReader(self).Find(
        UT_StringRef(path), opts, _ExtractPtr<GusdStageEditPtr>(edit));
}


UsdStageRefPtr
_FindOrOpen(GusdStageCache& self,
            const std::string& path,
            const GusdStageOpts& opts,
            const object& edit)
{
    return GusdStageCacheReader(self).FindOrOpen(
        UT_StringRef(path), opts, _ExtractPtr<GusdStageEditPtr>(edit));
}


tuple
_GetPrim(GusdStageCache& self,
         const std::string& path,
         const SdfPath& primPath,
         const object& edit,
         const GusdStageOpts& opts)
{
    auto pair = GusdStageCacheReader(self).GetPrim(
        UT_StringRef(path), primPath,
        _ExtractPtr<GusdStageEditPtr>(edit), opts);
    return boost::python::make_tuple(pair.first, _StageRefToObj(pair.second));
}


template <typename T, typename ExtractT=T>
GusdDefaultArray<T>
_ObjectToDefaultArray(const object& o)
{
    GusdDefaultArray<T> array;
    if(!o) {
        return array;
    }
    
    extract<ExtractT> constVal(o);
    if(constVal.check()) {
        array.SetDefault(T(constVal));
    }

    list vals = extract<list>(o);
    array.GetArray().setSize(len(vals));
    for(exint i = 0; i < array.size(); ++i) {
        array(i) = T(extract<ExtractT>(vals[i]));
    }
    return array;
}


template <typename T>
UT_Array<T>
_ListToArray(const list& l)
{
    UT_Array<T> array;
    array.setSize(len(l));
    for(exint i = 0; i < array.size(); ++i)
        array(i) = extract<T>(l[i]);
    return array;
}


std::vector<UsdPrim>
_GetPrims(GusdStageCache& self,
          const object& filePaths,
          const list& primPaths,
          const object& edits,
          const GusdStageOpts& opts)
{
    std::vector<UsdPrim> prims(len(primPaths));

    GusdStageCacheReader(self).GetPrims(
        _ObjectToDefaultArray<UT_StringHolder,std::string>(filePaths),
        _ListToArray<SdfPath>(primPaths),
        _ObjectToDefaultArray<GusdStageEditPtr>(edits),
        prims.data(), opts);

    return prims;
}
          

tuple
_GetPrimWithVariants(GusdStageCache& self,
                     const std::string& path,
                     const SdfPath& primPath,
                     const SdfPath& variants,
                     const GusdStageOpts& opts)
{
    auto pair = GusdStageCacheReader(self).GetPrimWithVariants(
        UT_StringRef(path), primPath, variants, opts);
    return boost::python::make_tuple(pair.first, _StageRefToObj(pair.second));
}


void
_Clear_Full(GusdStageCache& self)
{
    GusdStageCacheWriter(self).Clear();
}


void
_Clear_Partial(GusdStageCache& self, const list& paths)
{
    GusdStageCacheWriter(self).Clear(_ListToStringSet(paths));
}


list
_FindStages(GusdStageCache& self, const list& paths)
{
    UT_Set<UsdStageRefPtr> stages;
    GusdStageCacheWriter(self).FindStages(_ListToStringSet(paths), stages);

    list stageList;
    for(const auto& stage : stages) {
        stageList.append(_StageRefToObj(stage));
    }
    return stageList;
}


void
_ReloadStages(GusdStageCache& self, const list& paths)
{
    GusdStageCacheWriter(self).ReloadStages(_ListToStringSet(paths));
}


void wrapGusdStageCache()
{
    using This = GusdStageCache;

    class_<This, boost::noncopyable>("StageCache")
        
        .def("GetInstance", &This::GetInstance,
             return_value_policy<reference_existing_object>())
        .staticmethod("GetInstance")

        .def("Find", &_Find,
             (arg("path"),
              arg("opts")=GusdStageOpts::LoadAll(),
              arg("edit")=GusdStageEditPtr()),
             return_value_policy<TfPyRefPtrFactory<> >())

        .def("FindOrOpen", &_FindOrOpen,
             (arg("path"),
              arg("opts")=GusdStageOpts::LoadAll(),
              arg("edit")=GusdStageEditPtr()),
             return_value_policy<TfPyRefPtrFactory<> >())

        .def("GetPrim", &_GetPrim,
             (arg("path"),
              arg("primPath"),
              arg("edit")=GusdStageEditPtr(),
              arg("opts")=GusdStageOpts::LoadAll()))

        .def("GetPrims", &_GetPrims,
             (arg("filePaths"),
              arg("primPaths"),
              arg("edits")=object(),
              arg("opts")=GusdStageOpts::LoadAll()),
             return_value_policy<TfPySequenceToList>())

        .def("GetPrimWithVariants", &_GetPrimWithVariants,
             (arg("path"),
              arg("primPath"),
              arg("variants")=SdfPath(),
              arg("opts")=GusdStageOpts::LoadAll()))

        .def("Clear", &_Clear_Full)
        .def("Clear", &_Clear_Partial, (arg("paths")=object()))

        .def("FindStages", &_FindStages, (arg("paths")))

        .def("ReloadStages", &_ReloadStages, (arg("paths")))
        ;
}
