#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Gf

def _modify_settings(app_controller):
    app_controller._dataModel.viewSettings.showBBoxes = False
    app_controller._dataModel.viewSettings.showHUD = False
    app_controller._dataModel.viewSettings.autoComputeClippingPlanes = True

def _update_energy_filter_targets(filter_paths, app_controller):
    stage = app_controller._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    render_settings = stage.GetPrimAtPath('/Render/RenderSettings')
    energy_filter_rel = render_settings.GetRelationship('ri:energyFilters')
    energy_filter_rel.SetTargets(filter_paths)

def _update_energy_filter_param(filter_path, attr_name, attr_value, app_controller):
    stage = app_controller._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    energy_filter = stage.GetPrimAtPath(filter_path)
    energy_filter_param = energy_filter.GetAttribute(attr_name)
    energy_filter_param.Set(attr_value)


def _run_test(app_controller):
    _modify_settings(app_controller)

    filter1 = '/Render/EnergyFilter1'
    filter2 = '/Render/EnergyFilter2'

    # Initial state: EnergyFilter1 active (desaturate).
    app_controller._takeShot("energyFilter1.png", waitForConvergence=True)

    # Switch to EnergyFilter2 (red tint).
    _update_energy_filter_targets([filter2], app_controller)
    app_controller._takeShot("energyFilter2.png", waitForConvergence=True)

    # Modify EnergyFilter2's mult to a blue tint.
    _update_energy_filter_param(
        filter2, "inputs:ri:mult", Gf.Vec3f(0.2, 0.2, 3), app_controller)
    app_controller._takeShot("energyFilter2_modified.png", waitForConvergence=True)

    # Reset and connect both filters.
    _update_energy_filter_param(
        filter2, "inputs:ri:mult", Gf.Vec3f(3, 0.2, 0.2), app_controller)
    _update_energy_filter_targets([filter1, filter2], app_controller)
    app_controller._takeShot("energyFiltersMulti1.png", waitForConvergence=True)

    # Reverse filter order.
    _update_energy_filter_targets([filter2, filter1], app_controller)
    app_controller._takeShot("energyFiltersMulti2.png", waitForConvergence=True)


# Required entry point for the testusdview framework.
testUsdviewInputFunction = _run_test
