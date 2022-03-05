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

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyClassMethod.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/weakPtr.h"

#include "pxr/base/tf/pyArg.h"
#include "pxr/base/tf/pyPolymorphic.h"

#include <boost/assign/list_of.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/pure_virtual.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/tuple.hpp>

#include <boost/smart_ptr.hpp>

#include <functional>
#include <string>
#include <vector>


PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

// Base
typedef TfWeakPtr<class Tf_TestBase> Tf_TestBasePtr;
typedef TfWeakPtr<const class Tf_TestBase> Tf_TestBaseConstPtr;
typedef TfRefPtr<class Tf_TestBase> Tf_TestBaseRefPtr;
class Tf_TestBase : public TfRefBase, public TfWeakBase {
  public:
    virtual ~Tf_TestBase() {}
    virtual std::string Virtual() const = 0;
    virtual void Virtual2() const = 0;
    virtual void Virtual3(std::string const &arg) = 0;

    virtual std::string Virtual4() const { return "cpp base"; }

    std::string VirtualCaller() const { return UnwrappedVirtual(); }
    virtual std::string UnwrappedVirtual() const = 0;
  protected:
    Tf_TestBase() {}
};

// Derived
typedef TfWeakPtr<class Tf_TestDerived> Tf_TestDerivedPtr;
typedef TfRefPtr<class Tf_TestDerived> Tf_TestDerivedRefPtr;
class Tf_TestDerived : public Tf_TestBase {
  public:
    // CODE_COVERAGE_OFF_GCOV_BUG  woot
    virtual ~Tf_TestDerived() {}
    // CODE_COVERAGE_ON_GCOV_BUG
    virtual std::string Virtual() const { return "cpp derived"; }
    virtual void Virtual2() const {}
    virtual void Virtual3(std::string const &arg) {
        printf("cpp derived v3! : %s\n", arg.c_str());
    }
    virtual std::string UnwrappedVirtual() const { return "cpp derived"; }

    static Tf_TestDerivedRefPtr Factory() {
        return TfCreateRefPtr(new Tf_TestDerived());
    }
    static Tf_TestDerivedRefPtr NullFactory() {
        return Tf_TestDerivedRefPtr();
    }
  protected:
    Tf_TestDerived() {}
};


static std::string TakesConstBase(Tf_TestBaseConstPtr base) {
    return base->Virtual();
}

static Tf_TestBaseConstPtr ReturnsConstBase(Tf_TestBaseConstPtr base) {
    return base;
}

static Tf_TestBasePtr ReturnsBase(Tf_TestBasePtr base) {
    return base;
}

static Tf_TestBaseRefPtr ReturnsBaseRefPtr(Tf_TestBasePtr base) {
    return base;
}

static boost::python::tuple TakesBase(Tf_TestBasePtr base) {
    base->Virtual3("hello from TakesConstBase");
    base->Virtual2();
    bool isDerived = TfDynamic_cast<Tf_TestDerivedPtr>(base);
    return boost::python::make_tuple(isDerived, base->Virtual());
}

static std::string TakesDerived(Tf_TestDerivedPtr derived) {
    derived->Virtual3("A call to virtual 3!");
    return derived->Virtual();
}

static void TakesReference(Tf_TestDerivedRefPtr const &derived) {}

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define< Tf_TestBase >();
    TfType::Define< Tf_TestDerived, TfType::Bases< Tf_TestBase > >();
}

////////////////////////////////

template <typename T = Tf_TestBase>
struct polymorphic_Tf_TestBase : public T, public TfPyPolymorphic<T> {
    typedef polymorphic_Tf_TestBase This;
    virtual std::string Virtual() const {
        return this->template CallPureVirtual<std::string>("Virtual")();
    }
    virtual void Virtual2() const {
        return this->template CallPureVirtual<void>("Virtual2")();
    }
    virtual void Virtual3(std::string const &arg) {
        return this->template CallPureVirtual<void>("Virtual3")(arg);
    }
    std::string default_Virtual4() const { return T::Virtual4(); }
    virtual std::string Virtual4() const {
        return this->CallVirtual("Virtual4", &This::default_Virtual4)();
    }
    virtual std::string UnwrappedVirtual() const {
        return this->template CallPureVirtual<std::string>("UnwrappedVirtual")();
    }

};

static std::string
callVirtual(Tf_TestBase * base)
{
    return base->VirtualCaller();
}

