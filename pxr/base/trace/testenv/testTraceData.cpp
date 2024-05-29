//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/eventNode.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/stringUtils.h"
#include <iostream>


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

// Flatten the keys and values tuples so the result is suitable to construct a
// TraceAutoScope.
// This returns a tuple of the form
// (data, std::get<0>(keys), std::get<0>(values),
//        std::get<1>(keys), std::get<1>(values), ...)
template <typename KeysTuple, typename ValuesTuple, size_t... Indices>
auto Trace_MakeAutoScopeArgs(const TraceStaticKeyData& data,
                             KeysTuple&& keys,
                             ValuesTuple&& values,
                             std::index_sequence<Indices...>) {
    // Forward to preserve references
    return std::tuple_cat(
        std::forward_as_tuple(data),
        std::forward_as_tuple(
            std::get<Indices>(keys), std::get<Indices>(values)) ...);
}

// _TRACE_FUNCTION_ARGS_IMPL needs a value to match the last comma
// in the converted key and value sequences. This value is then dropped
// when the auto scope arguments are constructed.
#define _TRACE_UNUSED_TAIL nullptr

// Create a tuple to store the arguments to ensure the lifetime of the
// static key data outlives the referencing scope.
// Even though TraceScopeAuto isn't movable, std::make_from_tuple benefits
// from guaranteed copy collision.
// The index sequence used to zip keys and values is one less than the tuple
// size because a spurious element is added to each tuple to match the commas.
#define _TRACE_FUNCTION_ARGS_IMPL(keyDataIdentifier, keysIdentifier, scopeIdentifier, ...) \
constexpr static auto keysIdentifier = std::make_tuple( \
    _TRACE_ARGS_TO_KEYS(__VA_ARGS__) _TRACE_UNUSED_TAIL); \
auto scopeIdentifier = std::make_from_tuple<TraceScopeAuto>( \
    Trace_MakeAutoScopeArgs( \
        keyDataIdentifier, \
        keysIdentifier, \
        std::forward_as_tuple(_TRACE_ARGS_TO_VALUES(__VA_ARGS__) _TRACE_UNUSED_TAIL), \
        std::make_index_sequence< \
            std::tuple_size_v<decltype(keysIdentifier)> - 1>()));

#define TRACE_FUNCTION_ARGS_INSTANCE(instance, name, prettyName, ...) \
constexpr static TraceStaticKeyData TF_PP_CAT(TraceKeyData_, instance)( \
    name, prettyName); \
_TRACE_FUNCTION_ARGS_IMPL(TF_PP_CAT(TraceKeyData_, instance), \
                          TF_PP_CAT(TraceAutoKeysTuple_, instance), \
                          TF_PP_CAT(TraceAutoScope_, instance), \
                          __VA_ARGS__) \

#define TRACE_SCOPE_ARGS_INSTANCE(instance, name, ...) \
constexpr static TraceStaticKeyData TF_PP_CAT(TraceKeyData_, instance)(name); \
_TRACE_FUNCTION_ARGS_IMPL(TF_PP_CAT(TraceKeyData_, instance), \
                          TF_PP_CAT(TraceAutoKeysTuple_, instance), \
                          TF_PP_CAT(TraceAutoScope_, instance), \
                          __VA_ARGS__) \

#define _TRACE_KEY_FROM_TUPLE(elem) TF_PP_TUPLE_ELEM(0, elem)
#define _TRACE_VALUE_FROM_TUPLE(elem) TF_PP_TUPLE_ELEM(1, elem)

#define _TRACE_KEY_ELEM(elem) TraceStaticKeyData{_TRACE_KEY_FROM_TUPLE(elem)},
#define _TRACE_ARGS_TO_KEYS(...) TF_PP_FOR_EACH(_TRACE_KEY_ELEM, __VA_ARGS__)
#define _TRACE_VALUE_ELEM(elem) _TRACE_VALUE_FROM_TUPLE(elem),
#define _TRACE_ARGS_TO_VALUES(...) \
    TF_PP_FOR_EACH(_TRACE_VALUE_ELEM, __VA_ARGS__)

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
