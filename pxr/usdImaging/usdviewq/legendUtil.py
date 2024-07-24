#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
