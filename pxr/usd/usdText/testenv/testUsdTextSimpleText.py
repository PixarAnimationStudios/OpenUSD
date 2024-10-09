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

from pxr import Gf, Usd, UsdText, Sdf

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
        assert 'primvars:backgroundOpacity' in self.schema.GetSchemaAttributeNames()
        assert 'textMetricsUnit' in self.schema.GetSchemaAttributeNames()

    def test_Attributes(self):
        stage = Usd.Stage.Open('simpleText.usda')
        time = Usd.TimeCode.Default()
        schema = UsdText.SimpleText.Get(stage, '/TextA')
        textData = schema.GetTextDataAttr().Get(time);
        self.assertEqual(textData, 'The quick brown fox')
        displayColor = schema.GetDisplayColorAttr().Get(time);
        self.assertEqual(displayColor, [(1, 1, 0)]);
        backgroundColor = schema.GetBackgroundColorAttr().Get(time);
        self.assertEqual(backgroundColor, (0, 1, 0));
        backgroundOpacity = schema.GetBackgroundOpacityAttr().Get(time);
        self.assertTrue(Gf.IsClose(backgroundOpacity, 0.78, 1e-6));
        textMetricsUnit = schema.GetTextMetricsUnitAttr().Get(time);
        self.assertEqual(textMetricsUnit, UsdText.Tokens.publishingPoint);
               
        schema = UsdText.SimpleText.Get(stage, '/TextB')
        textData = schema.GetTextDataAttr().Get(time);
        self.assertEqual(textData, 'jumps over the')
        displayColor = schema.GetDisplayColorAttr().Get(time);
        self.assertEqual(displayColor, [(1, 0, 1)]);
        schema.GetBackgroundColorAttr().Set((1, 0, 0));
        backGroundColor = schema.GetBackgroundColorAttr().Get(time);
        self.assertEqual(backGroundColor, (1, 0, 0));
        schema.GetBackgroundOpacityAttr().Set(0.92);
        backGroundOpacity = schema.GetBackgroundOpacityAttr().Get(time);
        self.assertTrue(Gf.IsClose(backGroundOpacity, 0.92, 1e-6));
        schema.GetTextMetricsUnitAttr().Set(UsdText.Tokens.pixel);
        textMetricsUnit = schema.GetTextMetricsUnitAttr().Get(time);
        self.assertEqual(textMetricsUnit, UsdText.Tokens.pixel);
        
    def test_TextStyle(self):
        stage = Usd.Stage.Open('simpleText.usda')
        time = Usd.TimeCode.Default()
        prim = stage.GetPrimAtPath('/TextA')
        primPath = prim.GetPath()
        textStyleAPI = UsdText.TextStyleAPI.Apply(prim);
        if textStyleAPI.CanApply(prim):
            textStyleBinding = textStyleAPI.GetTextStyleBinding(primPath);
            style = textStyleBinding.GetTextStyle();
            typeface = style.GetFontTypefaceAttr().Get(time);
            self.assertEqual(typeface, 'Times New Roman')
            format = style.GetFontFormatAttr().Get(time);
            self.assertEqual(format, 'ttf/cff/otf')
            altTypeface = style.GetFontAltTypefaceAttr().Get(time);
            self.assertEqual(altTypeface, 'Arial')
            altFormat = style.GetFontAltFormatAttr().Get(time);
            self.assertEqual(altFormat, 'ttf/cff/otf')
            ifBold = style.GetFontBoldAttr().Get(time);
            self.assertTrue(ifBold)
            charHeight = style.GetCharHeightAttr().Get(time);
            self.assertEqual(charHeight, 100)
            charWidth = style.GetCharWidthFactorAttr().Get(time);
            self.assertTrue(Gf.IsClose(charWidth, 1.2, 1e-6));
            charSpacingFactor = style.GetCharSpacingFactorAttr().Get(time);
            self.assertTrue(Gf.IsClose(charSpacingFactor, 1.5, 1e-6));
            obliqueAngle = style.GetObliqueAngleAttr().Get(time);
            self.assertTrue(Gf.IsClose(obliqueAngle, 0.8, 1e-6));
            overlineType = style.GetOverlineTypeAttr().Get(time);
            self.assertEqual(overlineType, "normal")
        prim = stage.GetPrimAtPath('/TextB')
        primPath = prim.GetPath()
        textStyleAPI = UsdText.TextStyleAPI.Apply(prim);
        if textStyleAPI.CanApply(prim):
            textStyleBinding = textStyleAPI.GetTextStyleBinding(primPath);
            style = textStyleBinding.GetTextStyle();
            typeface = style.GetFontTypefaceAttr().Get(time);
            self.assertEqual(typeface, 'Arial')
            format = style.GetFontFormatAttr().Get(time);
            self.assertEqual(format, 'none')
            italic = style.GetFontItalicAttr().Get(time);
            self.assertEqual(italic, 1)
            weight = style.GetFontWeightAttr().Get(time);
            self.assertEqual(weight, 300)
            charHeight = style.GetCharHeightAttr().Get(time);
            self.assertEqual(charHeight, 70)
            style.GetCharWidthFactorAttr().Set(0.66);
            charWidth = style.GetCharWidthFactorAttr().Get(time);
            self.assertTrue(Gf.IsClose(charWidth, 0.66, 1e-6));
            style.GetObliqueAngleAttr().Set(1.22);
            obliqueAngle = style.GetObliqueAngleAttr().Get(time);
            self.assertTrue(Gf.IsClose(obliqueAngle, 1.22, 1e-6));
            style.GetOverlineTypeAttr().Set("normal");
            overlineType = style.GetOverlineTypeAttr().Get(time);
            self.assertEqual(overlineType, "normal");
            underlineType = style.GetUnderlineTypeAttr().Get(time);
            self.assertEqual(underlineType, "normal")
            strikethroughType = style.GetStrikethroughTypeAttr().Get(time);
            self.assertEqual(strikethroughType, "doubleLines")

if __name__ == '__main__':
    unittest.main(verbosity=2)
