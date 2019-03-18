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
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/notice.h"
#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyUtils.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/typeNotice.h"

#include <set>

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

enum _TestEnum {
    A, B, C
};

TF_DECLARE_WEAK_AND_REF_PTRS( CountedClass );

class CountedClass : public TfRefBase, public TfWeakBase {
public:
    static CountedClassRefPtr New() {
	return TfCreateRefPtr(new CountedClass());
    }

    static CountedClassRefPtr New(int initialVal) {
	return TfCreateRefPtr(new CountedClass(initialVal));
    }

    int GetNumber() const { return _number; }
    void SetNumber(int x) { _number = x; }

private:
    CountedClass() :
        _number(0)
    {
    }

    CountedClass(int initialVal) :
        _number(initialVal)
    {
    }

    CountedClass(const CountedClass &c) : 
        _number(c._number)
    {
    }

    int _number;
};

class CountedClassFactory : public TfType::FactoryBase {
public:
    static CountedClassRefPtr New() {
	return CountedClass::New();
    }

    static CountedClassRefPtr New(int initialVal) {
	return CountedClass::New(initialVal);
    }
};


TF_DECLARE_WEAK_PTRS(SingleClass);

class SingleClass : public TfWeakBase {
public:
    static SingleClass& GetInstance() {
	return TfSingleton<SingleClass>::GetInstance();
    }

    SingleClass() :
        _number(0)
    {
    }

    int GetNumber() const { return _number; }
    void SetNumber(int x) { _number = x; }

private:
    int _number;

    friend class TfSingleton<SingleClass>;
};

TF_INSTANTIATE_SINGLETON(SingleClass);


class ConcreteClass {
public:
    ConcreteClass() {number = 0;}
    ConcreteClass(int n) {number = n;}
    virtual ~ConcreteClass();
    virtual void ConcreteFunction() {}
    int number;
};

ConcreteClass::~ConcreteClass()
{
}

class IAbstractClass {
public:
    virtual ~IAbstractClass();
    virtual void AbstractFunction() = 0;
};

IAbstractClass::~IAbstractClass()
{
}

class ChildClass: public ConcreteClass, public IAbstractClass {
public:
    ChildClass() {number = 0;}
    ChildClass(int n) {number = n;}
    ChildClass(const ChildClass& c) {number = c.number * -1;}
    virtual void ConcreteFunction() {}
    virtual void AbstractFunction() {}
};

class GrandchildClass: public ChildClass {
public:
    virtual void ConcreteFunction() {}
    virtual void AbstractFunction() {}
};

// We'll never explicitly lookup a TfType for this class, but
// it should be initialized when we call GetDirectlyDerivedTypes() for Child.
class OtherGrandchildClass: public ChildClass {
public:
    virtual void ConcreteFunction() {}
    virtual void AbstractFunction() {}
};

class UnknownClass {
};

class SomeClassA : public ConcreteClass {
};

class SomeClassB : public IAbstractClass {
};

template <typename T>
T* _New() { return new T; }

class _NoticeListener : public TfWeakBase
{
public:
    _NoticeListener(std::set<TfType> *seenNotices) :
        _seenNotices(seenNotices)
    {
        TfNotice::Register( TfCreateWeakPtr(this),
                            & _NoticeListener::_HandleTypeDeclaredNotice );
    }

    void _HandleTypeDeclaredNotice( const TfTypeWasDeclaredNotice & n ) {
        TF_AXIOM( !n.GetType().IsUnknown() );
        _seenNotices->insert( n.GetType() );
    }

private:
    // Set of types we have seen notices for.
    std::set<TfType> *_seenNotices;
};

template <class T>
struct TfTest_PtrFactory : public TfType::FactoryBase {
    T* New() { return new T; }
};
template <class T>
struct TfTest_RefPtrFactory : public TfType::FactoryBase {
    TfRefPtr<T> New() { return T::New(); }
};
template <class T>
struct TfTest_SingletonFactory : public TfType::FactoryBase {
    T* New() { return &T::GetInstance(); }
};

