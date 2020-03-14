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
#include "pxr/base/plug/notice.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNoticeWrapper.h"

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
