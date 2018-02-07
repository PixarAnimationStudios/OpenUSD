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
#

import unittest
import os, json

from pxr.Usdviewq.settings2 import StateSource, Settings


TEST_SETTINGS_VERSION = "1"
STATE_FILE = "state.json"


class BasicModel(StateSource):
    """A StateSource which holds a few basic pieces of data."""

    def __init__(self, parent, name):
        StateSource.__init__(self, parent, name)
        self.testInt = self.stateProperty("testInt", default=0)
        self.testFloat = self.stateProperty("testFloat", default=0.0)
        self.testStr = self.stateProperty("testStr", default="")

    def onSaveState(self, state):
        state["testInt"] = self.testInt
        state["testFloat"] = self.testFloat
        state["testStr"] = self.testStr


class ValidatedBasicModel(StateSource):
    """A StateSource which holds a few basic pieces of data and uses custom
    validation.
    """

    def __init__(self, parent, name):
        StateSource.__init__(self, parent, name)

        # testInt required to be an int between 0 and 10 (inclusive)
        self.testInt = self.stateProperty("testInt", default=0,
                validator=lambda value: value in range(11))

        # testFloat required to be an int between 0.0 and 10.0 (inclusive)
        self.testFloat = self.stateProperty("testFloat", default=0.0,
                validator=lambda value: 0.0 <= value <= 10.0)

        # testStr required to not begin with an underscore.
        self.testStr = self.stateProperty("testStr", default="",
                validator=lambda value: not value.startswith("_"))

    def onSaveState(self, state):
        state["testInt"] = self.testInt
        state["testFloat"] = self.testFloat
        state["testStr"] = self.testStr


class ParentModel(StateSource):
    """A StateSource which holds both data and nested data sources."""

    def __init__(self, parent, name):
        StateSource.__init__(self, parent, name)
        self.parentTestInt = self.stateProperty("parentTestInt", default=0)
        self.parentTestFloat = self.stateProperty("parentTestFloat", default=0.0)
        self.parentTestStr = self.stateProperty("parentTestStr", default="")
        self.child1 = BasicModel(self, "child1")
        self.child2 = BasicModel(self, "child2")

    def onSaveState(self, state):
        state["parentTestInt"] = self.parentTestInt
        state["parentTestFloat"] = self.parentTestFloat
        state["parentTestStr"] = self.parentTestStr


