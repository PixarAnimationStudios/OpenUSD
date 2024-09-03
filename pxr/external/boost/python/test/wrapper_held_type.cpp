//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2005. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/register_ptr_to_python.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/wrapper.hpp"
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/implicit.hpp"

#include <memory>

struct data
{
    virtual ~data() {}; // silence compiler warnings
    virtual int id() const
    {
        return 42;
    }
};
    
std::shared_ptr<data> create_data()
{ 
    return std::shared_ptr<data>( new data ); 
}

void do_nothing( std::shared_ptr<data>& ){}

    
namespace bp = PXR_BOOST_NAMESPACE::python;

struct data_wrapper : data, bp::wrapper< data >
{
    data_wrapper(data const & arg )
    : data( arg )
      , bp::wrapper< data >()
    {}

    data_wrapper()
    : data()
      , bp::wrapper< data >()
    {}

    virtual int id() const
    {
        if( bp::override id = this->get_override( "id" ) )
            return bp::call<int>(id.ptr()); // id();
        else
            return data::id(  );
    }
    
    virtual int default_id(  ) const
    {
        return this->data::id( );
    }

};

PXR_BOOST_PYTHON_MODULE(wrapper_held_type_ext)
{
    bp::class_< data_wrapper, std::shared_ptr< data > >( "data" )    
        .def( "id", &data::id, &::data_wrapper::default_id );

    bp::def( "do_nothing", &do_nothing );
    bp::def( "create_data", &create_data );
}    

#include "module_tail.cpp"
