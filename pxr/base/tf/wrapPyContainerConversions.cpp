//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyContainerConversions.h"

#include "pxr/external/boost/python/to_python_converter.hpp"
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

template <typename CONTAINER_TYPE>
struct Set_ToPython
{
    static PyObject* convert(CONTAINER_TYPE const &c)
    {
        PyObject* set = PySet_New(NULL);
        TF_FOR_ALL(i, c) {
            PySet_Add(set, pxr_boost::python::object(*i).ptr());
        }
        return set;
    }
};

template<typename T>
void _RegisterToAndFromSetConversions()
{
    pxr_boost::python::to_python_converter<std::set<T >, Set_ToPython<std::set<T> > >();
    TfPyContainerConversions::from_python_sequence<std::set<T>,
						   TfPyContainerConversions::set_policy >();
}

} // anonymous namespace 

void wrapPyContainerConversions()
{
    typedef std::pair<int, int> _IntPair;
    typedef std::pair<long, long> _LongPair;
    typedef std::pair<float, float> _FloatPair;
    typedef std::pair<double, double> _DoublePair;
    typedef std::pair<std::string, std::string> _StringPair;

    pxr_boost::python::to_python_converter<std::vector<int>, 
                    TfPySequenceToPython<std::vector<int> > >();
    pxr_boost::python::to_python_converter<std::vector<unsigned int>, 
                    TfPySequenceToPython<std::vector<unsigned int> > >();
    pxr_boost::python::to_python_converter<std::vector<int64_t>, 
                    TfPySequenceToPython<std::vector<int64_t> > >();
    pxr_boost::python::to_python_converter<std::vector<uint64_t>, 
                    TfPySequenceToPython<std::vector<uint64_t> > >();
    pxr_boost::python::to_python_converter<std::vector<float>, 
                    TfPySequenceToPython<std::vector<float> > >();
    pxr_boost::python::to_python_converter<std::vector<double>, 
                    TfPySequenceToPython<std::vector<double> > >();
    pxr_boost::python::to_python_converter<std::vector<std::string>, 
                    TfPySequenceToPython<std::vector<std::string> > >();
    pxr_boost::python::to_python_converter<std::vector<_StringPair>, 
                    TfPySequenceToPython<std::vector<_StringPair> > >();

    TfPyContainerConversions::from_python_sequence<std::vector<int>,
        TfPyContainerConversions::variable_capacity_policy >();
    TfPyContainerConversions::from_python_sequence<std::vector<unsigned int>,
        TfPyContainerConversions::variable_capacity_policy >();
    TfPyContainerConversions::from_python_sequence<std::vector<int64_t>,
        TfPyContainerConversions::variable_capacity_policy >();
    TfPyContainerConversions::from_python_sequence<std::vector<uint64_t>,
        TfPyContainerConversions::variable_capacity_policy >();
    TfPyContainerConversions::from_python_sequence<std::vector<float>,
        TfPyContainerConversions::variable_capacity_policy >();
    TfPyContainerConversions::from_python_sequence<std::vector<double>,
        TfPyContainerConversions::variable_capacity_policy >();
    TfPyContainerConversions::from_python_sequence<std::vector<size_t>,
        TfPyContainerConversions::variable_capacity_policy >();
    TfPyContainerConversions::from_python_sequence<std::vector<unsigned long>,
        TfPyContainerConversions::variable_capacity_policy >();        
    TfPyContainerConversions::from_python_sequence<std::vector<std::string>,
        TfPyContainerConversions::variable_capacity_policy >();

    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<int> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<unsigned int> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<int64_t> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<uint64_t> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<float> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<double> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<size_t> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<unsigned long> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<std::vector<std::string> >,
        TfPyContainerConversions::variable_capacity_policy
    >();
    TfPyContainerConversions::from_python_sequence<
        std::vector<_StringPair>,
        TfPyContainerConversions::variable_capacity_policy
    >();

    pxr_boost::python::to_python_converter< _IntPair, 
        TfPyContainerConversions::to_tuple< _IntPair > >();
    pxr_boost::python::to_python_converter< _LongPair,
        TfPyContainerConversions::to_tuple< _LongPair > >();
    pxr_boost::python::to_python_converter< _FloatPair, 
        TfPyContainerConversions::to_tuple< _FloatPair > >();
    pxr_boost::python::to_python_converter< _DoublePair, 
        TfPyContainerConversions::to_tuple< _DoublePair > >();
    pxr_boost::python::to_python_converter< _StringPair, 
        TfPyContainerConversions::to_tuple< _StringPair > >();

    TfPyContainerConversions::from_python_tuple_pair< _IntPair >();
    TfPyContainerConversions::from_python_tuple_pair< _LongPair >();
    TfPyContainerConversions::from_python_tuple_pair< _FloatPair >();
    TfPyContainerConversions::from_python_tuple_pair< _DoublePair >();
    TfPyContainerConversions::from_python_tuple_pair< _StringPair >();

    _RegisterToAndFromSetConversions<int>();
    _RegisterToAndFromSetConversions<float>();
    _RegisterToAndFromSetConversions<double>();
    _RegisterToAndFromSetConversions<std::string>();
}
