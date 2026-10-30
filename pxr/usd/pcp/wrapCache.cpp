//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/pyUtils.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <memory>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static nanobind::tuple
_ComputeLayerStack( PcpCache &cache, 
                    const PcpLayerStackIdentifier &identifier ) 
{
    PcpErrorVector errors;
    PcpLayerStackRefPtr result = 
        cache.ComputeLayerStack(identifier, &errors);

    return nanobind::make_tuple(result, errors);
}

static nanobind::tuple
_ComputePrimIndex( nanobind::handle self, const SdfPath & path )
{
    PcpCache &cache = nanobind::cast<PcpCache &>(self);
    // Compute the prim index.
    PcpErrorVector errors;
    const PcpPrimIndex& primIndex = cache.ComputePrimIndex(path, &errors);

    nanobind::object pyPrimIndex = nanobind::cast(
        &primIndex, nanobind::rv_policy::reference_internal, self);

    // Return the prim index and errors to python.
    return nanobind::make_tuple(pyPrimIndex, errors);
}

static nanobind::object
_FindPrimIndex( nanobind::handle self, const SdfPath & path )
{
    PcpCache &cache = nanobind::cast<PcpCache &>(self);
    if (const PcpPrimIndex* primIndex = cache.FindPrimIndex(path)) {
        return nanobind::cast(
            primIndex, nanobind::rv_policy::reference_internal, self);
    }
    return nanobind::none();
}

static nanobind::tuple
_ComputePropertyIndex( PcpCache &cache, const SdfPath &path )
{
    PcpErrorVector errors;
    const PcpPropertyIndex &result = cache.ComputePropertyIndex(path, &errors);
    return nanobind::make_tuple(
        nanobind::cast(result, nanobind::rv_policy::copy), 
        errors);
}

static nanobind::object
_FindPropertyIndex( nanobind::handle self, const SdfPath & path )
{
    PcpCache &cache = nanobind::cast<PcpCache &>(self);
    if (const PcpPropertyIndex* propIndex = cache.FindPropertyIndex(path)) {
        return nanobind::cast(
            propIndex, nanobind::rv_policy::reference_internal, self);
    }
    return nanobind::none();
}

static nanobind::tuple
_ComputeRelationshipTargetPaths( PcpCache &cache, const SdfPath & path, 
                                 bool localOnly,
                                 const SdfSpecHandle & stopProperty,
                                 bool includeStopProperty )
{
    PcpErrorVector errors;
    SdfPathVector result;
    SdfPathVector deletedPaths;
    cache.ComputeRelationshipTargetPaths(path, &result, localOnly,
                                         stopProperty, includeStopProperty,
                                         &deletedPaths,
                                         &errors);
    return nanobind::make_tuple(result, deletedPaths, errors);
}

static nanobind::tuple
_ComputeAttributeConnectionPaths( PcpCache &cache, const SdfPath & path, 
                                  bool localOnly,
                                  const SdfSpecHandle & stopProperty,
                                  bool includeStopProperty )
{
    PcpErrorVector errors;
    SdfPathVector result;
    SdfPathVector deletedPaths;
    cache.ComputeAttributeConnectionPaths(path, &result, localOnly,
                                          stopProperty, includeStopProperty,
                                          &deletedPaths,
                                          &errors);
    return nanobind::make_tuple(result, deletedPaths, errors);
}

