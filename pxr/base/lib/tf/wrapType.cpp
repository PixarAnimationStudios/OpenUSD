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
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyObjectFinder.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/weakBase.h"

#include <boost/python/converter/registry.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/object.hpp>
#include <boost/python/has_back_reference.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/overloads.hpp>
#include <boost/preprocessor.hpp>

#include <string>

using namespace boost::python;
using namespace std;

////////////////////////////////////////////////////////////////////////
// Python -> C++ TfType conversion

// Provide conversion of either string typenames or Python class objects
// to TfTypes.
static TfType
_GetTfTypeFromPython(PyObject *p)
{
    if (PyString_Check(p))
        return TfType::FindByName( extract<string>(p)() );
    else
        return TfType::FindByPythonClass( object(borrowed(p)) );
}

// A from-Python converter that uses the _GetTfTypeFromPython function.
struct _TfTypeFromPython
{
    _TfTypeFromPython() {
        converter::registry::insert(&_Convertible, &_Construct,
                                    type_id<TfType>());
    }
private:
    static void* _Convertible(PyObject *p) {
        if (_GetTfTypeFromPython(p).IsUnknown()) {
            TfPyThrowTypeError(
                TfStringPrintf("cannot convert %s to TfType; has that type "
                               "been defined as a TfType?",
                               TfPyRepr( object(borrowed(p) ) ).c_str()
                              ) );
        }
        return p;
    }

    static void _Construct(PyObject *p,
                           converter::rvalue_from_python_stage1_data *data)
    {
        void* const storage = ((converter::rvalue_from_python_storage
                                <TfType>*)data)->storage.bytes;

        TfType type = _GetTfTypeFromPython(p);
        new (storage) TfType(type);
        data->convertible = storage;
    }
};

////////////////////////////////////////////////////////////////////////
// Base C++ class to test Python subclassing.
#if 0

class Tf_TestCppBase : public TfWeakBase
{
public:
    virtual ~Tf_TestCppBase() {}
//    virtual int TestVirtual() = 0;
};

