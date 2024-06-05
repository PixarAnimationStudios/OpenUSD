//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
