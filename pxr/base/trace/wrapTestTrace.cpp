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

#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/collector.h"

#include <boost/python/def.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/list.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

// ----------------------------------------
// A set of functions using TRACE_FUNCTION, TRACE_SCOPE

static void
TestNestingFunc2()
{
    TRACE_FUNCTION();
}

static void
TestNestingFunc3()
{
    TRACE_FUNCTION();
    TRACE_SCOPE("Foo");

    TraceCollector& gc = TraceCollector::GetInstance();
    TF_UNUSED(gc);
}

static void
TestNestingFunc1()
{
    TRACE_FUNCTION();
    TestNestingFunc2();
    TestNestingFunc3();
}

static void
TestNesting()
{
    TRACE_FUNCTION();
    TestNestingFunc1();
}

// ----------------------------------------
// A set of functions using TraceAuto

static void
TestAutoFunc2()
{
    TraceAuto t(TF_FUNC_NAME());
}

static void
TestAutoFunc3()
{
    TraceAuto t(TF_FUNC_NAME());
}

static void
TestAutoFunc1()
{
    TraceAuto t(TF_FUNC_NAME());
    TestAutoFunc2();
    TestAutoFunc3();
}

static void
TestAuto()
{
    TraceAuto t(TF_FUNC_NAME());
    TestAutoFunc1();
}

static std::string
TestEventName()
{
    return "C_PLUS_PLUS_EVENT";
}


static void
TestCreateEvents()
{
    TraceCollector* gc = &TraceCollector::GetInstance();
    gc->BeginEvent(TestEventName());
    gc->EndEvent(TestEventName());
    
}

void wrapTestTrace()
{    
    def("TestNesting", &::TestNesting);
    def("TestAuto", &::TestAuto);
    def("TestCreateEvents", &::TestCreateEvents);
    def("GetTestEventName", &::TestEventName);
}

