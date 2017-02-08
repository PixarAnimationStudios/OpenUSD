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
#include "pxr/usd/pcp/errors.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/base/tf/pyEnum.h"
#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

void 
wrapErrors()
{    
    TfPyWrapEnum<PcpErrorType>("ErrorType");

    // NOTE: Do not allow construction of these objects from Python.
    //       Boost Python's shared_ptr_from_python will attach a custom
    //       deleter that releases the PyObject, which requires the GIL.
    //       Therefore it's unsafe for C++ code to release a shared_ptr
    //       without holding the GIL.  That's impractical so we just
    //       ensure that we don't create error objects in Python.

    class_<PcpErrorBase, boost::noncopyable, PcpErrorBasePtr>
        ("ErrorBase", "", no_init)
        .add_property("errorType", &PcpErrorBase::errorType)
        .def("__str__", &PcpErrorBase::ToString)
        ;

    class_<PcpErrorTargetPathBase, boost::noncopyable,
        bases<PcpErrorBase>, PcpErrorTargetPathBasePtr >
        ("ErrorTargetPathBase", "", no_init)
        ;

    class_<PcpErrorArcCycle, bases<PcpErrorBase>, PcpErrorArcCyclePtr >
        ("ErrorArcCycle", "", no_init)
        ;

    class_<PcpErrorArcPermissionDenied, bases<PcpErrorBase>,
        PcpErrorArcPermissionDeniedPtr>
        ("ErrorArcPermissionDenied", "", no_init)
        ;

    class_<PcpErrorInconsistentPropertyType, bases<PcpErrorBase>,
        PcpErrorInconsistentPropertyTypePtr>
        ("ErrorInconsistentPropertyType", "", no_init)
        ;

    class_<PcpErrorInconsistentAttributeType, bases<PcpErrorBase>,
        PcpErrorInconsistentAttributeTypePtr>
        ("ErrorInconsistentAttributeType", "", no_init)
        ;

    class_<PcpErrorInconsistentAttributeVariability, bases<PcpErrorBase>,
        PcpErrorInconsistentAttributeVariabilityPtr>
        ("ErrorInconsistentAttributeVariability", "", no_init)
        ;

    class_<PcpErrorInternalAssetPath, bases<PcpErrorBase>,
        PcpErrorInternalAssetPathPtr>
        ("ErrorInternalAssetPath", "", no_init)
        ;

    class_<PcpErrorInvalidPrimPath, bases<PcpErrorBase>,
        PcpErrorInvalidPrimPathPtr>
        ("ErrorInvalidPrimPath", "", no_init)
        ;

    class_<PcpErrorInvalidAssetPathBase, boost::noncopyable, 
        bases<PcpErrorBase>, PcpErrorInvalidAssetPathBasePtr>
        ("ErrorInvalidAssetPathBase", "", no_init)
        ;

    class_<PcpErrorInvalidAssetPath, bases<PcpErrorInvalidAssetPathBase>,
        PcpErrorInvalidAssetPathPtr>
        ("ErrorInvalidAssetPath", "", no_init)
        ;

    class_<PcpErrorMutedAssetPath, bases<PcpErrorInvalidAssetPathBase>,
        PcpErrorMutedAssetPathPtr>
        ("ErrorMutedAssetPath", "", no_init)
        ;

    class_<PcpErrorInvalidInstanceTargetPath, bases<PcpErrorTargetPathBase>,
        PcpErrorInvalidInstanceTargetPathPtr>
        ("ErrorInvalidInstanceTargetPath", "", no_init)
        ;

    class_<PcpErrorInvalidExternalTargetPath, bases<PcpErrorTargetPathBase>,
        PcpErrorInvalidExternalTargetPathPtr>
        ("ErrorInvalidExternalTargetPath", "", no_init)
        ;

    class_<PcpErrorInvalidTargetPath, bases<PcpErrorTargetPathBase>,
        PcpErrorInvalidTargetPathPtr>
        ("ErrorInvalidTargetPath", "", no_init)
        ;

    class_<PcpErrorInvalidSublayerOffset, bases<PcpErrorBase>,
        PcpErrorInvalidSublayerOffsetPtr>
        ("ErrorInvalidSublayerOffset", "", no_init)
        ;

    class_<PcpErrorInvalidReferenceOffset, bases<PcpErrorBase>,
        PcpErrorInvalidReferenceOffsetPtr>
        ("ErrorInvalidReferenceOffset", "", no_init)
        ;

    class_<PcpErrorInvalidSublayerOwnership, bases<PcpErrorBase>,
        PcpErrorInvalidSublayerOwnershipPtr>
        ("ErrorInvalidSublayerOwnership", "", no_init)
        ;

    class_<PcpErrorInvalidSublayerPath, bases<PcpErrorBase>,
           PcpErrorInvalidSublayerPathPtr>
        ("ErrorInvalidSublayerPath", "", no_init)
        ;

    class_<PcpErrorInvalidVariantSelection, bases<PcpErrorBase>,
           PcpErrorInvalidVariantSelectionPtr>
        ("ErrorInvalidVariantSelection", "", no_init)
        ;

    class_<PcpErrorOpinionAtRelocationSource, bases<PcpErrorBase>, 
           PcpErrorOpinionAtRelocationSourcePtr>
        ("ErrorOpinionAtRelocationSource", "", no_init)
        ;
    
    class_<PcpErrorPrimPermissionDenied, bases<PcpErrorBase>,
        PcpErrorPrimPermissionDeniedPtr>
        ("ErrorPrimPermissionDenied", "", no_init)
        ;

    class_<PcpErrorPropertyPermissionDenied, bases<PcpErrorBase>,
        PcpErrorPropertyPermissionDeniedPtr>
        ("ErrorPropertyPermissionDenied", "", no_init)
        ;

    class_<PcpErrorSublayerCycle, bases<PcpErrorBase>, 
           PcpErrorSublayerCyclePtr>
        ("ErrorSublayerCycle", "", no_init)
        ;

    class_<PcpErrorTargetPermissionDenied, bases<PcpErrorTargetPathBase>,
        PcpErrorTargetPermissionDeniedPtr>
        ("ErrorTargetPermissionDenied", "", no_init)
        ;

    class_<PcpErrorUnresolvedPrimPath, bases<PcpErrorBase>,
           PcpErrorUnresolvedPrimPathPtr>
        ("ErrorUnresolvedPrimPath", "", no_init)
        ;

    // Register conversion for python list <-> vector<PcpErrorBasePtr>
    to_python_converter< PcpErrorVector, 
                         TfPySequenceToPython<PcpErrorVector> >();
    TfPyContainerConversions::from_python_sequence<
        PcpErrorVector,
        TfPyContainerConversions::variable_capacity_policy >();
}

PXR_NAMESPACE_CLOSE_SCOPE
