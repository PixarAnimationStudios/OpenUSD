#include "pxr/pxr.h"

#include "webSyncDriver.h"

#include <emscripten/bind.h>
using namespace emscripten;

std::shared_ptr<pxr::HdWebSyncDriver> CreateFromStage(emscripten::val renderDelegateInterface, pxr::UsdStageRefPtr const& stage) {
  std::shared_ptr<pxr::HdWebSyncDriver> result(new pxr::HdWebSyncDriver(renderDelegateInterface, stage));

  return result;
}

EMSCRIPTEN_BINDINGS(test_usd_imaging_emscripten) {
  class_<pxr::HdWebSyncDriver>("HdWebSyncDriver")
    .constructor<emscripten::val, std::string>()
    .class_function("CreateFromStage", &CreateFromStage)
    .function("Draw", &pxr::HdWebSyncDriver::Draw)
    .function("getFile", &pxr::HdWebSyncDriver::getFile)
    .function("GetStage", &pxr::HdWebSyncDriver::GetStage)
    .function("SetTime", &pxr::HdWebSyncDriver::SetTime)
    .function("GetTime", &pxr::HdWebSyncDriver::GetTime)
    .smart_ptr<std::shared_ptr<pxr::HdWebSyncDriver>>("std::shared_ptr<pxr::HdWebSyncDriver>")
    ;

  register_vector<int>("VectorInt");
  register_vector<double>("VectorDouble");
}