// Polymorphic wrapper.
class PolymorphicTestCppBase :
    public Tf_TestCppBase,
    public TfType::PyPolymorphicBase
{
public:
    static TfWeakPtr<PolymorphicTestCppBase> New() {
        // XXX leaks.
        return TfCreateWeakPtr(new PolymorphicTestCppBase());
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<Tf_TestCppBase>();
}

// Helper function to test TfType::Find(obj) on Python subclasses of
// wrapped C++ classes.
TfType
_TestFindType( Tf_TestCppBase & cppBase )
{
    printf("_TestFindType with '%s'\n",
           ArchGetDemangled(typeid(cppBase)).c_str());
    return TfType::Find(cppBase);
}

static void wrapTestCppBase()
{
    typedef Tf_TestCppBase This;
    typedef PolymorphicTestCppBase PolymorphicThis;

    class_<PolymorphicThis, TfWeakPtr<PolymorphicThis>, boost::noncopyable>
        ("TestCppBase", no_init)
        .def( TfPyWeakPtr() )
        .def( TfMakePyConstructor(&PolymorphicThis::New))
        ;

    def("_TestFindType", ::_TestFindType);
}
#endif

////////////////////////////////////////////////////////////////////////

static string
_Repr( const TfType & t )
{
    if (t.IsUnknown()) {
        return TF_PY_REPR_PREFIX + "Type.Unknown";
    } else {
        // Use TfPyRepr() to get Python-style escaping.
        return TF_PY_REPR_PREFIX +
            "Type.FindByName(" + TfPyRepr(t.GetTypeName()) + ")";
    }
}

static size_t
_Hash( const TfType & t )
{
    return TfHash()(t);
}

// These methods exist just to distinguish from equivalently named templates.
static bool
_IsA( const TfType & t1, const TfType & t2 )
{
    return t1.IsA(t2);
}

static TfType
_GetRoot()
{
    return TfType::GetRoot();
}

static TfType
_FindDerivedByName( TfType &t, const std::string & name )
{
    return t.FindDerivedByName(name);
}

static TfType
_FindByName( const std::string & name )
{
    return TfType::FindByName(name);
}

// Convert out parameter to return value
static std::vector<TfType>
_GetAllDerivedTypes( TfType & t )
{
    set<TfType> types;
    t.GetAllDerivedTypes(&types);
    return vector<TfType>( types.begin(), types.end() );
}

// Convert out parameter to return value
static std::vector<TfType>
_GetAllAncestorTypes( TfType & t )
{
    vector<TfType> types;
    t.GetAllAncestorTypes(&types);
    return types;
}

static TfType
_FindByPythonClass(const boost::python::object & classObj)
{
    // Guard against the potentially common mistake of calling Find() with a
    // string typename.  Rather than returning the unknown type (assuming
    // of course that we never declare Python's string type as a TfType),
    // we instead direct the caller to use FindByName().
    if (PyString_Check(classObj.ptr())) {
        TfPyThrowTypeError("String passed to Tf.Type.Find() -- you probably "
                           "want Tf.Type.FindByName() instead");
    }

    return TfType::FindByPythonClass(classObj);
}

static void
_DumpTypeHierarchyRecursive( TfType t, int depth=0 )
{
    std::string indent;
    for (int i=0; i < depth; ++i)
        indent += "    ";

    printf("%s%s\n", indent.c_str(), t.GetTypeName().c_str());
    std::vector<TfType> derived = t.GetDirectlyDerivedTypes();
    TF_FOR_ALL(it, derived) {
        _DumpTypeHierarchyRecursive( *it, depth+1 );
    }
}

static void
_DumpTypeHierarchy( TfType t )
{
    _DumpTypeHierarchyRecursive(t);
}

void wrapType()
{
    typedef TfType This;

    class_<This> classDef( "Type" );
    classDef
        .def( init<const TfType &>() )

        .def( self == self )
        .def( self != self )
        .def( self < self )
        .def( self > self )
        .def( self <= self )
        .def( self >= self )
        .def( "__repr__", &::_Repr)
        .def( "__hash__", &::_Hash)

        .def( "GetRoot", &::_GetRoot)
        .staticmethod("GetRoot")

        .def( "Find", &::_FindByPythonClass)
        .staticmethod("Find")

        .def( "FindByName", &::_FindByName)
        .staticmethod("FindByName")

        .def("FindDerivedByName", &::_FindDerivedByName)

        .def("IsA", &::_IsA )

        .add_property("isUnknown", &This::IsUnknown)
        .add_property("isEnumType", &This::IsEnumType)
        .add_property("isPlainOldDataType", &This::IsPlainOldDataType)
        .add_property("sizeof", &This::GetSizeof)

        .add_property("typeName",
            make_function( &This::GetTypeName,
                           return_value_policy<return_by_value>() ) )

        .add_property("pythonClass", &This::GetPythonClass)

        .add_property("baseTypes",
            make_function( &This::GetBaseTypes,
                return_value_policy< TfPySequenceToTuple >() ) )
        .add_property("derivedTypes",
            make_function( &This::GetDirectlyDerivedTypes,
                return_value_policy< TfPySequenceToTuple >() ) )

        .def("GetAliases", &This::GetAliases,
             return_value_policy< TfPySequenceToTuple >() )
        .def("GetAllDerivedTypes", &::_GetAllDerivedTypes,
             return_value_policy< TfPySequenceToTuple >() )
        .def("GetAllAncestorTypes", &::_GetAllAncestorTypes,
             return_value_policy< TfPySequenceToTuple >() )

        .def("Define", &TfType_DefinePythonTypeAndBases)
        .staticmethod("Define")

        .def("AddAlias", (void (TfType::*)(TfType, const std::string &) const)
                         &This::AddAlias)

        .def("_DumpTypeHierarchy", &::_DumpTypeHierarchy,
            "_DumpTypeHierarchy(TfType): "
            "Diagnostic method to print the type hierarchy beneath a given "
            "TfType.")
        .staticmethod("_DumpTypeHierarchy")
        ;

    // Create an attribute to use for the repr() for the Unknown type.
    // We do not wrap the GetUnknownType() method, favoring this instead.
    classDef.attr("Unknown") = TfType();

    // Register from-python conversions.
    _TfTypeFromPython();

    // Handle a sequence of TfTypes
    TfPyContainerConversions::from_python_sequence<
        std::vector< TfType >,
        TfPyContainerConversions::variable_capacity_policy >();

    TfPyContainerConversions::from_python_sequence<
        std::set<TfType> , 
        TfPyContainerConversions::set_policy >();

#if 0
    wrapTestCppBase();
#endif
}
