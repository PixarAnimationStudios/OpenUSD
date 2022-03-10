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
#include "pxr/usd/usdSkel/cache.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"

#include <boost/python.hpp>
#include <boost/python/extract.hpp>



PXR_NAMESPACE_USING_DIRECTIVE


namespace pxrUsdUsdSkelWrapCache {


std::vector<UsdSkelBinding>
_ComputeSkelBindings(const UsdSkelCache& self,  
                     const UsdSkelRoot& skelRoot,
                     const Usd_PrimFlagsPredicate predicate)
{   
    std::vector<UsdSkelBinding> bindings;
    self.ComputeSkelBindings(skelRoot, &bindings, predicate);
    return bindings;
}


UsdSkelBinding
_ComputeSkelBinding(const UsdSkelCache& self,
                    const UsdSkelRoot& skelRoot,
                    const UsdSkelSkeleton& skel,
                    const Usd_PrimFlagsPredicate predicate)
{
    UsdSkelBinding binding;
    self.ComputeSkelBinding(skelRoot, skel, &binding, predicate);
    return binding;
}

} // namespace


void wrapUsdSkelCache()
{
    using This = UsdSkelCache;

    boost::python::class_<This>("Cache", boost::python::init<>())

        .def("Clear", &This::Clear)

        .def("Populate", &This::Populate,
             (boost::python::arg("skelRoot"), boost::python::arg("predicate")))

        .def("GetSkelQuery", &This::GetSkelQuery)
        
        .def("GetSkinningQuery", &This::GetSkinningQuery)

        .def("GetAnimQuery",
             (UsdSkelAnimQuery (UsdSkelCache::*)(const UsdPrim&) const)
             &This::GetAnimQuery,
             (boost::python::arg("prim")))

        .def("GetAnimQuery",
             (UsdSkelAnimQuery (UsdSkelCache::*)(const UsdSkelAnimation&) const)
             &This::GetAnimQuery,
             (boost::python::arg("anim")))

        .def("ComputeSkelBindings", &pxrUsdUsdSkelWrapCache::_ComputeSkelBindings,
             boost::python::return_value_policy<TfPySequenceToList>(),
             (boost::python::arg("skelRoot"), boost::python::arg("predicate")))

        .def("ComputeSkelBinding", &pxrUsdUsdSkelWrapCache::_ComputeSkelBinding,
             (boost::python::arg("skelRoot"), boost::python::arg("skel"), boost::python::arg("predicate")))
        ;
}            
