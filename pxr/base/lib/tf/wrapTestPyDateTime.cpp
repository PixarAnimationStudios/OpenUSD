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

#include <boost/python/class.hpp>

#include <boost/date_time/posix_time/posix_time_types.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

    struct Tf_TestDateTime {

        static boost::posix_time::ptime
        RoundTrip(const boost::posix_time::ptime & ptime) {
            return ptime;
        }

        static boost::posix_time::ptime
        MakePtime(long year, long month, long day,
                  long hour, long minute, long second,
                  long usec) {
            return boost::posix_time::ptime(
                boost::gregorian::date(year, month, day),
                boost::posix_time::time_duration(hour, minute, second) +
                boost::posix_time::microseconds(usec));
        }

        static boost::posix_time::ptime
        MakeNotADateTime() {
            return boost::posix_time::not_a_date_time;
        }

        static boost::posix_time::ptime
        MakeNegInfinity() {
            return boost::posix_time::neg_infin;
        }

        static boost::posix_time::ptime
        MakePosInfinity() {
            return boost::posix_time::pos_infin;
        }

    };

} // anonymous namespace 


void wrapTf_TestPyDateTime()
{
    typedef Tf_TestDateTime This;

    class_<This>("Tf_TestDateTime", no_init)
        .def("RoundTrip", &This::RoundTrip)
        .staticmethod("RoundTrip")

        .def("MakePtime", &This::MakePtime)
        .staticmethod("MakePtime")

        .def("MakeNotADateTime", &This::MakeNotADateTime)
        .staticmethod("MakeNotADateTime")

        .def("MakeNegInfinity", &This::MakeNegInfinity)
        .staticmethod("MakeNegInfinity")

        .def("MakePosInfinity", &This::MakePosInfinity)
        .staticmethod("MakePosInfinity")

        ;
}
