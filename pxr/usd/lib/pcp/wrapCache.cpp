//
// Copyright 2016 Pixar
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
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/pyUtils.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>
#include <memory>

using namespace boost::python;
using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static boost::python::tuple
_ComputeLayerStack( PcpCache &cache, 
                    const PcpLayerStackIdentifier &identifier ) 
{
    PcpErrorVector errors;
    PcpLayerStackRefPtr result = 
        cache.ComputeLayerStack(identifier, &errors);

    typedef Tf_MakePyConstructor::RefPtrFactory<>::
        apply<PcpLayerStackRefPtr>::type RefPtrFactory;
    
    return boost::python::make_tuple(
        boost::python::object(
            boost::python::handle<>(RefPtrFactory()(result))), 
        errors);
}

static const PcpPrimIndex&
_WrapPrimIndex(PcpCache&, const PcpPrimIndex& primIndex)
{
    return primIndex;
}

static boost::python::tuple
_ComputePrimIndex( PcpCache& cache, const SdfPath & path )
{
    // Compute the prim index.
    PcpErrorVector errors;
    const PcpPrimIndex& primIndex = cache.ComputePrimIndex(path, &errors);

    // Wrap the prim index to python as an internal reference on cache.
    // The return_internal_reference<> says that the result is owned
    // by the first argument, the PcpCache, and shouldn't be destroyed
    // by Python.  The boost::ref() around the arguments ensure that
    // boost python will not make temporary copies of them.
    object pyWrapPrimIndex =
        make_function(_WrapPrimIndex, return_internal_reference<>());
    object pyPrimIndex(pyWrapPrimIndex(boost::ref(cache),
                                       boost::ref(primIndex)));

    // Return the prim index and errors to python.
    return boost::python::make_tuple(pyPrimIndex, errors);
}

static boost::python::object
_FindPrimIndex( PcpCache& cache, const SdfPath & path )
{
    if (const PcpPrimIndex* primIndex = cache.FindPrimIndex(path)) {
        // Wrap the prim index to python as an internal reference on cache.
        // The return_internal_reference<> says that the result is owned
        // by the first argument, the PcpCache, and shouldn't be destroyed
        // by Python.  The boost::ref() around the arguments ensure that
        // boost python will not make temporary copies of them.
        object pyWrapPrimIndex =
            make_function(_WrapPrimIndex, return_internal_reference<>());
        object pyPrimIndex(pyWrapPrimIndex(boost::ref(cache),
                                           boost::ref(*primIndex)));
        return pyPrimIndex;
    }
    return boost::python::object();
}

static boost::python::tuple
_ComputePropertyIndex( PcpCache &cache, const SdfPath &path )
{
    PcpErrorVector errors;
    const PcpPropertyIndex &result = cache.ComputePropertyIndex(path, &errors);
    return_by_value::apply<PcpPropertyIndex>::type converter;
    return boost::python::make_tuple(
        boost::python::object(boost::python::handle<>(converter(result))), 
        errors);
}

static const PcpPropertyIndex&
_WrapPropertyIndex(PcpCache&, const PcpPropertyIndex& propertyIndex)
{
    return propertyIndex;
}

static boost::python::object
_FindPropertyIndex( PcpCache& cache, const SdfPath & path )
{
    if (const PcpPropertyIndex* propIndex = cache.FindPropertyIndex(path)) {
        // Wrap the index to python as an internal reference on cache.
        // The return_internal_reference<> says that the result is owned
        // by the first argument, the PcpCache, and shouldn't be destroyed
        // by Python.  The boost::ref() around the arguments ensure that
        // boost python will not make temporary copies of them.
        object pyWrapPropertyIndex =
            make_function(_WrapPropertyIndex, return_internal_reference<>());
        object pyPropertyIndex(pyWrapPropertyIndex(boost::ref(cache),
                                           boost::ref(*propIndex)));
        return pyPropertyIndex;
    }
    return boost::python::object();
}

static boost::python::tuple
_ComputeRelationshipTargetPaths( PcpCache &cache, const SdfPath & path, 
                                 bool localOnly,
                                  const SdfSpecHandle & stopProperty,
                                  bool includeStopProperty )
{
    PcpErrorVector errors;
    SdfPathVector result;
    cache.ComputeRelationshipTargetPaths(path, &result, localOnly,
                                         stopProperty, includeStopProperty,
                                         &errors);
    return boost::python::make_tuple(result, errors);
}

static boost::python::tuple
_ComputeAttributeConnectionPaths( PcpCache &cache, const SdfPath & path, 
                                  bool localOnly,
                                  const SdfSpecHandle & stopProperty,
                                  bool includeStopProperty )
{
    PcpErrorVector errors;
    SdfPathVector result;
    cache.ComputeAttributeConnectionPaths(path, &result, localOnly,
                                          stopProperty, includeStopProperty,
                                          &errors);
    return boost::python::make_tuple(result, errors);
}

