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


PXR_NAMESPACE_USING_DIRECTIVE

// Wrap this with a function so that add_property does not
// try to return an lvalue (and fail)
static TfEnum _GetErrorType(PcpErrorBasePtr const& err)
{
    return err->errorType;
}

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

    boost::python::class_<PcpErrorBase, boost::noncopyable, PcpErrorBasePtr>
        ("ErrorBase", "", boost::python::no_init)
        .add_property("errorType", _GetErrorType)
        .def("__str__", &PcpErrorBase::ToString)
        ;

    boost::python::class_<PcpErrorTargetPathBase, boost::noncopyable,
        boost::python::bases<PcpErrorBase>, PcpErrorTargetPathBasePtr >
        ("ErrorTargetPathBase", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorArcCycle, boost::python::bases<PcpErrorBase>, PcpErrorArcCyclePtr >
        ("ErrorArcCycle", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorArcPermissionDenied, boost::python::bases<PcpErrorBase>,
        PcpErrorArcPermissionDeniedPtr>
        ("ErrorArcPermissionDenied", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorCapacityExceeded, boost::python::bases<PcpErrorBase>,
        PcpErrorCapacityExceededPtr >
        ("ErrorCapacityExceeded", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInconsistentPropertyType, boost::python::bases<PcpErrorBase>,
        PcpErrorInconsistentPropertyTypePtr>
        ("ErrorInconsistentPropertyType", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInconsistentAttributeType, boost::python::bases<PcpErrorBase>,
        PcpErrorInconsistentAttributeTypePtr>
        ("ErrorInconsistentAttributeType", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInconsistentAttributeVariability, boost::python::bases<PcpErrorBase>,
        PcpErrorInconsistentAttributeVariabilityPtr>
        ("ErrorInconsistentAttributeVariability", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInternalAssetPath, boost::python::bases<PcpErrorBase>,
        PcpErrorInternalAssetPathPtr>
        ("ErrorInternalAssetPath", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidPrimPath, boost::python::bases<PcpErrorBase>,
        PcpErrorInvalidPrimPathPtr>
        ("ErrorInvalidPrimPath", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidAssetPathBase, boost::noncopyable, 
        boost::python::bases<PcpErrorBase>, PcpErrorInvalidAssetPathBasePtr>
        ("ErrorInvalidAssetPathBase", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidAssetPath, boost::python::bases<PcpErrorInvalidAssetPathBase>,
        PcpErrorInvalidAssetPathPtr>
        ("ErrorInvalidAssetPath", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorMutedAssetPath, boost::python::bases<PcpErrorInvalidAssetPathBase>,
        PcpErrorMutedAssetPathPtr>
        ("ErrorMutedAssetPath", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidInstanceTargetPath, boost::python::bases<PcpErrorTargetPathBase>,
        PcpErrorInvalidInstanceTargetPathPtr>
        ("ErrorInvalidInstanceTargetPath", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidExternalTargetPath, boost::python::bases<PcpErrorTargetPathBase>,
        PcpErrorInvalidExternalTargetPathPtr>
        ("ErrorInvalidExternalTargetPath", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidTargetPath, boost::python::bases<PcpErrorTargetPathBase>,
        PcpErrorInvalidTargetPathPtr>
        ("ErrorInvalidTargetPath", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidSublayerOffset, boost::python::bases<PcpErrorBase>,
        PcpErrorInvalidSublayerOffsetPtr>
        ("ErrorInvalidSublayerOffset", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidReferenceOffset, boost::python::bases<PcpErrorBase>,
        PcpErrorInvalidReferenceOffsetPtr>
        ("ErrorInvalidReferenceOffset", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidSublayerOwnership, boost::python::bases<PcpErrorBase>,
        PcpErrorInvalidSublayerOwnershipPtr>
        ("ErrorInvalidSublayerOwnership", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidSublayerPath, boost::python::bases<PcpErrorBase>,
           PcpErrorInvalidSublayerPathPtr>
        ("ErrorInvalidSublayerPath", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorInvalidVariantSelection, boost::python::bases<PcpErrorBase>,
           PcpErrorInvalidVariantSelectionPtr>
        ("ErrorInvalidVariantSelection", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorOpinionAtRelocationSource, boost::python::bases<PcpErrorBase>, 
           PcpErrorOpinionAtRelocationSourcePtr>
        ("ErrorOpinionAtRelocationSource", "", boost::python::no_init)
        ;
    
    boost::python::class_<PcpErrorPrimPermissionDenied, boost::python::bases<PcpErrorBase>,
        PcpErrorPrimPermissionDeniedPtr>
        ("ErrorPrimPermissionDenied", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorPropertyPermissionDenied, boost::python::bases<PcpErrorBase>,
        PcpErrorPropertyPermissionDeniedPtr>
        ("ErrorPropertyPermissionDenied", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorSublayerCycle, boost::python::bases<PcpErrorBase>, 
           PcpErrorSublayerCyclePtr>
        ("ErrorSublayerCycle", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorTargetPermissionDenied, boost::python::bases<PcpErrorTargetPathBase>,
        PcpErrorTargetPermissionDeniedPtr>
        ("ErrorTargetPermissionDenied", "", boost::python::no_init)
        ;

    boost::python::class_<PcpErrorUnresolvedPrimPath, boost::python::bases<PcpErrorBase>,
           PcpErrorUnresolvedPrimPathPtr>
        ("ErrorUnresolvedPrimPath", "", boost::python::no_init)
        ;

    // Register conversion for python list <-> vector<PcpErrorBasePtr>
    boost::python::to_python_converter< PcpErrorVector, 
                         TfPySequenceToPython<PcpErrorVector> >();
    TfPyContainerConversions::from_python_sequence<
        PcpErrorVector,
        TfPyContainerConversions::variable_capacity_policy >();
}

TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidVariantSelection)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorPropertyPermissionDenied)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorBase)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorTargetPathBase)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorArcCycle)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorArcPermissionDenied)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorCapacityExceeded)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInconsistentPropertyType)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInconsistentAttributeType)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInconsistentAttributeVariability)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInternalAssetPath)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidPrimPath)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidAssetPathBase)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidAssetPath)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorMutedAssetPath)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidInstanceTargetPath)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidExternalTargetPath)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidTargetPath)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidSublayerOffset)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidReferenceOffset)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidSublayerOwnership)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorInvalidSublayerPath)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorOpinionAtRelocationSource)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorPrimPermissionDenied)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorSublayerCycle)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorTargetPermissionDenied)
TF_REFPTR_CONST_VOLATILE_GET(PcpErrorUnresolvedPrimPath)
