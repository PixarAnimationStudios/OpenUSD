//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/plug/notice.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNoticeWrapper.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/scope.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

TF_INSTANTIATE_NOTICE_WRAPPER(PlugNotice::Base, TfNotice);
TF_INSTANTIATE_NOTICE_WRAPPER(PlugNotice::DidRegisterPlugins, PlugNotice::Base);

} // anonymous namespace 

void
wrapNotice()
{
    scope noticeScope = class_<PlugNotice>("Notice", no_init);

    TfPyNoticeWrapper<PlugNotice::Base, TfNotice>::Wrap()
        ;

    TfPyNoticeWrapper<PlugNotice::DidRegisterPlugins, PlugNotice::Base>::Wrap()
        .def("GetNewPlugins", 
             make_function(&PlugNotice::DidRegisterPlugins::GetNewPlugins,
                           return_value_policy<TfPySequenceToList>()))
        ;
}