TF_REGISTRY_FUNCTION(TfType)
{
    // Define our types.
    // Check that we get TfTypeWasDeclaredNotice along the way.
    std::set<TfType> typesWeHaveSeenNoticesFor;
    _NoticeListener listener(&typesWeHaveSeenNoticesFor);

    TfType t1 = TfType::Define<CountedClass>();
    t1.SetFactory<TfTest_RefPtrFactory<CountedClassFactory> >();
    TF_AXIOM(typesWeHaveSeenNoticesFor.count( TfType::Find<CountedClass>()));

    TfType ts = TfType::Define<SingleClass>();
    ts.SetFactory<TfTest_SingletonFactory<SingleClass> >();

    TF_AXIOM(typesWeHaveSeenNoticesFor.count( TfType::Find<SingleClass>()));

    TfType t2 = TfType::Define<ConcreteClass>();
    t2.SetFactory<TfTest_PtrFactory<ConcreteClass> >();

    TF_AXIOM(typesWeHaveSeenNoticesFor.count( TfType::Find<ConcreteClass>()));

    TfType::Define<IAbstractClass>();
    TF_AXIOM(typesWeHaveSeenNoticesFor.count( TfType::Find<IAbstractClass>()));

    TfType t3 = TfType::Define<ChildClass,
                   TfType::Bases< ConcreteClass, IAbstractClass > >();
    t3.SetFactory<TfTest_PtrFactory<ChildClass> >();

    TF_AXIOM(typesWeHaveSeenNoticesFor.count( TfType::Find<ChildClass>()));

    TfType::Define<GrandchildClass,
                   TfType::Bases< ChildClass > >()
        ;
    TF_AXIOM(typesWeHaveSeenNoticesFor.count( TfType::Find<GrandchildClass>()));

    TfType::Define<OtherGrandchildClass,
                   TfType::Bases< ChildClass > >()
        ;
    TF_AXIOM(typesWeHaveSeenNoticesFor
           .count( TfType::Find<OtherGrandchildClass>() ) );
}

