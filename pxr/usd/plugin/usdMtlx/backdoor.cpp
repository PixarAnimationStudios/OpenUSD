//
// Copyright 2018 Pixar
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

#include "pxr/usd/usdMtlx/backdoor.h"
#include "pxr/usd/usdMtlx/reader.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/diagnostic.h"

#include <MaterialXFormat/XmlIo.h>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/// Read a MaterialX document then convert it using UsdMtlxRead().
template <typename R>
static
UsdStageRefPtr
_MtlxTest(R&& reader, bool nodeGraphs)
{
    try {
        auto doc = mx::createDocument();
        reader(doc);

        auto stage = UsdStage::CreateInMemory("tmp.usda", TfNullPtr);
        if (nodeGraphs) {
            UsdMtlxReadNodeGraphs(doc, stage);
        }
        else {
            UsdMtlxRead(doc, stage);
        }
        return stage;
    }
    catch (mx::ExceptionFoundCycle& x) {
        TF_RUNTIME_ERROR("MaterialX cycle found: %s", x.what());
        return TfNullPtr;
    }
    catch (mx::Exception& x) {
        TF_RUNTIME_ERROR("MaterialX read failed: %s", x.what());
        return TfNullPtr;
    }
}

} // anonymous namespace

UsdStageRefPtr
UsdMtlx_TestString(const std::string& buffer, bool nodeGraphs)
{
    return _MtlxTest([&](mx::DocumentPtr d){mx::readFromXmlString(d, buffer);},
                     nodeGraphs);
}

UsdStageRefPtr
UsdMtlx_TestFile(const std::string& pathname, bool nodeGraphs)
{
    return _MtlxTest([&](mx::DocumentPtr d){mx::readFromXmlFile(d, pathname);},
                     nodeGraphs);
}

PXR_NAMESPACE_CLOSE_SCOPE
