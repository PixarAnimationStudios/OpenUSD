//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/variableExpression.h"
#include "pxr/usd/sdf/variableExpressionAST.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace std {
template <>
struct is_copy_constructible<PXR_NS::SdfVariableExpressionASTNodes::NodeList>
    : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::SdfVariableExpressionASTNodes::NodeList>
    : std::false_type {};
template <>
struct is_copy_constructible<PXR_NS::SdfVariableExpressionASTNodes::ListNode>
    : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::SdfVariableExpressionASTNodes::ListNode>
    : std::false_type {};
template <>
struct is_copy_constructible<
    PXR_NS::SdfVariableExpressionASTNodes::FunctionNode> : std::false_type {};
template <>
struct is_move_constructible<
    PXR_NS::SdfVariableExpressionASTNodes::FunctionNode> : std::false_type {};
}  // namespace std

namespace nb = nanobind;

class Sdf_VariableExpressionASTNodesNS
{
};

static void
wrapVariableExpressionASTNodes(nanobind::module_& m)
{
    using namespace SdfVariableExpressionASTNodes;

    nb::class_<Sdf_VariableExpressionASTNodesNS> s(
        m, "VariableExpressionASTNodes");

    {
        using This = Node;
        nb::class_<This>(s, "Node")
            .def("GetExpression", &This::GetExpression)
            ;
    }

    {
        using This = NodeList;
        nb::class_<This>(s, "NodeList")
            // XXX: Nodes in list need to have custodian_and_ward policy?
            .def("GetNodes", 
                [](This& l) {
                    nb::list nodes;
                    for (auto* n : l.GetNodes()) {
                        nodes.append(nb::cast(n, nb::rv_policy::reference));
                    }
                    return nodes;
                })

            .def("Append", &This::Append)
            .def("Set", &This::Set)
            .def("Insert", &This::Insert)
            .def("Remove", &This::Remove)
            .def("Clear", &This::Clear)
            ;
    }

    {
        using This = LiteralNode;
        nb::class_<This, Node>(s, "LiteralNode")
            .def("GetValue", 
                [](const This& n) -> nb::object {
                    return std::visit(
                        [](auto&& arg) -> nb::object { 
                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (std::is_same_v<T, std::monostate>) {
                                return nb::none();
                            }
                            else {
                                return nb::cast(arg);
                            }
                        }, 
                        n.GetValue());
                    })

            .def("SetValue",
                [](This& n, const VtValue& v) {
                    if (v.IsEmpty()) {
                        n.SetValue({});
                    }
                    else if (v.IsHolding<bool>()) {
                        n.SetValue(v.UncheckedGet<bool>());
                    }
                    else if (v.CanCast<int64_t>()) {
                        // Depending on how big the numeric value is, v
                        // may be holding an int or a long. So we just cast
                        // to int64_t to handle both cases.
                        n.SetValue(VtValue::Cast<int64_t>(v)
                            .UncheckedGet<int64_t>());
                    }
                    else if (v.IsHolding<std::string>()) {
                        n.SetValue(v.UncheckedGet<std::string>());
                    }
                    else {
                        TfPyThrowValueError("Invalid value for node");
                    }
                })
            ;
    }

    {
        using This = VariableNode;
        nb::class_<This, Node>(s, "VariableNode")
            .def("GetName", &This::GetName)
            .def("SetName", &This::SetName)
            .def("GetFallbackValue",
                +[](This& n) { return n.GetFallbackValue(); },
                nb::rv_policy::reference_internal)
            .def("SetFallbackValue", &This::SetFallbackValue)
            .def("ClearFallbackValue", &This::ClearFallbackValue)
            ;
    }

    {
        using This = ListNode;
        nb::class_<This, Node>(s, "ListNode")
            .def("GetElements",
                [](This& n) -> NodeList& { return n.GetElements(); },
                nb::rv_policy::reference_internal)
            ;
    }

    {
        using This = FunctionNode;
        nb::class_<This, Node>(s, "FunctionNode")
            .def("GetName", &This::GetName)
            .def("SetName", &This::SetName)
            .def("GetArguments", 
                [](This& fn) -> NodeList& { return fn.GetArguments(); },
                nb::rv_policy::reference_internal)
            ;
    }
}

void
wrapVariableExpressionAST(nanobind::module_& m)
{
    wrapVariableExpressionASTNodes(m);

    using This = SdfVariableExpressionAST;
    nb::class_<This>(m, "VariableExpressionAST")
        .def(nb::init<>())
        .def(nb::init<const This&>()) // deep copy
        .def(nb::init<const std::string&>(), nb::arg("expression"))
        .def(nb::init<const SdfVariableExpression&>(), nb::arg("expression"))

        .def("__bool__", &This::operator bool)

        .def("GetErrors", &This::GetErrors)

        .def("GetRoot", [](This& ast) { return ast.GetRoot(); },
            nb::rv_policy::reference_internal)

        .def("GetExpression", &This::GetExpression)
        ;

}
