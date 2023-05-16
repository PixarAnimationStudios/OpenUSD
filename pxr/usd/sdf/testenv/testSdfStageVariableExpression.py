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

class TestSdfStageVariableExpression(unittest.TestCase):
    def assertEvaluates(self, e, stageVars, expected,
                          expectedUsedStageVars=None):
        expr = Sdf.StageVariableExpression(e)
        self.assertTrue(expr)

        result = expr.Evaluate(stageVars)
        self.assertFalse(result.errors)
        self.assertEqual(result.value, expected)
 
        if expectedUsedStageVars:
            self.assertEqual(set(result.usedStageVariables),
                             set(expectedUsedStageVars))

    def assertEvaluationErrors(self, e, stageVars, expectedErrors):
        expr = Sdf.StageVariableExpression(e)
        self.assertTrue(expr)

        result = expr.Evaluate(stageVars)
        self.assertIsNone(result.value)
        self.assertTrue(result.errors)
        self.assertEqual(set(result.errors), set(expectedErrors))

    def assertValid(self, e):
        expr = Sdf.StageVariableExpression(e)
        self.assertTrue(expr)
        self.assertFalse(expr.GetErrors())

    def assertInvalid(self, e):
        expr = Sdf.StageVariableExpression(e)
        self.assertFalse(expr)
        self.assertTrue(expr.GetErrors())

    def test_Default(self):
        e = Sdf.StageVariableExpression()
        self.assertFalse(e)
        self.assertEqual(str(e), "")
        self.assertEqual(e.GetErrors(), ["No expression specified"])

    def test_IsExpression(self):
        """Test Sdf.StageVariableExpression.IsExpression"""
        self.assertTrue(Sdf.StageVariableExpression.IsExpression("`foo`"))
        self.assertFalse(Sdf.StageVariableExpression.IsExpression("foo"))

    def test_IsValidStageVariableType(self):
        """Test Sdf.StageVariableExpression.IsValidStageVariableType"""
        IsValidStageVariableType = \
            Sdf.StageVariableExpression.IsValidStageVariableType

        self.assertTrue(IsValidStageVariableType("string"))
        self.assertFalse(IsValidStageVariableType(1.23))

    def test_StageVarExpressions(self):
        """Test stage variable expressions consisting of just a top-level
        substitution."""

        # Evaluating a top-level stage variable substitution should
        # yield the exact value given in the stage variables dictionary.
        self.assertEvaluates(
            "`${FOO}`", {"FOO" : "string"}, 
            expected="string",
            expectedUsedStageVars=["FOO"])

        # If no value is found for the specified stage var, we should
        # get an error.
        self.assertEvaluationErrors(
            "`${FOO}`", { },
            ["No value for stage var 'FOO'"])

        # Test invalid expressions
        self.assertInvalid("`${FO-O}`")
        self.assertInvalid("`${FOO`")
        
    def test_StringExpressions(self):
        """Test string expressions."""

        # Test both single- and double-quoted strings.
        self.assertEvaluates(
            '''`''`''', {},
            expected='',
            expectedUsedStageVars=[])

        self.assertEvaluates(
            '''`""`''', {},
            expected='',
            expectedUsedStageVars=[])

        self.assertEvaluates(
            '''`"basic_string"`''', {}, 
            expected="basic_string",
            expectedUsedStageVars=[])

        self.assertEvaluates(
            '''`'basic_string'`''', {}, 
            expected="basic_string",
            expectedUsedStageVars=[])

        self.assertEvaluates(
            r'''`"quoted_\"double\"_'single'_test"`''', {},
            expected='''quoted_"double"_'single'_test''',
            expectedUsedStageVars=[])

        self.assertEvaluates(
            r'''`'quoted_"double"_\'single\'_test'`''', {},
            expected='''quoted_"double"_'single'_test''',
            expectedUsedStageVars=[])

        self.assertEvaluates(
            '''`"string_${A}_${B}"`''',
            {"A" : "substitution", "B" : "works"},
            expected="string_substitution_works",
            expectedUsedStageVars=["A", "B"])

        self.assertEvaluates(
            '''`'string_${A}_${B}'`''',
            {"A" : "substitution", "B" : "works"},
            expected="string_substitution_works",
            expectedUsedStageVars=["A", "B"])

        # No substitutions occur here since the '$' is escaped,
        # so \${A} and \${B} aren't recognized as subsitutions.
        self.assertEvaluates(
            r'''`"nosubs_\${A}_\${B}"`''',
            {"A" : "substitution", "B" : "works"},
            expected="nosubs_${A}_${B}",
            expectedUsedStageVars=[])

        self.assertEvaluates(
            r'''`'nosubs_\${A}_\${B}'`''',
            {"A" : "substitution", "B" : "works"},
            expected="nosubs_${A}_${B}",
            expectedUsedStageVars=[])

        # Test invalid expressions
        self.assertInvalid('''`"unescaped_"quotes"_are_bad"`''')
        self.assertInvalid('''`'unescaped_'quotes'_are_bad'`''')

        self.assertInvalid('`"bad_stage_var_${FOO"`')
        self.assertInvalid('`"bad_stage_var_${FO-O}"`')

        self.assertInvalid("`'`")
        self.assertInvalid('`"`')

    def test_NestedExpressions(self):
        """Test evaluating expressions with stage variable substitutions
        when the stage variables are expressions themselves."""
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
        """Test that circular stage variable substitutions result in an
        error and not an infinite loop."""
        self.assertEvaluationErrors(
            "`${FOO}`",
            {"FOO" : "`${BAR}`",
             "BAR" : "`${BAZ}`",
             "BAZ" : "`${FOO}`"},
            ["Encountered circular stage variable substitutions: "
             "['FOO', 'BAR', 'BAZ', 'FOO']"])
    
    def test_ErrorInNestedExpression(self):
        """Test that errors in subexpressions surface in final result."""
        self.assertEvaluationErrors(
            "`${FOO}`",
            {"FOO" : "`'${BAR}'`",
             "BAR" : "`${BAZ`"},
            ["Missing ending '}' at character 6 (in stage variable 'BAR')"])

    def test_UnsupportedStageVariableType(self):
        """Test that references to stage variables whose values are an
        unsupported type result in an error."""
        self.assertEvaluationErrors(
            "`${FOO}`",
            {"FOO" : 1.234},
            ["Stage variable 'FOO' has unsupported type double"])

        self.assertEvaluationErrors(
            "`'test_${FOO}'`",
            {"FOO" : 1.234},
            ["Stage variable 'FOO' has unsupported type double"])

if __name__ == "__main__":
    unittest.main()
