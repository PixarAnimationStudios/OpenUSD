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

#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/eventNode.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/stringUtils.h"
#include <iostream>

#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

#if !defined(TRACE_FUNCTION_ARGS)

/// Records a begin and  data events when constructed and an end event when 
/// destructed, using the name of the function or method as the key for the
/// scope. The macro arguments must come in key value pairs.
#define TRACE_FUNCTION_ARGS(...) \
        TRACE_FUNCTION_ARGS_INSTANCE(__LINE__, __ARCH_FUNCTION__, \
            __ARCH_PRETTY_FUNCTION__, __VA_ARGS__)
#endif

#if !defined(TRACE_SCOPE_ARGS)
/// Records a begin and data events when constructed and an end event when
/// destructed, using \a name as the key. The macro data arguments must come in
/// key value pairs.
#define TRACE_SCOPE_ARGS(name, ...) \
        TRACE_SCOPE_ARGS_INSTANCE(__LINE__, name, __VA_ARGS__)
#endif

/// Records a data event using the name as the data key. The value can be a 
/// boolean, integeral types, a double or a string. The data will be stored with
/// the currently traced scope.
#define TRACE_DATA(name, value) \
    TraceCollector::GetInstance().StoreData(name, value);

#define TRACE_FUNCTION_ARGS_INSTANCE(instance, name, prettyName, ...) \
constexpr static TraceStaticKeyData BOOST_PP_CAT(TraceKeyData_, instance)( \
    name, prettyName); \
_TRACE_ARGS_TO_STATIC_VARS(BOOST_PP_CAT(TraceKeyData_, instance), __VA_ARGS__); \
TraceScopeAuto BOOST_PP_CAT(TraceScopeAuto_, instance)(\
        BOOST_PP_CAT(TraceKeyData_, instance), \
        _TRACE_ARGS_TO_FUNC_PARAMS(BOOST_PP_CAT(TraceKeyData_, instance), \
            __VA_ARGS__));

#define TRACE_SCOPE_ARGS_INSTANCE(instance, name, ...) \
constexpr static TraceStaticKeyData BOOST_PP_CAT(TraceKeyData_, instance)(name); \
_TRACE_ARGS_TO_STATIC_VARS(BOOST_PP_CAT(TraceKeyData_, instance), __VA_ARGS__); \
TraceScopeAuto BOOST_PP_CAT(TraceScopeAuto_, instance)(\
        BOOST_PP_CAT(TraceKeyData_, instance), \
        _TRACE_ARGS_TO_FUNC_PARAMS(BOOST_PP_CAT(TraceKeyData_, instance), \
            __VA_ARGS__));

#define _TRACE_KEY_FROM_TUPLE(r, data, elem) BOOST_PP_TUPLE_ELEM(2, 0, elem)
#define _TRACE_VALUE_FROM_TUPLE(r, data, elem) BOOST_PP_TUPLE_ELEM(2, 1, elem)

#define _TRACE_ARGS_TO_KEY_SEQ(...) \
    BOOST_PP_SEQ_TRANSFORM(\
        _TRACE_KEY_FROM_TUPLE, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)\
    )

#define _TRACE_ARGS_TO_VAL_SEQ(...) \
    BOOST_PP_SEQ_TRANSFORM(\
        _TRACE_VALUE_FROM_TUPLE, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)\
    )
#define _TRACE_KEY_KEY_DEF(r, data, i, elem) \
    constexpr static TraceStaticKeyData BOOST_PP_CAT(data, i)(elem);

#define _TRACE_ARGS_TO_STATIC_VARS(varname,...) \
    BOOST_PP_SEQ_FOR_EACH_I(_TRACE_KEY_KEY_DEF, varname, \
        _TRACE_ARGS_TO_KEY_SEQ(__VA_ARGS__))

#define _TRACE_ARG_FUNC_PARAM( r, data, i, elem) \
    BOOST_PP_COMMA_IF(i) BOOST_PP_CAT(data, i) BOOST_PP_COMMA() elem
#define _TRACE_ARGS_TO_FUNC_PARAMS(varname, ...) \
    BOOST_PP_SEQ_FOR_EACH_I(\
        _TRACE_ARG_FUNC_PARAM, varname, _TRACE_ARGS_TO_VAL_SEQ(__VA_ARGS__))

void TestFunc(short a, float b, bool c) {
    TRACE_FUNCTION_ARGS(("a", a), ("b", b), ("c", c));

    std::string str(TfStringPrintf("Test String %d", a));
    const char* cstr = str.c_str();
    TRACE_SCOPE_ARGS("Inner Scope", 
        ("str", str),
        ("cstr", cstr),
        ("str literal", "A String Literal"),
        ("sign string", a > 0 ? "Positive" : "Not Positive")
    );
}

