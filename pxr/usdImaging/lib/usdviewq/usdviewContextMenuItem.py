#
# Copyright 2016 Pixar
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
