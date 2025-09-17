//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include <vector>
#include <utility>
#include <locale>

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/token.h"

#include "pxr/external/boost/python/class.hpp"

using std::pair;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

class Tf_TestPyContainerConversions {
public:
    static void Init() {
        // Initialize any test-specific Python bindings and conversions.
        // This ensures these registrations aren't present outside of tests.
        using TupleType = std::tuple<int, int, int>;
        pxr_boost::python::to_python_converter<
            TupleType, TfPyContainerConversions::to_tuple<TupleType>>();
        TfPyContainerConversions::from_python_tuple<TupleType>();
    }

    static vector<double> GetVectorTimesTwo(const vector<int>& inVec) {
        vector<double> ret;
        for(size_t i = 0; i < inVec.size(); i++) {
            ret.push_back(inVec[i] * 2.0);
        }

        return ret;
    }

    static pair<double, double> GetPairTimesTwo(const pair<int, int>& inPair) {
        return pair<double, double>(inPair.first * 2.0, inPair.second * 2.0);
    }

    static std::tuple<int, int, int>
    GetTupleTimesTwo(const std::tuple<int, int, int>& inTuple) {
        return { std::get<0>(inTuple) * 2.0,
                 std::get<1>(inTuple) * 2.0,
                 std::get<2>(inTuple) * 2.0 };
    }

    // This method simply returns the vector of tokens its given. 
    // It's purpose is to allow testing container conversions both to and 
    // from Python.
    static vector<TfToken> GetTokens(const vector<TfToken>& inTokens) {
        return inTokens;
    }

};

} // anonymous namespace 

void wrapTf_TestPyContainerConversions()
{
    typedef Tf_TestPyContainerConversions This;

    class_<This, noncopyable>("Tf_TestPyContainerConversions")
        .def("Init", &This::Init)
        .staticmethod("Init")

        .def("GetVectorTimesTwo", &This::GetVectorTimesTwo)
        .staticmethod("GetVectorTimesTwo")
        
        .def("GetPairTimesTwo", &This::GetPairTimesTwo)
        .staticmethod("GetPairTimesTwo")

        .def("GetTupleTimesTwo", &This::GetTupleTimesTwo)
        .staticmethod("GetTupleTimesTwo")

        .def("GetTokens", &This::GetTokens)
        .staticmethod("GetTokens")
        ;
}
