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
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primFlags.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/scope.hpp>
#include <boost/functional/hash.hpp>

#include <string>



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

namespace pxrUsdUsdWrapPrimFlags {

// Hash implementations.
size_t __hash__Term(const Usd_Term &t) {
    size_t h = static_cast<size_t>(t.flag);
    boost::hash_combine(h, t.negated);
    return h;
}

size_t __hash__Predicate(const Usd_PrimFlagsPredicate &p) {
    return hash_value(p);
}

// Call implementations.
bool __call__Predicate(const Usd_PrimFlagsPredicate &p, const UsdPrim& prim){
    return p(prim);
}

} // anonymous namespace 

void wrapUsdPrimFlags()
{
    boost::python::class_<Usd_Term>("_Term", boost::python::no_init)
        .def(~boost::python::self)
        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def(boost::python::self & boost::python::self)
        .def(boost::python::self | boost::python::self)
        .def("__hash__", pxrUsdUsdWrapPrimFlags::__hash__Term)
        ;
    boost::python::implicitly_convertible<Usd_Term, Usd_PrimFlagsPredicate>();
    boost::python::implicitly_convertible<Usd_Term, Usd_PrimFlagsConjunction>();
    boost::python::implicitly_convertible<Usd_Term, Usd_PrimFlagsDisjunction>();

    boost::python::class_<Usd_PrimFlagsPredicate>("_PrimFlagsPredicate", boost::python::no_init)
        .def("Tautology", &Usd_PrimFlagsPredicate::Tautology)
        .staticmethod("Tautology")
        .def("Contradiction", &Usd_PrimFlagsPredicate::Contradiction)
        .staticmethod("Contradiction")
        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def("__hash__", pxrUsdUsdWrapPrimFlags::__hash__Predicate)
        .def("__call__", pxrUsdUsdWrapPrimFlags::__call__Predicate)
        ;

    boost::python::class_<Usd_PrimFlagsConjunction, boost::python::bases<Usd_PrimFlagsPredicate> >
        ("_PrimFlagsConjunction", boost::python::no_init)
        .def(~boost::python::self)
        .def(boost::python::self &= boost::python::other<Usd_Term>())
        .def(boost::python::self & boost::python::other<Usd_Term>())
        .def(boost::python::other<Usd_Term>() & boost::python::self)
        ;

    boost::python::class_<Usd_PrimFlagsDisjunction, boost::python::bases<Usd_PrimFlagsPredicate> >
        ("_PrimFlagsDisjunction", boost::python::no_init)
        .def(~boost::python::self)
        .def(boost::python::self |= boost::python::other<Usd_Term>())
        .def(boost::python::self | boost::python::other<Usd_Term>())
        .def(boost::python::other<Usd_Term>() | boost::python::self)
        ;

    boost::python::scope().attr("PrimIsActive") = Usd_Term(UsdPrimIsActive);
    boost::python::scope().attr("PrimIsLoaded") = Usd_Term(UsdPrimIsLoaded);
    boost::python::scope().attr("PrimIsModel") = Usd_Term(UsdPrimIsModel);
    boost::python::scope().attr("PrimIsGroup") = Usd_Term(UsdPrimIsGroup);
    boost::python::scope().attr("PrimIsAbstract") = Usd_Term(UsdPrimIsAbstract);
    boost::python::scope().attr("PrimIsDefined") = Usd_Term(UsdPrimIsDefined);
    boost::python::scope().attr("PrimIsInstance") = Usd_Term(UsdPrimIsInstance);
    boost::python::scope().attr("PrimHasDefiningSpecifier") 
        = Usd_Term(UsdPrimHasDefiningSpecifier);

    boost::python::scope().attr("PrimDefaultPredicate") = UsdPrimDefaultPredicate;
    boost::python::scope().attr("PrimAllPrimsPredicate") = UsdPrimAllPrimsPredicate;

    boost::python::def("TraverseInstanceProxies", 
        (Usd_PrimFlagsPredicate(*)())&UsdTraverseInstanceProxies);
    boost::python::def("TraverseInstanceProxies", 
        (Usd_PrimFlagsPredicate(*)(Usd_PrimFlagsPredicate))
            &UsdTraverseInstanceProxies, 
        boost::python::arg("predicate"));
}
