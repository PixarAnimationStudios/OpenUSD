#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
#
# A base class for all context menu items.
# This provides a simple behavior to ensure that a chosen
# context menu item is valid. This helps us avoid a situation
# in which a user right-clicks in an area with no item but still
# receives a context menu.
#
class UsdviewContextMenuItem():
    def isValid(self):
        ''' Menu items which have an invalid internal item are considered invalid.
            Header menus don't contain an internal _item attribute, so we
            return true in the case of the attribute being undefined.
            We use this function to give this state a clearer name.
        '''
        return not hasattr(self, "_item") or self._item is not None
