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
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "pxr/base/arch/defines.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/object.hpp>
#include <boost/python/import.hpp>
#include <boost/python/tuple.hpp>

#include <cmath>
#include <mutex>

#include <datetime.h> // from Python

// Note: This wrapper does not currently support timezones.  Timezone info
// will be dropped when moving between C++ and Python.

using namespace boost::python;
namespace boost_pt = boost::posix_time;

static const unsigned short usecNumDigits = 6;

static void _ImportPyDateTimeModuleOnce();

struct BoostPtimeToPyDateTime
{
    static PyObject* convert(const boost_pt::ptime & ptime) {

        _ImportPyDateTimeModuleOnce();

        if (not ptime.is_special()) {
            const boost::gregorian::date date = ptime.date();
            const boost_pt::time_duration time = ptime.time_of_day();
            // Convert fractional seconds to microseconds.
            long usecs;
            // Difference in number of digits used in boost's fractional seconds
            // representation and microseconds.
            const unsigned short d =
                usecNumDigits - boost_pt::time_duration::num_fractional_digits();
            if (d == 0) {
                usecs = time.fractional_seconds();
            }
            else {
                // Boost appears to support at most 9 digits (base 10) of
                // fractional seconds (ie. nanosecond resolution).  double
                // should be sufficiently accurate for convertion to
                // microseconds as it's good for about 16 digits.
                double scale = std::pow(10, d);
                usecs = time.fractional_seconds() * scale;
            }

            return PyDateTime_FromDateAndTime(
                date.year(), date.month(), date.day(),
                time.hours(), time.minutes(), time.seconds(),
                usecs);
        }
        else {
            // datetime.datetime doesn't support the special values that ptime
            // does (+/- infinity, not_a_date_time), so we map those values to
            // None.
            Py_INCREF(Py_None);
            return Py_None;
        }
    }
};

// Register a from-python conversion that lets clients pass python datetime
// objects to bindings expecting boost::posix_time::ptime.
struct Tf_BoostPtimeFromPyDateTime
{
    Tf_BoostPtimeFromPyDateTime() {
        boost::python::converter::registry::insert
            (&convertible, &construct, boost::python::type_id<boost_pt::ptime>());
    }
    static void *convertible(PyObject *obj) {
        _ImportPyDateTimeModuleOnce();
        return (obj and PyDateTime_Check(obj)) ? obj : 0;
    }
    static void construct(PyObject *src,
                          boost::python::converter::
                          rvalue_from_python_stage1_data *data) {
        _ImportPyDateTimeModuleOnce();

        const int year = PyDateTime_GET_YEAR(src);
        const int month = PyDateTime_GET_MONTH(src);
        const int day = PyDateTime_GET_DAY(src);

        const int hour = PyDateTime_DATE_GET_HOUR(src);
        const int minute = PyDateTime_DATE_GET_MINUTE(src);
        const int second = PyDateTime_DATE_GET_SECOND(src);
        const int usec = PyDateTime_DATE_GET_MICROSECOND(src);

        try {
            boost_pt::ptime ptime(boost::gregorian::date(year, month, day),
                                  boost_pt::time_duration(hour, minute, second) +
                                  boost_pt::microseconds(usec));

            void *storage =
                ((boost::python::converter::
                  rvalue_from_python_storage<boost_pt::ptime> *)data)->storage.bytes;
            new (storage) boost_pt::ptime(boost::gregorian::date(year, month, day),
                                          boost_pt::time_duration(hour, minute, second) +
                                          boost_pt::microseconds(usec));

            data->convertible = storage;
        }
        catch (std::out_of_range & exc) {
            // datetime.datetime supports a wider range of valid years than
            // gregorian::date.  Raise a value error with the message if we
            // can't convert.
            TfPyThrowValueError(
                TfStringPrintf("Can't convert to boost::posix_time::ptime: %s",
                               exc.what()));
        }
    }
};

void
wrapPyDateTime()
{
    to_python_converter<boost_pt::ptime, BoostPtimeToPyDateTime>();
    Tf_BoostPtimeFromPyDateTime();
}

// Python DateTime C API -- the PyDateTime_IMPORT macro requires ignoring
// -Wwrite-strings as it expands to a call that passes a string literal to
// Python API whose parameter is char * instead of const char *.  The
// problematic function signature is fixed in Python 3.x, but won't be fixed
// in 2.x.  (http://bugs.python.org/issue7463)
//
// XXX: We should remove this pragma as soon as it is reasonable to put the
//      warning flag in the SConscript instead (bug 44546) or it is no longer
//      necessary (ie. Python 3).
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
static void
_ImportPyDateTimeModuleOnce()
{
    static std::once_flag once;
    std::call_once(once, [](){
        // Python datetime C API requires that this is called before calling
        // any other datetime function.
        PyDateTime_IMPORT;
    });
}