template <typename T = Tf_TestDerived>
struct polymorphic_Tf_TestDerived : public polymorphic_Tf_TestBase<T> {
    typedef polymorphic_Tf_TestDerived This;
    std::string default_Virtual() const { return T::Virtual(); }
    virtual std::string Virtual() const {
        return this->CallVirtual("Virtual", &This::default_Virtual)();
    }
    void default_Virtual2() const { return T::Virtual2(); }
    virtual void Virtual2() const {
        return this->CallVirtual("Virtual2", &This::default_Virtual2)();
    }
    void default_Virtual3(std::string const &arg) { return T::Virtual3(arg); }
    virtual void Virtual3(std::string const &arg) {
        return this->CallVirtual("Virtual3", &This::default_Virtual3)(arg);
    }
};


template <typename T>
static TfRefPtr<T>
__Ref_init__() {
    return TfCreateRefPtr(new T);
}


enum TfPyTestErrorCodes {
    TF_TEST_ERROR_1,
    TF_TEST_ERROR_2
};

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(TF_TEST_ERROR_1);
    TF_ADD_ENUM_NAME(TF_TEST_ERROR_2);
}

static void mightRaise(bool raise) {
    if (raise) {
        TF_ERROR(TF_TEST_ERROR_1, "Test error 1!");
        TF_ERROR(TF_TEST_ERROR_2, "Test error 2!");
    }
}


static void doErrors() {
    TF_ERROR(TF_TEST_ERROR_1, "TestError 1!");
    TF_ERROR(TF_TEST_ERROR_2, "TestError 2!");
    TF_CODING_ERROR("nonfatal coding error %d", 1);
    TF_RUNTIME_ERROR("a random runtime error %d", 2);
    TF_WARN("diagnostic warning %d", 3);
    TF_STATUS("status message %d", 4);
};


struct _TestStaticMethodError {
    static void Error() {
        TF_ERROR(TF_TEST_ERROR_1, "Test error 1!");
    }
};



////////////////////////////////
// Enums

enum Tf_TestEnum {
    Tf_Alpha = 3,
    Tf_Bravo,
    Tf_Charlie,
    Tf_Delta,
};

enum class Tf_TestScopedEnum {
    Hydrogen = 1,
    Helium,
    Lithium,
    Beryllium,
    Boron    
};

struct Tf_Enum {
    enum TestEnum2 {
        One = 1,
        Two,
        Three,
    };
    enum TestEnum3 {
        _Alpha = 100,
        _Beta,
        _Gamma,
    };
    enum class TestScopedEnum {
        Alef = 300,
        Bet,
        Gimel
    };
};


TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(Tf_Alpha, "A");
    TF_ADD_ENUM_NAME(Tf_Bravo, "B");
    TF_ADD_ENUM_NAME(Tf_Charlie, "C");
    TF_ADD_ENUM_NAME(Tf_Delta, "D");
}

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(Tf_Enum::One);
    TF_ADD_ENUM_NAME(Tf_Enum::Two);
    TF_ADD_ENUM_NAME(Tf_Enum::Three);
}

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(Tf_Enum::_Alpha);
    TF_ADD_ENUM_NAME(Tf_Enum::_Beta);
    TF_ADD_ENUM_NAME(Tf_Enum::_Gamma);
}

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(Tf_Enum::TestScopedEnum::Alef);
    TF_ADD_ENUM_NAME(Tf_Enum::TestScopedEnum::Bet);
    TF_ADD_ENUM_NAME(Tf_Enum::TestScopedEnum::Gimel);
}

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(Tf_TestScopedEnum::Hydrogen, "H");
    TF_ADD_ENUM_NAME(Tf_TestScopedEnum::Lithium, "Li");
    TF_ADD_ENUM_NAME(Tf_TestScopedEnum::Beryllium, "Be");
    TF_ADD_ENUM_NAME(Tf_TestScopedEnum::Boron, "B");
}

static void takesTfEnum(TfEnum const &e) {
    printf("got enum '%s' with value '%d'\n",
           TfEnum::GetName(e).c_str(), e.GetValueAsInt());
}

static TfEnum returnsTfEnum(TfEnum  const &e) {
    printf("returning enum '%s' with value '%d'\n",
           TfEnum::GetName(e).c_str(), e.GetValueAsInt());
    return e;
}

static void takesTestEnum(Tf_TestEnum e) {
    printf("got enum %d with name '%s'\n",
           (int)e, TfEnum::GetName(e).c_str());
}

static void takesTestEnum2(Tf_Enum::TestEnum2 e) {
    printf("got enum %d with name '%s'\n",
           (int)e, TfEnum::GetName(e).c_str());
}

static void registerInvalidEnum(boost::python::object &obj) {
    boost::python::scope s = obj;

    // This should be used to produce a coding error. The _Alpha value will
    // conflict with the corresponding (sanitized) enum name that was already
    // wrapped for Tf_TestEnum.
    TfPyWrapEnum<Tf_Enum::TestEnum3>();
}



////////////////////////////////
// Function callback stuff.

static void callback(std::function<void ()> const &f) {
    f();
}

static std::string stringCallback(std::function<std::string ()> const &f) {
    return f();
}

