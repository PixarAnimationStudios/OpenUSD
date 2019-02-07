#
# Copyright 2019 Google LLC
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
from pxr import Usd
from pxr import UsdGeom
from pxr import UsdDraco

import collections
import os
import string
import sys


class Options(object):
  """Class for parsing and checking command-line arguments."""

  # Option names.
  HELP = '-h'
  INPUT = '-i'
  OUTPUT = '-o'
  QP = '-qp'
  QT = '-qt'
  QN = '-qn'
  CL = '-cl'
  PRESERVE_POLYGONS = '-preserve_polygons'
  DISCARD_SUBDIVISION = '-discard_subdivision'

  # Default option values.
  UNSPECIFIED = -1
  DEFAULTS = {
      QP: 14,
      QT: 12,
      QN: 10,
      CL: 10,
      PRESERVE_POLYGONS: UNSPECIFIED,
      DISCARD_SUBDIVISION: UNSPECIFIED,
  }

  # Required options.
  REQUIRED = {INPUT, OUTPUT}


  class Range:
    """Represents a range of option values."""

    def __init__(self, minValue, maxValue):
      """Constructs a range as an interval between two values."""

      self.minValue = minValue
      self.maxValue = maxValue

    def __contains__(self, value):
      """Overriding the in operator."""

      return self.minValue <= value <= self.maxValue


  def __init__(self, arguments):
    """Parses, checks, and stored command-line options."""

    self.options = self.parseArguments(arguments[1:])
    self.checkOptions(self.options)
    self.printOptions(self.options)

  def __getitem__(self, key):
    """Returns a requested option, e.g., options[Options.INPUT]."""

    return self.options[key]

  def usage(self):
    """Prints script usage and exits."""

    # Not advertising the option for discarding subdivision.
    sys.exit('Usage: python encode.py \\\n' + \
             '  ' + self.INPUT + ' <input file> \\\n' + \
             '  ' + self.OUTPUT + ' <output file> \\\n' + \
             '  ' + self.QP + ' <quantization bits for positions>  \\\n' + \
             '  ' + self.QT + ' <quantization bits for textures>  \\\n' + \
             '  ' + self.QN + ' <quantization bits for normals>  \\\n' + \
             '  ' + self.CL + ' <compression level [0-10], best=10>  \\\n' + \
             '  ' + self.PRESERVE_POLYGONS + ' 0|1')

  def parseArguments(self, arguments):
    """Parses command-line arguments and returns them as an options dict."""

    options = self.DEFAULTS
    for i in xrange(0, len(arguments), 2):
      if arguments[i] == self.HELP:
        self.usage()
      if not arguments[i].startswith('-') or i == len(arguments) - 1:
        self.usage()
      options[arguments[i]] = arguments[i + 1]
    return options


  def checkOptions(self, options):
    """Checks command-line options."""

    # Check whether all required options have been specified.
    if not all(option in options for option in self.REQUIRED):
      self.usage()

    # Check whether the input file exists.
    if not os.path.isfile(options[self.INPUT]):
      print 'Input file is missing.'
      self.usage()

    # Check wheter integer options can be parsed and are within a valid range.
    self.checkIntOption(options, self.QP, self.Range(0, 30), '[0-30]')
    self.checkIntOption(options, self.QT, self.Range(0, 30), '[0-30]')
    self.checkIntOption(options, self.QN, self.Range(0, 30), '[0-30]')
    self.checkIntOption(options, self.CL, self.Range(0, 10), '[0-10]')

    # Checks whether boolean options can be parsed.
    self.checkBooleanOption(options, self.PRESERVE_POLYGONS)
    self.checkBooleanOption(options, self.DISCARD_SUBDIVISION)

  def checkIntOption(self, options, name, validRange, want):
    """Parses an integer option and checks whether the value is valid."""

    # Check whether an integer option can be parsed.
    if not isinstance(options[name], int):
      try:
        options[name] = int(options[name])
      except ValueError:
        Options.printUnexpectedValue(name, want, options[name])
        self.usage()

    # Check whether an integer option is within a valid range.
    if options[name] not in validRange:
      Options.printUnexpectedValue(name, want, options[name])
      self.usage()

  def checkBooleanOption(self, options, name):
    """Checks and parses boolean option."""

    validRange = {0, 1, self.UNSPECIFIED}
    self.checkIntOption(options, name, validRange, '0|1')

  @staticmethod
  def printUnexpectedValue(name, want, got):
    """Prints a message when invalid option value is specified."""

    print 'Expected {0} for {1} argument, got {2}.'.format(want, name, got)

  def printOptions(self, options):
    """Prints options."""

    print 'options:'
    print '  input  : ' + options[self.INPUT]
    print '  output : ' + options[self.OUTPUT]
    print '  quantization bits for positions : ' + str(options[self.QP])
    print '  quantization bits for textures  : ' + str(options[self.QT])
    print '  quantization bits for normals   : ' + str(options[self.QN])
    print '  compression level : ' + str(options[self.CL])
    self.printBooleanOption('preserve polygons',
                            options[self.PRESERVE_POLYGONS])
    self.printBooleanOption('discard subdivision',
                            options[self.DISCARD_SUBDIVISION])

  def printBooleanOption(self, description, value):
    """Prints boolean options if specified by the user."""

    if value is self.UNSPECIFIED:
      return
    print '  {0} : {1}'.format(description, 'yes' if value == 1 else 'no')

  @staticmethod
  def inv(option):
    """Inverts a boolean (0|1) option if it is specified."""

    return option if option is Options.UNSPECIFIED else int(not option)


