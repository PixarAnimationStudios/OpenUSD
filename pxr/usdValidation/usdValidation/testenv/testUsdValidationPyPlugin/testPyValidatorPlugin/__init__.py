#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

"""Python validator plugin loaded via the Plug system.

When the Plug registry loads this module (triggered by
GetOrLoadValidatorByName for a validator declared in this plugin's
plugInfo.json), the top-level registration code below runs and
registers the Python task functions with the ValidationRegistry.

This is the Python equivalent of TF_REGISTRY_FUNCTION(UsdValidationRegistry)
in a C++ plugin.
"""

from pxr import Sdf, UsdValidation

_PLUGIN_NAME = "testPyValidatorPlugin"


def _stage_task(stage, timeRange):
    return [
        UsdValidation.ValidationError(
            "PyPluginStageError",
            UsdValidation.ValidationErrorType.Error,
            [UsdValidation.ValidationErrorSite(
                stage, Sdf.Path.absoluteRootPath)],
            "Python plugin stage validator ran",
        )
    ]


def _layer_task(layer):
    return [
        UsdValidation.ValidationError(
            "PyPluginLayerError",
            UsdValidation.ValidationErrorType.Warn,
            [UsdValidation.ValidationErrorSite(
                layer, Sdf.Path.absoluteRootPath)],
            "Python plugin layer validator ran",
        )
    ]


# --- Registration at import time (equivalent to TF_REGISTRY_FUNCTION) ---
_registry = UsdValidation.ValidationRegistry()
_registry.RegisterPluginStageValidator(
    _PLUGIN_NAME + ":PyStageValidator", _stage_task)
_registry.RegisterPluginLayerValidator(
    _PLUGIN_NAME + ":PyLayerValidator", _layer_task)
