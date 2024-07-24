#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
# as its metaclass.
class ConstantsGroup(object, metaclass=_MetaConstantsGroup):
    """The base constant group class, intended to be inherited by actual groups
    of constants.
    """
    def __new__(cls, *args, **kwargs):
        raise TypeError("ConstantsGroup objects cannot be created.")
