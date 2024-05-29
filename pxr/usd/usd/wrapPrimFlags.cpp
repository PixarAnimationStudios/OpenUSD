//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/hash.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primFlags.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/scope.hpp>

#include <string>

using namespace boost::python;

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

// Python does not allow overloading logical operators ('and', 'or', etc).  Also
// python's __nonzero__ (invoked by 'not') must return a python bool or int.
// Due to these limitations, we use the '&', '|', and '~' operators,
// corresponding to '&&', '||', and '!' in C++.
//
// To do this we supply the relevant operators here, but only here, for the sake
// of python wrapping.  These operators are not otherwise available.

// Operator & to create conjunctions.
Usd_PrimFlagsConjunction
operator &(Usd_Term l, Usd_Term r) { return l && r; }
Usd_PrimFlagsConjunction
operator &(Usd_PrimFlagsConjunction l, Usd_Term r) { return l && r; }
Usd_PrimFlagsConjunction
operator &(Usd_Term l, Usd_PrimFlagsConjunction r) { return l && r; }

// Operator | to create disjuncitons.
Usd_PrimFlagsDisjunction
operator |(Usd_Term l, Usd_Term r) { return l || r; }
Usd_PrimFlagsDisjunction
operator |(Usd_PrimFlagsDisjunction l, Usd_Term r) { return l || r; }
Usd_PrimFlagsDisjunction
operator |(Usd_Term l, Usd_PrimFlagsDisjunction r) { return l || r; }

// Operator ~ to logically negate.
static Usd_Term
operator~(Usd_Term term) { return !term; }
static Usd_PrimFlagsDisjunction
operator ~(Usd_PrimFlagsConjunction conj) { return !conj; }
static Usd_PrimFlagsConjunction
operator ~(Usd_PrimFlagsDisjunction disj) { return !disj; }

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Hash implementations.
size_t __hash__Term(const Usd_Term &t) {
    return TfHash::Combine(t.flag, t.negated);
}

size_t __hash__Predicate(const Usd_PrimFlagsPredicate &p) {
    return TfHash{}(p);
}

// Call implementations.
bool __call__Predicate(const Usd_PrimFlagsPredicate &p, const UsdPrim& prim){
    return p(prim);
}

} // anonymous namespace 

void wrapUsdPrimFlags()
{
    class_<Usd_Term>("_Term", no_init)
        .def(~self)
        .def(self == self)
        .def(self != self)
        .def(self & self)
        .def(self | self)
        .def("__hash__", __hash__Term)
        ;
    implicitly_convertible<Usd_Term, Usd_PrimFlagsPredicate>();
    implicitly_convertible<Usd_Term, Usd_PrimFlagsConjunction>();
    implicitly_convertible<Usd_Term, Usd_PrimFlagsDisjunction>();

    class_<Usd_PrimFlagsPredicate>("_PrimFlagsPredicate", no_init)
        .def("Tautology", &Usd_PrimFlagsPredicate::Tautology)
        .staticmethod("Tautology")
        .def("Contradiction", &Usd_PrimFlagsPredicate::Contradiction)
        .staticmethod("Contradiction")
        .def(self == self)
        .def(self != self)
        .def("__hash__", __hash__Predicate)
        .def("__call__", __call__Predicate)
        ;

    class_<Usd_PrimFlagsConjunction, bases<Usd_PrimFlagsPredicate> >
        ("_PrimFlagsConjunction", no_init)
        .def(~self)
        .def(self &= other<Usd_Term>())
        .def(self & other<Usd_Term>())
        .def(other<Usd_Term>() & self)
        ;

    class_<Usd_PrimFlagsDisjunction, bases<Usd_PrimFlagsPredicate> >
        ("_PrimFlagsDisjunction", no_init)
        .def(~self)
        .def(self |= other<Usd_Term>())
        .def(self | other<Usd_Term>())
        .def(other<Usd_Term>() | self)
        ;

    scope().attr("PrimIsActive") = Usd_Term(UsdPrimIsActive);
    scope().attr("PrimIsLoaded") = Usd_Term(UsdPrimIsLoaded);
    scope().attr("PrimIsModel") = Usd_Term(UsdPrimIsModel);
    scope().attr("PrimIsGroup") = Usd_Term(UsdPrimIsGroup);
    scope().attr("PrimIsAbstract") = Usd_Term(UsdPrimIsAbstract);
    scope().attr("PrimIsDefined") = Usd_Term(UsdPrimIsDefined);
    scope().attr("PrimIsInstance") = Usd_Term(UsdPrimIsInstance);
    scope().attr("PrimHasDefiningSpecifier") 
        = Usd_Term(UsdPrimHasDefiningSpecifier);

    scope().attr("PrimDefaultPredicate") = UsdPrimDefaultPredicate;
    scope().attr("PrimAllPrimsPredicate") = UsdPrimAllPrimsPredicate;

    def("TraverseInstanceProxies", 
        (Usd_PrimFlagsPredicate(*)())&UsdTraverseInstanceProxies);
    def("TraverseInstanceProxies", 
        (Usd_PrimFlagsPredicate(*)(Usd_PrimFlagsPredicate))
            &UsdTraverseInstanceProxies, 
        arg("predicate"));
}
