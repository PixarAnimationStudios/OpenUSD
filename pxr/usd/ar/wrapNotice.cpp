//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/ar/pyResolverContext.h"

#include "pxr/usd/ar/notice.h"

#include "pxr/base/tf/pyNoticeWrapper.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapNotice(nanobind::module_ &m)
{
    nanobind::class_<ArNotice> s(m, "Notice");

    TfNbNoticeWrapper<ArNotice::ResolverNotice, TfNotice>::Wrap(s);

    TfNbNoticeWrapper<
        ArNotice::ResolverChanged, ArNotice::ResolverNotice>::Wrap(s)
        .def("AffectsContext", &ArNotice::ResolverChanged::AffectsContext,
             nanobind::arg("context"))
        ;
}
