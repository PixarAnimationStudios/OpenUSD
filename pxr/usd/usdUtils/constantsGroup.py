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

"""A module for creating groups of constants. This is similar to the enum
module, but enum is not available in Python 2's standard library.
"""
import sys
import types

class _MetaConstantsGroup(type):
    """A meta-class which handles the creation and behavior of ConstantsGroups.
    """

    def __new__(metacls, cls, bases, classdict):
        """Discover constants and create a new ConstantsGroup class."""

        # If this is just the base ConstantsGroup class, simply create it and do
        # not search for constants.
        if cls == "ConstantsGroup":
            return super(_MetaConstantsGroup, metacls).__new__(metacls, cls, bases, classdict)

        # Search through the class-level properties and convert them into
        # constants.
        allConstants = list()
        for key, value in classdict.items():
            if (key.startswith("_") or isinstance(value, classmethod) or
                    isinstance(value, staticmethod)):
                # Ignore variables which start with an underscore, and
                # static/class methods.
                pass
            else:
                # Found a new constant.
                allConstants.append(value)

                # If the constant is a function/lambda, ensure that it is not
                # converted into a method.
                if isinstance(value, types.FunctionType):
                    classdict[key] = staticmethod(value)

        # All constants discovered, now create the `_all` property.
        classdict["_all"] = tuple(allConstants)

        # Finally, create the new ConstantsGroup class.
        return super(_MetaConstantsGroup, metacls).__new__(metacls, cls, bases, classdict)

    def __setattr__(cls, name, value):
        """Prevent modification of properties after a group is created."""
        raise AttributeError("Constant groups cannot be modified.")

    def __delattr__(cls, name):
        """Prevent deletion of properties after a group is created."""
        raise AttributeError("Constant groups cannot be modified.")

    def __len__(self):
        """Get the number of constants in the group."""
        return len(self._all)

    def __contains__(self, value):
        """Check if a constant exists in the group."""
        return (value in self._all)

    def __iter__(self):
        """Iterate over each constant in the group."""
        return iter(self._all)

# We want to define a ConstantsGroup class that uses _MetaConstantsGroup
# as its metaclass. The syntax for doing so in Python 3 is not backwards
# compatible for Python 2, so we cannot just conditionally define one
# form or the other because that will cause syntax errors when compiling
# this file. To avoid this we add a layer of indirection through exec().
if sys.version_info.major >= 3:
    defineConstantsGroup = '''
class ConstantsGroup(object, metaclass=_MetaConstantsGroup):
    """The base constant group class, intended to be inherited by actual groups
    of constants.
    """

    def __new__(cls, *args, **kwargs):
        raise TypeError("ConstantsGroup objects cannot be created.")
'''
else:
    defineConstantsGroup = '''
class ConstantsGroup(object):
    """The base constant group class, intended to be inherited by actual groups
    of constants.
    """
    __metaclass__ = _MetaConstantsGroup

    def __new__(cls, *args, **kwargs):
        raise TypeError("ConstantsGroup objects cannot be created.")
'''

exec(defineConstantsGroup)
