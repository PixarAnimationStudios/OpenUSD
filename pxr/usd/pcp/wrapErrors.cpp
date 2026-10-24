//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/base/tf/pyEnum.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Wrap this with a function so that add_property does not
// try to return an lvalue (and fail)
static TfEnum _GetErrorType(PcpErrorBase const& err)
{
    return err.errorType;
}

static PcpSite _GetRootSite(PcpErrorBase const& err)
{
    return err.rootSite;
}

} // anonymous namespace

void 
wrapErrors(nanobind::module_ &m)
{    
    namespace nb = nanobind;

    TfNbWrapEnum<PcpErrorType>(m, "ErrorType");

    // NOTE: Do not allow construction of these objects from Python.
    //       Boost Python's shared_ptr_from_python will attach a custom
    //       deleter that releases the PyObject, which requires the GIL.
    //       Therefore it's unsafe for C++ code to release a shared_ptr
    //       without holding the GIL.  That's impractical so we just
    //       ensure that we don't create error objects in Python.

    nb::class_<PcpErrorBase>(m, "ErrorBase", "")
        .def_prop_ro("errorType", _GetErrorType)
        .def_prop_ro("rootSite", _GetRootSite)
        .def("__str__", &PcpErrorBase::ToString)
        ;

    nb::class_<PcpErrorTargetPathBase, PcpErrorBase>(
        m, "ErrorTargetPathBase", "")
        ;

    nb::class_<PcpErrorRelocationBase, PcpErrorBase>(
        m, "ErrorRelocationBase", "")
        ;

    nb::class_<PcpErrorArcCycle, PcpErrorBase>(
        m, "ErrorArcCycle", "")
        ;

    nb::class_<PcpErrorArcPermissionDenied, PcpErrorBase>(
        m, "ErrorArcPermissionDenied", "")
        ;

    nb::class_<PcpErrorArcToProhibitedChild, PcpErrorBase>(
        m, "ErrorArcToProhibitedChild", "")
        ;

    nb::class_<PcpErrorCapacityExceeded, PcpErrorBase>(
        m, "ErrorCapacityExceeded", "")
        ;

    nb::class_<PcpErrorInconsistentPropertyType, PcpErrorBase>(
        m, "ErrorInconsistentPropertyType", "")
        ;

    nb::class_<PcpErrorInconsistentAttributeType, PcpErrorBase>(
        m, "ErrorInconsistentAttributeType", "")
        ;

    nb::class_<PcpErrorInconsistentAttributeVariability, PcpErrorBase>(
        m, "ErrorInconsistentAttributeVariability", "")
        ;

    nb::class_<PcpErrorInvalidPrimPath, PcpErrorBase>(
        m, "ErrorInvalidPrimPath", "")
        ;

    nb::class_<PcpErrorInvalidAssetPathBase, PcpErrorBase>(
        m, "ErrorInvalidAssetPathBase", "")
        ;

    nb::class_<PcpErrorInvalidAssetPath, PcpErrorInvalidAssetPathBase>(
        m, "ErrorInvalidAssetPath", "")
        ;

    nb::class_<PcpErrorMutedAssetPath, PcpErrorInvalidAssetPathBase>(
        m, "ErrorMutedAssetPath", "")
        ;

    nb::class_<PcpErrorInvalidInstanceTargetPath, PcpErrorTargetPathBase>(
        m, "ErrorInvalidInstanceTargetPath", "")
        ;

    nb::class_<PcpErrorInvalidExternalTargetPath, PcpErrorTargetPathBase>(
        m, "ErrorInvalidExternalTargetPath", "")
        ;

    nb::class_<PcpErrorInvalidTargetPath, PcpErrorTargetPathBase>(
        m, "ErrorInvalidTargetPath", "")
        ;

    nb::class_<PcpErrorInvalidSublayerOffset, PcpErrorBase>(
        m, "ErrorInvalidSublayerOffset", "")
        ;

    nb::class_<PcpErrorInvalidSublayerAndOffsetCount, PcpErrorBase>(
        m, "ErrorInvalidSublayerAndOffsetCount", "")
        ;

    nb::class_<PcpErrorInvalidReferenceOffset, PcpErrorBase>(
        m, "ErrorInvalidReferenceOffset", "")
        ;

    nb::class_<PcpErrorInvalidSublayerOwnership, PcpErrorBase>(
        m, "ErrorInvalidSublayerOwnership", "")
        ;

    nb::class_<PcpErrorInvalidSublayerPath, PcpErrorBase>(
        m, "ErrorInvalidSublayerPath", "")
        ;

    nb::class_<PcpErrorInvalidAuthoredRelocation, PcpErrorRelocationBase>(
        m, "ErrorInvalidAuthoredRelocation", "")
        ;

    nb::class_<PcpErrorInvalidConflictingRelocation, PcpErrorRelocationBase>(
        m, "ErrorInvalidConflictingRelocation", "")
        ;

    nb::class_<PcpErrorInvalidSameTargetRelocations, PcpErrorRelocationBase>(
        m, "ErrorInvalidSameTargetRelocations", "")
        ;

    nb::class_<PcpErrorOpinionAtRelocationSource, PcpErrorBase>(
        m, "ErrorOpinionAtRelocationSource", "")
        ;
    
    nb::class_<PcpErrorPrimPermissionDenied, PcpErrorBase>(
        m, "ErrorPrimPermissionDenied", "")
        ;

    nb::class_<PcpErrorPropertyPermissionDenied, PcpErrorBase>(
        m, "ErrorPropertyPermissionDenied", "")
        ;

    nb::class_<PcpErrorSublayerCycle, PcpErrorBase>(
        m, "ErrorSublayerCycle", "")
        ;

    nb::class_<PcpErrorTargetPermissionDenied, PcpErrorTargetPathBase>(
        m, "ErrorTargetPermissionDenied", "")
        ;

    nb::class_<PcpErrorUnresolvedPrimPath, PcpErrorBase>(
        m, "ErrorUnresolvedPrimPath", "")
        ;

    nb::class_<PcpErrorVariableExpressionError, PcpErrorBase>(
        m, "ErrorVariableExpressionError", "")
        ;
}
