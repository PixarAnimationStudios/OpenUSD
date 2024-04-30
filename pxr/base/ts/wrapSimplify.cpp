//
// Copyright 2023 Pixar
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
#include "pxr/base/ts/simplify.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>
#include <boost/python/def.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

static void _SimplifySpline(
    TsSpline& spline,
    const GfMultiInterval &intervals,
    double maxErrorFraction)
{
    TsSimplifySpline(&spline, intervals, maxErrorFraction);
}

static void _SimplifySplinesInParallel(
	boost::python::list& splines,
	const std::vector<GfMultiInterval> &intervals,
	double maxErrorFraction)
{
    // Because we are mutating the python 'splines' argument to be consistent
    // with the non-parallel API, we can't use
    // TfPyContainerConversions::from_python_sequence() which works only for
    // const or value arguments so we have to iterate the python list
    std::vector<TsSpline*> splinePtrs;
    for(int i=0; i < len(splines); ++i)
    {
        boost::python::extract<TsSpline&> extractSpline(splines[i]);
        if(extractSpline.check())
        {
            TsSpline& spline = extractSpline();
            splinePtrs.push_back(&spline);
        } 
        else
        {
            TfPyThrowTypeError("Expecting type TsSpline in splines.");
        }
    }
    TsSimplifySplinesInParallel(splinePtrs, intervals, maxErrorFraction);
}


void wrapSimplify()
{
    TfPyContainerConversions::from_python_sequence<
        std::vector< GfMultiInterval >, 
        TfPyContainerConversions::variable_capacity_policy >();

    def("SimplifySpline", _SimplifySpline);
    def("SimplifySplinesInParallel", &_SimplifySplinesInParallel,
        "(list<Ts.Spline> (mutated for result), list<Gf.MultiInterval>, maxErrorFraction)\n");
}