class UsdDracoEncoder(object):
  """Class for encoding USD scene with Draco compression library."""

  # USD mesh prim properties that are compressed by Draco.
  ENCODED_PROPERTIES = [
      UsdGeom.Tokens.extent,
      UsdGeom.Tokens.faceVertexCounts,
      UsdGeom.Tokens.faceVertexIndices,
      UsdGeom.Tokens.points,
      UsdGeom.Tokens.holeIndices,
  ]
  ENCODED_PRIMVARS = [
      'Texture_uv',
      UsdGeom.Tokens.normals,
  ]
  UNENCODED_PRIMVAR_INTERPS = [
      UsdGeom.Tokens.constant,
  ]

  def __init__(self, options):
    self.options = options
    self.usedFileNames = collections.defaultdict(int)

  def encodeStage(self, stage, dracoDir):
    """Encodes meshes in a given stage and writes them to a given directory."""

    if not os.path.exists(dracoDir):
      os.makedirs(dracoDir)

    # Compress all meshes in the scene with Draco.
    print 'meshes:'
    self.usedFileNames.clear()
    for prim in stage.TraverseAll():
      mesh = UsdGeom.Mesh(prim)
      if mesh:
        # Create a unique file name for the compressed mesh.
        fileName = dracoDir + self.getFileName(mesh) + '.drc'

        # Compress mesh and write to file.
        self.encodeMesh(mesh, fileName)
        print '  saved ' + fileName

  def encodeMesh(self, mesh, fileName):
    """Compresses mesh to file and strips geometry properties from USD mesh."""

    # Convert discard subdivision option to preserve options.
    preserve_pos_order = Options.inv(self.options[Options.DISCARD_SUBDIVISION])
    preserve_holes = Options.inv(self.options[Options.DISCARD_SUBDIVISION])

    # Compress mesh geometry with Draco and write it to file.
    success = UsdDraco._WriteDraco(mesh, fileName,
                                   self.options[Options.QP],
                                   self.options[Options.QT],
                                   self.options[Options.QN],
                                   self.options[Options.CL],
                                   self.options[Options.PRESERVE_POLYGONS],
                                   preserve_pos_order,
                                   preserve_holes)
    if not success:
      sys.exit('Could not encode mesh: ' + str(mesh.GetPrim().GetPrimPath()))

    # Strip encoded geometry properties from the USD mesh.
    for name in self.ENCODED_PROPERTIES:
      mesh.GetPrim().RemoveProperty(name)

    # Strip encoded geometry primvars from the USD mesh.
    for name in self.ENCODED_PRIMVARS:
      primvar = UsdGeom.PrimvarsAPI(mesh.GetPrim()).GetPrimvar(name)
      if primvar.GetInterpolation() not in self.UNENCODED_PRIMVAR_INTERPS:
        mesh.GetPrim().RemoveProperty('primvars:' + name)
        mesh.GetPrim().RemoveProperty('primvars:' + name + ':indices')

    # Add Draco file as a reference to the USD mesh.
    mesh.GetPrim().GetReferences().AddReference(fileName)

  def getFileName(self, mesh):
    """Returns a uique file name without extension for a given mesh."""

    path = str(mesh.GetPrim().GetPrimPath())
    fileName = ''.join(c if c not in string.punctuation else '_' for c in path)
    if fileName.startswith('_'):
      fileName = fileName[1:]
    self.usedFileNames[fileName] += 1
    if self.usedFileNames[fileName] > 1:
      fileName += '_' + str(self.usedFileNames[fileName])
    return fileName


def main():
  # Parse command-line options.
  options = Options(sys.argv)

  # Open USD stage.
  stage = Usd.Stage.Open(options[Options.INPUT])

  # Encode and save all meshes in USD stage with Draco.
  encoder = UsdDracoEncoder(options)
  encoder.encodeStage(stage, options[Options.OUTPUT] + '.draco/')

  # Save the modified USD stage that references encoded meshes.
  stage.GetRootLayer().Export(options[Options.OUTPUT])
  print 'stage:'
  print '  saved ' + options[Options.OUTPUT]


main()

