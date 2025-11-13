#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Sdr, Tf, Usd, UsdShade

class TestSdrShaderNodeQuery(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        registry = Sdr.Registry()
        fsPlugin = Sdr._FilesystemDiscoveryPlugin()
        registry.SetExtraDiscoveryPlugins([fsPlugin])

        stage = Usd.Stage.Open('SimpleNodes.usda')
        shaderNames = ["SimpleNodeA", "SimpleNodeB", "OtherNodeC"]
        shaderDefs = [UsdShade.Shader.Get(stage, f'/{n}') for n in shaderNames]
        discoveryResults = [UsdShade.ShaderDefUtils.GetDiscoveryResults(
            s, stage.GetRootLayer().realPath)[0] for s in shaderDefs]

        registry = Sdr.Registry()
        for result in discoveryResults:
            registry.AddDiscoveryResult(result)

        registry.SetExtraParserPlugins(
            [Tf.Type.FindByName('UsdShadeShaderDefParserPlugin')])
        ids = registry.GetShaderNodeIdentifiers()
        assert set(ids) == {"SimpleNodeA", "SimpleNodeB", "OtherNodeC"}

    def test_QuerySimple(self):
        registry = Sdr.Registry()

        # Test simple single result query
        query = Sdr.ShaderNodeQuery() \
                   .SelectDistinct([
                       Sdr.NodeFieldKey.Identifier,
                       Sdr.NodeFieldKey.SourceType]) \
                   .NodeValueIs("testMetadata", "bar") \
                   .NodeValueIs(Sdr.NodeFieldKey.Identifier, "SimpleNodeA") 

        queryResult = registry.RunQuery(query)
        self.assertEqual(queryResult.GetKeys(), [
            Sdr.NodeFieldKey.Identifier,
            Sdr.NodeFieldKey.SourceType])
        values = queryResult.GetValues()
        self.assertEqual(len(values), 1)
        self.assertEqual(queryResult.GetStringifiedValues(), values)
        identifier, sourceType = values[0]

        node = registry.GetShaderNodeByIdentifierAndType(
            identifier, sourceType)
        self.assertIsNotNone(node)
        self.assertEqual(node.GetIdentifier(), "SimpleNodeA")
        self.assertEqual(node.GetSourceType(), "glslfx")

        # Test simple exclusion query
        query = Sdr.ShaderNodeQuery() \
                   .SelectDistinct(Sdr.NodeFieldKey.Identifier) \
                   .SelectDistinct(Sdr.NodeFieldKey.SourceType) \
                   .NodeValueIsNot("testMetadata", "bar") \
                   .NodeValueIs(Sdr.NodeFieldKey.Identifier, "SimpleNodeA")
        queryResult = query.Run()
        self.assertEqual(queryResult.GetKeys(), [
            Sdr.NodeFieldKey.Identifier,
            Sdr.NodeFieldKey.SourceType])
        values = queryResult.GetValues()
        self.assertEqual(len(values), 0)
        self.assertEqual(queryResult.GetStringifiedValues(), values)

    def test_QueryMultipleResults(self):
        registry = Sdr.Registry()

        # Test query where multiple nodes match the criteria but
        # "Select Distinct" semantics ensures only one value is
        # returned
        query = Sdr.ShaderNodeQuery() \
                   .SelectDistinct("testMetadata") \
                   .NodeValueIsNot("testMetadata", None)
        queryResult = registry.RunQuery(query)
        values = queryResult.GetValues()
        self.assertEqual(values, [["bar"]])
        self.assertEqual(queryResult.GetStringifiedValues(), values)
        self.assertEqual(len(queryResult.GetAllShaderNodes()), 2)
        nodesByValues = queryResult.GetShaderNodesByValues()
        self.assertEqual(len(nodesByValues), 1)
        self.assertEqual(len(nodesByValues[0]), 2)

        # Test query that is semantically equivalent to the above
        # query
        query = Sdr.ShaderNodeQuery() \
                   .SelectDistinct("testMetadata") \
                   .NodeHasValueFor("testMetadata")
        queryResult = query.Run()
        values = queryResult.GetValues()
        self.assertEqual(values, [["bar"]])
        self.assertEqual(queryResult.GetStringifiedValues(), values)
        self.assertEqual(len(queryResult.GetAllShaderNodes()), 2)
        nodesByValues = queryResult.GetShaderNodesByValues()
        self.assertEqual(len(nodesByValues), 1)
        self.assertEqual(len(nodesByValues[0]), 2)

        # Test query where multiple nodes match the criteria and
        # multiple values are returned
        query = Sdr.ShaderNodeQuery() \
                   .SelectDistinct("testMetadataDifferent") \
                   .SelectDistinct("testMetadata") \
                   .NodeHasValueFor("testMetadataDifferent")
        queryResult = query.Run()
        keys = queryResult.GetKeys()
        values = queryResult.GetValues()
        self.assertEqual(len(values), 3)
        self.assertIn(["barA", "bar"], values)
        self.assertIn(["barB", "bar"], values)
        self.assertIn(["barC", None], values)
        valueStrs = queryResult.GetStringifiedValues()
        self.assertNotEqual(valueStrs, values)
        self.assertIn(["barC", ""], valueStrs)
        nodesByValues = queryResult.GetShaderNodesByValues()
        for i, valuesRow in enumerate(values):
            for j, value in enumerate(valuesRow):
                for node in nodesByValues[i]:
                    self.assertEqual(value, node.GetDataForKey(keys[j]))

    def test_QueryOnlyNodes(self):
        # Test query with NodeValueIsIn/NodeValueIsNotIn
        query = Sdr.ShaderNodeQuery() \
                   .NodeValueIsNotIn("testMetadataDifferent",
                                     ["barB", "barC"]) \
                   .NodeValueIsIn("testMetadataDifferent", ["barB", "barA"])
        queryResult = query.Run()
        self.assertEqual(len(queryResult.GetKeys()), 0)
        values = queryResult.GetValues()
        self.assertEqual(len(values), 0)
        self.assertEqual(queryResult.GetStringifiedValues(), values)
        self.assertEqual(len(queryResult.GetShaderNodesByValues()), 0)
        nodes = queryResult.GetAllShaderNodes()
        self.assertEqual(len(nodes), 1)
        self.assertEqual(nodes[0].GetIdentifier(), "SimpleNodeA")

    def test_SdrQueryCustomFilter(self):
        # Test query with a simple custom filter
        def filterFn(node):
            return "SimpleNode" in node.GetIdentifier()

        query = Sdr.ShaderNodeQuery() \
                   .SelectDistinct("testMetadataDifferent") \
                   .CustomFilter(filterFn)
        queryResult = query.Run()
        values = queryResult.GetValues()
        assert len(values) == 2
        assert ["barA"] in values
        assert ["barB"] in values

        # Test query with multiple custom filters
        def anotherFilterFn(node):
            return "inputA" in node.GetShaderInputNames()

        query = Sdr.ShaderNodeQuery() \
                   .SelectDistinct(Sdr.NodeFieldKey.Identifier) \
                   .CustomFilter(filterFn) \
                   .CustomFilter(anotherFilterFn)
        queryResult = query.Run()
        values = queryResult.GetValues()
        assert len(values) == 1
        nodes = queryResult.GetAllShaderNodes()
        assert values[0][0] == nodes[0].GetIdentifier()

        # Test query with an illegal custom filter, no node arg
        def malformedFn():
            print("hi!")

        query = Sdr.ShaderNodeQuery() \
                   .CustomFilter(malformedFn)
        with self.assertRaises(TypeError):
            query.Run()

        # Test query with deleted custom filter. "del" decreases
        # the reference count on the object, but query still holds
        # a reference to the object, so the query runs successfully.
        def deleteFn(node):
            return False

        query = Sdr.ShaderNodeQuery() \
                   .CustomFilter(deleteFn)
        del deleteFn
        queryResult = query.Run()
        self.assertEqual(len(queryResult.GetAllShaderNodes()), 0)

        # Test query with a filter returning a non-bool
        def weirdReturnFn(node):
            return node

        query = Sdr.ShaderNodeQuery() \
                   .CustomFilter(weirdReturnFn)
    
        with self.assertRaises(TypeError):
            query.Run()


if __name__ == '__main__':
    unittest.main()
