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
#include <boost/python.hpp>

#include "pxr/usd/pcp/errors.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/base/tf/pyEnum.h"

using namespace boost::python;

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


#if defined(ARCH_COMPILER_MSVC)
// There is a bug in the compiler which means we have to provide this
// implementation. See here for more information:
// https://connect.microsoft.com/VisualStudio/Feedback/Details/2852624
namespace boost
{
    template<> 
    const volatile PcpErrorInvalidVariantSelection* 
        get_pointer(const volatile PcpErrorInvalidVariantSelection* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorPropertyPermissionDenied* 
        get_pointer(const volatile PcpErrorPropertyPermissionDenied* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorBase* 
        get_pointer(const volatile PcpErrorBase* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorTargetPathBase* 
        get_pointer(const volatile PcpErrorTargetPathBase* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorArcCycle* 
        get_pointer(const volatile PcpErrorArcCycle* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorArcPermissionDenied* 
        get_pointer(const volatile PcpErrorArcPermissionDenied* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInconsistentPropertyType* 
        get_pointer(const volatile PcpErrorInconsistentPropertyType* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInconsistentAttributeType* 
        get_pointer(const volatile PcpErrorInconsistentAttributeType* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInconsistentAttributeVariability* 
        get_pointer(const volatile PcpErrorInconsistentAttributeVariability* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInternalAssetPath* 
        get_pointer(const volatile PcpErrorInternalAssetPath* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidPrimPath* 
        get_pointer(const volatile PcpErrorInvalidPrimPath* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidAssetPathBase* 
        get_pointer(const volatile PcpErrorInvalidAssetPathBase* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidAssetPath* 
        get_pointer(const volatile PcpErrorInvalidAssetPath* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorMutedAssetPath* 
        get_pointer(const volatile PcpErrorMutedAssetPath* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidInstanceTargetPath* 
        get_pointer(const volatile PcpErrorInvalidInstanceTargetPath* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidExternalTargetPath* 
        get_pointer(const volatile PcpErrorInvalidExternalTargetPath* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidTargetPath* 
        get_pointer(const volatile PcpErrorInvalidTargetPath* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidSublayerOffset* 
        get_pointer(const volatile PcpErrorInvalidSublayerOffset* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidReferenceOffset* 
        get_pointer(const volatile PcpErrorInvalidReferenceOffset* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidSublayerOwnership* 
        get_pointer(const volatile PcpErrorInvalidSublayerOwnership* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorInvalidSublayerPath* 
        get_pointer(const volatile PcpErrorInvalidSublayerPath* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorOpinionAtRelocationSource* 
        get_pointer(const volatile PcpErrorOpinionAtRelocationSource* p)
    { 
        return p; 
    }
    template<> 
    const volatile PcpErrorPrimPermissionDenied* 
        get_pointer(const volatile PcpErrorPrimPermissionDenied* p)
    { 
        return p; 
    }

    template<> 
    const volatile PcpErrorSublayerCycle* 
        get_pointer(const volatile PcpErrorSublayerCycle* p)
    { 
        return p; 
    }

    template<> 
    const volatile PcpErrorTargetPermissionDenied* 
        get_pointer(const volatile PcpErrorTargetPermissionDenied* p)
    { 
        return p; 
    }

    template<> 
    const volatile PcpErrorUnresolvedPrimPath* 
        get_pointer(const volatile PcpErrorUnresolvedPrimPath* p)
    { 
        return p; 
    }
}
#endif 