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

# Functionality for expanding and collapsing legend components in usdview

LEGEND_BUTTON_SELECTEDSTYLE = ('background: rgb(189, 155, 84); '
                               'color: rgb(227, 227, 227);')

# Set the start/end points of an animation
def _SetAnimValues(anim, a1, a2):
    anim.setStartValue(a1)
    anim.setEndValue(a2)

# A function which takes a two-pane area and transforms it to
# open or close the bottom pane.
#
#    legendHeight       
#    |   separator height
#    |   |  browser height
#    |   |  |->     ___________                ___________
#    |   |  |      |           |              |           |
#    |   |  |      |           |              |           |
#    |   |  |->    |           |    <--->     |           |
#    |---|-------> +++++++++++++              |           |
#    |             |           |    <--->     |           |
#    |             |           |              |           |
#    |----------->  -----------                +++++++++++
def ToggleLegendWithBrowser(legend, button, anim):
    legend.ToggleMinimized()

    # We are dragging downward, so collapse the legend and expand the
    # attribute viewer panel to take up the remaining space.
    if legend.IsMinimized():
        button.setStyleSheet('')
        _SetAnimValues(anim, legend.GetHeight(), 0)
    # We are expanding, so do the opposite.
    else:
        button.setStyleSheet(LEGEND_BUTTON_SELECTEDSTYLE)
        _SetAnimValues(anim, legend.GetHeight(), legend.GetResetHeight())

    anim.start()
