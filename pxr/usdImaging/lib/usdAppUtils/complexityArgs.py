#
# Copyright 2019 Pixar
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

class RefinementComplexities(object):
    """
    An enum-like container of standard complexity settings.
    """

    class _RefinementComplexity(object):
        """
        Class which represents a level of mesh refinement complexity. Each
        level has a string identifier, a display name, and a float complexity
        value.
        """

        def __init__(self, compId, name, value):
            self._id = compId
            self._name = name
            self._value = value

        def __repr__(self):
            return self.id

        @property
        def id(self):
            return self._id

        @property
        def name(self):
            return self._name

        @property
        def value(self):
            return self._value

    LOW = _RefinementComplexity("low", "Low", 1.0)
    MEDIUM = _RefinementComplexity("medium", "Medium", 1.1)
    HIGH = _RefinementComplexity("high", "High", 1.2)
    VERY_HIGH = _RefinementComplexity("veryhigh", "Very High", 1.3)

    _ordered = (LOW, MEDIUM, HIGH, VERY_HIGH)

    @classmethod
    def ordered(cls):
        """
        Get a tuple of all complexity levels in order.
        """
        return cls._ordered

    @classmethod
    def fromId(cls, compId):
        """
        Get a complexity from its identifier.
        """
        matches = [comp for comp in cls._ordered if comp.id == compId]
        if len(matches) == 0:
            raise ValueError("No complexity with id '{}'".format(compId))
        return matches[0]

    @classmethod
    def fromName(cls, name):
        """
        Get a complexity from its display name.
        """
        matches = [comp for comp in cls._ordered if comp.name == name]
        if len(matches) == 0:
            raise ValueError("No complexity with name '{}'".format(name))
        return matches[0]

    @classmethod
    def next(cls, comp):
        """
        Get the next highest level of complexity. If already at the highest
        level, return it.
        """
        if comp not in cls._ordered:
            raise ValueError("Invalid complexity: {}".format(comp))
        nextIndex = min(
            len(cls._ordered) - 1,
            cls._ordered.index(comp) + 1)
        return cls._ordered[nextIndex]

    @classmethod
    def prev(cls, comp):
        """
        Get the next lowest level of complexity. If already at the lowest
        level, return it.
        """
        if comp not in cls._ordered:
            raise ValueError("Invalid complexity: {}".format(comp))
        prevIndex = max(0, cls._ordered.index(comp) - 1)
        return cls._ordered[prevIndex]


def AddCmdlineArgs(argsParser, defaultValue=RefinementComplexities.LOW,
        altHelpText=''):
    """
    Adds complexity-related command line arguments to argsParser.

    The resulting 'complexity' argument will be one of the standard
    RefinementComplexities.
    """
    helpText = altHelpText
    if not helpText:
        helpText = ('level of refinement to use (default=%(default)s)')

    argsParser.add_argument('--complexity', '-c', action='store',
        type=RefinementComplexities.fromId,
        default=defaultValue,
        choices=[c for c in RefinementComplexities.ordered()],
        help=helpText)
