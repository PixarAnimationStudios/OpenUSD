//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primFlags.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

#include <string>

namespace nb = nanobind;
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

void wrapUsdPrimFlags(nb::module_ &m)
{
    nb::class_<Usd_Term>(m, "_Term")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def(~nb::self)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self & nb::self)
        .def(nb::self | nb::self)
        .def("__hash__", __hash__Term)
        ;

    nb::class_<Usd_PrimFlagsPredicate>(m, "_PrimFlagsPredicate")
        .def(nb::init_implicit<Usd_Term>())
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def_static("Tautology", &Usd_PrimFlagsPredicate::Tautology)
        .def_static("Contradiction", &Usd_PrimFlagsPredicate::Contradiction)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__hash__", __hash__Predicate)
        .def("__call__", __call__Predicate)
        ;

    nb::class_<Usd_PrimFlagsConjunction, Usd_PrimFlagsPredicate>(
        m, "_PrimFlagsConjunction")
        .def(nb::init_implicit<Usd_Term>())
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def(~nb::self)
        .def("__iand__", [](Usd_PrimFlagsConjunction &self, Usd_Term r) -> Usd_PrimFlagsConjunction & {
            return self &= r;
        }, nb::rv_policy::reference)
        .def("__and__", [](const Usd_PrimFlagsConjunction &self, Usd_Term r) {
            return self & r;
        })
        .def("__rand__", [](const Usd_PrimFlagsConjunction &self, Usd_Term l) {
            return l & self;
        })
        ;

    nb::class_<Usd_PrimFlagsDisjunction, Usd_PrimFlagsPredicate>(
        m, "_PrimFlagsDisjunction")
        .def(nb::init_implicit<Usd_Term>())
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def(~nb::self)
        .def("__ior__", [](Usd_PrimFlagsDisjunction &self, Usd_Term r) -> Usd_PrimFlagsDisjunction & {
            return self |= r;
        }, nb::rv_policy::reference)
        .def("__or__", [](const Usd_PrimFlagsDisjunction &self, Usd_Term r) {
            return self | r;
        })
        .def("__ror__", [](const Usd_PrimFlagsDisjunction &self, Usd_Term l) {
            return l | self;
        })
        ;

    m.attr("PrimIsActive") = Usd_Term(UsdPrimIsActive);
    m.attr("PrimIsLoaded") = Usd_Term(UsdPrimIsLoaded);
    m.attr("PrimIsModel") = Usd_Term(UsdPrimIsModel);
    m.attr("PrimIsGroup") = Usd_Term(UsdPrimIsGroup);
    m.attr("PrimIsAbstract") = Usd_Term(UsdPrimIsAbstract);
    m.attr("PrimIsDefined") = Usd_Term(UsdPrimIsDefined);
    m.attr("PrimIsInstance") = Usd_Term(UsdPrimIsInstance);
    m.attr("PrimHasClassSpecifier") = Usd_Term(UsdPrimHasClassSpecifier);
    m.attr("PrimHasDefiningSpecifier") 
        = Usd_Term(UsdPrimHasDefiningSpecifier);

    m.attr("PrimDefaultPredicate") = UsdPrimDefaultPredicate;
    m.attr("PrimAllPrimsPredicate") = UsdPrimAllPrimsPredicate;

    m.def("TraverseInstanceProxies", 
        (Usd_PrimFlagsPredicate(*)())&UsdTraverseInstanceProxies);
    m.def("TraverseInstanceProxies", 
        (Usd_PrimFlagsPredicate(*)(Usd_PrimFlagsPredicate))
            &UsdTraverseInstanceProxies, 
        nb::arg("predicate"));
}
