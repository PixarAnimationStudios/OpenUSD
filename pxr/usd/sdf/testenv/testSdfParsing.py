#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

# This tests a set of sample usda files that are either expected to load
# successfully, or to emit warnings.  Files with _bad_ in the name are
# expected to emit warnings, but in no case should they cause a crash.
#
# For files that we expect to read successfully, we take the further step
# of writing them out, reading in what we wrote, and writing it out again,
# and then comparing the two versions written to make sure they are the
# same.  This is to detect any accumulative error, such as quoting or
# escaping errors.

from __future__ import print_function
import sys, os, difflib, unittest, platform
from pxr import Tf, Sdf

# Default encoding on Windows in python 3+ is not UTF-8, but it is on Linux &
# Mac.  Provide a wrapper here so we specify that in that case.
def _open(*args, **kw):
    if platform.system() == "Windows":
        kw['encoding'] = 'utf8'
        return open(*args, **kw)
    else:
        return open(*args, **kw)


def removeFiles(*filenames):
    """Removes the given files (if one of the args is itself a tuple or list, "unwrap" it.)"""
    for f in filenames:
        if isinstance(f, (tuple, list)):
            removeFiles(*f)
        else:
            try:
                os.remove(f)
            except OSError:
                pass

class TestSdfParsing(unittest.TestCase):
    def test_Basic(self):
        # NOTE: Any file with "_bad_" in the name is a file that should not load successfully.
        # NOTE: This list is reverse sorted.  Add new tests at the top!
        # This will mean that your new test runs first and you can spot
        # failures much quicker.
        testFiles = '''
        226_version_1.1.usda
        225_multiline_with_SplineKnotParamList.usda
        224_spline_post_shaping_with_comment.usda
        223_bad_spline_post_shaping_spacing.usda
        222_dict_key_control_characters.usda
        221_bad_spline_type.usda
        220_splines.usda
        219_utf8_bad_type_name.usda
        218_utf8_bad_identifier.usda
        217_utf8_identifiers.usda
        216_bad_variant_in_relocates_path.usda
        215_bad_variant_in_specializes_path.usda
        214_bad_variant_in_inherits_path.usda
        213_bad_variant_in_payload_path.usda
        212_bad_variant_in_reference_path.usda
        211_bad_authored_opaque_attributes.usda
        210_opaque_attributes.usda
        209_bad_escaped_string4.usda
        208_bad_escaped_string3.usda
        207_bad_escaped_string2.usda
        206_bad_escaped_string1.usda
        205_bad_assetPaths.usda
        204_really_empty.usda
        203_newlines.usda
        202_displayGroups.usda
        201_format_specifiers_in_strings.usda
        200_bad_emptyFile.usda
        199_bad_colorSpace_metadata.usda
        198_colorSpace_metadata.usda
        197_bad_colorConfiguration_metadata.usda
        196_colorConfiguration_metadata.usda
        195_specializes.usda
        194_bad_customLayerData_metadata.usda
        193_customLayerData_metadata.usda
        192_listop_metadata.usda
        191_instanceable.usda
        190_property_assetInfo.usda
        189_prim_assetInfo.usda
        188_defaultRefTarget_metadata.usda
        187_displayName_metadata.usda
        186_bad_prefix_substitution_key.usda
        185_namespaced_properties.usda
        184_def_AnyType.usda
        183_unknown_type_and_metadata.usda
        183_time_samples.usda
        182_bad_variant_in_relationship.usda
        181_bad_variant_in_connection.usda
        180_asset_paths.usda
        179_bad_shaped_attr_dimensions1_oldtypes.usda
        179_bad_shaped_attr_dimensions1.usda
        178_invalid_typeName.usda
        177_bad_empty_lists.usda
        176_empty_lists.usda
        175_asset_path_with_colons.usda
        163_bad_variant_selection2.usda
        162_bad_variant_selection1.usda
        161_bad_variant_name2.usda
        160_bad_variant_name1.usda
        159_symmetryFunction_empty.usda
        155_bad_relationship_noLoadHint.usda
        154_relationship_noLoadHint.usda
        153_bad_payloads.usda
        152_payloads.usda
        150_bad_kind_metadata_1.usda
        149_kind_metadata.usda
        148_relocates_empty_map.usda
        147_bad_relocates_formatting_5.usda
        146_bad_relocates_formatting_4.usda
        145_bad_relocates_formatting_3.usda
        144_bad_relocates_formatting_2.usda
        143_bad_relocates_formatting_1.usda
        142_bad_relocates_paths_3.usda
        141_bad_relocates_paths_2.usda
        140_bad_relocates_paths_1.usda
        139_relocates_metadata.usda
        133_bad_reference.usda
        132_references.usda
        127_varyingRelationship.usda
        119_bad_permission_metadata_3.usda
        118_bad_permission_metadata_2.usda
        117_bad_permission_metadata.usda
        116_permission_metadata.usda
        115_symmetricPeer_metadata.usda
        114_bad_prefix_metadata.usda
        113_displayName_metadata.usda
        112_nested_dictionaries_oldtypes.usda
        112_nested_dictionaries.usda
        111_string_arrays.usda
        108_bad_inheritPath.usda
        104_uniformAttributes.usda
        103_bad_attributeVariability.usda
        100_bad_roleNameChange.usda
        99_bad_typeNameChange.usda
        98_bad_valueType.usda
        97_bad_valueType.usda
        96_bad_valueType_oldtypes.usda
        96_bad_valueType.usda
        95_bad_hiddenRel.usda
        94_bad_hiddenAttr.usda
        93_hidden.usda
        92_bad_variantSelectionType.usda
        91_bad_valueType.usda
        90_bad_dupePrim.usda
        89_bad_attribute_displayUnit.usda
        88_attribute_displayUnit.usda
        86_bad_tuple_dimensions5.usda
        85_bad_tuple_dimensions4_oldtypes.usda
        85_bad_tuple_dimensions4.usda
        84_bad_tuple_dimensions3_oldtypes.usda
        84_bad_tuple_dimensions3.usda
        83_bad_tuple_dimensions2_oldtypes.usda
        83_bad_tuple_dimensions2.usda
        82_bad_tuple_dimensions1_oldtypes.usda
        82_bad_tuple_dimensions1.usda
        81_namespace_reorder.usda
        80_bad_hidden.usda
        76_relationship_customData.usda
        75_attribute_customData.usda
        74_prim_customData.usda
        71_empty_shaped_attrs.usda
        70_bad_list.usda
        69_bad_list.usda
        66_bad_attrVariability.usda
        64_bad_boolPrimInstantiate.usda
        61_bad_primName.usda
        60_bad_groupListEditing.usda
        59_bad_connectListEditing.usda
        58_bad_relListEditing.usda
        57_bad_relListEditing.usda
        56_bad_value_oldtypes.usda
        56_bad_value.usda
        55_bad_value_oldtypes.usda
        55_bad_value.usda
        54_bad_value.usda
        53_bad_typeName.usda
        52_bad_attr.usda
        51_propPath.usda
        50_bad_primPath.usda
        49_bad_list.usda
        47_miscSceneInfo.usda
        46_weirdStringContent.usda
        45_rareValueTypes.usda
        42_bad_noNewlineBetweenComps.usda
        41_noEndingNewline.usda
        40_repeater.usda
        39_variants.usda
        38_attribute_connections.usda
        37_keyword_properties.usda
        36_tasks.usda
        33_bad_relationship_duplicate_target.usda
        32_relationship_syntax.usda
        31_attribute_values_oldtypes.usda
        31_attribute_values.usda
        30_bad_specifier.usda
        29_bad_newline9.usda
        28_bad_newline8.usda
        27_bad_newline7.usda
        26_bad_newline6.usda
        25_bad_newline5.usda
        24_bad_newline4.usda
        23_bad_newline3.usda
        22_bad_newline2.usda
        21_bad_newline1.usda
        20_optionalsemicolons.usda
        16_bad_list.usda
        15_bad_list.usda
        14_bad_value.usda
        13_bad_value.usda
        12_bad_value.usda
        11_debug.usda
        10_bad_value.usda
        09_bad_type.usda
        08_bad_file.usda
        06_largevalue.usda
        05_bad_file.usda
        04_general_oldtypes.usda
        04_general.usda
        03_bad_file.usda
        02_simple.usda
        01_empty.usda
        '''.split()
        # NOTE:  READ IF YOU ARE ADDING TEST FILES
        # This list is reverse sorted.  Add new tests at the top!
        # This will mean that your new test runs first and you can spot
        # failures much quicker.

        # Disabled tests for invalid metadata field names because now invalid metadata
        # fields are stored as opaque text and passed through loading and saving
        # instead of causing parse errors
        #
        # 19_bad_relationshipaccess.usda
        # 18_bad_primaccess.usda
        # 17_bad_attributeaccess.usda

        # Disabled tests - this has not failed properly ever, but a bug in this script masked the problem
        # 34_bad_relationship_duplicate_target_attr.usda

        # Create a temporary file for diffs and choose where to get test data.
        def CreateTempFile(name):
            import tempfile
            layerFileOut = tempfile.NamedTemporaryFile(
                suffix='_' + name + '_testSdfParsing1.usda', delete=False)
            # Close the temporary file.  We only wanted a temporary file name
            # and we'll open/close/remove this file once per test file.  On
            # Unix this isn't necessary because holding a file open doesn't
            # prevent unlinking it but on Windows we'll get access denied if
            # we don't close our handle.
            layerFileOut.close()
            return layerFileOut
        
        layerFileOut = CreateTempFile('Export')
        layerFileOut2 = CreateTempFile('ExportToString')
        layerFileOut3 = CreateTempFile('MetadataOnly')

        layerDir = os.path.join(os.getcwd(), 'testSdfParsing.testenv')
        baselineDir = os.path.join(layerDir, 'baseline')

        print("LAYERDIR: %s"%layerDir)

        # Register test plugin containing plugin metadata definitions.
        from pxr import Plug
        Plug.Registry().RegisterPlugins(layerDir)

        def GenerateBaselines(testFiles, baselineDir):
            for file in testFiles:
                if file.startswith('#') or file == '' or '_bad_' in file:
                    continue
                
                try:
                    layerFile = "%s/%s" % (layerDir, file)
                    exportFile = "%s/%s" % (baselineDir, file)

                    layer = Sdf.Layer.FindOrOpen(layerFile)
                    layer.Export(exportFile)

                except:
                    print('Unable to export layer %s to %s' % (layerFile, exportFile))

        # Helper code to generate baseline layers. Uncomment to export 'good' layers
        # in the test list to the specified directory.
        #
        # GenerateBaselines(testFiles, '/tmp/baseline')
        # sys.exit(1)

        for file in testFiles:
            if file.startswith('#'):
                continue

            # Remove stale files or files from last pass
            removeFiles(layerFileOut.name)

            layerFile = file
            if file != "":
                layerFile = "%s/%s"%(layerDir, file)

            if (file == "") or ('_bad_' in file):
                print('\nTest bad file "%s"' % layerFile)
                print('\tReading "%s"' % layerFile)
                try:
                    layer = Sdf.Layer.FindOrOpen( layerFile )
                except Tf.ErrorException:
                    # Parsing errors should always be Tf.ErrorExceptions
                    print('\tErrors encountered, as expected')
                    print('\tPassed')
                    continue
                except:
                    # Empty file fails with a different error, and that's ok
                    if file != "":
                        print('\tNon-TfError encountered')
                        print('\tFAILED')
                        raise RuntimeError("failure to load '%s' should cause Tf.ErrorException, not some other failure" % layerFile)
                    else:
                        print('\tErrors encountered, as expected')
                        print('\tPassed')
                        continue
                else:
                    raise RuntimeError("should not be able to load '%s'" % layerFile)

            print('\nTest %s' % layerFile)

            print('\tReading...')
            layer = Sdf.Layer.FindOrOpen(layerFile)
            self.assertTrue(layer is not None,
                            "failed to open @%s@" % layerFile)

            metadataOnlyLayer = Sdf.Layer.OpenAsAnonymous(
                layerFile, metadataOnly=True)
            self.assertTrue(metadataOnlyLayer is not None,
                            "failed to open @%s@ for metadata only" % layerFile)

            print('\tWriting...')
            try:
                # Use Sdf.Layer.Export to write out the layer contents directly.
                self.assertTrue(layer.Export( layerFileOut.name ))

                # Use Sdf.Layer.ExportToString and write out the returned layer
                # string to a file.
                with _open(layerFileOut2.name, 'w') as f:
                    f.write(layer.ExportToString())

                self.assertTrue(metadataOnlyLayer.Export(layerFileOut3.name))
                    
            except Exception as e:
                if '_badwrite_' in file:
                    # Write errors should always be Tf.ErrorExceptions
                    print('\tErrors encountered during write, as expected')
                    print('\tPassed')
                    continue
                else:
                    raise RuntimeError("failure to write '%s': %s" % (layerFile, e))

            # Compare the exported layer against baseline results. Note that we can't
            # simply compare against the original layer, because exporting may have
            # applied formatting and other changes that cause the files to be different,
            # even though the scene description is the same.
            print('\tComparing against expected results...')

            expectedFile = "%s/%s" % (baselineDir, file)

            def doDiff(testFile, expectedFile, metadataOnly=False):

                fd = _open(testFile, "r")
                layerData = fd.readlines()
                fd.close()
                fd = _open(expectedFile, "r")
                expectedLayerData = fd.readlines()
                fd.close()

                # If we're expecting metadata only, find the metadata section in
                # the baseline output by looking for the "(" and ")" delimiters
                # and use that as the expected layer data.
                if metadataOnly:
                    try:
                        mdStart = expectedLayerData.index("(\n", 1)
                        mdEnd = expectedLayerData.index(")\n", mdStart)
                        expectedLayerData = expectedLayerData[0:mdEnd+1]+["\n"]
                    except ValueError:
                        # If there's no layer metadata, we expect to just have
                        # the first line of the baseline with the #usda cookie.
                        expectedLayerData = [expectedLayerData[0], "\n"]

                    # The exported metadata is always version 1.0 because
                    # it never encounters any advanced features that would
                    # require a higher version. This will need to be changed
                    # if we ever default to a usda version > 1.0.
                    expectedLayerData[0] = "#usda 1.0\n"
                diff = list(difflib.unified_diff(
                    layerData, expectedLayerData,
                    testFile, expectedFile))
                if diff:
                    print("ERROR: '%s' and '%s' are different." % \
                          (testFile, expectedFile))
                    for line in diff:
                        print(line, end=' ')
                    sys.exit(1)

            doDiff(layerFileOut.name, expectedFile)
            doDiff(layerFileOut2.name, expectedFile)
            doDiff(layerFileOut3.name, expectedFile, metadataOnly=True)
            
            print('\tPassed')

        removeFiles(layerFileOut.name, layerFileOut2.name)

        self.assertEqual(None, Sdf.Layer.FindOrOpen(''))

        print('\nTest SUCCEEDED')

if __name__ == "__main__":
    unittest.main()
