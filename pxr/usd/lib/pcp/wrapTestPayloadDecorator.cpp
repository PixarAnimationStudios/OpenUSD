//
// Copyright 2019 Pixar
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
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/payloadContext.h"
#include "pxr/usd/pcp/payloadDecorator.h"

#include <boost/noncopyable.hpp>
#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct _TestPayloadDecorator
{
    // Derived class implementation of PcpPayloadDecorator purely for wrapping 
    // to python for use by unit tests. This test decorator accepts string 
    // values from two fields "documentation" and "kind", and adds them to 
    // the file format args keyed as "doc" and "kind" respectively. These
    // values are converted to lower case strings as well to help test the
    // field change relevance API.
    class Pcp_PyTestPayloadDecorator : public PcpPayloadDecorator
    {
    public:
        static TfRefPtr<Pcp_PyTestPayloadDecorator> New()
        {
            return TfCreateRefPtr(new Pcp_PyTestPayloadDecorator());
        }
        ~Pcp_PyTestPayloadDecorator() override {};

    private:
        Pcp_PyTestPayloadDecorator() 
        {
            // Initialize the relevant field mapping
            _fieldToArgMap[TfToken("documentation")] = "doc";
            _fieldToArgMap[TfToken("kind")] = "kind";
        };

        // Helper for converting VtValue to lower case string.
        static std::string _GetLowerCaseStringValue(const VtValue &val)
        {
            std::string value;
            // String values may be sent to this decorator as tokens so we 
            // check for both strings and tokens.
            if (val.IsHolding<std::string>()){
                value = val.UncheckedGet<std::string>();
            } else if (val.IsHolding<TfToken>()) {
                value = val.UncheckedGet<TfToken>().GetString();
            }
            return TfStringToLower(value);
        }

        void _DecoratePayload(
              const SdfPath& primIndexPath,
              const SdfPayload& payload,
              const PcpPayloadContext& context,
              SdfLayer::FileFormatArguments* args) override
         {
            // Simple decoration. Grabs the first value found for the relevant
            // fields, converts it to lower case and adds it under the mapped
            // key in the args.
            for (const auto &fieldArgPair : _fieldToArgMap) {
                std::string value;
                context.ComposeValue(fieldArgPair.first, 
                    [&value](VtValue *val) {
                        value = _GetLowerCaseStringValue(*val);
                        return true;
                    }
                );
                (*args)[fieldArgPair.second] = 
                    !value.empty() ? TfStringToLower(value) : "none";
            }
         };
                   
         bool _IsFieldRelevantForDecoration(const TfToken& field) override
         {
             for (const auto &fieldArgPair : _fieldToArgMap) {
                 if (field == fieldArgPair.first) {
                     return true;
                 }
             }
             return false;
         };
                           
         bool _IsFieldChangeRelevantForDecoration(
              const SdfPath& primIndexPath,
              const SdfLayerHandle& siteLayer,
              const SdfPath& sitePath,
              const TfToken& field,
              const std::pair<VtValue, VtValue>& oldAndNewValues) override
         {
             // This function should never be called if 
             // _IsFieldRelevantForDecoration wasn't called. This verify is the
             // equivalent of an assert when in a python test.
             TF_VERIFY(_IsFieldRelevantForDecoration(field));

             // Case only changes are irrelevant in our test.
             return (_GetLowerCaseStringValue(oldAndNewValues.first) !=
                 _GetLowerCaseStringValue(oldAndNewValues.second));
         };

         std::map<TfToken, std::string> _fieldToArgMap;
    };

    // Creates a PcpCache with our test decorator for the given layer. This
    // the only way to directly create a PcpCache with a decorator in python.
    static boost::shared_ptr<PcpCache>
    CreateTestDecoratorPcpCache(const SdfLayerHandle &layer)
    {
        return boost::shared_ptr<PcpCache>(
            new PcpCache(PcpLayerStackIdentifier(layer),
                         std::string(), 
                         false, 
                         Pcp_PyTestPayloadDecorator::New()));
    }
};

} // anonymous namespace 

void
wrapTestPayloadDecorator()
{
    register_ptr_to_python<boost::shared_ptr<PcpCache> >(); 
    class_<_TestPayloadDecorator>("_TestPayloadDecorator")
        .def("CreatePcpCacheWithTestDecorator", 
             &_TestPayloadDecorator::CreateTestDecoratorPcpCache)
            .staticmethod("CreatePcpCacheWithTestDecorator")
        ;
}
