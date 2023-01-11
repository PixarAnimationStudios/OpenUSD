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

import unittest

from pxr import Usd, UsdText, Sdf

class TestUsdTextMarkupText(unittest.TestCase):
    def setUp(self):
        self.layer = Sdf.Layer.CreateAnonymous()
        self.stage = Usd.Stage.Open(self.layer.identifier)
        self.texts = self.stage.DefinePrim("/TestMarkupText", "MarkupText")
        self.schema = UsdText.MarkupText(self.texts)
        
    def test_create(self):
        assert self.texts
        assert self.texts.GetName() == 'TestMarkupText'     
        
    def test_schema(self):
        assert self.schema
        assert 'markupString' in self.schema.GetSchemaAttributeNames()
        assert 'markupLanguage' in self.schema.GetSchemaAttributeNames()
        assert 'primvars:backgroundColor' in self.schema.GetSchemaAttributeNames()

    def test_Attributes(self):
        stage = Usd.Stage.Open('markupText.usda')
        time = Usd.TimeCode.Default()
        schema = UsdText.MarkupText.Get(stage, '/TextA')
        markupString = schema.GetMarkupStringAttr().Get(time)
        self.assertEqual(markupString, '\\fArial|markup string')
        displayColor = schema.GetDisplayColorAttr().Get(time)
        self.assertEqual(displayColor,[(0, 1, 0)])
        schema.GetBackgroundColorAttr().Set((1, 0, 0))
        backGroundColor = schema.GetBackgroundColorAttr().Get(time);
        self.assertEqual(backGroundColor,(1, 0, 0))
        
        prim = stage.GetPrimAtPath('/TextA')
        textStyleAPI = UsdText.TextStyleAPI(prim);
        self.assertEqual(textStyleAPI.CanApply(prim),1)
        
    def test_TextStyle(self):
        stage = Usd.Stage.Open('markupText.usda')
        time = Usd.TimeCode.Default()
        prim = stage.GetPrimAtPath('/TextA')
        primPath = prim.GetPath()
        textStyleAPI = UsdText.TextStyleAPI.Apply(prim)
        if textStyleAPI.CanApply(prim):
            textStyleBinding = textStyleAPI.GetTextStyleBinding(primPath)
            style = textStyleBinding.GetTextStyle()
            typeface = style.GetTypefaceAttr().Get(time)
            self.assertEqual(typeface, 'Times New Roman')
            textHeight = style.GetTextHeightAttr().Get(time)
            self.assertEqual(textHeight, 20)
            bold = style.GetBoldAttr().Get(time)
            self.assertEqual(bold, 0)
        prim = stage.GetPrimAtPath('/TextB')
        primPath = prim.GetPath()
    
    def test_TextColumnStyle(self):
        stage = Usd.Stage.Open('markupText.usda')
        time = Usd.TimeCode.Default()
        prim = stage.GetPrimAtPath('/TextA')
        primPath = prim.GetPath()
        columnStyleAPI = UsdText.ColumnStyleAPI.Apply(prim);
        if columnStyleAPI.CanApply(prim):
            columnStyleBinding = columnStyleAPI.GetColumnStyleBinding(primPath)
            columns = columnStyleBinding.GetColumnStyles()
            columnWidth = columns[0].GetColumnWidthAttr().Get(time)
            self.assertEqual(columnWidth, 1000)
            columnHeight = columns[0].GetColumnHeightAttr().Get(time)
            self.assertEqual(columnHeight, 300)
            offset = columns[0].GetOffsetAttr().Get(time)
            self.assertEqual(offset, (0.0, 0.0))
        
    def test_TextParagraphStyle(self):
        stage = Usd.Stage.Open('markupText.usda')
        time = Usd.TimeCode.Default()
        prim = stage.GetPrimAtPath('/TextA')
        primPath = prim.GetPath()
        paragraphStyleAPI = UsdText.ParagraphStyleAPI.Apply(prim)
        if paragraphStyleAPI.CanApply(prim):
            paragraphStyleBinding = paragraphStyleAPI.GetTextParagraphStyleBinding(primPath)
            paragraphStyles = paragraphStyleBinding.GetParagraphStyles()
            leftIndent = paragraphStyles[0].GetLeftIndentAttr().Get(time)
            self.assertEqual(leftIndent, 15.0)
            rightIndent = paragraphStyles[0].GetRightIndentAttr().Get(time)
            self.assertEqual(rightIndent, 30.0)
            firstLineIndent = paragraphStyles[0].GetFirstLineIndentAttr().Get(time)
            self.assertEqual(firstLineIndent, 55.0)
            paragraphSpace = paragraphStyles[0].GetParagraphSpaceAttr().Get(time)
            self.assertEqual(paragraphSpace, 15.0)
            tabStopPositions = paragraphStyles[0].GetTabStopPositionsAttr().Get(time)
            self.assertEqual(tabStopPositions, [150.0, 350.0, 550.0, 750.0])
            tabStopTypes = paragraphStyles[0].GetTabStopTypesAttr().Get(time)
            self.assertEqual(tabStopTypes, ["centerTab", "centerTab", "centerTab", "centerTab"])

if __name__ == '__main__':
    unittest.main(verbosity=2)
