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

from pxr import Sdf
import unittest

class TestSdfVariableExpression(unittest.TestCase):
    def assertEvaluates(self, e, exprVars, expected,
                          expectedUsedVars=None):
        expr = Sdf.VariableExpression(e)
        self.assertTrue(expr)

        result = expr.Evaluate(exprVars)
        self.assertFalse(result.errors)
        self.assertEqual(result.value, expected)
 
        if expectedUsedVars:
            self.assertEqual(set(result.usedVariables), set(expectedUsedVars))

    def assertEvaluationErrors(self, e, exprVars, expectedErrors):
        expr = Sdf.VariableExpression(e)
        self.assertTrue(expr)

        result = expr.Evaluate(exprVars)
        self.assertIsNone(result.value)
        self.assertTrue(result.errors)
        self.assertEqual(set(result.errors), set(expectedErrors))

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
        self.assertFalse(IsValidVariableType(1.23))

    def test_VarExpressions(self):
        """Test variable expressions consisting of just a top-level
        substitution."""

        # Evaluating a top-level variable substitution should
        # yield the exact value given in the variables dictionary.
        self.assertEvaluates(
            "`${FOO}`", {"FOO" : "string"}, 
            expected="string",
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

        self.assertInvalid('`"bad_stage_var_${FOO"`')
        self.assertInvalid('`"bad_stage_var_${FO-O}"`')

        self.assertInvalid("`'`")
        self.assertInvalid('`"`')

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
