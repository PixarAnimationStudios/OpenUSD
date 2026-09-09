#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Tf
import unittest

ASTNodes = Sdf.VariableExpressionASTNodes

class TestSdfVariableExpressionAST(unittest.TestCase):
    def assertEqualExpression(self, expr1, expr2):
        self.assertEqual(str(expr1), str(expr2))

    def assertValidAST(self, ast):
        self.assertTrue(ast)
        self.assertEqual(len(ast.GetErrors()), 0)
        self.assertIsNotNone(ast.GetRoot())
        self.assertEqualExpression(
            ast.GetRoot().GetExpression(), ast.GetExpression())

    def assertInvalidAST(self, ast, expectedErrors):
        self.assertFalse(ast)
        self.assertEqual(ast.GetErrors(), expectedErrors)
        self.assertIsNone(ast.GetRoot())
        self.assertEqualExpression(
            ast.GetExpression(), Sdf.VariableExpression())

    def assertExpectedNodeList(self, nodeList, expectedExprs):
        listElementNodes = nodeList.GetNodes()
        for (listElementNode, expectedExpr) in \
            zip(nodeList.GetNodes(), expectedExprs):
            self.assertEqualExpression(
                listElementNode.GetExpression(), expectedExpr)

    def test_Basic(self):
        self.assertInvalidAST(
            Sdf.VariableExpressionAST(),
            expectedErrors=["No expression specified"])

        self.assertInvalidAST(
            Sdf.VariableExpressionAST('bogus_expr'),
            expectedErrors=["Expressions must begin with '`' at character 0"])

        # Verify that it's safe to access an AST node after the AST itself
        # is no longer referenced.
        ast = Sdf.VariableExpressionAST(
            Sdf.VariableExpression.MakeLiteral(1))

        node = ast.GetRoot()
        self.assertIsNotNone(node)
        self.assertEqual(node.GetValue(), 1)

        del ast
        self.assertIsNotNone(node)
        self.assertEqual(node.GetValue(), 1)
        node.SetValue(2)
        self.assertEqual(node.GetValue(), 2)

    def test_Literal(self):
        def _makeExpr(value):
            if value is None:
                return Sdf.VariableExpression.MakeNone()
            else:
                return Sdf.VariableExpression.MakeLiteral(value)

        def _testLiteralAST(ast, expectedValue):
            self.assertValidAST(ast)

            literalNode = ast.GetRoot()
            self.assertIsInstance(literalNode, ASTNodes.LiteralNode)
            self.assertEqual(literalNode.GetValue(), expectedValue)
            self.assertEqualExpression(
                ast.GetExpression(), _makeExpr(expectedValue))

        testValues = [
            0, 1, 2**31-1, 2**31, "foo", "foo_${BAR}", True, False, None]
        for value in testValues:
            expr = _makeExpr(value)
            print()
            print(f"- Testing expression {expr}")
            ast = Sdf.VariableExpressionAST(expr)
            _testLiteralAST(ast, value)

            for newValue in testValues:
                print(f"  - Setting value to {repr(newValue)}")
                ast.GetRoot().SetValue(newValue)
                _testLiteralAST(ast, newValue)

    def test_Variable(self):
        def _testVariableAST(ast, expectedName):
            self.assertValidAST(ast)
            
            varNode = ast.GetRoot()
            self.assertIsInstance(varNode, ASTNodes.VariableNode)
            self.assertEqual(varNode.GetName(), expectedName)
            self.assertEqualExpression(
                ast.GetExpression(), 
                Sdf.VariableExpression.MakeVariable(expectedName))

        expr = Sdf.VariableExpression.MakeVariable("FOO")
        ast = Sdf.VariableExpressionAST(expr)
        _testVariableAST(ast, "FOO")

        ast.GetRoot().SetName("BAR")
        _testVariableAST(ast, "BAR")

    def testVariableFallback(self):
        def _testVariableFallbackAST(ast, expectedName, expectedFallback):
            self.assertValidAST(ast)
            
            varNode = ast.GetRoot()
            self.assertIsInstance(varNode, ASTNodes.VariableNode)
            self.assertEqual(varNode.GetName(), expectedName)
            self.assertEqualExpression(
                ast.GetExpression(), 
                Sdf.VariableExpression.MakeVariable(expectedName, expectedFallback))

        expr = Sdf.VariableExpression.MakeVariable("FOO", "BAR")
        ast = Sdf.VariableExpressionAST(expr)
        _testVariableFallbackAST(ast, "FOO", "BAR")

        expr = Sdf.VariableExpression.MakeVariable("FOO", True)
        ast = Sdf.VariableExpressionAST(expr)
        _testVariableFallbackAST(ast, "FOO", True)

        expr = Sdf.VariableExpression.MakeVariable("FOO", 42)
        ast = Sdf.VariableExpressionAST(expr)
        _testVariableFallbackAST(ast, "FOO", 42)

        expr = Sdf.VariableExpression.MakeVariable("FOO", [1, 2, 3])
        ast = Sdf.VariableExpressionAST(expr)
        _testVariableFallbackAST(ast, "FOO", [1, 2, 3])

        with self.assertRaises(TypeError):
            Sdf.VariableExpression.MakeVariable("FOO", 3.14)

        # Functions are not supported as fallback values.
        funcFallback = Sdf.VariableExpression.MakeFunction(
            "if", Sdf.VariableExpression.MakeLiteral(True),
            Sdf.VariableExpression.MakeLiteral(10),
            Sdf.VariableExpression.MakeLiteral(20))
        self.assertFalse(
            Sdf.VariableExpression.MakeVariable("FOO", funcFallback))

        # Inline variable references are not allowed within a fallback string,
        self.assertInvalidAST(
            Sdf.VariableExpressionAST("`${FOO:'${BAR}'}`"),
            expectedErrors=[
                "Variable references are not allowed in fallback string "
                "values at character 8"])

    def testVariableFallbackAccessors(self):
        ast = Sdf.VariableExpressionAST(
            Sdf.VariableExpression.MakeVariable("FOO"))
        varNode = ast.GetRoot()
        self.assertIsNone(varNode.GetFallbackValue())

        ast = Sdf.VariableExpressionAST(
            Sdf.VariableExpression.MakeVariable("FOO", 42))
        varNode = ast.GetRoot()
        fallback = varNode.GetFallbackValue()
        self.assertIsInstance(fallback, ASTNodes.LiteralNode)
        self.assertEqual(fallback.GetValue(), 42)

        varNode.SetFallbackValue(Sdf.VariableExpressionAST(
            Sdf.VariableExpression.MakeLiteral("usd")).GetRoot())
        self.assertIsInstance(
            varNode.GetFallbackValue(), ASTNodes.LiteralNode)
        self.assertEqualExpression(
            ast.GetExpression(),
            Sdf.VariableExpression.MakeVariable("FOO", "usd"))

        varNode.SetFallbackValue(Sdf.VariableExpressionAST(
            Sdf.VariableExpression.MakeList(
                Sdf.VariableExpression.MakeLiteral(1),
                Sdf.VariableExpression.MakeLiteral(2))).GetRoot())
        self.assertIsInstance(varNode.GetFallbackValue(), ASTNodes.ListNode)

        varNode.ClearFallbackValue()
        self.assertIsNone(varNode.GetFallbackValue())
        self.assertEqualExpression(
            ast.GetExpression(),
            Sdf.VariableExpression.MakeVariable("FOO"))

        with self.assertRaises(Tf.ErrorException):
            varNode.SetFallbackValue(Sdf.VariableExpressionAST(
                Sdf.VariableExpression.MakeVariable("BAR")).GetRoot())
        with self.assertRaises(Tf.ErrorException):
            varNode.SetFallbackValue(Sdf.VariableExpressionAST(
                Sdf.VariableExpression.MakeNone()).GetRoot())
        # Functions are not supported as fallback values.
        with self.assertRaises(Tf.ErrorException):
            varNode.SetFallbackValue(Sdf.VariableExpressionAST(
                Sdf.VariableExpression.MakeFunction(
                    "if", Sdf.VariableExpression.MakeLiteral(True),
                    Sdf.VariableExpression.MakeLiteral(10),
                    Sdf.VariableExpression.MakeLiteral(20))).GetRoot())

    def checkNodeList(self, getNodeListFromAST, makeExprForAST):
        def _testNodeList(ast, expectedExprs):
            self.assertExpectedNodeList(
                getNodeListFromAST(ast), expectedExprs)
            self.assertEqualExpression(
                ast.GetExpression(), makeExprForAST(expectedExprs))

        # Create an AST for an expression containing a node list with
        # a few initial elements.
        expectedExprs = [
            Sdf.VariableExpression.MakeLiteral("a"),
            Sdf.VariableExpression.MakeLiteral("b")
        ]

        expr = makeExprForAST(expectedExprs)
        ast = Sdf.VariableExpressionAST(expr)
        self.assertExpectedNodeList(getNodeListFromAST(ast), expectedExprs)

        # Test appending an element to the AST's node list
        newExpr = Sdf.VariableExpression.MakeLiteral(False)
        newExprAST = Sdf.VariableExpressionAST(newExpr)

        astCopy = Sdf.VariableExpressionAST(ast)
        getNodeListFromAST(astCopy).Append(newExprAST.GetRoot())
        _testNodeList(astCopy, expectedExprs + [newExpr])

        # Test setting elements in the AST's node list
        for i in range(len(expectedExprs)):
            astCopy = Sdf.VariableExpressionAST(ast)
            getNodeListFromAST(astCopy).Set(i, newExprAST.GetRoot())
            _testNodeList(
                astCopy, expectedExprs[0:i] + [newExpr] + expectedExprs[i+1:])
        
        # Test setting element out of bounds
        astCopy = Sdf.VariableExpressionAST(ast)
        with self.assertRaises(Tf.ErrorException):
            getNodeListFromAST(astCopy).Set(
                len(expectedExprs), newExprAST.GetRoot())

        # Test inserting elements in the AST's node list
        for i in range(len(expectedExprs) + 1):
            astCopy = Sdf.VariableExpressionAST(ast)
            getNodeListFromAST(astCopy).Insert(i, newExprAST.GetRoot())
            _testNodeList(
                astCopy, expectedExprs[0:i] + [newExpr] + expectedExprs[i:])

        # Test inserting element out of bounds
        astCopy = Sdf.VariableExpressionAST(ast)
        with self.assertRaises(Tf.ErrorException):
            getNodeListFromAST(astCopy).Insert(
                len(expectedExprs) + 1, newExprAST.GetRoot())

        # Test removing individual elements in the AST's node list
        for i in range(len(expectedExprs)):
            astCopy = Sdf.VariableExpressionAST(ast)
            getNodeListFromAST(astCopy).Remove(i)
            _testNodeList(astCopy, expectedExprs[0:i] + expectedExprs[i+1:])
        
        # Test removing element out of bounds
        astCopy = Sdf.VariableExpressionAST(ast)
        with self.assertRaises(Tf.ErrorException):
            getNodeListFromAST(astCopy).Remove(len(expectedExprs))

        # Test clearing the AST's node list entirely
        astCopy = Sdf.VariableExpressionAST(ast)
        getNodeListFromAST(astCopy).Clear()
        _testNodeList(astCopy, [])

    def test_List(self):
        def _testListAST(ast, expectedElementExprs):
            self.assertValidAST(ast)

            listNode = ast.GetRoot()
            self.assertIsInstance(listNode, ASTNodes.ListNode)
            self.assertExpectedNodeList(
                listNode.GetElements(), expectedElementExprs)

        # Test AST for an empty list expression
        expr = Sdf.VariableExpression.MakeList()
        ast = Sdf.VariableExpressionAST(expr)
        _testListAST(ast, [])

        # Test AST for a more complicated list expression containing 
        # a variety of elements.
        elemExprs = [
            Sdf.VariableExpression.MakeLiteral(True),
            Sdf.VariableExpression.MakeVariable("FOO"),
            Sdf.VariableExpression.MakeFunction(
                "foo",
                Sdf.VariableExpression.MakeListOfLiterals(["a", "b"]),
                Sdf.VariableExpression.MakeLiteral("a"))
        ]

        expr = Sdf.VariableExpression.MakeList(*elemExprs)
        ast = Sdf.VariableExpressionAST(expr)
        _testListAST(ast, elemExprs)

        self.checkNodeList(
            lambda ast : ast.GetRoot().GetElements(),
            lambda exprs : Sdf.VariableExpression.MakeList(*exprs))

    def test_Function(self):
        def _testFunctionAST(ast, expectedName, expectedArgExprs):
            self.assertValidAST(ast)

            functionNode = ast.GetRoot()
            self.assertIsInstance(functionNode, ASTNodes.FunctionNode)
            self.assertEqual(functionNode.GetName(), expectedName)
            self.assertExpectedNodeList(
                functionNode.GetArguments(), expectedArgExprs)

        functionName = "func"
        functionArgs = [
            Sdf.VariableExpression.MakeLiteral(True),
            Sdf.VariableExpression.MakeVariable("FOO"),
            Sdf.VariableExpression.MakeListOfLiterals(["a", "b"]),
            Sdf.VariableExpression.MakeFunction(
                "foo",
                Sdf.VariableExpression.MakeListOfLiterals(["a", "b"]),
                Sdf.VariableExpression.MakeLiteral("a"))
        ]

        expr = Sdf.VariableExpression.MakeFunction(functionName, *functionArgs)
        ast = Sdf.VariableExpressionAST(expr)
        _testFunctionAST(ast, functionName, functionArgs)

        self.checkNodeList(
            lambda ast: ast.GetRoot().GetArguments(),
            lambda exprs: Sdf.VariableExpression.MakeFunction("foo", *exprs))

if __name__ == "__main__":
    unittest.main(verbosity=2)
