//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyPolymorphic.h"

#include <functional>
#include <string>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

PXR_NAMESPACE_OPEN_SCOPE

// Base
typedef TfWeakPtr<class Tf_TestBase> Tf_TestBasePtr;
typedef TfWeakPtr<const class Tf_TestBase> Tf_TestBaseConstPtr;
typedef TfRefPtr<class Tf_TestBase> Tf_TestBaseRefPtr;
class Tf_TestBase : public TfRefBase, public TfWeakBase {
  public:
    virtual ~Tf_TestBase() {}
    virtual string Virtual() const = 0;
    virtual void Virtual2() const = 0;
    virtual void Virtual3(string const &arg) = 0;

    virtual string Virtual4() const { return "cpp base"; }

    string VirtualCaller() const { return UnwrappedVirtual(); }
    virtual string UnwrappedVirtual() const = 0;
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
    virtual string Virtual() const { return "cpp derived"; }
    virtual void Virtual2() const {}
    virtual void Virtual3(string const &arg) {
        printf("cpp derived v3! : %s\n", arg.c_str());
    }
    virtual string UnwrappedVirtual() const { return "cpp derived"; }

    static Tf_TestDerivedRefPtr Factory() {
        return TfCreateRefPtr(new Tf_TestDerived());
    }
    static Tf_TestDerivedRefPtr NullFactory() {
        return Tf_TestDerivedRefPtr();
    }
  protected:
    Tf_TestDerived() {}
};


static string TakesConstBase(Tf_TestBaseConstPtr base) {
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

static nanobind::tuple TakesBase(Tf_TestBasePtr base) {
    base->Virtual3("hello from TakesConstBase");
    base->Virtual2();
    bool isDerived = TfDynamic_cast<Tf_TestDerivedPtr>(base);
    return nanobind::make_tuple(isDerived, base->Virtual());
}

static string TakesDerived(Tf_TestDerivedPtr derived) {
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
    virtual string Virtual() const {
        return this->template CallPureVirtual<string>("Virtual")();
    }
    virtual void Virtual2() const {
        return this->template CallPureVirtual<void>("Virtual2")();
    }
    virtual void Virtual3(string const &arg) {
        return this->template CallPureVirtual<void>("Virtual3")(arg);
    }
    string default_Virtual4() const { return T::Virtual4(); }
    virtual string Virtual4() const {
        return this->CallVirtual("Virtual4", &This::default_Virtual4)();
    }
    virtual string UnwrappedVirtual() const {
        return this->template CallPureVirtual<string>("UnwrappedVirtual")();
    }

};

static string
callVirtual(Tf_TestBase * base)
{
    return base->VirtualCaller();
}

template <typename T = Tf_TestDerived>
struct polymorphic_Tf_TestDerived : public polymorphic_Tf_TestBase<T> {
    typedef polymorphic_Tf_TestDerived This;
    string default_Virtual() const { return T::Virtual(); }
    virtual string Virtual() const {
        return this->CallVirtual("Virtual", &This::default_Virtual)();
    }
    void default_Virtual2() const { return T::Virtual2(); }
    virtual void Virtual2() const {
        return this->CallVirtual("Virtual2", &This::default_Virtual2)();
    }
    void default_Virtual3(string const &arg) { return T::Virtual3(arg); }
    virtual void Virtual3(string const &arg) {
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

template <int I>
struct _TestErrorClass
{
    _TestErrorClass()
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from default constructor");
    }

    _TestErrorClass(std::string const&)
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from constructor");
    }
    
    static _TestErrorClass Create()
    {
        TfErrorMark m;
        auto obj = _TestErrorClass();
        m.Clear();
        return obj;
    }

    static void ClassMethod(nanobind::object& pyClassObj)
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from class method");
    }

    static void ClassMethod(nanobind::object& pyClassObj, std::string const&)
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from class method overload");
    }

    static void StaticMethod()
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from static method");
    }

    static void StaticMethod(std::string const&)
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from static method overload");
    }

    static std::string StaticGetter()
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from static property getter");
        return "StaticGetter";
    }

    static void StaticSetter(std::string const&)
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from static property setter");
    }

    void InstanceMethod() const
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from instance method");
    }

    void InstanceMethod(std::string const&) const
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from instance method overload");
    }

    std::string Getter() const
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from property getter");
        return "Getter";
    }

    void Setter(std::string const&)
    {
        TF_ERROR(TF_TEST_ERROR_1, "Error from property setter");
    }

    using Self = _TestErrorClass<I>;
    static nanobind::class_<Self> Wrap(nanobind::handle scope, char const* name)
    {
        namespace nb = nanobind;
        return nb::class_<Self>(scope, name)
            .def(nb::init<>())
            .def(nb::init<std::string const&>())

            .def_static("Create", &Self::Create)

            .def("ClassMethod", 
                (void(*)(nb::object&))&Self::ClassMethod)
            .def("ClassMethod", 
                (void(*)(nb::object&, std::string const&))&Self::ClassMethod)
            .def(TfPyClassMethod("ClassMethod"))

            .def_static("StaticMethod", 
                (void(*)())&Self::StaticMethod)
            .def_static("StaticMethod", 
                (void(*)(std::string const&))&Self::StaticMethod)

            .def("InstanceMethod", 
                (void(Self::*)() const)&Self::InstanceMethod)
            .def("InstanceMethod", 
                (void(Self::*)(std::string const&) const)&Self::InstanceMethod)

            .def_prop_rw("property", &Self::Getter, &Self::Setter)
            .def_prop_rw("property_2",
                +[](Self const& s) { return s.Getter(); },
                +[](Self& s, std::string const& v) { s.Setter(v); })
            
            .def_prop_rw_static("static_property", 
                +[](nb::handle) { return Self::StaticGetter(); },
                +[](nb::handle, std::string const& v) { Self::StaticSetter(v); })
            .def_prop_rw_static("static_property_2",
                +[](nb::handle) { return Self::StaticGetter(); },
                +[](nb::handle, std::string const& v) { Self::StaticSetter(v); })
            ;
    }

};

