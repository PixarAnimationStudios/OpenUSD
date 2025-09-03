#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Sdf, Usd

class TestUsdAttributeLimits(unittest.TestCase):
    def test_LimitsObject(self):
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim('/test_LimitsObject')
        attr = prim.CreateAttribute('attr', Sdf.ValueTypeNames.Int)

        # Invalid limits
        invalid = Usd.AttributeLimits()
        self.assertFalse(invalid.IsValid())
        self.assertFalse(invalid)
        self.assertEqual(invalid.GetAttribute(), Usd.Attribute())
        self.assertEqual(invalid.GetSubDictKey(), '')

        # Soft limits
        soft = attr.GetSoftLimits()
        self.assertTrue(soft.IsValid())
        self.assertTrue(soft)
        self.assertEqual(soft.GetAttribute(), attr)
        self.assertEqual(soft.GetSubDictKey(), Usd.LimitsKeys.Soft)

        # Hard limits
        hard = attr.GetHardLimits()
        self.assertTrue(hard.IsValid())
        self.assertTrue(hard)
        self.assertEqual(hard.GetAttribute(), attr)
        self.assertEqual(hard.GetSubDictKey(), Usd.LimitsKeys.Hard)

        # Custom limits
        custom = attr.GetLimits('customLimits')
        self.assertTrue(custom.IsValid())
        self.assertTrue(custom)
        self.assertEqual(custom.GetAttribute(), attr)
        self.assertEqual(custom.GetSubDictKey(), 'customLimits')

        # Equality ops
        self.assertEqual(soft, soft)
        self.assertEqual(hard, hard)
        self.assertEqual(custom, custom)

        self.assertNotEqual(soft, hard)
        self.assertNotEqual(hard, custom)
        self.assertNotEqual(soft, custom)

        self.assertEqual(attr.GetLimits('foo'),
                         attr.GetLimits('foo'))
        self.assertNotEqual(attr.GetLimits('foo'),
                            attr.GetLimits('bar'))

    def test_Opinions(self):
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().ImportFromString('''
            #usda 1.0

            def "test_Opinions"
            {
                int attr = 1 (
                    limits = {
                        dictionary hard = {
                            int minimum = 1
                            int maximum = 10
                            int customInt = 25
                        }
                        dictionary soft = {
                            int maximum = 7
                            double customDouble = 10.5
                        }
                    }
                )
            }
            ''')

        attr = stage.GetAttributeAtPath('/test_Opinions.attr')
        hard = attr.GetHardLimits()
        soft = attr.GetSoftLimits()

        self.assertTrue(attr.HasAuthoredLimits())

        # Verify opinions are present
        self.assertTrue(hard.HasAuthored())
        self.assertTrue(hard.HasAuthoredMinimum())
        self.assertTrue(hard.HasAuthoredMaximum())
        self.assertTrue(hard.HasAuthored('customInt'))
        self.assertFalse(hard.HasAuthored('non-existent'))

        # Clear individual fields and re-check
        self.assertTrue(hard.ClearMinimum())
        self.assertFalse(hard.HasAuthoredMinimum())

        self.assertTrue(hard.ClearMaximum())
        self.assertFalse(hard.HasAuthoredMaximum())

        self.assertTrue(hard.Clear('customInt'))
        self.assertFalse(hard.HasAuthored('customInt'))

        # Nothing should be left
        self.assertFalse(hard.HasAuthored())

        # Clear whole subdict at once
        self.assertTrue(soft.HasAuthored())
        self.assertFalse(soft.HasAuthoredMinimum())
        self.assertTrue(soft.HasAuthoredMaximum())
        self.assertTrue(soft.HasAuthored('customDouble'))

        self.assertTrue(soft.Clear())

        self.assertFalse(soft.HasAuthored())
        self.assertFalse(soft.HasAuthoredMinimum())
        self.assertFalse(soft.HasAuthoredMaximum())
        self.assertFalse(soft.HasAuthored('customDouble'))

        self.assertFalse(attr.HasAuthoredLimits())

    def test_BasicUsage(self):
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().ImportFromString('''
            #usda 1.0

            def "test_BasicUsage"
            {
                int attr = 1 (
                    limits = {
                        dictionary hard = {
                            int minimum = 1
                            int maximum = 10
                            int customInt = 25
                        }
                        dictionary soft = {
                            int minimum = 3
                            int maximum = 7
                            double customDouble = 10.5
                        }
                        dictionary customLimits = {
                            int minimum = 20
                            int maximum = 30
                            int customBool = 0
                        }
                    }
                )
            }
            ''')

        attr = stage.GetAttributeAtPath('/test_BasicUsage.attr')

        # Validate loaded values
        self.assertTrue(attr.HasAuthoredLimits())
        self.assertEqual(
            attr.GetLimits(),
            {
                Usd.LimitsKeys.Hard : {
                    Usd.LimitsKeys.Minimum : 1,
                    Usd.LimitsKeys.Maximum : 10,
                    'customInt' : 25
                },
                Usd.LimitsKeys.Soft : {
                    Usd.LimitsKeys.Minimum : 3,
                    Usd.LimitsKeys.Maximum : 7,
                    'customDouble' : 10.5
                },
                'customLimits' : {
                    Usd.LimitsKeys.Minimum : 20,
                    Usd.LimitsKeys.Maximum : 30,
                    'customBool' : False
                }
            }
        )

        # Check hard limits
        hard = attr.GetHardLimits()
        self.assertEqual(hard.GetMinimum(), 1)
        self.assertEqual(hard.GetMaximum(), 10)

        self.assertEqual(hard.Get(Usd.LimitsKeys.Minimum), 1)
        self.assertEqual(hard.Get(Usd.LimitsKeys.Maximum), 10)
        self.assertEqual(hard.Get('customInt'), 25)
        self.assertEqual(hard.Get('non-existent'), None)

        # Passing an empy key should fail
        self.assertEqual(hard.Get(''), None)
        with self.assertRaises(BaseException):
            hard.Set('', 5)

        # Check soft limits
        soft = attr.GetSoftLimits()
        self.assertEqual(soft.GetMinimum(), 3)
        self.assertEqual(soft.GetMaximum(), 7)

        self.assertEqual(soft.Get(Usd.LimitsKeys.Minimum), 3)
        self.assertEqual(soft.Get(Usd.LimitsKeys.Maximum), 7)
        self.assertEqual(soft.Get('customDouble'), 10.5)
        self.assertEqual(soft.Get('non-existent'), None)

        # Passing an empy key should fail
        self.assertEqual(soft.Get(''), None)
        with self.assertRaises(BaseException):
            soft.Set('', 5)

        # Check custom limits
        custom = attr.GetLimits('customLimits')
        self.assertEqual(custom.GetMinimum(), 20)
        self.assertEqual(custom.GetMaximum(), 30)

        self.assertEqual(custom.Get(Usd.LimitsKeys.Minimum), 20)
        self.assertEqual(custom.Get(Usd.LimitsKeys.Maximum), 30)
        self.assertEqual(custom.Get('customBool'), False)
        self.assertEqual(custom.Get('non-existent'), None)

        # Passing an empy key should fail
        self.assertEqual(custom.Get(''), None)
        with self.assertRaises(BaseException):
            custom.Set('', 5)

        # Modify hard limits
        self.assertTrue(hard.SetMinimum(0))
        self.assertEqual(hard.GetMinimum(), 0)

        self.assertTrue(hard.SetMaximum(11))
        self.assertEqual(hard.GetMaximum(), 11)

        self.assertTrue(hard.Set('customInt', 50))
        self.assertEqual(hard.Get('customInt'), 50)

        self.assertTrue(hard.Set('newValue', 100))
        self.assertEqual(hard.Get('newValue'), 100)

        # Modify soft limits
        self.assertTrue(soft.SetMinimum(2))
        self.assertEqual(soft.GetMinimum(), 2)

        self.assertTrue(soft.SetMaximum(8))
        self.assertEqual(soft.GetMaximum(), 8)

        self.assertTrue(soft.Set('customDouble', 12.75))
        self.assertEqual(soft.Get('customDouble'), 12.75)

        self.assertTrue(soft.Set('newValue', 'a value'))
        self.assertEqual(soft.Get('newValue'), 'a value')

        # Modify custom limits
        self.assertTrue(custom.SetMinimum(40))
        self.assertEqual(custom.GetMinimum(), 40)

        self.assertTrue(custom.SetMaximum(60))
        self.assertEqual(custom.GetMaximum(), 60)

        self.assertTrue(custom.Set('customBool', True))
        self.assertEqual(custom.Get('customBool'), True)

        self.assertTrue(custom.Set('newValue', { 'nested' : 'subdict' }))
        self.assertEqual(custom.Get('newValue'), { 'nested' : 'subdict' })

        # Re-check entire dict w/modifications
        self.assertEqual(
            attr.GetLimits(),
            {
                Usd.LimitsKeys.Hard : {
                    Usd.LimitsKeys.Minimum : 0,
                    Usd.LimitsKeys.Maximum : 11,
                    'customInt' : 50,
                    'newValue' : 100
                },
                Usd.LimitsKeys.Soft : {
                    Usd.LimitsKeys.Minimum : 2,
                    Usd.LimitsKeys.Maximum : 8,
                    'customDouble' : 12.75,
                    'newValue' : 'a value'
                },
                'customLimits' : {
                    Usd.LimitsKeys.Minimum : 40,
                    Usd.LimitsKeys.Maximum : 60,
                    'customBool' : True,
                    'newValue' : { 'nested' : 'subdict' }
                }
            }
        )

        # Replace wholesale
        newLimits = {
            Usd.LimitsKeys.Hard : {
                Usd.LimitsKeys.Minimum : 80,
                Usd.LimitsKeys.Maximum : 90
            },
            Usd.LimitsKeys.Soft : {
                Usd.LimitsKeys.Minimum : 50,
                Usd.LimitsKeys.Maximum : 70
            },
            'customLimits' : {
                Usd.LimitsKeys.Minimum : 5,
                Usd.LimitsKeys.Maximum : 15
            }
        }

        self.assertTrue(attr.SetLimits(newLimits))
        self.assertEqual(attr.GetLimits(), newLimits)

        self.assertEqual(hard.GetMinimum(), 80)
        self.assertEqual(hard.GetMaximum(), 90)
        self.assertEqual(soft.GetMinimum(), 50)
        self.assertEqual(soft.GetMaximum(), 70)
        self.assertEqual(custom.GetMinimum(), 5)
        self.assertEqual(custom.GetMaximum(), 15)

        # Replace subdicts
        self.assertTrue(hard.Set({
            Usd.LimitsKeys.Minimum : 120,
            Usd.LimitsKeys.Maximum : 140
        }))
        self.assertTrue(soft.Set({
            Usd.LimitsKeys.Minimum : 130,
            Usd.LimitsKeys.Maximum : 150
        }))
        self.assertTrue(custom.Set({
            Usd.LimitsKeys.Minimum : 20,
            Usd.LimitsKeys.Maximum : 30
        }))

        self.assertEqual(hard.GetMinimum(), 120)
        self.assertEqual(hard.GetMaximum(), 140)
        self.assertEqual(soft.GetMinimum(), 130)
        self.assertEqual(soft.GetMaximum(), 150)
        self.assertEqual(custom.GetMinimum(), 20)
        self.assertEqual(custom.GetMaximum(), 30)

        # Clear
        self.assertTrue(attr.ClearLimits())
        self.assertTrue(not attr.HasAuthoredLimits())
        self.assertEqual(attr.GetLimits(), {})

        self.assertEqual(hard.GetMinimum(), None)
        self.assertEqual(hard.GetMaximum(), None)
        self.assertEqual(hard.Get(Usd.LimitsKeys.Minimum), None)
        self.assertEqual(hard.Get(Usd.LimitsKeys.Maximum), None)

        self.assertEqual(soft.GetMinimum(), None)
        self.assertEqual(soft.GetMaximum(), None)
        self.assertEqual(soft.Get(Usd.LimitsKeys.Minimum), None)
        self.assertEqual(soft.Get(Usd.LimitsKeys.Maximum), None)

        self.assertEqual(custom.GetMinimum(), None)
        self.assertEqual(custom.GetMaximum(), None)
        self.assertEqual(custom.Get(Usd.LimitsKeys.Minimum), None)
        self.assertEqual(custom.Get(Usd.LimitsKeys.Maximum), None)

    def test_Validation(self):
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim('/test_Validation')
        attr = prim.CreateAttribute('attr', Sdf.ValueTypeNames.Int)
        soft = attr.GetSoftLimits()

        # Default-constructed result object should be invalid
        result = Usd.AttributeLimits.ValidationResult()
        self.assertFalse(result)
        self.assertFalse(result.success)
        self.assertEqual(result.invalidValuesDict, {})
        self.assertEqual(result.conformedSubDict, {})
        self.assertEqual(result.GetErrorString(), '')

        # Empty subdict should be valid
        result = soft.Validate({})
        self.assertTrue(result)
        self.assertTrue(result.success)
        self.assertEqual(result.invalidValuesDict, {})
        self.assertEqual(result.conformedSubDict, {})
        self.assertEqual(result.GetErrorString(), '')

        # Good values should be valid
        subdict = {
            Usd.LimitsKeys.Minimum : 5,
            Usd.LimitsKeys.Maximum : 10,
            'customStr' : 'foo'
        }
        result = soft.Validate(subdict)
        self.assertTrue(result)
        self.assertTrue(result.success)
        self.assertEqual(result.invalidValuesDict, {})
        self.assertEqual(result.conformedSubDict, subdict)
        self.assertEqual(result.GetErrorString(), '')

        # Conformable values should be valid
        result = soft.Validate({
            Usd.LimitsKeys.Minimum : True,
            Usd.LimitsKeys.Maximum : 42.2,
            'customStr' : 'foo'
        })
        self.assertTrue(result)
        self.assertTrue(result.success)
        self.assertEqual(result.invalidValuesDict, {})
        self.assertEqual(result.conformedSubDict, {
            Usd.LimitsKeys.Minimum : 1,
            Usd.LimitsKeys.Maximum : 42,
            'customStr' : 'foo'
        })
        self.assertEqual(result.GetErrorString(), '')

        # Non-conformable value should not be valid
        result = soft.Validate({
            Usd.LimitsKeys.Minimum : 1,
            Usd.LimitsKeys.Maximum : 'forty-two',
            'customStr' : 'foo'
        })
        self.assertFalse(result)
        self.assertFalse(result.success)
        self.assertEqual(result.invalidValuesDict, {
            Usd.LimitsKeys.Maximum : 'forty-two'
        })
        self.assertEqual(result.conformedSubDict, {})
        self.assertTrue(len(result.GetErrorString()) > 0)

    def test_SetWrongType(self):
        from pixar import Tf
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim('/test_SetWrongType')
        attr = prim.CreateAttribute('attr', Sdf.ValueTypeNames.Int)
        hard = attr.GetHardLimits()

        # Min/max types must match or be castable to the attribute's value type
        with self.assertRaises(Tf.ErrorException):
            hard.SetMinimum('a string')
        with self.assertRaises(Tf.ErrorException):
            hard.SetMaximum('a string')

        with self.assertRaises(Tf.ErrorException):
            hard.SetMinimum({ 'a' : 'b' })
        with self.assertRaises(Tf.ErrorException):
            hard.SetMaximum({ 'a' : 'b' })

        with self.assertRaises(Tf.ErrorException):
            hard.SetMinimum(None)
        with self.assertRaises(Tf.ErrorException):
            hard.SetMaximum(None)

        # Whole-dict variant is enforced too
        with self.assertRaises(Tf.ErrorException):
            attr.SetLimits({
                Usd.LimitsKeys.Hard : {
                    Usd.LimitsKeys.Minimum: 'min'
                }
            })
        with self.assertRaises(Tf.ErrorException):
            attr.SetLimits({
                Usd.LimitsKeys.Hard : {
                    Usd.LimitsKeys.Minimum: 5,     # okay
                    Usd.LimitsKeys.Maximum: 'max'  # bad
                }
            })

        # And whole-subdict
        with self.assertRaises(Tf.ErrorException):
            hard.Set({
                Usd.LimitsKeys.Minimum: 'min'
            })
        with self.assertRaises(Tf.ErrorException):
            hard.Set({
                Usd.LimitsKeys.Minimum: 5,     # okay
                Usd.LimitsKeys.Maximum: 'max'  # bad
            })

        # Castable types should be ok
        self.assertTrue(hard.SetMinimum(47.5))
        self.assertEqual(hard.GetMinimum(), 47)
        self.assertTrue(hard.SetMaximum(48.2))
        self.assertEqual(hard.GetMaximum(), 48)

        self.assertTrue(hard.SetMinimum(True))
        self.assertEqual(hard.GetMinimum(), 1)
        self.assertTrue(hard.SetMaximum(False))
        self.assertEqual(hard.GetMaximum(), 0)

        # Whole-dict castable
        attr.SetLimits({
            Usd.LimitsKeys.Hard : {
                Usd.LimitsKeys.Minimum: 50.2,
                Usd.LimitsKeys.Maximum: True
            }
        })
        self.assertEqual(hard.GetMinimum(), 50)
        self.assertEqual(hard.GetMaximum(), 1)

        # Whole-subdict castable
        hard.Set({
            Usd.LimitsKeys.Minimum: 42.2,
            Usd.LimitsKeys.Maximum: False
        })
        self.assertEqual(hard.GetMinimum(), 42)
        self.assertEqual(hard.GetMaximum(), 0)

    def test_GetWrongType(self):
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().ImportFromString('''
            #usda 1.0

            def "test_GetWrongType"
            {
                int attr = 1 (
                    limits = {
                        dictionary hard = {
                            double minimum = 1.5
                            bool maximum = 0
                        }
                        dictionary soft = {
                            string minimum = 'min'
                            dictionary maximum = {
                                string value = "max"
                            }
                        }
                    }
                )
            }
            ''')

        attr = stage.GetAttributeAtPath('/test_GetWrongType.attr')
        hard = attr.GetHardLimits()
        soft = attr.GetSoftLimits()

        # The Python wrappers for the min/max getters use the VtValue API which
        # will return unexpected values as-is and not error or cast.
        self.assertEqual(
            attr.GetLimits(),
            {
                Usd.LimitsKeys.Hard : {
                    Usd.LimitsKeys.Minimum : 1.5,
                    Usd.LimitsKeys.Maximum : False
                },
                Usd.LimitsKeys.Soft : {
                    Usd.LimitsKeys.Minimum : 'min',
                    Usd.LimitsKeys.Maximum : { 'value' : 'max' }
                }
            }
        )

        self.assertEqual(hard.GetMinimum(), 1.5)
        self.assertEqual(hard.GetMaximum(), False)
        self.assertEqual(soft.GetMinimum(), 'min')
        self.assertEqual(soft.GetMaximum(), { 'value' : 'max' })

if __name__ == "__main__":
    unittest.main()
