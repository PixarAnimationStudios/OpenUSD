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
#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/scope.hpp>
#include <boost/functional/hash.hpp>

#include "pxr/usd/usd/primFlags.h"

#include <string>

using namespace boost::python;

using std::string;

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

// Hash implementations.
size_t __hash__Term(const Usd_Term &t) {
    size_t h = static_cast<size_t>(t.flag);
    boost::hash_combine(h, t.negated);
    return h;
}

size_t __hash__Predicate(const Usd_PrimFlagsPredicate &p) {
    return hash_value(p);
}

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
}
