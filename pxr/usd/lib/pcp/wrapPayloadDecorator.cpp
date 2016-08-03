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
#include "pxr/usd/pcp/payloadDecorator.h"

#include "pxr/usd/pcp/payloadContext.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/payload.h"
#include "pxr/usd/sdf/pyUtils.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPolymorphic.h"
#include "pxr/base/tf/pyPtrHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/pure_virtual.hpp>

using namespace boost::python;

class Pcp_PolymorphicPayloadDecorator
    : public PcpPayloadDecorator
    , public TfPyPolymorphic<PcpPayloadDecorator>
{
public:
    static TfRefPtr<Pcp_PolymorphicPayloadDecorator> New()
    {
        return TfCreateRefPtr(new Pcp_PolymorphicPayloadDecorator);
    }

    virtual ~Pcp_PolymorphicPayloadDecorator();

    // Adapt the pointer-based API to Python. Python subclasses will need to
    // implement DecoratePayload functions that take an Sdf.Payload and 
    // return a dictionary.
    SdfLayer::FileFormatArguments DecoratePayload(
        const SdfPayload& payload,
        const PcpPayloadContext& context)
    {
        SdfLayer::FileFormatArguments args;
        PcpPayloadDecorator::DecoratePayload(payload, context, &args);
        return args;
    }

    virtual void _DecoratePayload(
        const SdfPayload& payload,
        const PcpPayloadContext& context,
        SdfLayer::FileFormatArguments* args)
    {
        *args = _DecoratePayload(payload, context);
    }

    virtual SdfLayer::FileFormatArguments _DecoratePayload(
        const SdfPayload& payload,
        const PcpPayloadContext& context)
    {
        const dict argsDict = 
            CallPureVirtual<dict>("_DecoratePayload")(payload, context);
        return _GetArgumentsFromDict(argsDict);
    }

    virtual bool _IsFieldRelevantForDecoration(
        const SdfLayerHandle& layer,
        const SdfPath& path,
        const TfToken& field)
    {
        return CallPureVirtual<bool>("_IsFieldRelevantForDecoration")
            (layer, path, field);
    }

private:
    SdfLayer::FileFormatArguments _GetArgumentsFromDict(const dict& dict)
    {
        SdfLayer::FileFormatArguments args;
        std::string errMsg;
        if (not SdfFileFormatArgumentsFromPython(dict, &args, &errMsg)) {
            TF_CODING_ERROR(errMsg.c_str());
        }
        return args;
    }
};

Pcp_PolymorphicPayloadDecorator::~Pcp_PolymorphicPayloadDecorator()
{
    // Do nothing
}

void
wrapPayloadDecorator()
{
    typedef PcpPayloadDecorator This;
    typedef Pcp_PolymorphicPayloadDecorator PolymorphicThis;
    typedef TfWeakPtr<PolymorphicThis> PolymorphicThisPtr;

    class_<PolymorphicThis, PolymorphicThisPtr, boost::noncopyable>
        ("PayloadDecorator", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(&PolymorphicThis::New))

        .def("DecoratePayload", 
            (SdfLayer::FileFormatArguments(This::*)
                (const SdfPayload&, const PcpPayloadContext&))
            &This::DecoratePayload)

        .def("IsFieldRelevantForDecoration", 
            &This::IsFieldRelevantForDecoration)

        .def("_DecoratePayload", 
            (SdfLayer::FileFormatArguments(PolymorphicThis::*)
                (const SdfPayload&, const PcpPayloadContext&))
            &PolymorphicThis::_DecoratePayload)

        .def("_IsFieldRelevantForDecoration", 
            pure_virtual(&PolymorphicThis::_IsFieldRelevantForDecoration))
        ;
}

#if defined(ARCH_COMPILER_MSVC)
// There is a bug in the compiler which means we have to provide this
// implementation. See here for more information:
// https://connect.microsoft.com/VisualStudio/Feedback/Details/2852624
namespace boost
{
    template<> 
    const volatile PcpPayloadDecorator* 
        get_pointer(const volatile PcpPayloadDecorator* p)
    { 
        return p; 
    }
    template<> 
    const volatile Pcp_PolymorphicPayloadDecorator* 
        get_pointer(const volatile Pcp_PolymorphicPayloadDecorator* p)
    { 
        return p; 
    }
}
#endif 