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

class TestUsdTextSimpleText(unittest.TestCase):
    def setUp(self):
        self.layer = Sdf.Layer.CreateAnonymous()
        self.stage = Usd.Stage.Open(self.layer.identifier)
        self.texts = self.stage.DefinePrim("/TestSimpleText", "SimpleText")
        self.schema = UsdText.SimpleText(self.texts)
        
    def test_create(self):
        assert self.texts
        assert self.texts.GetName() == 'TestSimpleText'     
        
    def test_schema(self):
        assert self.schema
        assert 'textData' in self.schema.GetSchemaAttributeNames()
        assert 'primvars:backgroundColor' in self.schema.GetSchemaAttributeNames()

    def test_Attributes(self):
        stage = Usd.Stage.Open('simpleText.usda')
        time = Usd.TimeCode.Default()
        schema = UsdText.SimpleText.Get(stage, '/TextA')
        textData = schema.GetTextDataAttr().Get(time);
        self.assertEqual(textData, 'The quick brown fox')
        displayColor = schema.GetDisplayColorAttr().Get(time);
        self.assertEqual(displayColor,[(1, 1, 0)]);
        schema.GetBackgroundColorAttr().Set((1, 0, 0));
        backGroundColor = schema.GetBackgroundColorAttr().Get(time);
        self.assertEqual(backGroundColor,(1, 0, 0));
               
        schema = UsdText.SimpleText.Get(stage, '/TextB')
        textData = schema.GetTextDataAttr().Get(time);
        self.assertEqual(textData, 'jumps over the')
        
    def test_TextStyle(self):
        stage = Usd.Stage.Open('simpleText.usda')
        time = Usd.TimeCode.Default()
        prim = stage.GetPrimAtPath('/TextA')
        primPath = prim.GetPath()
        textStyleAPI = UsdText.TextStyleAPI.Apply(prim);
        if textStyleAPI.CanApply(prim):
            textStyleBinding = textStyleAPI.GetTextStyleBinding(primPath);
            style = textStyleBinding.GetTextStyle();
            typeface = style.GetTypefaceAttr().Get(time);
            self.assertEqual(typeface, 'Times New Roman')
            textHeight = style.GetTextHeightAttr().Get(time);
            self.assertEqual(textHeight, 100)
            bold = style.GetBoldAttr().Get(time);
            self.assertEqual(bold, 1)
            overrideType = style.GetOverlineTypeAttr().Get(time);
            self.assertEqual(overrideType, "normal")
        prim = stage.GetPrimAtPath('/TextB')
        primPath = prim.GetPath()
        textStyleAPI = UsdText.TextStyleAPI.Apply(prim);
        if textStyleAPI.CanApply(prim):
            textStyleBinding = textStyleAPI.GetTextStyleBinding(primPath);
            style = textStyleBinding.GetTextStyle();
            typeface = style.GetTypefaceAttr().Get(time);
            self.assertEqual(typeface, 'Arial')
            textHeight = style.GetTextHeightAttr().Get(time);
            self.assertEqual(textHeight, 70)
            italic = style.GetItalicAttr().Get(time);
            self.assertEqual(italic, 1)
            underlineType = style.GetUnderlineTypeAttr().Get(time);
            self.assertEqual(underlineType, "normal")

if __name__ == '__main__':
    unittest.main(verbosity=2)