class TestSettings2(unittest.TestCase):

    def _writeStateFile(self, state):
        """Write a state to the state file."""
        with open(STATE_FILE, "w") as fp:
            json.dump(state, fp)


    def _compareStateFile(self, state):
        """Compare a state against a state file's state."""
        with open(STATE_FILE, "r") as fp:
            fileState = json.load(fp)
        self.assertDictEqual(state, fileState)

    def _removeStateFile(self):
        """Delete the state file."""
        try:
            os.remove(STATE_FILE)
        except OSError:
            pass # Ignore if file already doesn't exist.

    def test_Basic(self):
        """Test the basic operation of Settings by loading and updating a valid
        state file.
        """

        self._writeStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 1,
                "testFloat": 2.0,
                "testStr": "3"
            },
            "model2": {
                "testInt": 4,
                "testFloat": 5.0,
                "testStr": "6"
            }
        }})

        # Create a new settings object with two data sources.
        settings = Settings(TEST_SETTINGS_VERSION, STATE_FILE)
        model1 = BasicModel(settings, "model1")
        model2 = BasicModel(settings, "model2")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.testInt, 1)
        self.assertEqual(model1.testFloat, 2.0)
        self.assertEqual(model1.testStr, "3")
        self.assertEqual(model2.testInt, 4)
        self.assertEqual(model2.testFloat, 5.0)
        self.assertEqual(model2.testStr, "6")

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        self._compareStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 6,
                "testFloat": 5.0,
                "testStr": "4"
            },
            "model2": {
                "testInt": 3,
                "testFloat": 2.0,
                "testStr": "1"
            }
        }})
        self._removeStateFile()

    def test_Incomplete(self):
        """Test that an incomplete state file is automatically filled with
        default values.
        """

        self._writeStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 1,
                "testFloat": 2.0
                # missing testStr
            }
            # missing model2
        }})

        # Create a new settings object with two data sources.
        settings = Settings(TEST_SETTINGS_VERSION, STATE_FILE)
        model1 = BasicModel(settings, "model1")
        model2 = BasicModel(settings, "model2")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.testInt, 1)
        self.assertEqual(model1.testFloat, 2.0)
        self.assertEqual(model1.testStr, "") # default
        self.assertEqual(model2.testInt, 0) # default
        self.assertEqual(model2.testFloat, 0.0) # default
        self.assertEqual(model2.testStr, "") # default

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        self._compareStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 6,
                "testFloat": 5.0,
                "testStr": "4"
            },
            "model2": {
                "testInt": 3,
                "testFloat": 2.0,
                "testStr": "1"
            }
        }})
        self._removeStateFile()

    def test_Extra(self):
        """Test that extra data in a state file is preserved after a save."""

        self._writeStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 1,
                "testFloat": 2.0,
                "testStr": "3",
                "testInt2": 4 # extra
            },
            "model2": {
                "testInt": 4,
                "testFloat": 5.0,
                "testStr": "6"
            },
            "model3": { # extra
                "testInt": 7,
                "testFloat": 8.0,
                "testStr": "9"
            }
        }})

        # Create a new settings object with two data sources.
        settings = Settings(TEST_SETTINGS_VERSION, STATE_FILE)
        model1 = BasicModel(settings, "model1")
        model2 = BasicModel(settings, "model2")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.testInt, 1)
        self.assertEqual(model1.testFloat, 2.0)
        self.assertEqual(model1.testStr, "3")
        self.assertEqual(model2.testInt, 4)
        self.assertEqual(model2.testFloat, 5.0)
        self.assertEqual(model2.testStr, "6")

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        self._compareStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 6,
                "testFloat": 5.0,
                "testStr": "4",
                "testInt2": 4 # extra
            },
            "model2": {
                "testInt": 3,
                "testFloat": 2.0,
                "testStr": "1"
            },
            "model3": { # extra
                "testInt": 7,
                "testFloat": 8.0,
                "testStr": "9"
            }
        }})
        self._removeStateFile()

    def test_InvalidState(self):
        """Test that invalid state properties and children sources are reset to
        default values.
        """

        self._writeStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 1.0,
                "testFloat": "2.0",
                "testStr": 3
            },
            "model2": 4
        }})

        # Create a new settings object with two data sources.
        settings = Settings(TEST_SETTINGS_VERSION, STATE_FILE)
        model1 = BasicModel(settings, "model1")
        model2 = BasicModel(settings, "model2")

        # Verify that all defaults were loaded.
        self.assertEqual(model1.testInt, 0)
        self.assertEqual(model1.testFloat, 0.0)
        self.assertEqual(model1.testStr, "")
        self.assertEqual(model2.testInt, 0)
        self.assertEqual(model2.testFloat, 0.0)
        self.assertEqual(model2.testStr, "")

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        self._compareStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 6,
                "testFloat": 5.0,
                "testStr": "4"
            },
            "model2": {
                "testInt": 3,
                "testFloat": 2.0,
                "testStr": "1"
            }
        }})
        self._removeStateFile()

    def test_CustomValidation(self):
        """Test that if custom validators are used, they force defaults to be
        loaded on failure.
        """

        self._writeStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 1,
                "testFloat": 2.0,
                "testStr": "3"
            },
            "model2": {
                "testInt": 11, # out of range
                "testFloat": -1.0, # out of range
                "testStr": "_6" # starts with underscore
            }
        }})

        # Create a new settings object with two data sources.
        settings = Settings(TEST_SETTINGS_VERSION, STATE_FILE)
        model1 = ValidatedBasicModel(settings, "model1")
        model2 = ValidatedBasicModel(settings, "model2")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.testInt, 1)
        self.assertEqual(model1.testFloat, 2.0)
        self.assertEqual(model1.testStr, "3")
        self.assertEqual(model2.testInt, 0) # default
        self.assertEqual(model2.testFloat, 0.0) # default
        self.assertEqual(model2.testStr, "") # default

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        self._compareStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 6,
                "testFloat": 5.0,
                "testStr": "4"
            },
            "model2": {
                "testInt": 3,
                "testFloat": 2.0,
                "testStr": "1"
            }
        }})
        self._removeStateFile()

    def test_InvalidJSON(self):
        """Test that an invalid JSON file is not modified by Usdview. If an
        invalid JSON file is read, the Settings object should become ephemeral
        so it does not wipe a user's settings over a small formatting error.
        """

        invalidJSON = "This is not valid JSON."

        with open(STATE_FILE, "w") as fp:
            fp.write(invalidJSON)

        # Create a new settings object with two data sources.
        settings = Settings(TEST_SETTINGS_VERSION, STATE_FILE)
        model1 = BasicModel(settings, "model1")
        model2 = BasicModel(settings, "model2")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.testInt, 0)
        self.assertEqual(model1.testFloat, 0.0)
        self.assertEqual(model1.testStr, "")
        self.assertEqual(model2.testInt, 0)
        self.assertEqual(model2.testFloat, 0.0)
        self.assertEqual(model2.testStr, "")

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        # Invalid JSON state should not be overwritten by save.
        with open(STATE_FILE, "r") as fp:
            stateContents = fp.read()
        self.assertEqual(stateContents, invalidJSON)

        self._removeStateFile()

    def test_Missing(self):
        """Test that a missing state file causes fresh state file to be
        generated.
        """

        # Make sure state file does not exist.
        self.assertFalse(os.path.isfile(STATE_FILE))

        # Create a new settings object with two data sources.
        settings = Settings(TEST_SETTINGS_VERSION, STATE_FILE)
        model1 = BasicModel(settings, "model1")
        model2 = BasicModel(settings, "model2")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.testInt, 0)
        self.assertEqual(model1.testFloat, 0.0)
        self.assertEqual(model1.testStr, "")
        self.assertEqual(model2.testInt, 0)
        self.assertEqual(model2.testFloat, 0.0)
        self.assertEqual(model2.testStr, "")

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        self._compareStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 6,
                "testFloat": 5.0,
                "testStr": "4"
            },
            "model2": {
                "testInt": 3,
                "testFloat": 2.0,
                "testStr": "1"
            }
        }})
        self._removeStateFile()

    def test_Ephemeral(self):
        """Test that when no state filename is given that defaults are loaded
        and saves do nothing.
        """

        self._writeStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 1,
                "testFloat": 2.0,
                "testStr": "3"
            },
            "model2": {
                "testInt": 4,
                "testFloat": 5.0,
                "testStr": "6"
            }
        }})

        # Create a new settings object with two data sources.
        settings = Settings(TEST_SETTINGS_VERSION) # No file given.
        model1 = BasicModel(settings, "model1")
        model2 = BasicModel(settings, "model2")

        # Verify that the state was NOT loaded from file.
        self.assertEqual(model1.testInt, 0)
        self.assertEqual(model1.testFloat, 0.0)
        self.assertEqual(model1.testStr, "")
        self.assertEqual(model2.testInt, 0)
        self.assertEqual(model2.testFloat, 0.0)
        self.assertEqual(model2.testStr, "")

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        # Verify that state was NOT saved to file.
        self._compareStateFile({TEST_SETTINGS_VERSION: {
            "model1": {
                "testInt": 1,
                "testFloat": 2.0,
                "testStr": "3"
            },
            "model2": {
                "testInt": 4,
                "testFloat": 5.0,
                "testStr": "6"
            }
        }})
        self._removeStateFile()

    def test_Nested(self):
        """Test nested StateSource objects."""

        self._writeStateFile({TEST_SETTINGS_VERSION: {
            "parent": {
                "parentTestInt": 1,
                "parentTestFloat": 2.0,
                "parentTestStr": "3",
                "child1": {
                    "testInt": 4,
                    "testFloat": 5.0,
                    "testStr": "6"
                },
                "child2": {
                    "testInt": 7,
                    "testFloat": 8.0,
                    "testStr": "9"
                }
            }
        }})

        # Create a new settings object with a single nested data source.
        settings = Settings(TEST_SETTINGS_VERSION, STATE_FILE)
        model1 = ParentModel(settings, "parent")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.parentTestInt, 1)
        self.assertEqual(model1.parentTestFloat, 2.0)
        self.assertEqual(model1.parentTestStr, "3")
        self.assertEqual(model1.child1.testInt, 4)
        self.assertEqual(model1.child1.testFloat, 5.0)
        self.assertEqual(model1.child1.testStr, "6")
        self.assertEqual(model1.child2.testInt, 7)
        self.assertEqual(model1.child2.testFloat, 8.0)
        self.assertEqual(model1.child2.testStr, "9")

        # Update the state.
        model1.parentTestInt = 9
        model1.parentTestFloat = 8.0
        model1.parentTestStr = "7"
        model1.child1.testInt = 6
        model1.child1.testFloat = 5.0
        model1.child1.testStr = "4"
        model1.child2.testInt = 3
        model1.child2.testFloat = 2.0
        model1.child2.testStr = "1"

        settings.save()

        self._compareStateFile({TEST_SETTINGS_VERSION: {
            "parent": {
                "parentTestInt": 9,
                "parentTestFloat": 8.0,
                "parentTestStr": "7",
                "child1": {
                    "testInt": 6,
                    "testFloat": 5.0,
                    "testStr": "4"
                },
                "child2": {
                    "testInt": 3,
                    "testFloat": 2.0,
                    "testStr": "1"
                }
            }
        }})
        self._removeStateFile()

    def test_Versions(self):
        """Test that setting version accesses the correct entry in the state
        file and does not touch the other states.
        """

        version1 = "1"
        version2 = "2"

        self._writeStateFile({
            version1: {
                "model1": {
                    "testInt": 1,
                    "testFloat": 2.0,
                    "testStr": "3"
                },
                "model2": {
                    "testInt": 4,
                    "testFloat": 5.0,
                    "testStr": "6"
                }
            },
            version2: {
                "model1": {
                    "testInt": 7,
                    "testFloat": 8.0,
                    "testStr": "9"
                },
                "model2": {
                    "testInt": 10,
                    "testFloat": 11.0,
                    "testStr": "12"
                }
            }
        })

        # Create a new settings object with two data sources.
        settingsV1 = Settings(version1, STATE_FILE)
        model1 = BasicModel(settingsV1, "model1")
        model2 = BasicModel(settingsV1, "model2")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.testInt, 1)
        self.assertEqual(model1.testFloat, 2.0)
        self.assertEqual(model1.testStr, "3")
        self.assertEqual(model2.testInt, 4)
        self.assertEqual(model2.testFloat, 5.0)
        self.assertEqual(model2.testStr, "6")

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settingsV1.save()

        self._compareStateFile({
            version1: {
                "model1": {
                    "testInt": 6,
                    "testFloat": 5.0,
                    "testStr": "4"
                },
                "model2": {
                    "testInt": 3,
                    "testFloat": 2.0,
                    "testStr": "1"
                }
            },
            version2: {
                "model1": {
                    "testInt": 7,
                    "testFloat": 8.0,
                    "testStr": "9"
                },
                "model2": {
                    "testInt": 10,
                    "testFloat": 11.0,
                    "testStr": "12"
                }
            }
        })

        # Create a new settings object with two data sources.
        settingsV2 = Settings(version2, STATE_FILE)
        model1 = BasicModel(settingsV2, "model1")
        model2 = BasicModel(settingsV2, "model2")

        # Verify that the state was loaded properly.
        self.assertEqual(model1.testInt, 7)
        self.assertEqual(model1.testFloat, 8.0)
        self.assertEqual(model1.testStr, "9")
        self.assertEqual(model2.testInt, 10)
        self.assertEqual(model2.testFloat, 11.0)
        self.assertEqual(model2.testStr, "12")

        # Update the state.
        model1.testInt = 12
        model1.testFloat = 11.0
        model1.testStr = "10"
        model2.testInt = 9
        model2.testFloat = 8
        model2.testStr = "7"

        settingsV2.save()

        self._compareStateFile({
            version1: {
                "model1": {
                    "testInt": 6,
                    "testFloat": 5.0,
                    "testStr": "4"
                },
                "model2": {
                    "testInt": 3,
                    "testFloat": 2.0,
                    "testStr": "1"
                }
            },
            version2: {
                "model1": {
                    "testInt": 12,
                    "testFloat": 11.0,
                    "testStr": "10"
                },
                "model2": {
                    "testInt": 9,
                    "testFloat": 8.0,
                    "testStr": "7"
                }
            }
        })
        self._removeStateFile()

    def test_VersionDoesntExist(self):
        """Test that if the target version doesn't exist in the state file, it
        is created and loaded with the defaults.
        """

        version1 = "1"
        version2 = "2"
        targetVersion = "3"

        self._writeStateFile({
            version1: {
                "model1": {
                    "testInt": 1,
                    "testFloat": 2.0,
                    "testStr": "3"
                },
                "model2": {
                    "testInt": 4,
                    "testFloat": 5.0,
                    "testStr": "6"
                }
            },
            version2: {
                "model1": {
                    "testInt": 7,
                    "testFloat": 8.0,
                    "testStr": "9"
                },
                "model2": {
                    "testInt": 10,
                    "testFloat": 11.0,
                    "testStr": "12"
                }
            }
        })

        # Create a new settings object with two data sources.
        settings = Settings(targetVersion, STATE_FILE)
        model1 = BasicModel(settings, "model1")
        model2 = BasicModel(settings, "model2")

        # Verify that all defaults were loaded.
        self.assertEqual(model1.testInt, 0)
        self.assertEqual(model1.testFloat, 0.0)
        self.assertEqual(model1.testStr, "")
        self.assertEqual(model2.testInt, 0)
        self.assertEqual(model2.testFloat, 0.0)
        self.assertEqual(model2.testStr, "")

        # Update the state.
        model1.testInt = 6
        model1.testFloat = 5.0
        model1.testStr = "4"
        model2.testInt = 3
        model2.testFloat = 2.0
        model2.testStr = "1"

        settings.save()

        self._compareStateFile({
            version1: {
                "model1": {
                    "testInt": 1,
                    "testFloat": 2.0,
                    "testStr": "3"
                },
                "model2": {
                    "testInt": 4,
                    "testFloat": 5.0,
                    "testStr": "6"
                }
            },
            version2: {
                "model1": {
                    "testInt": 7,
                    "testFloat": 8.0,
                    "testStr": "9"
                },
                "model2": {
                    "testInt": 10,
                    "testFloat": 11.0,
                    "testStr": "12"
                }
            },
            targetVersion: {
                "model1": {
                    "testInt": 6,
                    "testFloat": 5.0,
                    "testStr": "4"
                },
                "model2": {
                    "testInt": 3,
                    "testFloat": 2.0,
                    "testStr": "1"
                }
            }
        })

        self._removeStateFile()

    def test_NoParent(self):
        """Test a StateSource with a parent set to None rather than another
        StateSource.
        """

        # Create a new state source with no parent.
        model1 = BasicModel(None, "model1")

        # Verify that the default state was loaded.
        self.assertEqual(model1.testInt, 0)
        self.assertEqual(model1.testFloat, 0.0)
        self.assertEqual(model1.testStr, "")


if __name__ == "__main__":
    unittest.main(verbosity=2)
