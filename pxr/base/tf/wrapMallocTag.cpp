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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/scope.hpp"

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static bool
_Initialize()
{
    string reason;
    return TfMallocTag::Initialize(&reason);
}

static bool
_Initialize2(const std::string& captureTag)
{
    string reason;
    bool result = TfMallocTag::Initialize(&reason);
    if (result) {
        TfMallocTag::SetCapturedMallocStacksMatchList(captureTag);
    }
    return result;
}

static TfMallocTag::CallTree
_GetCallTree()
{
    TfMallocTag::CallTree ret;
    TfMallocTag::GetCallTree(&ret);
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
                                   NULL, NULL, &name, NULL);
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

} // anonymous namespace 

void wrapMallocTag()
{
    typedef TfMallocTag This;
    
    scope mallocTag = class_<This>("MallocTag", no_init)
        .def("Initialize", _Initialize2)
        .def("Initialize", _Initialize).staticmethod("Initialize")
        .def("IsInitialized", This::IsInitialized).staticmethod("IsInitialized")
        .def("GetTotalBytes", This::GetTotalBytes).staticmethod("GetTotalBytes")
        .def("GetMaxTotalBytes", This::GetMaxTotalBytes).staticmethod("GetMaxTotalBytes")
        .def("GetCallTree", _GetCallTree).staticmethod("GetCallTree")

        .def("SetCapturedMallocStacksMatchList",
             This::SetCapturedMallocStacksMatchList)
            .staticmethod("SetCapturedMallocStacksMatchList")
        .def("GetCallStacks", _GetCallStacks,
             return_value_policy<TfPySequenceToList>())
            .staticmethod("GetCallStacks")

        .def("SetDebugMatchList", This::SetDebugMatchList)
            .staticmethod("SetDebugMatchList")
        ;

    {
    scope callTree = class_<This::CallTree>("CallTree", no_init)
        .def("GetPrettyPrintString", _GetPrettyPrintString)
        .def("GetCallSites", _GetCallSites,
             return_value_policy<TfPySequenceToList>())
        .def("GetRoot", _GetRoot)
        .def("Report", _Report,
            (arg("rootName")=std::string()))
        .def("Report", _ReportToFile,
             (arg("fileName"), arg("rootName")=std::string()))
        .def("LoadReport", _LoadReport,
             (arg("fileName")))
        .def("LogReport", _LogReport,
             (arg("rootName")=std::string()))
        ;

    class_<This::CallTree::PathNode>("PathNode", no_init)
        .def_readonly("nBytes", &This::CallTree::PathNode::nBytes)
        .def_readonly("nBytesDirect", &This::CallTree::PathNode::nBytesDirect)
        .def_readonly("nAllocations", &This::CallTree::PathNode::nAllocations)
        .def_readonly("siteName", &This::CallTree::PathNode::siteName)
        .def("GetChildren", _GetChildren,
             return_value_policy<TfPySequenceToList>())
        ;

    class_<This::CallTree::CallSite>("CallSite", no_init)
        .def_readonly("name", &This::CallTree::CallSite::name)
        .def_readonly("nBytes", &This::CallTree::CallSite::nBytes)
        ;
    }

}
