#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

from pxr import Sdf, Vt
import unittest

class TestSdfVariableExpression(unittest.TestCase):
    def assertEvaluates(self, e, exprVars, expected,
                          expectedUsedVars=None):
        def _GetFailedAssertMessage():
            return "Evaluating {} with vars {}".format(e, exprVars)

        expr = Sdf.VariableExpression(e)
        self.assertTrue(expr, _GetFailedAssertMessage())

        result = expr.Evaluate(exprVars)
        self.assertFalse(result.errors, _GetFailedAssertMessage())

        self.assertEqual(
            result.value, expected, _GetFailedAssertMessage())
 
        if expectedUsedVars:
            self.assertEqual(
                set(result.usedVariables), set(expectedUsedVars), 
                _GetFailedAssertMessage())

    def assertEvaluationErrors(self, e, exprVars, expectedErrors):
        def _GetFailedAssertMessage():
            return "Evaluating {} with vars {}".format(e, exprVars)

        expr = Sdf.VariableExpression(e)
        self.assertTrue(expr, _GetFailedAssertMessage())

        result = expr.Evaluate(exprVars)
        self.assertIsNone(result.value, _GetFailedAssertMessage())
        self.assertTrue(result.errors, _GetFailedAssertMessage())
        self.assertEqual(
            set(result.errors), set(expectedErrors),
            _GetFailedAssertMessage())

    def assertValid(self, e):
        expr = Sdf.VariableExpression(e)
        self.assertTrue(expr)
        self.assertFalse(expr.GetErrors())

    def assertInvalid(self, e):
        expr = Sdf.VariableExpression(e)
        self.assertFalse(expr)
        self.assertTrue(expr.GetErrors())

    def test_AuthoringExpressionVariables(self):
        layer = Sdf.Layer.CreateAnonymous()
        self.assertFalse(layer.HasExpressionVariables())
        self.assertEqual(layer.expressionVariables, {})

        expressionVars = { "str" :"Y", "num": 0, "b" : False }
        layer.expressionVariables = expressionVars
        self.assertTrue(layer.HasExpressionVariables())
        self.assertEqual(layer.expressionVariables, expressionVars)

        layer.ClearExpressionVariables()
        self.assertFalse(layer.HasExpressionVariables())
        self.assertEqual(layer.expressionVariables, {})

    def test_Default(self):
        e = Sdf.VariableExpression()
        self.assertFalse(e)
        self.assertEqual(str(e), "")
        self.assertEqual(e.GetErrors(), ["No expression specified"])

    def test_IsExpression(self):
        """Test Sdf.VariableExpression.IsExpression"""
        self.assertTrue(Sdf.VariableExpression.IsExpression("`foo`"))
        self.assertFalse(Sdf.VariableExpression.IsExpression("foo"))

    def test_IsValidVariableType(self):
        """Test Sdf.VariableExpression.IsValidVariableType"""
        IsValidVariableType = Sdf.VariableExpression.IsValidVariableType

        self.assertTrue(IsValidVariableType("string"))
        self.assertTrue(IsValidVariableType(True))
        self.assertTrue(IsValidVariableType(None))

        # Explicitly verify that both int and int64 are considered valid
        # variable types, even though the result of evaluation will always
        # be an int64.
        self.assertTrue(IsValidVariableType(Vt.Int(123)))
        self.assertTrue(IsValidVariableType(Vt.Int64(2**63-1)))

        self.assertTrue(IsValidVariableType(Vt.StringArray(['a', 'b'])))
        self.assertTrue(IsValidVariableType(Vt.BoolArray([True, False])))
        self.assertTrue(IsValidVariableType(Vt.IntArray([123, 456])))
        self.assertTrue(IsValidVariableType(Vt.Int64Array([123, 456])))

        self.assertFalse(IsValidVariableType(1.23))
        self.assertFalse(IsValidVariableType(Vt.DoubleArray([1.23, 4.56])))

    def test_VarExpressions(self):
        """Test variable expressions consisting of just a top-level
        substitution."""

        # Evaluating a top-level variable substitution should
        # yield the exact value given in the variables dictionary.
        self.assertEvaluates(
            "`${FOO}`", {"FOO" : "string"}, 
            expected="string",
            expectedUsedVars=["FOO"])

        self.assertEvaluates(
            "`${FOO}`", {"FOO" : 42},
            expected=42,
            expectedUsedVars=["FOO"])

        self.assertEvaluates(
            "`${FOO}`", {"FOO" : True},
            expected=True,
            expectedUsedVars=["FOO"])

        self.assertEvaluates(
            "`${FOO}`", {"FOO" : None},
            expected=None,
            expectedUsedVars=["FOO"])

        self.assertEvaluates(
            "`${FOO}`", {"FOO" : Vt.StringArray(['foo', 'bar'])},
            expected=['foo', 'bar'],
            expectedUsedVars=["FOO"])

        self.assertEvaluates(
            "`${FOO}`", {"FOO" : Vt.IntArray([1, 2, 3])},
            expected=[1, 2, 3],
            expectedUsedVars=["FOO"])

        self.assertEvaluates(
            "`${FOO}`", {"FOO" : Vt.Int64Array([1, 2, 3])},
            expected=[1, 2, 3],
            expectedUsedVars=["FOO"])

        self.assertEvaluates(
            "`${FOO}`", {"FOO" : Vt.BoolArray([True, False])},
            expected=[True, False],
            expectedUsedVars=["FOO"])

        # If no value is found for the specified var, we should
        # get an error.
        self.assertEvaluationErrors(
            "`${FOO}`", { },
            ["No value for variable 'FOO'"])

        # Test invalid expressions
        self.assertInvalid("`${FO-O}`")
        self.assertInvalid("`${FOO`")
        
    def test_StringExpressions(self):
        """Test string expressions."""

        # Test both single- and double-quoted strings.
        self.assertEvaluates(
            '''`''`''', {},
            expected='',
            expectedUsedVars=[])

        self.assertEvaluates(
            '''`""`''', {},
            expected='',
            expectedUsedVars=[])

        self.assertEvaluates(
            '''`"basic_string"`''', {}, 
            expected="basic_string",
            expectedUsedVars=[])

        self.assertEvaluates(
            '''`'basic_string'`''', {}, 
            expected="basic_string",
            expectedUsedVars=[])

        self.assertEvaluates(
            r'''`"quoted_\"double\"_'single'_test"`''', {},
            expected='''quoted_"double"_'single'_test''',
            expectedUsedVars=[])

        self.assertEvaluates(
            r'''`'quoted_"double"_\'single\'_test'`''', {},
            expected='''quoted_"double"_'single'_test''',
            expectedUsedVars=[])

        self.assertEvaluates(
            '''`"string_${A}_${B}"`''',
            {"A" : "substitution", "B" : "works"},
            expected="string_substitution_works",
            expectedUsedVars=["A", "B"])

        self.assertEvaluates(
            '''`'string_${A}_${B}'`''',
            {"A" : "substitution", "B" : "works"},
            expected="string_substitution_works",
            expectedUsedVars=["A", "B"])

        # 'None' is considered an empty string for substitution purposes.
        self.assertEvaluates(
            '''`'none_sub_${A}'`''',
            {"A" : None},
            expected="none_sub_",
            expectedUsedVars=["A"])

        # Substitutions using other non-string types are disallowed.
        self.assertEvaluationErrors(
            '''`'bad_sub_${A}'`''', {"A" : 0},
            ["String value required for substituting variable 'A', got int."])

        self.assertEvaluationErrors(
            '''`'bad_sub_${A}'`''', {"A" : True},
            ["String value required for substituting variable 'A', got bool."])

        # No substitutions occur here since the '$' is escaped,
        # so \${A} and \${B} aren't recognized as subsitutions.
        self.assertEvaluates(
            r'''`"nosubs_\${A}_\${B}"`''',
            {"A" : "substitution", "B" : "works"},
            expected="nosubs_${A}_${B}",
            expectedUsedVars=[])

        self.assertEvaluates(
            r'''`'nosubs_\${A}_\${B}'`''',
            {"A" : "substitution", "B" : "works"},
            expected="nosubs_${A}_${B}",
            expectedUsedVars=[])

        # Test invalid expressions
        self.assertInvalid('''`"unescaped_"quotes"_are_bad"`''')
        self.assertInvalid('''`'unescaped_'quotes'_are_bad'`''')

        self.assertInvalid('`"bad_var_${FOO"`')
        self.assertInvalid('`"bad_var_${FO-O}"`')

        self.assertInvalid("`'`")
        self.assertInvalid('`"`')

    def test_IntegerExpressions(self):
        """Test integer expressions."""
        self.assertEvaluates("`0`", {}, 0)
        self.assertEvaluates("`-1`", {}, -1)
        self.assertEvaluates("`1`", {}, 1)

        # Test that integer values within an int64's range are allowed
        # and are invalid if they're outside that range.
        self.assertEvaluates("`{}`".format(2**63-1), {}, 2**63-1)
        self.assertInvalid("`{}`".format(2**63))

        self.assertEvaluates("`{}`".format(-2**63), {}, -2**63)
        self.assertInvalid("`{}`".format(-2**63-1))

        self.assertInvalid("`42abc`")

        # Explicitly test that both int and int64 values in the expression
        # variables dictionary are accepted during evaluation, even though
        # the final result will always be an int64.
        self.assertEvaluates("`${FOO}`", {"FOO" : Vt.Int(0)}, 0)
        self.assertEvaluates("`${FOO}`", {"FOO" : Vt.Int64(2**63-1)}, 2**63-1)

    def test_BooleanExpressions(self):
        """Test boolean expressions."""
        self.assertEvaluates("`True`", {}, True)
        self.assertEvaluates("`true`", {}, True)
        self.assertEvaluates("`False`", {}, False)
        self.assertEvaluates("`false`", {}, False)

        self.assertInvalid("`Truee`")
        self.assertInvalid("`truee`")
        self.assertInvalid("`TRUE`")

        self.assertInvalid("`Falsee`")
        self.assertInvalid("`falsee`")
        self.assertInvalid("`FALSE`")

    def test_NoneExpressions(self):
        """Test None expressions."""
        self.assertEvaluates("`None`", {}, None)
        self.assertEvaluates("`none`", {}, None)

        self.assertInvalid("`Nonee`")
        self.assertInvalid("`nonee`")

    def test_Lists(self):
        """Test list expressions."""
        self.assertEvaluates("`[]`", {}, [])

        # Test list of integers.
        self.assertEvaluates(
            "`[1]`", {}, Vt.Int64Array([1]))
        self.assertEvaluates(
            "`[1, 2]`", {}, Vt.Int64Array([1, 2]))
        self.assertEvaluates(
            "`[1, 2, 3]`", {}, Vt.Int64Array([1, 2, 3]))
        self.assertEvaluates(
            "`[${FOO}, 2, 3]`", {'FOO' : 1}, Vt.Int64Array([1, 2, 3]))
        self.assertEvaluates(
            "`[1, ${FOO}, 3]`", {'FOO' : 2}, Vt.Int64Array([1, 2, 3]))
        self.assertEvaluates(
            "`[1, 2, ${FOO}]`", {'FOO' : 3}, Vt.Int64Array([1, 2, 3]))

        # Test list of strings.
        self.assertEvaluates(
            "`['a']`", {}, Vt.StringArray(['a']))
        self.assertEvaluates(
            "`['a', 'b']`", {}, Vt.StringArray(['a', 'b']))
        self.assertEvaluates(
            "`['a', 'b', 'c']`", {}, Vt.StringArray(['a', 'b', 'c']))
        self.assertEvaluates(
            "`['${FOO}a', 'b', 'c']`", {'FOO' :'a'}, 
            Vt.StringArray(['aa', 'b', 'c']))
        self.assertEvaluates(
            "`['a', '${FOO}b', 'c']`", {'FOO' :'b'}, 
            Vt.StringArray(['a', 'bb', 'c']))
        self.assertEvaluates(
            "`['a', 'b', '${FOO}c']`", {'FOO' :'c'}, 
            Vt.StringArray(['a', 'b', 'cc']))

        # Test list of variable substitutions.
        self.assertEvaluates(
            "`[${FOO}]`", {'FOO':'a'}, Vt.StringArray(['a']))
        self.assertEvaluates(
            "`[${FOO}, ${BAR}]`", {'FOO':'a', 'BAR':'b'}, 
            Vt.StringArray(['a', 'b']))
        self.assertEvaluates(
            "`[${FOO}, ${BAR}, ${BAZ}]`", 
            {'FOO':'a', 'BAR':'b', 'BAZ':'c'}, 
            Vt.StringArray(['a', 'b', 'c']))

        # Lists cannot contain None values.
        self.assertEvaluationErrors(
            "`[None]`", {}, 
            ["Unexpected value of type None in list at element 0"])
        self.assertEvaluationErrors(
            "`[None, 2, 3]`", {},
            ["Unexpected value of type None in list at element 0"])
        self.assertEvaluationErrors(
            "`[1, ${FOO}, 3]`", {'FOO' : None},
            ["Unexpected value of type None in list at element 1"])

        # Lists cannot contain other lists.
        self.assertInvalid("`[[1, 2]]`")
        self.assertEvaluationErrors(
            "`[${L}]`", {'L': "`[]`"},
            ["Unexpected value of type list in list at element 0"])
        self.assertEvaluationErrors(
            "`[${L}]`", {'L': Vt.IntArray([1,2]) },
            ["Unexpected value of type list in list at element 0"])

        # Lists must contain elements of the same type.
        self.assertEvaluationErrors(
            "`[1, 'foo', False, ${L}]`", {'L': "`[]`"},
            ['Unexpected value of type string in list at element 1',
             'Unexpected value of type bool in list at element 2',
             'Unexpected value of type list in list at element 3'])

        self.assertInvalid("`[`")
        self.assertInvalid("`[foo]`")

    def test_If(self):
        """Test if function."""
        self.assertInvalid("`if()`")
        self.assertInvalid("`if(True)`")

        self.assertEvaluates("`if(True, 'true', 'false')`", {}, 'true')
        self.assertEvaluates("`if(False, 'true', 'false')`", {}, 'false')

        self.assertEvaluates("`if(True, 'true')`", {}, 'true')
        self.assertEvaluates("`if(False, 'true')`", {}, None)

        self.assertEvaluates("`if(${B}, 1, 0)`", {'B' : True}, 1)
        self.assertEvaluates("`if(${B}, 1, 0)`", {'B' : False}, 0)

        self.assertEvaluates(
            "`if(${B}, ${X}, ${Y})`", {'B' : True, 'X': 1, 'Y': 0}, 1)
        self.assertEvaluates(
            "`if(${B}, ${X}, ${Y})`", {'B' : False, 'X': 1, 'Y': 0}, 0)

        self.assertEvaluates(
            "`if(${B}, if(${X}, 1, 2), if(${Y}, 3, 4))`", 
            {'B' : True, 'X': True, 'Y': True}, 1)
        self.assertEvaluates(
            "`if(${B}, if(${X}, 1, 2), if(${Y}, 3, 4))`", 
            {'B' : True, 'X': False, 'Y': True}, 2)
        self.assertEvaluates(
            "`if(${B}, if(${X}, 1, 2), if(${Y}, 3, 4))`", 
            {'B' : False, 'X': True, 'Y': True}, 3)
        self.assertEvaluates(
            "`if(${B}, if(${X}, 1, 2), if(${Y}, 3, 4))`", 
            {'B' : False, 'X': True, 'Y': False}, 4)

        self.assertEvaluationErrors(
            "`if('non_bool', 1, 0)`", {},
            ['if: Condition must be a boolean value'])
        self.assertEvaluationErrors(
            "`if(${B}, 1, 0)`", {'B' : 'non_bool'},
            ['if: Condition must be a boolean value'])

        # if and else values must be of the same type, or None.
        self.assertEvaluates("`if(False, 1, None)`", {}, None)
        self.assertEvaluates("`if(False, None, 1)`", {}, 1)
        self.assertEvaluationErrors(
            "`if(False, 1, 'foo')`", {},
            ['if: if-value and else-value must evaluate to the same type '
             'or None.'])
        self.assertEvaluationErrors(
            "`if(False, 'foo', 1)`", {},
            ['if: if-value and else-value must evaluate to the same type '
             'or None.'])

        # Evaluation errors from subexpressions should be reported to clients.
        self.assertEvaluationErrors(
            "`if(eq(1, '1'), 1, 0)`", {},
            ['eq: Cannot compare values of type int and string'])

    def test_Comparisons(self):
        """Test comparison functions: eq, neq, lt, leq, gt, geq"""

        def _Test(fnName, comparator):
            def _MakeExpression(*operands):
                operandStrs = [str(s) for s in operands]
                x = "`{fnName}({operands})`".format(
                    fnName=fnName, operands=",".join(operandStrs))
                return x

            # All comparison functions take 2 arguments.
            self.assertInvalid(_MakeExpression()) # e.g., "`eq()`"
            self.assertInvalid(_MakeExpression(1)) # e.g., "`eq(1)`"
            self.assertInvalid(_MakeExpression(1, 2, 3)) # e.g., "`eq(1,2,3)`"

            # Verify valid comparisons return the expected results.
            # Note that comparing None values is only supported by the
            # "eq" and "neq" functions.
            testCases = [
                (0, 1), (1, 1), (1, 2),
                (r"'a'", r"'a'"), (r"'a'", r"'b'"), (r"'b'", r"'c'"),
                (True, True), (True, False), (False, True)
            ]

            if fnName == "eq" or fnName == "neq":
                testCases.append((None, None))

            for testCase in testCases:
                # e.g., check that "`eq(1, 1)`" evaluates to the same
                # result as 1 == 1.
                self.assertEvaluates(
                    _MakeExpression(*testCase), {}, comparator(*testCase))
                
            # Verify that invalid comparisons return the expected errors.
            testCases = [
                (0, r"'a'",
                 "{}: Cannot compare values of type int and string"
                 .format(fnName)),
                (0, False,
                 "{}: Cannot compare values of type int and bool"
                 .format(fnName)),
                (0, "None",
                 "{}: Cannot compare values of type int and None"
                 .format(fnName)),
                (r"'a'", False,
                 "{}: Cannot compare values of type string and bool"
                 .format(fnName)),
                (r"'a'", "None", 
                 "{}: Cannot compare values of type string and None"
                 .format(fnName)),
                (False, "None",
                 "{}: Cannot compare values of type bool and None"
                 .format(fnName)),
            ]

            if fnName != "eq" and fnName != "neq":
                testCases.append(
                    (None, None, 
                     "{}: Comparison operation not supported for None"
                     .format(fnName)))

            for testCase in testCases:
                self.assertEvaluationErrors(
                    "`{fn}({0},{1})`".format(fn=fnName, *testCase), {}, 
                    [testCase[2]])

        _Test("eq", lambda x,y: x == y)
        _Test("neq", lambda x,y: x != y)
        _Test("lt", lambda x,y: x < y)
        _Test("leq", lambda x,y: x <= y)
        _Test("gt", lambda x,y: x > y)
        _Test("geq", lambda x,y: x >= y)

    def test_AndOr(self):
        """Test logical and/or functions"""
        
        def _Test(fnName, comparator):
            def _MakeExpression(*operands):
                operandStrs = [str(s) for s in operands]
                x = "`{fnName}({operands})`".format(
                    fnName=fnName, operands=",".join(operandStrs))
                return x

            # "and" / "or" both require at least 2 arguments.
            self.assertInvalid(_MakeExpression())
            self.assertInvalid(_MakeExpression(True))

            # Verify valid combinations return the the expected result.
            testCases = [
                (True, True),
                (True, False),
                (False, False),
                (True, True, True),
                (True, True, False)
            ]

            for testCase in testCases:
                self.assertEvaluates(
                    _MakeExpression(*testCase), {}, comparator(testCase))

            # Verify that invalid combinations return the expected errors.
            testCases = [
                (True, 1, 
                 "{}: Invalid type int for argument 1".format(fnName)),
                (True, "'foo'", 
                 "{}: Invalid type string for argument 1".format(fnName)),
                (True, "None",
                 "{}: Invalid type None for argument 1".format(fnName))
            ]

            for testCase in testCases:
                self.assertEvaluationErrors(
                    _MakeExpression(*testCase[0:2]), {}, [testCase[2]])

        _Test("and", lambda l: all(l))
        _Test("or", lambda l: any(l))

    def test_Not(self):
        """Test logical not function."""
        self.assertInvalid("`not()`")
        self.assertInvalid("`not(True, False)`")

        self.assertEvaluates("`not(True)`", {}, False)
        self.assertEvaluates("`not(False)`", {}, True)

        self.assertEvaluationErrors(
            "`not(1)`", {}, ["not: Invalid type int for argument"])
        self.assertEvaluationErrors(
            "`not('foo')`", {}, ["not: Invalid type string for argument"])
        self.assertEvaluationErrors(
            "`not(None)`", {}, ["not: Invalid type None for argument"])

    def test_Contains(self):
        """Test contains function."""
        # Test searching in lists.
        self.assertEvaluates(
            "`contains([], 1)`", {}, False)
        self.assertEvaluates(
            "`contains(${L}, 1)`", {'L' : Vt.IntArray([])}, False)
        self.assertEvaluates(
            "`contains(${L}, 1)`", {'L' : "`[]`"}, False)
        
        self.assertEvaluates("`contains([1, 2, 3], 1)`", {}, True)
        self.assertEvaluates("`contains([1, 2, 3], 0)`", {}, False)

        self.assertEvaluates(
            "`contains(${L}, 1)`", {'L' : Vt.IntArray([1, 2, 3])}, True)
        self.assertEvaluates(
            "`contains(${L}, 0)`", {'L' : Vt.IntArray([1, 2, 3])}, False)

        self.assertEvaluates("`contains([1, 2, 3], ${A})`", {'A' : 2}, True)
        self.assertEvaluates("`contains([1, 2, 3], ${A})`", {'A' : 4}, False)

        self.assertEvaluates(
            "`contains([${A}, ${B}], 'a')`", {'A':'a', 'B':'b'}, True)
        self.assertEvaluates(
            "`contains([${A}, ${B}], 'c')`", {'A':'a', 'B':'b'}, False)

        self.assertEvaluationErrors(
            "`contains([1, 2, 3], 'a')`", {},
            ["contains: Invalid search value"])
        self.assertEvaluationErrors(
            "`contains([1, 2, 3], None)`", {},
            ["contains: Invalid search value"])

        # Test searching in strings.
        self.assertEvaluates("`contains('', 'a')`", {}, False)

        self.assertEvaluates("`contains('abc', 'a')`", {}, True)
        self.assertEvaluates("`contains('abc', 'z')`", {}, False)

        self.assertEvaluates("`contains('abc', ${A})`", {'A':'a'}, True)
        self.assertEvaluates("`contains('abc', ${Z})`", {'Z':'z'}, False)

        self.assertEvaluates(
            "`contains('${A}${B}', 'a')`", {'A':'a', 'B':'b'}, True)
        self.assertEvaluates(
            "`contains('${A}${B}', 'z')`", {'A':'a', 'B':'b'}, False)

        self.assertEvaluationErrors(
            "`contains('abcd', 1)`", {},
            ["contains: Invalid search value"])
        self.assertEvaluationErrors(
            "`contains('abcd', None)`", {},
            ["contains: Invalid search value"])

        # Other errors
        self.assertInvalid("`contains('abcd')`")
        self.assertEvaluationErrors(
            "`contains(1, 1)`", {},
            ["contains: Value to search must be a list or string"])
        self.assertEvaluationErrors(
            "`contains(None, 1)`", {},
            ["contains: Value to search must be a list or string"])

    def test_At(self):
        """Test at function."""
        # Test getting value from list
        self.assertEvaluates("`at([1, 2, 3], 0)`", {}, 1)
        self.assertEvaluates("`at([1, 2, 3], 1)`", {}, 2)
        self.assertEvaluates("`at([1, 2, 3], 2)`", {}, 3)
        self.assertEvaluates("`at([1, 2, 3], -1)`", {}, 3)
        self.assertEvaluates("`at([1, 2, 3], -2)`", {}, 2)
        self.assertEvaluates("`at([1, 2, 3], -3)`", {}, 1)

        self.assertEvaluationErrors(
            "`at([1, 2, 3], 3)`", {}, ['at: Index out of range'])
        self.assertEvaluationErrors(
            "`at([1, 2, 3], -4)`", {}, ['at: Index out of range'])

        self.assertEvaluationErrors(
            "`at([], 0)`", {}, ['at: Index out of range'])

        self.assertEvaluationErrors(
            "`at([1, 2, 3], 'foo')`", {}, ['at: Index must be an integer'])
        self.assertEvaluationErrors(
            "`at([1, 2, 3], True)`", {}, ['at: Index must be an integer'])
        self.assertEvaluationErrors(
            "`at([1, 2, 3], None)`", {}, ['at: Index must be an integer'])

        # Tests getting value from string
        self.assertEvaluates("`at('abc', 0)`", {}, 'a')
        self.assertEvaluates("`at('abc', 1)`", {}, 'b')
        self.assertEvaluates("`at('abc', 2)`", {}, 'c')
        self.assertEvaluates("`at('abc', -1)`", {}, 'c')
        self.assertEvaluates("`at('abc', -2)`", {}, 'b')
        self.assertEvaluates("`at('abc', -3)`", {}, 'a')

        self.assertEvaluationErrors(
            "`at('abc', 3)`", {}, ['at: Index out of range'])
        self.assertEvaluationErrors(
            "`at('abc', -4)`", {}, ['at: Index out of range'])

        self.assertEvaluationErrors(
            "`at('', 0)`", {}, ['at: Index out of range'])

        self.assertEvaluationErrors(
            "`at('abc', 'foo')`", {}, ['at: Index must be an integer'])
        self.assertEvaluationErrors(
            "`at('abc', True)`", {}, ['at: Index must be an integer'])
        self.assertEvaluationErrors(
            "`at('abc', None)`", {}, ['at: Index must be an integer'])

    def test_Len(self):
        """Tests len function."""
        self.assertInvalid("`len()`")
    
        self.assertEvaluates("`len([])`", {}, 0)
        self.assertEvaluates("`len([1, 2, 3])`", {}, 3)

        self.assertEvaluates("`len('')`", {}, 0)
        self.assertEvaluates("`len('abc')`", {}, 3)

        self.assertEvaluationErrors(
            "`len(1)`", {}, ['len: Unsupported type'])
        self.assertEvaluationErrors(
            "`len(False)`", {}, ['len: Unsupported type'])
        self.assertEvaluationErrors(
            "`len(None)`", {}, ['len: Unsupported type'])

    def test_Defined(self):
        """Tests defined function."""
        self.assertInvalid("`defined()`")

        self.assertEvaluates("`defined('X')`", {}, False)
        self.assertEvaluates("`defined('X')`", {'X':0}, True)
        self.assertEvaluates("`defined('X','Y')`", {'X':0}, False)
        self.assertEvaluates("`defined('X','Y')`", {'X':0, 'Y':1}, True)

        self.assertEvaluationErrors(
            "`defined(1)`", {},
            ["defined: Invalid type int for argument 0"])
        self.assertEvaluationErrors(
            "`defined(None)`", {},
            ["defined: Invalid type None for argument 0"])

    def test_NestedExpressions(self):
        """Test evaluating expressions with variable substitutions
        when the variables are expressions themselves."""
        self.assertEvaluates(
            "`${FOO}`",
            {"FOO" : "`${BAR}`", "BAR" : "ok"}, 
            "ok")

        self.assertEvaluates(
            "`${FOO}`",
            {"FOO" : "`'subexpression_${BAR}'`", 
             "BAR" : "`'${BAZ}'`",
             "BAZ" : "`'works_ok'`"}, 
            "subexpression_works_ok")

        self.assertEvaluates(
            "`'${A}_${B}'`",
            {"A" : "`'subexpression_${FOO}'`", 
             "FOO" : "`'${BAR}'`",
             "BAR" : "`'works_ok'`",
             "B" : "`${A}`"}, 
            "subexpression_works_ok_subexpression_works_ok")

    def test_CircularSubstitutions(self):
        """Test that circular variable substitutions result in an
        error and not an infinite loop."""
        self.assertEvaluationErrors(
            "`${FOO}`",
            {"FOO" : "`${BAR}`",
             "BAR" : "`${BAZ}`",
             "BAZ" : "`${FOO}`"},
            ["Encountered circular variable substitutions: "
             "['FOO', 'BAR', 'BAZ', 'FOO']"])
    
    def test_ErrorInNestedExpression(self):
        """Test that errors in subexpressions surface in final result."""
        self.assertEvaluationErrors(
            "`${FOO}`",
            {"FOO" : "`'${BAR}'`",
             "BAR" : "`${BAZ`"},
            ["Missing ending '}' at character 6 (in variable 'BAR')"])

    def test_UnsupportedVariableType(self):
        """Test that references to variables whose values are an
        unsupported type result in an error."""
        self.assertEvaluationErrors(
            "`${FOO}`",
            {"FOO" : 1.234},
            ["Variable 'FOO' has unsupported type double"])

        self.assertEvaluationErrors(
            "`'test_${FOO}'`",
            {"FOO" : 1.234},
            ["Variable 'FOO' has unsupported type double"])

if __name__ == "__main__":
    unittest.main()