TraceEventNodeRefPtr FindNode(
    TraceEventNodeRefPtr root,
    const std::string& name) {
    if (root->GetKey() == name) {
        return root;
    }
    for (TraceEventNodeRefPtr child : root->GetChildrenRef()) {
        TraceEventNodeRefPtr result = FindNode(child, name);
        if (result) {
            return result;
        }
    }
    return TraceEventNodeRefPtr();
}

int
main(int argc, char *argv[])
{
    TraceCollector* collector = &TraceCollector::GetInstance();
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    collector->SetEnabled(true);
    TestFunc(1, 2.5, true);
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    // Validate the data that was stored in the trace
    {
        TraceEventNodeConstRefPtr funcNode = 
            FindNode(reporter->GetEventRoot(), "TestFunc");
        TF_AXIOM(funcNode);
        const TraceEventNode::AttributeMap& attrs = 
            funcNode->GetAttributes();
        TF_AXIOM(attrs.size() == 3);
        TF_AXIOM(attrs.count(TfToken("a")) == 1);
        TF_AXIOM(*attrs.find(TfToken("a"))->second.GetInt() == 1);
        TF_AXIOM(attrs.count(TfToken("b")) == 1);
        TF_AXIOM(*attrs.find(TfToken("b"))->second.GetFloat() ==  2.5);
        TF_AXIOM(attrs.count(TfToken("c")) == 1);
        TF_AXIOM(*attrs.find(TfToken("c"))->second.GetBool() == true);
    }

    {
        TraceEventNodeConstRefPtr funcNode = 
            FindNode(reporter->GetEventRoot(), "Inner Scope");
        TF_AXIOM(funcNode);
        const TraceEventNode::AttributeMap& attrs = 
            funcNode->GetAttributes();
        TF_AXIOM(attrs.size() == 4);
        TF_AXIOM(attrs.count(TfToken("str")) == 1);
        TF_AXIOM(*attrs.find(TfToken("str"))->second.GetString() 
            == "Test String 1");
        TF_AXIOM(*attrs.find(TfToken("cstr"))->second.GetString() 
            == "Test String 1");
        TF_AXIOM(attrs.count(TfToken("str literal")) == 1);
        TF_AXIOM(*attrs.find(TfToken("str literal"))->second.GetString() 
            == "A String Literal");
        TF_AXIOM(attrs.count(TfToken("sign string")) == 1);
        TF_AXIOM(*attrs.find(TfToken("sign string"))->second.GetString() 
            == "Positive");
    }

    reporter->ClearTree();
    collector->SetEnabled(true);
    TestFunc(-8, 9.0, false);
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    // Validate the data that was stored in the trace
    {
        TraceEventNodeConstRefPtr funcNode = 
            FindNode(reporter->GetEventRoot(), "TestFunc");
        TF_AXIOM(funcNode);
        const TraceEventNode::AttributeMap& attrs = 
            funcNode->GetAttributes();
        TF_AXIOM(attrs.size() == 3);
        TF_AXIOM(attrs.count(TfToken("a")) == 1);
        TF_AXIOM(*attrs.find(TfToken("a"))->second.GetInt() == -8);
        TF_AXIOM(attrs.count(TfToken("b")) == 1);
        TF_AXIOM(*attrs.find(TfToken("b"))->second.GetFloat() ==  9.0);
        TF_AXIOM(attrs.count(TfToken("c")) == 1);
        TF_AXIOM(*attrs.find(TfToken("c"))->second.GetBool() == false);
    }

    {
        TraceEventNodeConstRefPtr funcNode = 
            FindNode(reporter->GetEventRoot(), "Inner Scope");
        TF_AXIOM(funcNode);
        const TraceEventNode::AttributeMap& attrs = 
            funcNode->GetAttributes();
        TF_AXIOM(attrs.size() == 4);
        TF_AXIOM(attrs.count(TfToken("str")) == 1);
        TF_AXIOM(*attrs.find(TfToken("str"))->second.GetString() 
            == "Test String -8");
        TF_AXIOM(*attrs.find(TfToken("cstr"))->second.GetString() 
            == "Test String -8");
        TF_AXIOM(attrs.count(TfToken("str literal")) == 1);
        TF_AXIOM(*attrs.find(TfToken("str literal"))->second.GetString() 
            == "A String Literal");
        TF_AXIOM(attrs.count(TfToken("sign string")) == 1);
        TF_AXIOM(*attrs.find(TfToken("sign string"))->second.GetString() 
            == "Not Positive");
    }
}
