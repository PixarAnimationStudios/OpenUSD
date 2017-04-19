#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

# This tests a set of sample sdf files that are either expected to load
# succesfully, or to emit warnings.  Files with _bad_ in the name are
# expected to emit warnings, but in no case should they cause a crash.
#
# For files that we expect to read succesfully, we take the further step
# of writing them out, reading in what we wrote, and writing it out again,
# and then comparing the two versions written to make sure they are the
# same.  This is to detect any accumulative error, such as quoting or
# escaping errors.

import sys, os, difflib, unittest
from pxr import Tf, Sdf

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
        # NOTE: Any file with "_bad_" in the name is a file that should not load succesfully.
        # NOTE: This list is reverse sorted.  Add new tests at the top!
        # This will mean that your new test runs first and you can spot
        # failures much quicker.
        testFiles = '''
        195_specializes.sdf
        194_bad_customLayerData_metadata.sdf
        193_customLayerData_metadata.sdf
        192_listop_metadata.sdf
        191_instanceable.sdf
        190_property_assetInfo.sdf
        189_prim_assetInfo.sdf
        188_defaultRefTarget_metadata.sdf
        187_displayName_metadata.sdf
        186_bad_prefix_substitution_key.sdf
        185_namespaced_properties.sdf
        184_def_AnyType.sdf
        183_unknown_type_and_metadata.sdf
        183_time_samples.sdf
        182_bad_variant_in_relationship.sdf
        181_bad_variant_in_connection.sdf
        180_asset_paths.sdf
        179_bad_shaped_attr_dimensions1.sdf
        178_invalid_typeName.sdf
        177_bad_empty_lists.sdf
        176_empty_lists.sdf
        175_asset_path_with_colons.sdf
        164_attr_mappers_and_markers.sdf
        163_bad_variant_selection2.sdf
        162_bad_variant_selection1.sdf
        161_bad_variant_name2.sdf
        160_bad_variant_name1.sdf
        159_symmetryFunction_empty.sdf
        155_bad_relationship_noLoadHint.sdf
        154_relationship_noLoadHint.sdf
        153_bad_payloads.sdf
        152_payloads.sdf
        150_bad_kind_metadata_1.sdf
        149_kind_metadata.sdf
        148_relocates_empty_map.sdf
        147_bad_relocates_formatting_5.sdf
        146_bad_relocates_formatting_4.sdf
        145_bad_relocates_formatting_3.sdf
        144_bad_relocates_formatting_2.sdf
        143_bad_relocates_formatting_1.sdf
        142_bad_relocates_paths_3.sdf
        141_bad_relocates_paths_2.sdf
        140_bad_relocates_paths_1.sdf
        139_relocates_metadata.sdf
        133_bad_reference.sdf
        132_references.sdf
        127_varyingRelationship.sdf
        119_bad_permission_metadata_3.sdf
        118_bad_permission_metadata_2.sdf
        117_bad_permission_metadata.sdf
        116_permission_metadata.sdf
        115_symmetricPeer_metadata.sdf
        114_bad_prefix_metadata.sdf
        113_displayName_metadata.sdf
        112_nested_dictionaries.sdf
        111_string_arrays.sdf
        108_bad_inheritPath.sdf
        105_mapperMetadata.sdf
        104_uniformAttributes.sdf
        103_bad_attributeVariability.sdf
        101_relationalAttrOverrides.sdf
        100_bad_roleNameChange.sdf
        99_bad_typeNameChange.sdf
        98_bad_valueType.sdf
        97_bad_valueType.sdf
        96_bad_valueType.sdf
        95_bad_hiddenRel.sdf
        94_bad_hiddenAttr.sdf
        93_hidden.sdf
        92_bad_variantSelectionType.sdf
        91_bad_valueType.sdf
        90_bad_dupePrim.sdf
        89_bad_attribute_displayUnit.sdf
        88_attribute_displayUnit.sdf
        86_bad_tuple_dimensions5.sdf
        85_bad_tuple_dimensions4.sdf
        84_bad_tuple_dimensions3.sdf
        83_bad_tuple_dimensions2.sdf
        82_bad_tuple_dimensions1.sdf
        81_namespace_reorder.sdf
        80_bad_hidden.sdf
        77_connections_at_markers.sdf
        76_relationship_customData.sdf
        75_attribute_customData.sdf
        74_prim_customData.sdf
        71_empty_shaped_attrs.sdf
        70_bad_list.sdf
        69_bad_list.sdf
        66_bad_attrVariability.sdf
        64_bad_boolPrimInstantiate.sdf
        61_bad_primName.sdf
        60_bad_groupListEditing.sdf
        59_bad_connectListEditing.sdf
        58_bad_relListEditing.sdf
        57_bad_relListEditing.sdf
        56_bad_value.sdf
        55_bad_value.sdf
        54_bad_value.sdf
        53_bad_typeName.sdf
        52_bad_attr.sdf
        51_propPath.sdf
        50_bad_primPath.sdf
        49_bad_list.sdf
        47_miscSceneInfo.sdf
        46_weirdStringContent.sdf
        45_rareValueTypes.sdf
        42_bad_noNewlineBetweenComps.sdf
        41_noEndingNewline.sdf
        40_repeater.sdf
        39_variants.sdf
        38_attribute_connections.sdf
        37_keyword_properties.sdf
        36_tasks.sdf
        33_bad_relationship_duplicate_target.sdf
        32_relationship_syntax.sdf
        31_attribute_values.sdf
        30_bad_specifier.sdf
        29_bad_newline9.sdf
        28_bad_newline8.sdf
        27_bad_newline7.sdf
        26_bad_newline6.sdf
        25_bad_newline5.sdf
        24_bad_newline4.sdf
        23_bad_newline3.sdf
        22_bad_newline2.sdf
        21_bad_newline1.sdf
        20_optionalsemicolons.sdf
        16_bad_list.sdf
        15_bad_list.sdf
        14_bad_value.sdf
        13_bad_value.sdf
        12_bad_value.sdf
        11_debug.sdf
        10_bad_value.sdf
        09_bad_type.sdf
        08_bad_file.sdf
        06_largevalue.sdf
        05_bad_file.sdf
        04_general.sdf
        03_bad_file.sdf
        02_simple.sdf
        01_empty.sdf
        '''.split()
        # NOTE:  READ IF YOU ARE ADDING TEST FILES
        # This list is reverse sorted.  Add new tests at the top!
        # This will mean that your new test runs first and you can spot
        # failures much quicker.

        # Disabled tests for invalid metadata field names because now invalid metadata
        # fields are stored as opaque text and passed through loading and saving
        # instead of causing parse errors
        #
        # 19_bad_relationshipaccess.sdf
        # 18_bad_primaccess.sdf
        # 17_bad_attributeaccess.sdf

        # Disabled tests - this has not failed properly ever, but a bug in this script masked the problem
        # 34_bad_relationship_duplicate_target_attr.sdf

        # Register test plugin containing plugin metadata definitions.
        import tempfile

        if os.environ.get('SDF_WRITE_OLD_TYPENAMES') == '1':
            layerFileOut = tempfile.NamedTemporaryFile(suffix='testSdfParsingOld1.sdf')
            layerDir = os.path.join(os.getcwd(), 'testSdfParsingOld.testenv')
            baselineDir = os.path.join(layerDir, 'baseline')
        elif os.environ.get('SDF_CONVERT_TO_NEW_TYPENAMES') == "1":
            layerFileOut = tempfile.NamedTemporaryFile(suffix='testSdfParsingNew1.sdf')
            layerDir = os.path.join(os.getcwd(), 'testSdfParsingNew.testenv')
            baselineDir = os.path.join(layerDir, 'baseline_newtypes')
        else:
            layerFileOut = tempfile.NamedTemporaryFile(suffix='testSdfParsing1.sdf')
            layerDir = os.path.join(os.getcwd(), 'testSdfParsing.testenv')
            baselineDir = os.path.join(layerDir, 'baseline')

        print "LAYERDIR: %s"%layerDir

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
                    print 'Unable to export layer %s to %s' % (layerFile, exportFile)

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
                print '\nTest bad file "%s"' % layerFile
                print '\tReading "%s"' % layerFile
                try:
                    layer = Sdf.Layer.FindOrOpen( layerFile )
                except Tf.ErrorException:
                    # Parsing errors should always be Tf.ErrorExceptions
                    print '\tErrors encountered, as expected'
                    print '\tPassed'
                    continue
                except:
                    # Empty file fails with a different error, and that's ok
                    if file != "":
                        print '\tNon-TfError encountered'
                        print '\tFAILED'
                        raise RuntimeError, "failure to load '%s' should cause Tf.ErrorException, not some other failure" % layerFile
                    else:
                        print '\tErrors encountered, as expected'
                        print '\tPassed'
                        continue
                else:
                    raise RuntimeError, "should not be able to load '%s'" % layerFile

            print '\nTest %s' % layerFile

            print '\tReading...'
            layer = Sdf.Layer.FindOrOpen( layerFile )
            self.assertTrue(layer is not None)
            print '\tWriting...'
            try:
                self.assertTrue(layer.Export( layerFileOut.name ))
            except:
                if '_badwrite_' in file:
                    # Write errors should always be Tf.ErrorExceptions
                    print '\tErrors encountered during write, as expected'
                    print '\tPassed'
                    continue
                else:
                    raise RuntimeError, "failure to write '%s'" % layerFile

            # Compare the exported layer against baseline results. Note that we can't
            # simply compare against the original layer, because exporting may have
            # applied formatting and other changes that cause the files to be different,
            # even though the scene description is the same.
            print '\tComparing against expected results...'

            expectedFile = "%s/%s" % (baselineDir, file)

            fd = open(layerFileOut.name, "r")
            layerData = fd.readlines()
            fd.close()
            fd = open(expectedFile, "r")
            expectedLayerData = fd.readlines()
            fd.close()

            diff = list(difflib.unified_diff(
                layerData, expectedLayerData,
                layerFileOut.name, expectedFile))
            if diff:
                print "ERROR: '%s' and '%s' are different." % \
                    (layerFileOut.name, expectedFile)
                for line in diff:
                    print line,
                sys.exit(1)

            print '\tPassed'

        removeFiles(layerFileOut.name)

        self.assertEqual(None, Sdf.Layer.FindOrOpen(''))

        print '\nTest SUCCEEDED'

if __name__ == "__main__":
    unittest.main()
