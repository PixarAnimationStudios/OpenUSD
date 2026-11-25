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

def _get_session_stage(app_controller):
    stage = app_controller._dataModel.stage
    stage.SetEditTarget(stage.GetSessionLayer())
    return stage

def _update_energy_filter_targets(filter_paths, app_controller):
    stage = _get_session_stage(app_controller)
    render_settings = stage.GetPrimAtPath('/Render/RenderSettings')
    energy_filter_rel = render_settings.GetRelationship('ri:energyFilters')
    energy_filter_rel.SetTargets(filter_paths)

def _update_energy_filter_attr(filter_path, attr_name, attr_value,
                               app_controller):
    stage = _get_session_stage(app_controller)
    energy_filter = stage.GetPrimAtPath(filter_path)
    energy_filter.GetAttribute(attr_name).Set(attr_value)

def _set_energy_filter_orders(orders, app_controller):
    """Sets ri:energyFilter:order on each filter path in `orders`."""
    for filter_path, order in orders.items():
        _update_energy_filter_attr(
            filter_path, 'ri:energyFilter:order', order, app_controller)


def _run_test(app_controller):
    _modify_settings(app_controller)

    filter1 = '/Render/EnergyFilter1'   # desaturate, order 0 in test.usda
    filter2 = '/Render/EnergyFilter2'   # red tint,   order 1 in test.usda

    # 1. Initial state: EnergyFilter1 only (desaturate).
    app_controller._takeShot("energyFilter1.png", waitForConvergence=True)

    # 2. Retarget to EnergyFilter2 only (red tint).
    _update_energy_filter_targets([filter2], app_controller)
    app_controller._takeShot("energyFilter2.png", waitForConvergence=True)

    # 3. Change EnergyFilter2's mult to a blue tint.
    _update_energy_filter_attr(
        filter2, "inputs:ri:mult", Gf.Vec3f(0.2, 0.2, 3), app_controller)
    app_controller._takeShot("energyFilter2_modified.png",
                             waitForConvergence=True)

    # 4. Reset the tint and target both filters. Orders filter1=0,
    #    filter2=1: desaturate, then tint -> a monochrome red image.
    _update_energy_filter_attr(
        filter2, "inputs:ri:mult", Gf.Vec3f(3, 0.2, 0.2), app_controller)
    _update_energy_filter_targets([filter1, filter2], app_controller)
    app_controller._takeShot("energyFiltersMulti1.png",
                             waitForConvergence=True)

    # 5. Swap the evaluation order by editing the order attributes (the
    #    relationship is unchanged). Orders filter1=1, filter2=0: tint,
    #    then desaturate -> a gray image.
    _set_energy_filter_orders({filter1: 1, filter2: 0}, app_controller)
    app_controller._takeShot("energyFiltersMulti2.png",
                             waitForConvergence=True)


# Required entry point for the testusdview framework.
testUsdviewInputFunction = _run_test
