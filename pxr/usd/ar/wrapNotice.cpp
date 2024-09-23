//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/notice.h"

#include "pxr/base/tf/pyNoticeWrapper.h"

#include "pxr/external/boost/python/scope.hpp"
#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace
{

TF_INSTANTIATE_NOTICE_WRAPPER(
    ArNotice::ResolverNotice, TfNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(
    ArNotice::ResolverChanged, ArNotice::ResolverNotice);

} // end anonymous namespace

void
wrapNotice()
{
    scope s = class_<ArNotice>("Notice", no_init);
    
    TfPyNoticeWrapper<ArNotice::ResolverNotice, TfNotice>::Wrap();

    TfPyNoticeWrapper<
        ArNotice::ResolverChanged, ArNotice::ResolverNotice>::Wrap()
        .def("AffectsContext", &ArNotice::ResolverChanged::AffectsContext,
             args("context"))
        ;
}