static void
_SetVariantFallbacks(PcpCache & cache, const nanobind::dict& d)
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
                      nanobind::handle depMaskObj,
                      bool recurseOnSite,
                      bool recurseOnIndex,
                      bool filterForExistingCachesOnly)
{
    PcpDependencyFlags depMask = 0;
    TfEnum tfEnum;
    if (Tf_PyEnumRegistry::GetInstance().FindEnum(depMaskObj.ptr(), &tfEnum) &&
        tfEnum.IsA<PcpDependencyType>()) {
        depMask = tfEnum.GetValueAsInt();
    } else if (!nanobind::try_cast<PcpDependencyFlags>(depMaskObj, depMask, false)) {
        throw nanobind::type_error(
            "Expected Pcp.DependencyType or int for dependencyType");
    }
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
wrapCache(nanobind::module_ &m)
{
    namespace nb = nanobind;

    nb::class_<PcpCache>(m, "Cache")
        .def(nb::init<const PcpLayerStackIdentifier&,
                      const std::string&, 
                      bool>(),
             nb::arg("layerStackIdentifier"),
             nb::arg("fileFormatTarget") = std::string(),
             nb::arg("usd") = false)

        // Note: The following parameters are not wrapped as a properties
        // because setting them may require returning additional out-
        // parameters representing the resulting cache invalidation.
        .def("GetLayerStackIdentifier", &PcpCache::GetLayerStackIdentifier,
             nb::rv_policy::copy)
        .def("SetVariantFallbacks", &_SetVariantFallbacks)
        .def("GetVariantFallbacks", &PcpCache::GetVariantFallbacks,
             nb::rv_policy::copy)
        .def("GetUsedLayers", [](const PcpCache &self) {
            return TfNbSequenceToList(self.GetUsedLayers());
        })
        .def("GetUsedLayersRevision", &PcpCache::GetUsedLayersRevision)
        .def("IsPayloadIncluded", &PcpCache::IsPayloadIncluded)
        .def("RequestPayloads", &_RequestPayloads)
        .def("RequestLayerMuting", &_RequestLayerMuting,
             nb::arg("layersToMute"),
             nb::arg("layersToUnmute"))
        .def("GetMutedLayers", &PcpCache::GetMutedLayers,
             nb::rv_policy::copy)
        .def("IsLayerMuted", 
             (bool(PcpCache::*)(const std::string&) const)
                &PcpCache::IsLayerMuted,
             nb::arg("layerIdentifier"))

        .def_prop_ro("layerStack", &PcpCache::GetLayerStack)
        .def_prop_ro("fileFormatTarget", &PcpCache::GetFileFormatTarget,
                     nb::rv_policy::copy)

        .def("ComputeLayerStack", &_ComputeLayerStack)
        .def("HasRootLayerStack",
             (bool (PcpCache::*)(PcpLayerStackPtr const &) const)
             &PcpCache::HasRootLayerStack)
        .def("UsesLayerStack", &PcpCache::UsesLayerStack)
        .def("ComputePrimIndex", &_ComputePrimIndex)
        .def("FindPrimIndex", &_FindPrimIndex)
        .def("ComputePropertyIndex", &_ComputePropertyIndex)
        .def("FindPropertyIndex", &_FindPropertyIndex)

        .def("ComputeRelationshipTargetPaths", 
             &_ComputeRelationshipTargetPaths,
             nb::arg("relPath"),
             nb::arg("localOnly") = false,
             nb::arg("stopProperty") = SdfSpecHandle(),
             nb::arg("includeStopProperty") = false)
        .def("ComputeAttributeConnectionPaths", 
             &_ComputeAttributeConnectionPaths,
             nb::arg("relPath"),
             nb::arg("localOnly") = false,
             nb::arg("stopProperty") = SdfSpecHandle(),
             nb::arg("includeStopProperty") = false)

        .def("FindSiteDependencies",
             &_FindSiteDependencies,
             nb::arg("siteLayerStack"),
             nb::arg("sitePath"),
             nb::arg("dependencyType") = TfEnum(PcpDependencyTypeAnyNonVirtual),
             nb::arg("recurseOnSite") = false,
             nb::arg("recurseOnIndex") = false,
             nb::arg("filterForExistingCachesOnly") = false)
        .def("FindAllLayerStacksUsingLayer",
             &PcpCache::FindAllLayerStacksUsingLayer,
             nb::rv_policy::copy)

        .def("IsInvalidAssetPath", &PcpCache::IsInvalidAssetPath)
        .def("IsInvalidSublayerIdentifier", 
             &PcpCache::IsInvalidSublayerIdentifier)

        .def("HasAnyDynamicFileFormatArgumentFieldDependencies", 
             &PcpCache::HasAnyDynamicFileFormatArgumentFieldDependencies)
        .def("HasAnyDynamicFileFormatArgumentAttributeDependencies", 
             &PcpCache::HasAnyDynamicFileFormatArgumentAttributeDependencies)
        .def("IsPossibleDynamicFileFormatArgumentField", 
             &PcpCache::IsPossibleDynamicFileFormatArgumentField)
        .def("IsPossibleDynamicFileFormatArgumentAttribute", 
             &PcpCache::IsPossibleDynamicFileFormatArgumentAttribute)
        .def("GetDynamicFileFormatArgumentDependencyData", 
             &PcpCache::GetDynamicFileFormatArgumentDependencyData,
             nb::rv_policy::reference)

        .def("GetPrimsUsingExpressionVariablesFromLayerStack",
             &PcpCache::GetPrimsUsingExpressionVariablesFromLayerStack,
             nb::arg("layerStack"),
             nb::rv_policy::copy)
        .def("GetExpressionVariablesFromLayerStackUsedByPrim",
             [](const PcpCache &self, const SdfPath &primIndexPath,
                const PcpLayerStackPtr &layerStack) {
                 return TfNbSequenceToList(
                     self.GetExpressionVariablesFromLayerStackUsedByPrim(
                         primIndexPath, layerStack));
             },
             nb::arg("layerStack"), nb::arg("primIndexPath"))

        .def("PrintStatistics", &PcpCache::PrintStatistics)
        .def("Reload", &_Reload)
        ;

}
