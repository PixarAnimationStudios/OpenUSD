//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/usdMtlx/backdoor.h"
#include "pxr/usd/usdMtlx/reader.h"
#include "pxr/usd/usdMtlx/utils.h"
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
        auto doc = reader();
        if (!doc) {
            return TfNullPtr;
        }

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
    return _MtlxTest(
        [&](){
            auto d = mx::createDocument();
            mx::readFromXmlString(d, buffer);
            return d; 
        },
        nodeGraphs);
}

UsdStageRefPtr
UsdMtlx_TestFile(const std::string& pathname, bool nodeGraphs)
{
    return _MtlxTest(
        [&](){ return UsdMtlxReadDocument(pathname); },
        nodeGraphs);
}

PXR_NAMESPACE_CLOSE_SCOPE
