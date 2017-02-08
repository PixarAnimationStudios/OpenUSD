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

#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python/to_python_converter.hpp>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

template <typename CONTAINER_TYPE>
struct Set_ToPython
{
    static PyObject* convert(CONTAINER_TYPE const &c)
    {
        PyObject* set = PySet_New(NULL);
        TF_FOR_ALL(i, c) {
            PySet_Add(set, boost::python::object(*i).ptr());
        }
        return set;
    }
};

template<typename T>
void _RegisterToAndFromSetConversions()
{
    boost::python::to_python_converter<std::set<T >, Set_ToPython<std::set<T> > >();
    TfPyContainerConversions::from_python_sequence<std::set<T>,
						   TfPyContainerConversions::set_policy >();
}

void wrapPyContainerConversions()
{
    typedef std::pair<int, int> _IntPair;
    typedef std::pair<long, long> _LongPair;
    typedef std::pair<float, float> _FloatPair;
    typedef std::pair<double, double> _DoublePair;
    typedef std::pair<std::string, std::string> _StringPair;

    boost::python::to_python_converter<std::vector<int>, 
                    TfPySequenceToPython<std::vector<int> > >();
    boost::python::to_python_converter<std::vector<unsigned int>, 
                    TfPySequenceToPython<std::vector<unsigned int> > >();
    boost::python::to_python_converter<std::vector<int64_t>, 
                    TfPySequenceToPython<std::vector<int64_t> > >();
    boost::python::to_python_converter<std::vector<uint64_t>, 
                    TfPySequenceToPython<std::vector<uint64_t> > >();
    boost::python::to_python_converter<std::vector<float>, 
                    TfPySequenceToPython<std::vector<float> > >();
    boost::python::to_python_converter<std::vector<double>, 
                    TfPySequenceToPython<std::vector<double> > >();
    boost::python::to_python_converter<std::vector<std::string>, 
                    TfPySequenceToPython<std::vector<std::string> > >();
    boost::python::to_python_converter<std::vector<_StringPair>, 
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

    boost::python::to_python_converter< _IntPair, 
        TfPyContainerConversions::to_tuple< _IntPair > >();
    boost::python::to_python_converter< _LongPair,
        TfPyContainerConversions::to_tuple< _LongPair > >();
    boost::python::to_python_converter< _FloatPair, 
        TfPyContainerConversions::to_tuple< _FloatPair > >();
    boost::python::to_python_converter< _DoublePair, 
        TfPyContainerConversions::to_tuple< _DoublePair > >();
    boost::python::to_python_converter< _StringPair, 
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

PXR_NAMESPACE_CLOSE_SCOPE