static std::string stringStringCallback(std::function<std::string (std::string)> const &f) {
    return f("c++ is calling...");
}

static std::string callUnboundInstance(std::function<std::string (std::string)> const &f,
                                  std::string const &str) {
    return f(str);
}

static TfStaticData<std::function<std::string ()> > _testCallback;

static void setTestCallback(std::function<std::string ()> const &func) {
    *_testCallback = func;
}

static std::string invokeTestCallback() {
    if (*_testCallback)
        return (*_testCallback)();
    return std::string();
}


////////////////////////////////
// Sending notice from C++ sender

static void sendTfNoticeWithSender(Tf_TestBasePtr const &base) {
    TfNotice().Send(base);
}

////////////////////////////////
// TfPyClassMethod()

class Tf_ClassWithClassMethod {
public:
    Tf_ClassWithClassMethod() {}
    virtual ~Tf_ClassWithClassMethod() {}
};

static boost::python::tuple
_TestClassMethod( boost::python::object & pyClassObj, const boost::python::object & callable )
{
    return boost::python::make_tuple(
        pyClassObj, TfPyCall<boost::python::object>(callable)() );
}

// ////////////////////////////////
// // keywords and overloading.


// static void f1(string const &a1, int x, int y) {
//     printf("f1 with %s, %d, %d\n", a1.c_str(), x, y);
// }

// static void f2(string const &a1, string const &a2, int x, int y) {
//     printf("f2 with %s, %s, %d, %d\n", a1.c_str(), a2.c_str(), x, y);
// }

static std::string
_ThrowCppException()
{
    // Take the lock.
    TfPyLock lock;
    // Release the lock.
    lock.BeginAllowThreads();
    // Generate an exception.
    throw std::logic_error("error");
    // Not necessary, but shows usage.
    lock.EndAllowThreads();
    return std::string();
}

static size_t TakesVecVecString(std::vector<std::vector<std::string> > arg)
{
    return arg.size();
}

////////////////////////////////
// TfPyArg and TfMakeConstructorWithVarArgs

TF_DECLARE_WEAK_AND_REF_PTRS(Tf_ClassWithVarArgInit);

class Tf_ClassWithVarArgInit : public TfRefBase, public TfWeakBase
{
public:
    bool allowExtraArgs;
    boost::python::tuple args;
    boost::python::dict kwargs;
};

static Tf_ClassWithVarArgInitRefPtr 
_MakeClassWithVarArgInit(bool allowExtraArgs, 
                         const boost::python::tuple& args, const boost::python::dict& kwargs)
{
    // To Python consumer, this class has 3 explicit optional arguments, named 
    // 'a', 'b', and 'c'.
    const TfPyArgs optionalArgs = boost::assign::list_of<>
        (TfPyArg("a", ""))
        (TfPyArg("b", ""))
        (TfPyArg("c", ""));

    const std::pair<boost::python::tuple, boost::python::dict> params = 
        TfPyProcessOptionalArgs(args, kwargs, optionalArgs, allowExtraArgs);

    Tf_ClassWithVarArgInitRefPtr rval = 
        TfCreateRefPtr(new Tf_ClassWithVarArgInit);
    rval->allowExtraArgs = allowExtraArgs;
    rval->args = params.first;
    rval->kwargs = params.second;

    return rval;
}


////////////////////////////////
// Bytearray conversion

static boost::python::object
_ConvertByteListToByteArray(const boost::python::list& byteList)
{
    std::vector<char> inputList;
    for (int i = 0; i < boost::python::len(byteList); ++i) {
        inputList.push_back(boost::python::extract<char>(byteList[i]));
    }

    return TfPyCopyBufferToByteArray(inputList.data(), inputList.size());
}


PXR_NAMESPACE_CLOSE_SCOPE

