#!/pxrpythonsubst
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

from __future__ import print_function

from pxr import Usd
from pxr import UsdGeom
from pxr import UsdDraco

import argparse
import collections
import os
import string
import sys


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

  def __init__(self, options):
    self.options = options
    self.usedFileNames = collections.defaultdict(int)

  def encodeStage(self, stage, dracoDir):
    """Encodes meshes in a given stage and writes them to a given directory."""

    if not os.path.exists(dracoDir):
      os.makedirs(dracoDir)

    # Compress all meshes in the scene with Draco.
    if self.options.verbose:
      print('Meshes:')
    self.usedFileNames.clear()
    for prim in stage.TraverseAll():
      mesh = UsdGeom.Mesh(prim)
      if mesh:
        # Create a unique file name for the compressed mesh.
        fileName = dracoDir + self.getFileName(mesh) + '.drc'

        # Compress mesh and write to file.
        self.encodeMesh(stage, mesh, fileName)
        if self.options.verbose:
          print('  saved ' + fileName)

  def fromBool(self, option, invert):
    """Returns a numeric value for a possibly unspecified boolean 0|1 option."""

    # Convert unspecified option to a number.
    UNSPECIFIED = -1
    if option is None:
      return UNSPECIFIED

    # Invert option value if requested.
    return 1 - option if invert else option

  def encodeMesh(self, stage, mesh, fileName):
    """Compresses mesh to file and strips geometry properties from USD mesh."""

    # Prepare options for Draco encoder.
    preserve_polygons = self.fromBool(self.options.preserve_polygons, False)
    preserve_pos_order = self.fromBool(self.options.discard_subdivision, True)
    preserve_holes = self.fromBool(self.options.discard_subdivision, True)

    # Compress mesh geometry with Draco and write it to file.
    success = UsdDraco._WriteDraco(mesh, fileName,
                                   self.options.qp,
                                   self.options.qt,
                                   self.options.qn,
                                   self.options.cl,
                                   preserve_polygons,
                                   preserve_pos_order,
                                   preserve_holes)
    if not success:
      sys.exit('Could not encode mesh: ' + str(mesh.GetPrim().GetPrimPath()))

    # Strip encoded geometry properties from the USD mesh.
    for name in self.ENCODED_PROPERTIES:
      self.removePropertyOrExit(stage, mesh, name)

    # Strip encoded geometry primvars from the USD mesh.
    for primvar in UsdGeom.PrimvarsAPI(mesh.GetPrim()).GetPrimvars():
      if UsdDraco._PrimvarSupported(primvar):
        name = primvar.GetName()
        self.removePropertyOrExit(stage, mesh, name)
        self.removePropertyOrExit(stage, mesh, name + ':indices')

    # Add Draco file as a reference to the USD mesh.
    mesh.GetPrim().GetReferences().AddReference(fileName)

  def removePropertyOrExit(self, stage, mesh, name):
    """Removes mesh property if possible or exits with error."""

    # Do nothing if mesh has no property with a given name.
    if not mesh.GetPrim().HasProperty(name):
      return

    # Get a list of property specs that provide opinions for this property.
    prop = mesh.GetPrim().GetProperty(name)
    specs = prop.GetPropertyStack(Usd.TimeCode.Default())

    # Do nothing if property has no opinions.
    if len(specs) == 0:
      return

    # Exit with error (or optionally warn the user) if property has multiple
    # opinions or a single opinion that will not be cleared.
    if len(specs) > 1:
      self.warnOrExit(prop, True)
    else:
      editTarget = stage.GetEditTarget()
      if specs[0] != editTarget.GetPropertySpecForScenePath(prop.GetPath()):
        self.warnOrExit(prop, False)

    # Clear property.
    mesh.GetPrim().RemoveProperty(name)

  def warnOrExit(self, prop, plural):
    """Depending on a flag, prints a warning or exits with error."""

    # Prepare message for the user.
    level = 'WARNING' if self.options.ignore_opinion_errors else 'ERROR'
    opinion = 'Opinions' if plural else 'Opinion'
    message = (level + ': Results may be invalid. ' + opinion +
               ' will not be cleared for property: ' + str(prop.GetPath()))

    # Print message and possibly exit.
    if self.options.ignore_opinion_errors:
      print(message)
    else:
      sys.exit(message)

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


def ParseOptions():
  """Parses, checks, prints, and returns command-line options."""

  # Define command-line options.
  parser = argparse.ArgumentParser(
      description='Compress usd file to a specified output file and'
                  'Draco-compressed files in the corresponding directory.')
  parser = argparse.ArgumentParser()
  parser.add_argument('input', help='input file')
  parser.add_argument('-o', '--out', required=True, metavar='file',
                      dest='output', action='store', help='output file')
  parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                      help='enable verbose output mode')
  parser.add_argument('-qp', type=int, default=14, choices=range(0, 31),
                      metavar='0-30', help='quantization bits for positions')
  parser.add_argument('-qt', type=int, default=12, choices=range(0, 31),
                      metavar='0-30', help='quantization bits for textures')
  parser.add_argument('-qn', type=int, default=10, choices=range(0, 31),
                      metavar='0-30', help='quantization bits for normals')
  parser.add_argument('-cl', type=int, default=10, choices=range(0, 11),
                      metavar='0-10', help='compression level, best=10')
  parser.add_argument('--preserve_polygons', type=int, choices={0, 1},
                      metavar='0|1')
  parser.add_argument('--discard_subdivision', type=int, choices={0, 1},
                      metavar='0|1', help=argparse.SUPPRESS)
  parser.add_argument('--ignore_opinion_errors', action='store_true',
                      help='proceed when opinions cannot be cleared')

  # Parse command-line options.
  options = parser.parse_args()

  # Perform additional option checks.
  if not os.path.isfile(options.input):
    print('Input file is missing.')
    parser.print_usage()
    exit(1)

  # Print options in verbose mode.
  if options.verbose:
    print('Options:')
    print('  input  : ' + options.input)
    print('  output : ' + options.output)
    print('  quantization bits for positions : ' + str(options.qp))
    print('  quantization bits for textures  : ' + str(options.qt))
    print('  quantization bits for normals   : ' + str(options.qn))
    print('  compression level : ' + str(options.cl))
    if options.preserve_polygons is not None:
      print('  preserve polygons : ' +
          ('yes' if options.preserve_polygons == 1 else 'no'))
    if options.discard_subdivision is not None:
      print('  discard subdivision : ' +
          ('yes' if options.discard_subdivision == 1 else 'no'))
    if options.ignore_opinion_errors:
      print('  ignore opinion errors')
  return options


def main():
  # Parse command-line options.
  options = ParseOptions()

  # Open USD stage.
  stage = Usd.Stage.Open(options.input)

  # Encode and save all meshes in USD stage with Draco.
  encoder = UsdDracoEncoder(options)
  encoder.encodeStage(stage, options.output + '.draco/')

  # Save the modified USD stage that references encoded meshes.
  stage.GetRootLayer().Export(options.output)
  if options.verbose:
    print('Stage:')
    print('  saved ' + options.output)


main()

