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
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/typeNotice.h"

#include <boost/bind.hpp>
#include <set>

using namespace std;


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
        assert( not n.GetType().IsUnknown() );
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
    assert(typesWeHaveSeenNoticesFor.count( TfType::Find<CountedClass>()));

    TfType ts = TfType::Define<SingleClass>();
    ts.SetFactory<TfTest_SingletonFactory<SingleClass> >();

    assert(typesWeHaveSeenNoticesFor.count( TfType::Find<SingleClass>()));

    TfType t2 = TfType::Define<ConcreteClass>();
    t2.SetFactory<TfTest_PtrFactory<ConcreteClass> >();

    assert(typesWeHaveSeenNoticesFor.count( TfType::Find<ConcreteClass>()));

    TfType::Define<IAbstractClass>();
    assert(typesWeHaveSeenNoticesFor.count( TfType::Find<IAbstractClass>()));

    TfType t3 = TfType::Define<ChildClass,
                   TfType::Bases< ConcreteClass, IAbstractClass > >();
    t3.SetFactory<TfTest_PtrFactory<ChildClass> >();

    assert(typesWeHaveSeenNoticesFor.count( TfType::Find<ChildClass>()));

    TfType::Define<GrandchildClass,
                   TfType::Bases< ChildClass > >()
        ;
    assert(typesWeHaveSeenNoticesFor.count( TfType::Find<GrandchildClass>()));

    TfType::Define<OtherGrandchildClass,
                   TfType::Bases< ChildClass > >()
        ;
    assert(typesWeHaveSeenNoticesFor
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

    assert( tUnknown.IsUnknown() );
    assert( not tRoot.IsUnknown() );
    assert( not tChild.IsUnknown() );
    assert( not tAbstract.IsUnknown() );
    assert( not tChild.IsUnknown() );
    assert( not tGrandchild.IsUnknown() );
    assert( not tCounted.IsUnknown() );
    assert( not tSingle.IsUnknown() );

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
    assert( knownTypeSet.size() == numKnownTypes );

    // Now include unknown type
    std::set<TfType> allTypeSet = knownTypeSet;
    allTypeSet.insert( tUnknown );
    assert( allTypeSet.size() == numKnownTypes+1 );

    // Expect types to be unique
    assert( allTypeSet.count( tUnknown ) == 1 );
    assert( allTypeSet.count( tRoot ) == 1 );
    assert( allTypeSet.count( tConcrete ) == 1 );
    assert( allTypeSet.count( tAbstract ) == 1 );
    assert( allTypeSet.count( tChild ) == 1 );
    assert( allTypeSet.count( tGrandchild ) == 1 );
    assert( allTypeSet.count( tCounted ) == 1 );
    assert( allTypeSet.count( tSingle ) == 1 );

    ////////////////////////////////////////////////////////////////////////
    // All typeNames should be distinct.

    std::set<std::string> typeNameSet;
    TF_FOR_ALL(it, allTypeSet)
        typeNameSet.insert( it->GetTypeName() );
    assert( typeNameSet.size() == allTypeSet.size() );
    
    ////////////////////////////////////////////////////////////////////////
    // Test IsA

    // IsA<Unknown> -> error
    {
        TfErrorMark m;
        m.SetMark();
        assert( not tUnknown.IsA(tUnknown) );
        m.Clear();
    }

    TF_FOR_ALL(it, knownTypeSet) {
        assert( it->IsA(tRoot) );
        assert( it->IsA(*it) );

        // IsA<Unknown> -> error
        {
            TfErrorMark m;
            m.SetMark();
            assert( not it->IsA(tUnknown) );
            assert( not it->IsA<UnknownClass>() );
            m.Clear();
        }
    }

    assert( tChild.IsA(tConcrete) );
    assert( tChild.IsA(tAbstract) );
    assert( tChild.IsA<ConcreteClass>() );
    assert( tChild.IsA<IAbstractClass>() );

    assert( tConcrete.IsA<ConcreteClass>() );
    assert( not tConcrete.IsA<ChildClass>() );

    assert( tAbstract.IsA(tAbstract) );
    assert( not tAbstract.IsA(tChild) );

    assert( tGrandchild.IsA(tAbstract) );
    assert( tGrandchild.IsA(tConcrete) );
    assert( tGrandchild.IsA(tChild) );
    assert( tGrandchild.IsA<IAbstractClass>() );
    assert( tGrandchild.IsA<ConcreteClass>() );
    assert( tGrandchild.IsA<ChildClass>() );

    ////////////////////////////////////////////////////////////////////////
    // Test GetTypeid()

    assert( TfSafeTypeCompare(tRoot.GetTypeid(), typeid(void)) );
    assert( TfSafeTypeCompare(tConcrete.GetTypeid(), typeid(ConcreteClass)) );
    assert( TfSafeTypeCompare(tAbstract.GetTypeid(), typeid(IAbstractClass)) );
    assert( TfSafeTypeCompare(tChild.GetTypeid(), typeid(ChildClass)) );
    assert( TfSafeTypeCompare(tGrandchild.GetTypeid(),
                              typeid(GrandchildClass)) );

    ////////////////////////////////////////////////////////////////////////
    // Test Find()

    ConcreteClass concreteObj;
    ChildClass childObj;
    assert( tConcrete == TfType::Find(concreteObj) ); 
    assert( tConcrete != TfType::Find(childObj) ); 
    assert( tChild == TfType::Find(childObj) ); 
    assert( tChild != TfType::Find(concreteObj) ); 
    assert( tAbstract == TfType::FindByName("IAbstractClass") );
    assert( tConcrete == TfType::FindByName("ConcreteClass") );
    assert( tChild == TfType::FindByName("ChildClass") );
    assert( tAbstract == TfType::Find(tAbstract.GetTypeid()) );
    assert( tChild == TfType::Find(tChild.GetTypeid()) );

    // Test Find() for pointers to polymorphic types:
    // Raw pointer (T*)
    assert( tConcrete == TfType::Find( &concreteObj ) );
    // TfRefPtr
    CountedClassRefPtr countedRef = CountedClass::New();
    assert( tCounted == TfType::Find( countedRef ) );
    // TfWeakPtr
    CountedClassPtr countedWeak(countedRef);
    assert( tCounted == TfType::Find( countedWeak ) );

   //////////////////////////////////////////////////////////////////////// 
    // Test Get{Base,Derived}Types()

    assert( tRoot.GetBaseTypes().empty() );
    assert( not tRoot.GetDirectlyDerivedTypes().empty() );

    assert( tUnknown.GetBaseTypes().empty() );
    assert( tUnknown.GetDirectlyDerivedTypes().empty() );

    std::vector<TfType> rootDerivatives = tRoot.GetDirectlyDerivedTypes();
    std::vector<TfType> abstractParents = tAbstract.GetBaseTypes();
    std::vector<TfType> concreteParents = tConcrete.GetBaseTypes();
    std::vector<TfType> childParents = tChild.GetBaseTypes();
    std::vector<TfType> childDerivatives = tChild.GetDirectlyDerivedTypes();
    std::vector<TfType> grandchildParents = tGrandchild.GetBaseTypes();
    std::vector<TfType> grandchildDerivatives = tGrandchild.GetDirectlyDerivedTypes();

    // Test inheritance within our known hierarchy
    assert( childParents.size() == 2 && childDerivatives.size() == 2 );
    assert(
        ((childParents[0] == tConcrete && childParents[1] == tAbstract) ||
        ((childParents[0] == tAbstract && childParents[1] == tConcrete))) );
    assert(childDerivatives[0] == tGrandchild );
    assert(grandchildParents.size() == 1 && grandchildDerivatives.empty());
    assert(grandchildParents[0] == tChild);

    // These types should inherit the root directly
    assert( tAbstract.GetBaseTypes() == std::vector<TfType>(1, tRoot) );
    assert( tConcrete.GetBaseTypes() == std::vector<TfType>(1, tRoot) );
    assert( std::find( rootDerivatives.begin(), rootDerivatives.end(),
                       tAbstract ) != rootDerivatives.end() );
    assert( std::find( rootDerivatives.begin(), rootDerivatives.end(),
                       tConcrete ) != rootDerivatives.end() );

    // These types should not inherit the root directly
    assert( std::find( rootDerivatives.begin(), rootDerivatives.end(),
                       tChild ) == rootDerivatives.end() );
    assert( std::find( rootDerivatives.begin(), rootDerivatives.end(),
                       tGrandchild ) == rootDerivatives.end() );
    assert( std::find( childDerivatives.begin(), childDerivatives.end(),
                       tRoot ) == childDerivatives.end() );
    assert( std::find( grandchildDerivatives.begin(),
                       grandchildDerivatives.end(),
                       tRoot ) == grandchildDerivatives.end() );

   //////////////////////////////////////////////////////////////////////// 
    // Test casts

    ChildClass childForCast;
    GrandchildClass grandchildForCast;

    // Try simple upcast
    ConcreteClass* childToConcrete = 
        (ConcreteClass*)tChild.CastToAncestor(tConcrete, &childForCast);
    assert( childToConcrete != NULL );
    assert( TfType::Find(*childToConcrete) == tChild );

    // Try simple upcast to 2nd base
    IAbstractClass* childToIAbstract = 
        (IAbstractClass*)tChild.CastToAncestor(tAbstract, &childForCast);
    assert( childToIAbstract != NULL );
    assert( TfType::Find(*childToIAbstract) == tChild );

    // Try 2-level upcast
    ConcreteClass* grandchildToConcrete = 
        (ConcreteClass*)tGrandchild.CastToAncestor
        (tConcrete, &grandchildForCast);
    assert( grandchildToConcrete != NULL );
    assert( TfType::Find(*grandchildToConcrete) == tGrandchild );

    // Try downcast to same type
    GrandchildClass* grandchildFromGrandchild =
        (GrandchildClass*)tGrandchild.CastFromAncestor
        (tGrandchild, (ChildClass*)&grandchildForCast);
    assert( grandchildFromGrandchild != NULL );
    assert( TfType::Find(*grandchildFromGrandchild) == tGrandchild );

    // Try upcast to same type
    grandchildFromGrandchild =
        (GrandchildClass*)tGrandchild.CastToAncestor
        (tGrandchild, (ChildClass*)&grandchildForCast);
    assert( grandchildFromGrandchild != NULL );
    assert( TfType::Find(*grandchildFromGrandchild) == tGrandchild );

    // Try incorrect upcast
    GrandchildClass* childToGrandchild =
        (GrandchildClass*)tChild.CastToAncestor(tGrandchild, &childForCast);
    assert( childToGrandchild == NULL );

    // Try incorrect downcast
    ChildClass* childFromGrandchild =
        (ChildClass*)tChild.CastFromAncestor
        (tGrandchild, &grandchildForCast);
    assert( childFromGrandchild == NULL );

    // Try incorrect casts to/from unknown type.
    // We don't have an actual Unknown C++ type, so we fashion a bogus
    // pointer to supply; we expect all the cast functions to return 0.
    void *bogusPtr = reinterpret_cast<void*>(1234);
    assert( not tChild.CastFromAncestor( tUnknown, bogusPtr ) );
    assert( not tChild.CastToAncestor( tUnknown, bogusPtr ) );
    assert( not tUnknown.CastFromAncestor( tChild, &childForCast) );
    assert( not tUnknown.CastToAncestor( tChild, &childForCast) );
    
   //////////////////////////////////////////////////////////////////////// 
    // Test manufacture

    // Factory w/ 0 arguments
    CountedClassRefPtr orig;
    assert(not orig);
    orig = tCounted.GetFactory<CountedClassFactory>()->New();
    assert(orig);
    assert(orig->GetNumber() == 0);

    // Factory w/ 1 arguments
    orig.Reset();
    assert(not orig);
    orig = tCounted.GetFactory<CountedClassFactory>()->New(123);
    assert(orig);
    assert(orig->GetNumber() == 123);

    // Test argument promotion
    orig.Reset();
    assert(not orig);
    orig = tCounted.GetFactory<CountedClassFactory>()->New(true);
    assert(orig);
    assert(orig->GetNumber() == int(true));

    // Singleton manufacture
    SingleClass *s1=0, *s2=0;
    s1 = tSingle.GetFactory<TfTest_SingletonFactory<SingleClass> >()->New();
    assert( s1 != s2 );
    s2 = tSingle.GetFactory<TfTest_SingletonFactory<SingleClass> >()->New();
    assert( s1 );
    assert( s2 );
    assert( s1 == s2 );
    s1->SetNumber(123);
    assert( s1->GetNumber() == 123 );
    assert( s2->GetNumber() == 123 );

    // Test manufacture of polymorphic type
    ChildClass* cc = tChild.GetFactory<TfTest_PtrFactory<ChildClass> >()->New();
    ConcreteClass *pc = cc;
    assert( pc );

    // Test attempt to manufacture of unknown & root types
    {
        TfErrorMark m;
        m.SetMark();
        assert( not tUnknown.GetFactory<TfType::FactoryBase>() );
        assert( not tRoot.GetFactory<TfType::FactoryBase>() );
        m.Clear();
    }

    ////////////////////////////////////////////////////////////////////////
    // Test traits queries

    // POD types
    assert( TfType::Find<int>().IsPlainOldDataType() );
    assert( not TfType::Find<std::string>().IsPlainOldDataType() );

    // Enum types
    TfType::Define<_TestEnum>();
    assert( not TfType::Find<_TestEnum>().IsUnknown() );
    assert( TfType::Find<_TestEnum>().IsEnumType() );
    assert( not TfType::Find<int>().IsEnumType() );

    ////////////////////////////////////////////////////////////////////////
    // We should only have C++ types in this test

    // Start up Python.
    TfPyInitialize();
    TF_FOR_ALL(it, allTypeSet)
        assert( TfPyIsNone( it->GetPythonClass().Get() ) );

    ////////////////////////////////////////////////////////////////////////
    // Test looking up types via aliases

    TfType tClassA = TfType::Define<SomeClassA, TfType::Bases<ConcreteClass>>();
    assert(tClassA);
    tClassA.AddAlias(tConcrete, "SomeClassB");
    TfType tClassB = TfType::Define<SomeClassB, TfType::Bases<IAbstractClass>>();
    assert(tClassB);
    TfType found = tConcrete.FindDerivedByName("SomeClassB");
    assert(found == tClassA);

    return true;
}

TF_ADD_REGTEST(TfType);

