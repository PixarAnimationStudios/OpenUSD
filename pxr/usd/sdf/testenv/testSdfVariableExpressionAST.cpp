//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdf/variableExpressionAST.h"
#include "pxr/usd/sdf/variableExpression.h"

#include <cstdio>
#include <type_traits>

PXR_NAMESPACE_USING_DIRECTIVE;

namespace AST = SdfVariableExpressionASTNodes;

template <class NodeType, class ExpectedNodeType, class NodePtr>
static void
_TestCast(NodePtr node)
{
    if constexpr (std::is_same_v<NodeType, ExpectedNodeType>) {
        TF_AXIOM(node->template As<NodeType>());
    }
    else {
        TF_AXIOM(!node->template As<NodeType>());
    }
}

template <class ExpectedNodeType, class NodePtr>
static void
_TestCasts(NodePtr node)
{
    _TestCast<AST::LiteralNode, ExpectedNodeType>(node);
    _TestCast<AST::VariableNode, ExpectedNodeType>(node);
    _TestCast<AST::ListNode, ExpectedNodeType>(node);
    _TestCast<AST::FunctionNode, ExpectedNodeType>(node);
}

static void _TestLiteralNode(
    const std::string& exprStr, 
    const std::string& expectedStr = "")
{
    SdfVariableExpressionAST ast(exprStr);
    _TestCasts<AST::LiteralNode>(ast.GetRoot());
    _TestCasts<AST::LiteralNode>(std::as_const(ast).GetRoot());

    AST::LiteralNode* literalNode = ast.GetRoot()->As<AST::LiteralNode>();
    SdfVariableExpression expr = literalNode->GetExpressionBuilder();
    TF_AXIOM(
        expr.GetString() == (expectedStr.empty() ? exprStr : expectedStr));
}

static void
_TestLiteral()
{
    _TestLiteralNode("`1`");
    _TestLiteralNode("`True`");
    _TestLiteralNode("`False`");
    _TestLiteralNode("`\"DoubleQuote\"`");
    _TestLiteralNode("`'SingleQuote'`", "`\"SingleQuote\"`");
    _TestLiteralNode("`\"shot_${SHOT_ID}.usda\"`");

    // Literal string nodes may contain variable references with fallback values
    _TestLiteralNode(
        "`\"${FOO:'/shots'}/shot.usda\"`",
        "`'${FOO:\"/shots\"}/shot.usda'`");
}

static void
_TestVariable()
{
    SdfVariableExpressionAST ast("`${FOO}`");
    _TestCasts<AST::VariableNode>(ast.GetRoot());
    _TestCasts<AST::VariableNode>(std::as_const(ast).GetRoot());

    AST::VariableNode* variableNode = ast.GetRoot()->As<AST::VariableNode>();
    SdfVariableExpression expr = variableNode->GetExpressionBuilder();
    TF_AXIOM(expr.GetString() == "`${FOO}`");
    
}

template <class ExpectedFallbackType>
static void
_TestFallbackVariable(
    const std::string& exprStr, const std::string& expectedStr = "")
{
    SdfVariableExpressionAST ast(exprStr);
    _TestCasts<AST::VariableNode>(ast.GetRoot());
    _TestCasts<AST::VariableNode>(std::as_const(ast).GetRoot());
    _TestCasts<ExpectedFallbackType>(
        ast.GetRoot()->As<AST::VariableNode>()->GetFallbackValue());

    AST::VariableNode* variableNode = ast.GetRoot()->As<AST::VariableNode>();
    SdfVariableExpression expr = variableNode->GetExpressionBuilder();
    TF_AXIOM(
        expr.GetString() == (expectedStr.empty() ? exprStr : expectedStr));
}

static void
_TestVariablesWithFallbackValues()
{
    _TestFallbackVariable<AST::LiteralNode>("`${FALLBACK:False}`");
    _TestFallbackVariable<AST::LiteralNode>("`${FALLBACK:True}`");
    _TestFallbackVariable<AST::LiteralNode>("`${FALLBACK:42}`");
    _TestFallbackVariable<AST::LiteralNode>("`${FALLBACK:\"I<3Usd\"}`");
    // note single quoted strings are round tripped as double quoted.
    _TestFallbackVariable<AST::LiteralNode>(
        "`${FALLBACK:'I<3Usd'}`", "`${FALLBACK:\"I<3Usd\"}`");

    _TestFallbackVariable<AST::ListNode>("`${FALLBACK:[1, 2, 3]}`");
    _TestFallbackVariable<AST::ListNode>("`${FALLBACK:[\"a\", \"b\"]}`");
}

static void
_TestList()
{
    SdfVariableExpressionAST ast("`[1, 2]`");
    _TestCasts<AST::ListNode>(ast.GetRoot());
    _TestCasts<AST::ListNode>(std::as_const(ast).GetRoot());

    AST::ListNode* listNode = ast.GetRoot()->As<AST::ListNode>();
    SdfVariableExpression expr = listNode->GetExpressionBuilder();
    TF_AXIOM(expr.GetString() == "`[1, 2]`");

    // Verify that using the expression builder does _not_ modify the
    // original AST.
    SdfVariableExpression::ListBuilder listBuilder =
        listNode->GetExpressionBuilder();
    listBuilder.AddLiteralValues(std::vector<int64_t>{3});
    const SdfVariableExpression newExpr(listBuilder);
    
    TF_AXIOM(listNode->GetExpression().GetString() == "`[1, 2]`");
    TF_AXIOM(newExpr.GetString() == "`[1, 2, 3]`");
}

static void
_TestFunction()
{
    SdfVariableExpressionAST ast("`func()`");
    _TestCasts<AST::FunctionNode>(ast.GetRoot());
    _TestCasts<AST::FunctionNode>(std::as_const(ast).GetRoot());

    AST::FunctionNode* fnNode = ast.GetRoot()->As<AST::FunctionNode>();
    SdfVariableExpression expr = fnNode->GetExpressionBuilder();
    TF_AXIOM(expr.GetString() == "`func()`");

    // Verify that using the expression builder does _not_ modify the
    // original AST.
    SdfVariableExpression::FunctionBuilder fnBuilder =
        fnNode->GetExpressionBuilder();
    fnBuilder.AddArgument(SdfVariableExpression::MakeLiteral(int64_t(1)));
    const SdfVariableExpression newExpr(fnBuilder);

    TF_AXIOM(fnNode->GetExpression().GetString() == "`func()`");
    TF_AXIOM(newExpr.GetString() == "`func(1)`");
}

int main()
{
    _TestLiteral();
    _TestVariable();
    _TestVariablesWithFallbackValues();
    _TestList();
    _TestFunction();

    printf("PASSED!\n");
    return 0;
}
