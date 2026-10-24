//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/mallocTag.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/symbols.h"

#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static bool
_Initialize()
{
    string reason;
    return TfMallocTag::Initialize(&reason);
}

static TfMallocTag::CallTree
_GetCallTree(const bool skipRepeated)
{
    TfMallocTag::CallTree ret;
    TfMallocTag::GetCallTree(&ret, skipRepeated);
    return ret;
}

static
std::vector<std::string>
_GetCallStacks()
{
    std::vector<std::vector<uintptr_t> > stacks =
        TfMallocTag::GetCapturedMallocStacks();

    // Cache address to function name map, one lookup per address.
    std::map<uintptr_t, std::string> functionNames;
    TF_FOR_ALL(stack, stacks) {
        TF_FOR_ALL(func, *stack) {
            std::string& name = functionNames[*func];
            if (name.empty()) {
                ArchGetAddressInfo(reinterpret_cast<void*>(*func),
                                   nullptr, nullptr, &name, nullptr);
                if (name.empty()) {
                    name = "<unknown>";
                }
            }
        }
    }

    std::vector<std::string> result;
    TF_FOR_ALL(stack, stacks) {
        result.push_back(std::string());
        std::string& trace = result.back();
        TF_FOR_ALL(func, *stack) {
            trace += TfStringPrintf("  0x%016lx: %s\n",
                                    (unsigned long)*func,
                                    functionNames[*func].c_str());
        }
        trace += '\n';
    }
    return result;
}

static string
_GetPrettyPrintString(TfMallocTag::CallTree const &self)
{
    return self.GetPrettyPrintString();
}

static vector<TfMallocTag::CallTree::CallSite>
_GetCallSites(TfMallocTag::CallTree const &self)
{
    return self.callSites;
}

static TfMallocTag::CallTree::PathNode
_GetRoot(TfMallocTag::CallTree const &self)
{
    return self.root;
}

static vector<TfMallocTag::CallTree::PathNode>
_GetChildren(TfMallocTag::CallTree::PathNode const &self)
{
    return self.children;
}

static void
_Report(
    TfMallocTag::CallTree const &self,
    std::string const &rootName)
{
    self.Report(std::cout, rootName);
}

static void
_ReportToFile(
    TfMallocTag::CallTree const &self,
    std::string const &fileName,
    std::string const &rootName)
{
    std::ofstream os(fileName.c_str());
    self.Report(os, rootName);
}

static bool
_LoadReport(
    TfMallocTag::CallTree  &self,
    std::string const &fileName)
{
    std::ifstream in(fileName.c_str());
    if (!in.good()) {
        TF_RUNTIME_ERROR(
            "Failed to open file '%s'.", fileName.c_str());
        return false;
    }

    return self.LoadReport(in);
}

static std::string
_LogReport(
    TfMallocTag::CallTree const &self,
    std::string const &rootName)
{
    string tmpFile;
    int fd = ArchMakeTmpFile(std::string("callSiteReport") +
        (rootName.empty() ? "" : "_") + rootName, &tmpFile);
    if (fd == -1) {
        TF_RUNTIME_ERROR(
            "Failed to make temporary file '%s'.", tmpFile.c_str());
        return std::string();
    }

    ArchCloseFile(fd);
    _ReportToFile(self, tmpFile, rootName);
    return tmpFile;
}

struct Tf_PyMallocTagPauseControl
{
public:
    using CreateFn = TfMallocTag::PauseControl (*)();
    
    explicit Tf_PyMallocTagPauseControl(CreateFn create) : _create(create) {}
    
    TfMallocTag::PauseControl &__enter__() {
        _ctrl.reset(new TfMallocTag::PauseControl { _create() });
        return *_ctrl;
    }

    void __exit__(nanobind::handle, nanobind::handle, nanobind::handle) {
        _ctrl->Unpause();
    }
private:
    std::unique_ptr<TfMallocTag::PauseControl> _ctrl;
    CreateFn _create;
};

} // anonymous namespace 

void wrapMallocTag(nanobind::module_& m)
{
    typedef TfMallocTag This;
    namespace nb = nanobind;
    
    nb::class_<This> mallocTag(m, "MallocTag");
    mallocTag
        .def_static("Initialize", _Initialize)
        .def_static("IsInitialized", This::IsInitialized)
        .def_static("Shutdown", This::Shutdown)
        .def_static("Clear", This::Clear)
        .def_static("PauseThisThread", +[]() {
            return new Tf_PyMallocTagPauseControl([]() {
                return TfMallocTag::PauseThisThread();
            });
        }, nb::rv_policy::take_ownership)
        .def_static("PauseAllThreads", +[]() {
            return new Tf_PyMallocTagPauseControl([]() {
                return TfMallocTag::PauseAllThreads();
            });
        }, nb::rv_policy::take_ownership)
        .def_static("DeferredPause", +[]() {
            return new Tf_PyMallocTagPauseControl([]() {
                return TfMallocTag::DeferredPause();
            });
        }, nb::rv_policy::take_ownership)
        .def_static("GetTotalBytes", This::GetTotalBytes)
        .def_static("GetMaxTotalBytes", This::GetMaxTotalBytes)
        .def_static("GetCallTree", _GetCallTree, nb::arg("skipRepeated")=true)
        .def_static("GetPerfStats", This::GetPerfStats,
                    nb::arg("includePerThread")=false)
        .def_static("SetCapturedMallocStacksMatchList",
             This::SetCapturedMallocStacksMatchList)
        .def_static("GetCallStacks", _GetCallStacks)
        .def_static("SetDebugMatchList", This::SetDebugMatchList)
        ;

    {
        using This = TfMallocTag::PauseControl;
        nb::class_<This>(mallocTag, "_PauseCtrlCtx")
            .def("PauseThisThread", &This::PauseThisThread)
            .def("PauseAllThreads", &This::PauseAllThreads)
            .def("Unpause",         &This::Unpause)
            .def("IsPaused",        &This::IsPaused)
            ;
    }
    {
        using This = Tf_PyMallocTagPauseControl;
        nb::class_<This>(mallocTag, "PauseControl")
            .def("__enter__", &This::__enter__, nb::rv_policy::reference_internal)
            .def("__exit__",  &This::__exit__)
            ;
    }
    {
        nb::class_<This::CallTree> callTree(mallocTag, "CallTree");
        callTree
            .def(nb::init<>())
            .def("GetPrettyPrintString", _GetPrettyPrintString)
            .def("GetCallSites", _GetCallSites)
            .def("GetRoot", _GetRoot)
            .def("Report", _Report,
                nb::arg("rootName")=std::string())
            .def("Report", _ReportToFile,
                 nb::arg("fileName"), nb::arg("rootName")=std::string())
            .def("LoadReport", _LoadReport,
                 nb::arg("fileName"))
            .def("LogReport", _LogReport,
                 nb::arg("rootName")=std::string())
            ;

        nb::class_<This::CallTree::PathNode>(callTree, "PathNode")
            .def_ro("nBytes", &This::CallTree::PathNode::nBytes)
            .def_ro("nBytesDirect", &This::CallTree::PathNode::nBytesDirect)
            .def_ro("nAllocations", &This::CallTree::PathNode::nAllocations)
            .def_ro("siteName", &This::CallTree::PathNode::siteName)
            .def("GetChildren", _GetChildren)
            ;

        nb::class_<This::CallTree::CallSite>(callTree, "CallSite")
            .def_ro("name", &This::CallTree::CallSite::name)
            .def_ro("nBytes", &This::CallTree::CallSite::nBytes)
            ;
    }
}