void wrapTf_TestTfPython()
{

//      def("f", f1, (arg("x")=3, arg("y")=4));
//      def("f", f2, (arg("x")=5, arg("y")=6));

    boost::python::def("_ConvertByteListToByteArray", _ConvertByteListToByteArray);

    boost::python::def("_sendTfNoticeWithSender", sendTfNoticeWithSender);
    
    boost::python::def("_callback", callback);
    boost::python::def("_stringCallback", stringCallback);
    TfPyFunctionFromPython<std::string (std::string)>();
    boost::python::def("_stringStringCallback", stringStringCallback);
    boost::python::def("_setTestCallback", setTestCallback);
    boost::python::def("_invokeTestCallback", invokeTestCallback);
    boost::python::def("_callUnboundInstance", callUnboundInstance);

    TfPyWrapEnum<Tf_TestEnum>();

    {
        boost::python::scope enumScope = boost::python::class_<Tf_Enum>("_Enum", boost::python::no_init);
        TfPyWrapEnum<Tf_Enum::TestEnum2>();
        TfPyWrapEnum<Tf_Enum::TestScopedEnum>();
    }

    TfPyWrapEnum<Tf_TestScopedEnum>();

    boost::python::def("_takesTfEnum", takesTfEnum);
    boost::python::def("_returnsTfEnum", returnsTfEnum);
    boost::python::def("_takesTestEnum", takesTestEnum);
    boost::python::def("_takesTestEnum2", takesTestEnum2);
    boost::python::def("_registerInvalidEnum", registerInvalidEnum);

    
    boost::python::def("_doErrors", doErrors);
    
    boost::python::def("_mightRaise", mightRaise);

    boost::python::def("_ThrowCppException", _ThrowCppException);

    boost::python::def("_TakesVecVecString", TakesVecVecString);
    
    boost::python::class_<_TestStaticMethodError>("_TestStaticMethodError", boost::python::no_init)
        .def("Error", &_TestStaticMethodError::Error)
        .staticmethod("Error")
        ;

    boost::python::def("_TakesReference", TakesReference);
    boost::python::def("_TakesConstBase", TakesConstBase);
    boost::python::def("_ReturnsConstBase", ReturnsConstBase);
    boost::python::def("_TakesBase", TakesBase);
    boost::python::def("_ReturnsBase", ReturnsBase);
    boost::python::def("_ReturnsBaseRefPtr", ReturnsBaseRefPtr);
    boost::python::def("_TakesDerived", TakesDerived);
    
    boost::python::def("_DerivedFactory", Tf_TestDerived::Factory,
        boost::python::return_value_policy<TfPyRefPtrFactory<> >());

    boost::python::def("_DerivedNullFactory", Tf_TestDerived::NullFactory,
        boost::python::return_value_policy<TfPyRefPtrFactory<> >());
    
    boost::python::class_<polymorphic_Tf_TestBase<>,
        TfWeakPtr<polymorphic_Tf_TestBase<> >, boost::noncopyable>
        ("_TestBase", boost::python::no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(__Ref_init__<polymorphic_Tf_TestBase<> >))
        .def("Virtual", boost::python::pure_virtual(&Tf_TestBase::Virtual))
        .def("Virtual2", boost::python::pure_virtual(&Tf_TestBase::Virtual2))
        .def("Virtual3", boost::python::pure_virtual(&Tf_TestBase::Virtual3))
        .def("Virtual4", &Tf_TestBase::Virtual4,
             &polymorphic_Tf_TestBase<>::default_Virtual4)
        .def("TestCallVirtual", &callVirtual)
        ;
 
    boost::python::class_<polymorphic_Tf_TestDerived<>,
        TfWeakPtr<polymorphic_Tf_TestDerived<> >,
        boost::python::bases<Tf_TestBase>, boost::noncopyable>
        ("_TestDerived", boost::python::no_init)
        .def(TfPyRefAndWeakPtr())
        .def("__init__",
             TfMakePyConstructor(__Ref_init__<polymorphic_Tf_TestDerived<> >))
        .def("Virtual", &Tf_TestDerived::Virtual,
             &polymorphic_Tf_TestDerived<>::default_Virtual)
        .def("Virtual2", &Tf_TestDerived::Virtual2,
             &polymorphic_Tf_TestDerived<>::default_Virtual2)
        .def("Virtual3", &Tf_TestDerived::Virtual3,
             &polymorphic_Tf_TestDerived<>::default_Virtual3)
        ;

    boost::python::class_<Tf_ClassWithClassMethod>("_ClassWithClassMethod", boost::python::init<>() )
        .def("Test", &_TestClassMethod)
        .def(TfPyClassMethod("Test"))
        ;

    boost::python::class_<Tf_ClassWithVarArgInit, Tf_ClassWithVarArgInitPtr>
        ("_ClassWithVarArgInit", boost::python::no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructorWithVarArgs(&_MakeClassWithVarArgInit))
        .add_property("allowExtraArgs", 
                      &Tf_ClassWithVarArgInit::allowExtraArgs)
        .add_property("args", &Tf_ClassWithVarArgInit::args)
        .add_property("kwargs", &Tf_ClassWithVarArgInit::kwargs)
        ;

}

TF_REFPTR_CONST_VOLATILE_GET(Tf_ClassWithVarArgInit)
TF_REFPTR_CONST_VOLATILE_GET(Tf_TestBase)
TF_REFPTR_CONST_VOLATILE_GET(Tf_TestDerived)
TF_REFPTR_CONST_VOLATILE_GET(polymorphic_Tf_TestBase<class Tf_TestBase>)
TF_REFPTR_CONST_VOLATILE_GET(polymorphic_Tf_TestDerived<class Tf_TestDerived>)
