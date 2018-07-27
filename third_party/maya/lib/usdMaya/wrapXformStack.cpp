//
// Copyright 2017 Pixar
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
#include "usdMaya/xformStack.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"

#include <maya/MEulerRotation.h>
#include <maya/MTransformationMatrix.h>

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/import.hpp>
#include <boost/python/raw_function.hpp>

using namespace std;
using boost::python::object;
using boost::python::class_;
using boost::python::no_init;
using boost::python::self;
using boost::python::extract;
using boost::python::return_value_policy;
using boost::python::return_by_value;
using boost::python::copy_const_reference;
using boost::python::reference_existing_object;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    // We wrap this class, instead of UsdMayaXformOpClassification directly, mostly
    // just so we can handle the .IsNull() -> None conversion
    class _PyXformOpClassification {
    public:
        _PyXformOpClassification(const UsdMayaXformOpClassification& opClass)
            : _opClass(opClass)
        {
        }

        bool operator == (const _PyXformOpClassification& other)
        {
            return _opClass == other._opClass;
        }

        TfToken const &GetName() const
        {
            return _opClass.GetName();
        }

        // In order to return wrapped UsdGeomXformOp::Type objects, we need to import
        // the UsdGeom module
        static void
        ImportUsdGeomOnce()
        {
            static std::once_flag once;
            std::call_once(once, [](){
                boost::python::import("pxr.UsdGeom");
            });
        }

        UsdGeomXformOp::Type GetOpType() const
        {
            ImportUsdGeomOnce();
            return _opClass.GetOpType();
        }

        bool IsInvertedTwin() const
        {
            return _opClass.IsInvertedTwin();
        }

        bool IsCompatibleType(UsdGeomXformOp::Type otherType) const
        {
            return _opClass.IsCompatibleType(otherType);
        }

        std::vector<TfToken> CompatibleAttrNames() const
        {
            return _opClass.CompatibleAttrNames();
        }

        // to-python conversion of const UsdMayaXformOpClassification.
        static PyObject*
        convert(const UsdMayaXformOpClassification& opClass) {
            TfPyLock lock;
            if (opClass.IsNull())
            {
                return boost::python::incref(Py_None);
            }
            else
            {
                object obj((_PyXformOpClassification(opClass)));
                // Incref because ~object does a decref
                return boost::python::incref(obj.ptr());
            }
        }

    private:
        UsdMayaXformOpClassification _opClass;
    };

    class _PyXformStack
    {
    public:
        static inline object
        convert_index(size_t index)
        {
            if (index == UsdMayaXformStack::NO_INDEX)
            {
                return object(); // return None (handles the incref)
            }
            return object(index);
        }

        // Don't want to make this into a generic conversion rule, via
        //    to_python_converter<UsdMayaXformStack::IndexPair, _PyXformStack>(),
        // because that would make this apply to ANY pair of unsigned ints, which
        // could be dangerous
        static object
        convert_index_pair(const UsdMayaXformStack::IndexPair& indexPair)
        {
            return boost::python::make_tuple(
                    convert_index(indexPair.first),
                    convert_index(indexPair.second));
        }

        static PyObject*
        convert(const UsdMayaXformStack::OpClassPair& opPair)
        {
            boost::python::tuple result = boost::python::make_tuple(
                    opPair.first, opPair.second);
            // Incref because ~object does a decref
            return boost::python::incref(result.ptr());
        }

        static const UsdMayaXformOpClassification&
        getitem(const UsdMayaXformStack& stack, long index)
        {
            auto raise_index_error = [] () {
                PyErr_SetString(PyExc_IndexError, "index out of range");
                boost::python::throw_error_already_set();
            };

            if (index < 0)
            {
                if (static_cast<size_t>(-index) > stack.GetSize()) raise_index_error();
                return stack[stack.GetSize() + index];
            }
            else
            {
                if (static_cast<size_t>(index) >= stack.GetSize()) raise_index_error();
                return stack[index];
            }
        }

        static object
        GetInversionTwins(const UsdMayaXformStack& stack)
        {
            boost::python::list result;
            for(const auto& idxPair : stack.GetInversionTwins())
            {
                result.append(convert_index_pair(idxPair));
            }
            return result;
        }

        static object
        FindOpIndex(
                const UsdMayaXformStack& stack,
                const TfToken& opName,
                bool isInvertedTwin=false)
        {
            return convert_index(stack.FindOpIndex(opName, isInvertedTwin));
        }

        static object
        FindOpIndexPair(
                const UsdMayaXformStack& stack,
                const TfToken& opName)
        {
            return convert_index_pair(stack.FindOpIndexPair(opName));
        }
    };
}



void wrapXformStack()
{
    class_<_PyXformOpClassification>("XformOpClassification", no_init)
        .def(self == self)
        .def("GetName", &_PyXformOpClassification::GetName,
                return_value_policy<return_by_value>())
        .def("GetOpType", &_PyXformOpClassification::GetOpType)
        .def("IsInvertedTwin", &_PyXformOpClassification::IsInvertedTwin)
        .def("IsCompatibleType", &_PyXformOpClassification::IsCompatibleType)
        .def("CompatibleAttrNames", &_PyXformOpClassification::CompatibleAttrNames)
        ;

    boost::python::to_python_converter<UsdMayaXformOpClassification,
            _PyXformOpClassification>();

    class_<UsdMayaXformStack>("XformStack", no_init)
        .def("GetOps", &UsdMayaXformStack::GetOps,
                return_value_policy<TfPySequenceToList>())
        .def("GetInversionTwins", &_PyXformStack::GetInversionTwins)
        .def("GetNameMatters", &UsdMayaXformStack::GetNameMatters)
        .def("__getitem__", &_PyXformStack::getitem,
                return_value_policy<return_by_value>())
        .def("__len__", &UsdMayaXformStack::GetSize)
        .def("GetSize", &UsdMayaXformStack::GetSize)
        .def("FindOpIndex", &_PyXformStack::FindOpIndex,
                (boost::python::arg("opName"), boost::python::arg("isInvertedTwin")=false))
        .def("FindOp", &UsdMayaXformStack::FindOp,
                (boost::python::arg("opName"), boost::python::arg("isInvertedTwin")=false),
                return_value_policy<copy_const_reference>())
        .def("FindOpIndexPair", &_PyXformStack::FindOpIndexPair)
        .def("FindOpPair", &UsdMayaXformStack::FindOpPair)
        .def("MatchingSubstack", &UsdMayaXformStack::MatchingSubstack)
        .def("MayaStack", &UsdMayaXformStack::MayaStack,
                return_value_policy<return_by_value>())
        .staticmethod("MayaStack")
        .def("CommonStack", &UsdMayaXformStack::CommonStack,
                return_value_policy<return_by_value>())
        .staticmethod("CommonStack")
        .def("MatrixStack", &UsdMayaXformStack::MatrixStack,
                return_value_policy<return_by_value>())
        .staticmethod("MatrixStack")
        .def("FirstMatchingSubstack", &UsdMayaXformStack::FirstMatchingSubstack)
        .staticmethod("FirstMatchingSubstack")
    ;

    boost::python::to_python_converter<UsdMayaXformStack::OpClassPair,
            _PyXformStack>();

    boost::python::to_python_converter<std::vector<UsdMayaXformStack::OpClass >,
        TfPySequenceToPython<std::vector<UsdMayaXformStack::OpClass > > >();

    TfPyContainerConversions::from_python_sequence<std::vector<UsdMayaXformStack const *>,
        TfPyContainerConversions::variable_capacity_policy >();
}
