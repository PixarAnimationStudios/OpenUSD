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

#include "pxr/base/tf/token.h"

#include <boost/python/class.hpp>
#include <vector>
#include <utility>

using namespace boost::python;
using std::pair;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

class Tf_TestPyContainerConversions {
public:
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

    // This method simply returns the vector of tokens its given. 
    // It's purpose is to allow testing container conversions both to and 
    // from Python.
    static vector<TfToken> GetTokens(const vector<TfToken>& inTokens) {
        return inTokens;
    }

};

void wrapTf_TestPyContainerConversions()
{
    typedef Tf_TestPyContainerConversions This;

    class_<This, boost::noncopyable>("Tf_TestPyContainerConversions")
        .def("GetVectorTimesTwo", &This::GetVectorTimesTwo)
        .staticmethod("GetVectorTimesTwo")
        
        .def("GetPairTimesTwo", &This::GetPairTimesTwo)
        .staticmethod("GetPairTimesTwo")

        .def("GetTokens", &This::GetTokens)
        .staticmethod("GetTokens")
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE
