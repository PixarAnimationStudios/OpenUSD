//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/base/tf/pyEnum.h"
#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

// Wrap this with a function so that add_property does not
// try to return an lvalue (and fail)
static TfEnum _GetErrorType(PcpErrorBasePtr const& err)
{
    return err->errorType;
}

static PcpSite _GetRootSite(PcpErrorBasePtr const& err)
{
    return err->rootSite;
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

    class_<PcpErrorBase, noncopyable, PcpErrorBasePtr>
        ("ErrorBase", "", no_init)
        .add_property("errorType", _GetErrorType)
        .add_property("rootSite", _GetRootSite)
        .def("__str__", &PcpErrorBase::ToString)
        ;

    class_<PcpErrorTargetPathBase, noncopyable,
        bases<PcpErrorBase>, PcpErrorTargetPathBasePtr >
        ("ErrorTargetPathBase", "", no_init)
        ;

    class_<PcpErrorRelocationBase, noncopyable,
        bases<PcpErrorBase>, PcpErrorRelocationBasePtr >
        ("ErrorRelocationBase", "", no_init)
        ;

    class_<PcpErrorArcCycle, bases<PcpErrorBase>, PcpErrorArcCyclePtr >
        ("ErrorArcCycle", "", no_init)
        ;

    class_<PcpErrorArcPermissionDenied, bases<PcpErrorBase>,
        PcpErrorArcPermissionDeniedPtr>
        ("ErrorArcPermissionDenied", "", no_init)
        ;

    class_<PcpErrorArcToProhibitedChild, bases<PcpErrorBase>,
        PcpErrorArcToProhibitedChildPtr>
        ("ErrorArcToProhibitedChild", "", no_init)
        ;

    class_<PcpErrorCapacityExceeded, bases<PcpErrorBase>,
        PcpErrorCapacityExceededPtr >
        ("ErrorCapacityExceeded", "", no_init)
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

    class_<PcpErrorInvalidPrimPath, bases<PcpErrorBase>,
        PcpErrorInvalidPrimPathPtr>
        ("ErrorInvalidPrimPath", "", no_init)
        ;

    class_<PcpErrorInvalidAssetPathBase, noncopyable, 
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

    class_<PcpErrorInvalidAuthoredRelocation, bases<PcpErrorRelocationBase>,
        PcpErrorInvalidAuthoredRelocationPtr>
        ("ErrorInvalidAuthoredRelocation", "", no_init)
        ;

    class_<PcpErrorInvalidConflictingRelocation, bases<PcpErrorRelocationBase>,
        PcpErrorInvalidConflictingRelocationPtr>
        ("ErrorInvalidConflictingRelocation", "", no_init)
        ;

    class_<PcpErrorInvalidSameTargetRelocations, bases<PcpErrorRelocationBase>,
        PcpErrorInvalidSameTargetRelocationsPtr>
        ("ErrorInvalidSameTargetRelocations", "", no_init)
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

    class_<PcpErrorVariableExpressionError, bases<PcpErrorBase>,
           PcpErrorVariableExpressionErrorPtr>
        ("ErrorVariableExpressionError", "", no_init)
        ;

    // Register conversion for python list <-> vector<PcpErrorBasePtr>
    to_python_converter< PcpErrorVector, 
                         TfPySequenceToPython<PcpErrorVector> >();
    TfPyContainerConversions::from_python_sequence<
        PcpErrorVector,
        TfPyContainerConversions::variable_capacity_policy >();
}
