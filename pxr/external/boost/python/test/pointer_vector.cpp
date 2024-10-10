//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Joel de Guzman 2005-2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/suite/indexing/vector_indexing_suite.hpp"
#include <vector>

using namespace PXR_BOOST_NAMESPACE::python;

class Abstract
{
public:   
    virtual ~Abstract() {}; // silence compiler warningsa
    virtual std::string    f() =0;
};

class Concrete1 : public Abstract
{
public:   
    virtual std::string    f()    { return "harru"; }
};

typedef std::vector<Abstract*>   ListOfObjects;

class DoesSomething
{
public:
    DoesSomething()    {}
   
    ListOfObjects   returnList()    
    {
        ListOfObjects lst; 
        lst.push_back(new Concrete1()); return lst; 
    }
};

PXR_BOOST_PYTHON_MODULE(pointer_vector_ext)
{       
class_<Abstract, noncopyable>("Abstract", no_init)
    .def("f", &Abstract::f)
    ;

class_<ListOfObjects>("ListOfObjects")
   .def( vector_indexing_suite<ListOfObjects>() )  
    ;

class_<DoesSomething>("DoesSomething")
    .def("returnList", &DoesSomething::returnList)
    ;
}