using _TestErrorClass1 = _TestErrorClass<1>;
using _TestErrorClass2 = _TestErrorClass<2>;

static void TestErrorFunction()
{
    TF_ERROR(TF_TEST_ERROR_1, "Error from function");
}

static void TestErrorFunction(std::string const&)
{
    TF_ERROR(TF_TEST_ERROR_1, "Error from function overload");
}

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
    enum class TestKeywords {
        // Example keywords specific to Python 3
        None,
        False,
        True,
        // Example keywords specific to Python 2
        print,
        // Example keywords common to Python 2 and 3
        import,
        global,
            
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

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(Tf_Enum::TestKeywords::None);
    TF_ADD_ENUM_NAME(Tf_Enum::TestKeywords::False, "False");
    TF_ADD_ENUM_NAME(Tf_Enum::TestKeywords::True, "True");
    TF_ADD_ENUM_NAME(Tf_Enum::TestKeywords::print, "print");
    TF_ADD_ENUM_NAME(Tf_Enum::TestKeywords::import);
    TF_ADD_ENUM_NAME(Tf_Enum::TestKeywords::global, "global");
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

static void registerInvalidEnum(nanobind::handle obj) {
    // This should be used to produce a coding error. The _Alpha value will
    // conflict with the corresponding (sanitized) enum name that was already
    // wrapped for Tf_TestEnum.
    TfNbWrapEnum<Tf_Enum::TestEnum3>(obj);
}



////////////////////////////////
// Function callback stuff.

static void callback(std::function<void ()> const &f) {
    f();
}

static string stringCallback(std::function<string ()> const &f) {
    return f();
}

static string stringStringCallback(std::function<string (string)> const &f) {
    return f("c++ is calling...");
}

static string callUnboundInstance(std::function<string (string)> const &f,
                                  string const &str) {
    return f(str);
}

static TfStaticData<std::function<string ()> > _testCallback;

static void setTestCallback(std::function<string ()> const &func) {
    *_testCallback = func;
}

static string invokeTestCallback() {
    if (*_testCallback)
        return (*_testCallback)();
    return string();
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

static nanobind::tuple
_TestClassMethod( nanobind::object & pyClassObj, const nanobind::object & callable )
{
    return nanobind::make_tuple(
        pyClassObj, TfPyCall<nanobind::object>(callable)() );
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
    nanobind::tuple args;
    nanobind::dict kwargs;
};

static Tf_ClassWithVarArgInitRefPtr 
_MakeClassWithVarArgInit(bool allowExtraArgs, 
                         const nanobind::tuple& args, const nanobind::dict& kwargs)
{
    // To Python consumer, this class has 3 explicit optional arguments, named 
    // 'a', 'b', and 'c'.
    TfPyArgs optionalArgs {{
        TfPyArg("a", ""),
        TfPyArg("b", ""),
        TfPyArg("c", "") }};

    const std::pair<nanobind::tuple, nanobind::dict> params = 
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

static nanobind::object
_ConvertByteListToByteArray(const nanobind::list& byteList)
{
    std::vector<char> inputList;
    for (size_t i = 0; i < byteList.size(); ++i) {
        inputList.push_back(nanobind::cast<char>(byteList[i]));
    }

    return TfPyCopyBufferToByteArray(inputList.data(), inputList.size());
}


PXR_NAMESPACE_CLOSE_SCOPE

void wrapTf_TestTfPython(nanobind::module_ &m)
{
    namespace nb = nanobind;

    m.def("_ConvertByteListToByteArray", _ConvertByteListToByteArray);

    m.def("_sendTfNoticeWithSender", sendTfNoticeWithSender);
    
    m.def("_callback", callback);
    m.def("_stringCallback", stringCallback);
    TfPyFunctionFromPython<string (string)>();
    m.def("_stringStringCallback", stringStringCallback);
    m.def("_setTestCallback", [](nb::object const &callable) {
        setTestCallback(
            TfPyFunctionFromPython<string ()>::ConstructFunction(callable.ptr()));
    });
    m.def("_invokeTestCallback", invokeTestCallback);
    m.def("_callUnboundInstance", [](nb::object const &callable, string const &str) {
        return callUnboundInstance(
            TfPyFunctionFromPython<string (string)>::ConstructFunction(callable.ptr()),
            str);
    });

    TfNbWrapEnum<Tf_TestEnum>(m);

    {
        nb::class_<Tf_Enum> enumScope(m, "_Enum");
        TfNbWrapEnum<Tf_Enum::TestEnum2>(enumScope);
        TfNbWrapEnum<Tf_Enum::TestScopedEnum>(enumScope);
        TfNbWrapEnum<Tf_Enum::TestKeywords>(enumScope);
    }

    TfNbWrapEnum<Tf_TestScopedEnum>(m);

    m.def("_takesTfEnum", takesTfEnum);
    m.def("_returnsTfEnum", returnsTfEnum);
    m.def("_takesTestEnum", takesTestEnum);
    m.def("_takesTestEnum2", takesTestEnum2);
    m.def("_registerInvalidEnum", registerInvalidEnum);

    
    m.def("_doErrors", doErrors);
    
    m.def("_mightRaise", mightRaise);

    m.def("_ThrowCppException", _ThrowCppException);

    m.def("_TakesVecVecString", TakesVecVecString);

    {
        m.def("_TestErrorFunction", (void(*)())&TestErrorFunction);
        m.def("_TestErrorFunction", 
            (void(*)(std::string const&))&TestErrorFunction);
        auto s = _TestErrorClass1::Wrap(m, "_TestErrorClass1");

        s.def_static("_TestErrorFunction", (void(*)())&TestErrorFunction);
        s.def_static("_TestErrorFunction", 
            (void(*)(std::string const&))&TestErrorFunction);
        _TestErrorClass2::Wrap(s, "_TestErrorClass2");        
    }

    m.def("_TakesReference", TakesReference);
    m.def("_TakesConstBase", TakesConstBase);
    m.def("_ReturnsConstBase", ReturnsConstBase);
    m.def("_TakesBase", TakesBase);
    m.def("_ReturnsBase", ReturnsBase);
    m.def("_ReturnsBaseRefPtr", ReturnsBaseRefPtr);
    m.def("_TakesDerived", TakesDerived);
    
    m.def("_DerivedFactory", Tf_TestDerived::Factory);

    m.def("_DerivedNullFactory", Tf_TestDerived::NullFactory);
    
    nb::class_<Tf_TestBase>(
        m, "_TestBase", nb::is_weak_referenceable(), nb::dynamic_attr())
        .def(TfNbRefAndWeakPtr())
        .def(TfNbMakePyConstructor(__Ref_init__<polymorphic_Tf_TestBase<> >))
        .def("Virtual", &Tf_TestBase::Virtual)
        .def("Virtual2", &Tf_TestBase::Virtual2)
        .def("Virtual3", &Tf_TestBase::Virtual3)
        .def("Virtual4", [](Tf_TestBase const &self) {
            return self.Tf_TestBase::Virtual4();
        })
        .def("TestCallVirtual", &callVirtual)
        ;
 
    nb::class_<Tf_TestDerived, Tf_TestBase>(
        m, "_TestDerived", nb::is_weak_referenceable(), nb::dynamic_attr())
        .def(TfNbRefAndWeakPtr())
        .def(TfNbMakePyConstructor(__Ref_init__<polymorphic_Tf_TestDerived<> >))
        .def("Virtual", [](Tf_TestDerived const &self) {
            return self.Tf_TestDerived::Virtual();
        })
        .def("Virtual2", [](Tf_TestDerived const &self) {
            self.Tf_TestDerived::Virtual2();
        })
        .def("Virtual3", [](Tf_TestDerived &self, string const &arg) {
            self.Tf_TestDerived::Virtual3(arg);
        })
        ;

    nb::class_<Tf_ClassWithClassMethod>(m, "_ClassWithClassMethod")
        .def(nb::init<>())
        .def("Test", &_TestClassMethod)
        .def(TfPyClassMethod("Test"))
        ;

    nb::class_<Tf_ClassWithVarArgInit>(
        m, "_ClassWithVarArgInit", nb::is_weak_referenceable(), nb::dynamic_attr())
        .def(TfNbRefAndWeakPtr())
        .def_static("__new__", [](nb::handle cls, nb::args args, nb::kwargs kwargs) {
            if (args.size() < 1) {
                TfPyThrowTypeError(
                    "Arguments to __init__ did not match C++ signature:\n"
                    "\t__init__(bool, ...)");
            }
            bool allowExtraArgs = nb::cast<bool>(args[0]);
            nb::tuple nbArgs = nb::steal<nb::tuple>(
                PyTuple_GetSlice(args.ptr(), 1, args.size()));
            nb::dict nbKwargs = (kwargs.is_valid() && kwargs.ptr())
                ? nb::borrow<nb::dict>(kwargs.ptr())
                : nb::dict();
            TfErrorMark mark;
            Tf_ClassWithVarArgInitRefPtr ptr =
                _MakeClassWithVarArgInit(allowExtraArgs, nbArgs, nbKwargs);
            if (TfPyConvertTfErrorsToPythonException(mark)) {
                throw nb::python_error();
            }
            Tf_ClassWithVarArgInitPtr weakPtr(get_pointer(ptr));
            nb::object self = nb::inst_reference(cls, get_pointer(ptr));
            Tf_PySetPythonIdentity(weakPtr, self.ptr());
            Tf_PyAddPythonOwnership(ptr, weakPtr.GetUniqueIdentifier(), self.ptr());
            return self;
        })
        .def("__init__", [](nb::handle, nb::args, nb::kwargs) {})
        .def_ro("allowExtraArgs", &Tf_ClassWithVarArgInit::allowExtraArgs)
        .def_ro("args", &Tf_ClassWithVarArgInit::args)
        .def_ro("kwargs", &Tf_ClassWithVarArgInit::kwargs)
        ;

}