static bool
Test_TfType()
{
    TfType tUnknown;
    TfType tRoot = TfType::GetRoot();
    TfType tConcrete = TfType::Find<ConcreteClass>();
    TfType tAbstract = TfType::Find<IAbstractClass>();
    TfType tChild = TfType::Find<ChildClass>();
    TfType tGrandchild = TfType::Find<GrandchildClass>();
    TfType tCounted = TfType::Find<CountedClass>();
    TfType tSingle = TfType::Find<SingleClass>();
    const size_t numKnownTypes = 7;

    ////////////////////////////////////////////////////////////////////////
    // IsUnknown()

    TF_AXIOM( tUnknown.IsUnknown() );
    TF_AXIOM( !tRoot.IsUnknown() );
    TF_AXIOM( !tChild.IsUnknown() );
    TF_AXIOM( !tAbstract.IsUnknown() );
    TF_AXIOM( !tChild.IsUnknown() );
    TF_AXIOM( !tGrandchild.IsUnknown() );
    TF_AXIOM( !tCounted.IsUnknown() );
    TF_AXIOM( !tSingle.IsUnknown() );

    ////////////////////////////////////////////////////////////////////////
    // All types should be distinct.

    std::set<TfType> knownTypeSet;
    knownTypeSet.insert( tRoot );
    knownTypeSet.insert( tConcrete );
    knownTypeSet.insert( tAbstract );
    knownTypeSet.insert( tChild );
    knownTypeSet.insert( tGrandchild );
    knownTypeSet.insert( tCounted );
    knownTypeSet.insert( tSingle );
    TF_AXIOM( knownTypeSet.size() == numKnownTypes );

    // Now include unknown type
    std::set<TfType> allTypeSet = knownTypeSet;
    allTypeSet.insert( tUnknown );
    TF_AXIOM( allTypeSet.size() == numKnownTypes+1 );

    // Expect types to be unique
    TF_AXIOM( allTypeSet.count( tUnknown ) == 1 );
    TF_AXIOM( allTypeSet.count( tRoot ) == 1 );
    TF_AXIOM( allTypeSet.count( tConcrete ) == 1 );
    TF_AXIOM( allTypeSet.count( tAbstract ) == 1 );
    TF_AXIOM( allTypeSet.count( tChild ) == 1 );
    TF_AXIOM( allTypeSet.count( tGrandchild ) == 1 );
    TF_AXIOM( allTypeSet.count( tCounted ) == 1 );
    TF_AXIOM( allTypeSet.count( tSingle ) == 1 );

    ////////////////////////////////////////////////////////////////////////
    // All typeNames should be distinct.

    std::set<std::string> typeNameSet;
    TF_FOR_ALL(it, allTypeSet)
        typeNameSet.insert( it->GetTypeName() );
    TF_AXIOM( typeNameSet.size() == allTypeSet.size() );
    
    ////////////////////////////////////////////////////////////////////////
    // Test IsA

    // IsA<Unknown> -> error
    {
        TfErrorMark m;
        m.SetMark();
        TF_AXIOM( !tUnknown.IsA(tUnknown) );
        m.Clear();
    }

    TF_FOR_ALL(it, knownTypeSet) {
        TF_AXIOM( it->IsA(tRoot) );
        TF_AXIOM( it->IsA(*it) );

        // IsA<Unknown> -> error
        {
            TfErrorMark m;
            m.SetMark();
            TF_AXIOM( !it->IsA(tUnknown) );
            TF_AXIOM( !it->IsA<UnknownClass>() );
            m.Clear();
        }
    }

    TF_AXIOM( tChild.IsA(tConcrete) );
    TF_AXIOM( tChild.IsA(tAbstract) );
    TF_AXIOM( tChild.IsA<ConcreteClass>() );
    TF_AXIOM( tChild.IsA<IAbstractClass>() );

    TF_AXIOM( tConcrete.IsA<ConcreteClass>() );
    TF_AXIOM( !tConcrete.IsA<ChildClass>() );

    TF_AXIOM( tAbstract.IsA(tAbstract) );
    TF_AXIOM( !tAbstract.IsA(tChild) );

    TF_AXIOM( tGrandchild.IsA(tAbstract) );
    TF_AXIOM( tGrandchild.IsA(tConcrete) );
    TF_AXIOM( tGrandchild.IsA(tChild) );
    TF_AXIOM( tGrandchild.IsA<IAbstractClass>() );
    TF_AXIOM( tGrandchild.IsA<ConcreteClass>() );
    TF_AXIOM( tGrandchild.IsA<ChildClass>() );

    ////////////////////////////////////////////////////////////////////////
    // Test GetTypeid()

    TF_AXIOM( TfSafeTypeCompare(tRoot.GetTypeid(), typeid(void)) );
    TF_AXIOM( TfSafeTypeCompare(tConcrete.GetTypeid(), typeid(ConcreteClass)) );
    TF_AXIOM( TfSafeTypeCompare(tAbstract.GetTypeid(), typeid(IAbstractClass)) );
    TF_AXIOM( TfSafeTypeCompare(tChild.GetTypeid(), typeid(ChildClass)) );
    TF_AXIOM( TfSafeTypeCompare(tGrandchild.GetTypeid(),
                              typeid(GrandchildClass)) );

    ////////////////////////////////////////////////////////////////////////
    // Test Find()

    ConcreteClass concreteObj;
    ChildClass childObj;
    TF_AXIOM( tConcrete == TfType::Find(concreteObj) ); 
    TF_AXIOM( tConcrete != TfType::Find(childObj) ); 
    TF_AXIOM( tChild == TfType::Find(childObj) ); 
    TF_AXIOM( tChild != TfType::Find(concreteObj) ); 
    TF_AXIOM( tAbstract == TfType::FindByName("IAbstractClass") );
    TF_AXIOM( tConcrete == TfType::FindByName("ConcreteClass") );
    TF_AXIOM( tChild == TfType::FindByName("ChildClass") );
    TF_AXIOM( tAbstract == TfType::Find(tAbstract.GetTypeid()) );
    TF_AXIOM( tChild == TfType::Find(tChild.GetTypeid()) );

    // Test Find() for pointers to polymorphic types:
    // Raw pointer (T*)
    TF_AXIOM( tConcrete == TfType::Find( &concreteObj ) );
    // TfRefPtr
    CountedClassRefPtr countedRef = CountedClass::New();
    TF_AXIOM( tCounted == TfType::Find( countedRef ) );
    // TfWeakPtr
    CountedClassPtr countedWeak(countedRef);
    TF_AXIOM( tCounted == TfType::Find( countedWeak ) );

   //////////////////////////////////////////////////////////////////////// 
    // Test Get{Base,Derived}Types()

    TF_AXIOM( tRoot.GetBaseTypes().empty() );
    TF_AXIOM( !tRoot.GetDirectlyDerivedTypes().empty() );

    TF_AXIOM( tUnknown.GetBaseTypes().empty() );
    TF_AXIOM( tUnknown.GetDirectlyDerivedTypes().empty() );

    std::vector<TfType> rootDerivatives = tRoot.GetDirectlyDerivedTypes();
    std::vector<TfType> abstractParents = tAbstract.GetBaseTypes();
    std::vector<TfType> concreteParents = tConcrete.GetBaseTypes();
    std::vector<TfType> childParents = tChild.GetBaseTypes();
    std::vector<TfType> childDerivatives = tChild.GetDirectlyDerivedTypes();
    std::vector<TfType> grandchildParents = tGrandchild.GetBaseTypes();
    std::vector<TfType> grandchildDerivatives = tGrandchild.GetDirectlyDerivedTypes();

    // Test inheritance within our known hierarchy
    TF_AXIOM( childParents.size() == 2 && childDerivatives.size() == 2 );
    TF_AXIOM(
        ((childParents[0] == tConcrete && childParents[1] == tAbstract) ||
        ((childParents[0] == tAbstract && childParents[1] == tConcrete))) );
    TF_AXIOM(childDerivatives[0] == tGrandchild );
    TF_AXIOM(grandchildParents.size() == 1 && grandchildDerivatives.empty());
    TF_AXIOM(grandchildParents[0] == tChild);

    // These types should inherit the root directly
    TF_AXIOM( tAbstract.GetBaseTypes() == std::vector<TfType>(1, tRoot) );
    TF_AXIOM( tConcrete.GetBaseTypes() == std::vector<TfType>(1, tRoot) );
    TF_AXIOM( std::find( rootDerivatives.begin(), rootDerivatives.end(),
                       tAbstract ) != rootDerivatives.end() );
    TF_AXIOM( std::find( rootDerivatives.begin(), rootDerivatives.end(),
                       tConcrete ) != rootDerivatives.end() );

    // These types should not inherit the root directly
    TF_AXIOM( std::find( rootDerivatives.begin(), rootDerivatives.end(),
                       tChild ) == rootDerivatives.end() );
    TF_AXIOM( std::find( rootDerivatives.begin(), rootDerivatives.end(),
                       tGrandchild ) == rootDerivatives.end() );
    TF_AXIOM( std::find( childDerivatives.begin(), childDerivatives.end(),
                       tRoot ) == childDerivatives.end() );
    TF_AXIOM( std::find( grandchildDerivatives.begin(),
                       grandchildDerivatives.end(),
                       tRoot ) == grandchildDerivatives.end() );

   //////////////////////////////////////////////////////////////////////// 
    // Test casts

    ChildClass childForCast;
    GrandchildClass grandchildForCast;

    // Try simple upcast
    ConcreteClass* childToConcrete = 
        (ConcreteClass*)tChild.CastToAncestor(tConcrete, &childForCast);
    TF_AXIOM( childToConcrete != NULL );
    TF_AXIOM( TfType::Find(*childToConcrete) == tChild );

    // Try simple upcast to 2nd base
    IAbstractClass* childToIAbstract = 
        (IAbstractClass*)tChild.CastToAncestor(tAbstract, &childForCast);
    TF_AXIOM( childToIAbstract != NULL );
    TF_AXIOM( TfType::Find(*childToIAbstract) == tChild );

    // Try 2-level upcast
    ConcreteClass* grandchildToConcrete = 
        (ConcreteClass*)tGrandchild.CastToAncestor
        (tConcrete, &grandchildForCast);
    TF_AXIOM( grandchildToConcrete != NULL );
    TF_AXIOM( TfType::Find(*grandchildToConcrete) == tGrandchild );

    // Try downcast to same type
    GrandchildClass* grandchildFromGrandchild =
        (GrandchildClass*)tGrandchild.CastFromAncestor
        (tGrandchild, (ChildClass*)&grandchildForCast);
    TF_AXIOM( grandchildFromGrandchild != NULL );
    TF_AXIOM( TfType::Find(*grandchildFromGrandchild) == tGrandchild );

    // Try upcast to same type
    grandchildFromGrandchild =
        (GrandchildClass*)tGrandchild.CastToAncestor
        (tGrandchild, (ChildClass*)&grandchildForCast);
    TF_AXIOM( grandchildFromGrandchild != NULL );
    TF_AXIOM( TfType::Find(*grandchildFromGrandchild) == tGrandchild );

    // Try incorrect upcast
    GrandchildClass* childToGrandchild =
        (GrandchildClass*)tChild.CastToAncestor(tGrandchild, &childForCast);
    TF_AXIOM( childToGrandchild == NULL );

    // Try incorrect downcast
    ChildClass* childFromGrandchild =
        (ChildClass*)tChild.CastFromAncestor
        (tGrandchild, &grandchildForCast);
    TF_AXIOM( childFromGrandchild == NULL );

    // Try incorrect casts to/from unknown type.
    // We don't have an actual Unknown C++ type, so we fashion a bogus
    // pointer to supply; we expect all the cast functions to return 0.
    void *bogusPtr = reinterpret_cast<void*>(1234);
    TF_AXIOM( !tChild.CastFromAncestor( tUnknown, bogusPtr ) );
    TF_AXIOM( !tChild.CastToAncestor( tUnknown, bogusPtr ) );
    TF_AXIOM( !tUnknown.CastFromAncestor( tChild, &childForCast) );
    TF_AXIOM( !tUnknown.CastToAncestor( tChild, &childForCast) );
    
   //////////////////////////////////////////////////////////////////////// 
    // Test manufacture

    // Factory w/ 0 arguments
    CountedClassRefPtr orig;
    TF_AXIOM(!orig);
    orig = tCounted.GetFactory<CountedClassFactory>()->New();
    TF_AXIOM(orig);
    TF_AXIOM(orig->GetNumber() == 0);

    // Factory w/ 1 arguments
    orig.Reset();
    TF_AXIOM(!orig);
    orig = tCounted.GetFactory<CountedClassFactory>()->New(123);
    TF_AXIOM(orig);
    TF_AXIOM(orig->GetNumber() == 123);

    // Test argument promotion
    orig.Reset();
    TF_AXIOM(!orig);
    orig = tCounted.GetFactory<CountedClassFactory>()->New(true);
    TF_AXIOM(orig);
    TF_AXIOM(orig->GetNumber() == int(true));

    // Singleton manufacture
    SingleClass *s1=0, *s2=0;
    s1 = tSingle.GetFactory<TfTest_SingletonFactory<SingleClass> >()->New();
    TF_AXIOM( s1 != s2 );
    s2 = tSingle.GetFactory<TfTest_SingletonFactory<SingleClass> >()->New();
    TF_AXIOM( s1 );
    TF_AXIOM( s2 );
    TF_AXIOM( s1 == s2 );
    s1->SetNumber(123);
    TF_AXIOM( s1->GetNumber() == 123 );
    TF_AXIOM( s2->GetNumber() == 123 );

    // Test manufacture of polymorphic type
    ChildClass* cc = tChild.GetFactory<TfTest_PtrFactory<ChildClass> >()->New();
    ConcreteClass *pc = cc;
    TF_AXIOM( pc );

    // Test attempt to manufacture of unknown & root types
    {
        TfErrorMark m;
        m.SetMark();
        TF_AXIOM( !tUnknown.GetFactory<TfType::FactoryBase>() );
        TF_AXIOM( !tRoot.GetFactory<TfType::FactoryBase>() );
        m.Clear();
    }

    ////////////////////////////////////////////////////////////////////////
    // Test traits queries

    // POD types
    TF_AXIOM( TfType::Find<int>().IsPlainOldDataType() );
    TF_AXIOM( !TfType::Find<std::string>().IsPlainOldDataType() );

    // Enum types
    TfType::Define<_TestEnum>();
    TF_AXIOM( !TfType::Find<_TestEnum>().IsUnknown() );
    TF_AXIOM( TfType::Find<_TestEnum>().IsEnumType() );
    TF_AXIOM( !TfType::Find<int>().IsEnumType() );

    ////////////////////////////////////////////////////////////////////////
    // We should only have C++ types in this test

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    // Start up Python.
    TfPyInitialize();
    TF_FOR_ALL(it, allTypeSet)
        TF_AXIOM( TfPyIsNone( it->GetPythonClass().Get() ) );
#endif // PXR_PYTHON_SUPPORT_ENABLED

    ////////////////////////////////////////////////////////////////////////
    // Test looking up types via aliases

    TfType tClassA = TfType::Define<SomeClassA, TfType::Bases<ConcreteClass>>();
    TF_AXIOM(tClassA);
    tClassA.AddAlias(tConcrete, "SomeClassB");
    TfType tClassB = TfType::Define<SomeClassB, TfType::Bases<IAbstractClass>>();
    TF_AXIOM(tClassB);
    TfType found = tConcrete.FindDerivedByName("SomeClassB");
    TF_AXIOM(found == tClassA);

    ////////////////////////////////////////////////////////////////////////
    // Test that bases are registered with the correct order and that errors
    // are posted as needed.

    TfType someBaseA = TfType::Declare("SomeBaseA");
    TfType someBaseB = TfType::Declare("SomeBaseB");
    TF_AXIOM(someBaseA && someBaseB);
    
    {
        using TypeVector = std::vector<TfType>;    

        // Define SomeDerivedClass with base SomeBaseB: No error expected. 
        TfErrorMark m;
        TfType t = TfType::Declare("SomeDerivedClass", { someBaseB });
        TF_AXIOM(t.GetBaseTypes() == TypeVector({ someBaseB }));
        TF_AXIOM(m.IsClean());

        // Now redeclare with more bases and an order change: No error expected,
        // but someBaseA needs to be first base now.
        t = TfType::Declare("SomeDerivedClass", { someBaseA, someBaseB });
        TF_AXIOM(t.GetBaseTypes() == TypeVector({ someBaseA, someBaseB }));
        TF_AXIOM(m.IsClean());
    
        // Redefine with flipped order: error expected.
        TfType::Declare("SomeDerivedClass", { someBaseB, someBaseA });
        TF_AXIOM(!m.IsClean() && m.begin()->GetCommentary() ==
            "Specified base type order differs for SomeDerivedClass: had "
            "(SomeBaseA, SomeBaseB), now (SomeBaseB, SomeBaseA).  If this is "
            "a type declared in a plugin, check that the plugin metadata is "
            "correct.");
        TF_AXIOM(m.Clear());

        // Redefine with one base missing: error expected.
        TfType::Declare("SomeDerivedClass", { someBaseA });
        TF_AXIOM(!m.IsClean() && m.begin()->GetCommentary() ==
            "TfType 'SomeDerivedClass' was previously declared to have "
            "'SomeBaseB' as a base, but a subsequent declaration does not "
            "include this as a base.  The newly given bases were: (SomeBaseA).  "
            "If this is a type declared in a plugin, check that the plugin "
            "metadata is correct.");
        TF_AXIOM(m.Clear());
    }

    return true;
}

TF_ADD_REGTEST(TfType);

