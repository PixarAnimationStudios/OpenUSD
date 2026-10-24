//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/notice.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/pyNoticeWrapper.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapNotice(nanobind::module_ &m)
{
    nanobind::class_<PlugNotice> noticeScope(m, "Notice");

    TfNbNoticeWrapper<PlugNotice::Base, TfNotice>::Wrap(noticeScope)
        ;

    TfNbNoticeWrapper<
        PlugNotice::DidRegisterPlugins, PlugNotice::Base>::Wrap(noticeScope)
        .def("GetNewPlugins", &PlugNotice::DidRegisterPlugins::GetNewPlugins,
             nanobind::rv_policy::copy)
        ;
}