static void
_SetVariantFallbacks(PcpCache & cache, const dict& d)
{
    PcpVariantFallbackMap fallbacks;
    if (PcpVariantFallbackMapFromPython(d, &fallbacks)) {
        cache.SetVariantFallbacks(fallbacks);
    }
}

static void
_RequestPayloads( PcpCache & cache,
                  const std::vector<SdfPath> & pathsToInclude,
                  const std::vector<SdfPath> & pathsToExclude )
{
    SdfPathSet include( pathsToInclude.begin(), pathsToInclude.end() );
    SdfPathSet exclude( pathsToExclude.begin(), pathsToExclude.end() );
    cache.RequestPayloads(include, exclude);
}

static void
_RequestLayerMuting( PcpCache & cache,
                     const std::vector<std::string> & layersToMute,
                     const std::vector<std::string> & layersToUnmute )
{
    cache.RequestLayerMuting(layersToMute, layersToUnmute);
}

static PcpDependencyVector
_FindSiteDependencies(const PcpCache& cache,
                    const PcpLayerStackPtr& layerStack,
                    const SdfPath& path,
                    PcpDependencyFlags depMask,
                    bool recurseOnSite,
                    bool recurseOnIndex,
                    bool filterForExistingCachesOnly)
{
    return cache.FindSiteDependencies(layerStack, path, depMask,
                                    recurseOnSite,
                                    recurseOnIndex,
                                    filterForExistingCachesOnly);
}

static void
_Reload( PcpCache & cache )
{
    PcpChanges changes;
    cache.Reload(&changes);
    changes.Apply();
}

} // anonymous namespace 

void 
wrapCache()
{
    class_<PcpCache, boost::noncopyable> 
        ("Cache", 
         init<const PcpLayerStackIdentifier&,
              const std::string&, 
              bool,
              const PcpPayloadDecoratorRefPtr&>(
              (arg("layerStackIdentifier"),
               arg("targetSchema") = std::string(),
               arg("usd") = false,
               arg("payloadDecorator") = PcpPayloadDecoratorRefPtr())))

        // Note: The following parameters are not wrapped as a properties
        // because setting them may require returning additional out-
        // parameters representing the resulting cache invalidation.
        .def("GetLayerStackIdentifier", &PcpCache::GetLayerStackIdentifier)
        .def("SetVariantFallbacks", &_SetVariantFallbacks)
        .def("GetVariantFallbacks", &PcpCache::GetVariantFallbacks,
             return_value_policy<TfPyMapToDictionary>())
        .def("GetUsedLayers", &PcpCache::GetUsedLayers,
             return_value_policy<TfPySequenceToList>())
        .def("IsPayloadIncluded", &PcpCache::IsPayloadIncluded)
        .def("RequestPayloads", &_RequestPayloads)
        .def("RequestLayerMuting", &_RequestLayerMuting,
             (args("layersToMute"),
              args("layersToUnmute")))
        .def("GetMutedLayers", &PcpCache::GetMutedLayers,
             return_value_policy<TfPySequenceToList>())
        .def("IsLayerMuted", 
             (bool(PcpCache::*)(const std::string&) const)
                &PcpCache::IsLayerMuted,
             (args("layerIdentifier")))

        .add_property("layerStack", &PcpCache::GetLayerStack)
        .add_property("targetSchema", 
                      make_function(&PcpCache::GetTargetSchema,
                                    return_value_policy<return_by_value>()))

        .def("ComputeLayerStack", &_ComputeLayerStack)
        .def("ComputePrimIndex", &_ComputePrimIndex)
        .def("FindPrimIndex", &_FindPrimIndex)
        .def("ComputePropertyIndex", &_ComputePropertyIndex)
        .def("FindPropertyIndex", &_FindPropertyIndex)

        .def("ComputeRelationshipTargetPaths", 
             &_ComputeRelationshipTargetPaths,
             (args("relPath"),
              args("localOnly") = false,
              args("stopProperty") = SdfSpecHandle(),
              args("includeStopProperty") = false))
        .def("ComputeAttributeConnectionPaths", 
             &_ComputeAttributeConnectionPaths,
             (args("relPath"),
              args("localOnly") = false,
              args("stopProperty") = SdfSpecHandle(),
              args("includeStopProperty") = false))

        .def("FindSiteDependencies",
             &_FindSiteDependencies,
             (args("siteLayerStack"),
              args("sitePath"),
              args("dependencyType") = PcpDependencyTypeAnyNonVirtual,
              args("recurseOnSite") = false,
              args("recurseOnIndex") = false,
              args("filterForExistingCachesOnly") = false),
             return_value_policy<TfPySequenceToList>())
        .def("FindAllLayerStacksUsingLayer",
             &PcpCache::FindAllLayerStacksUsingLayer,
             return_value_policy<TfPySequenceToList>())

        .def("IsInvalidAssetPath", &PcpCache::IsInvalidAssetPath)
        .def("IsInvalidSublayerIdentifier", 
             &PcpCache::IsInvalidSublayerIdentifier)

        .def("PrintStatistics", &PcpCache::PrintStatistics)
        .def("Reload", &_Reload)
        ;
}
