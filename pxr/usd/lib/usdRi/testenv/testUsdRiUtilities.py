#!/usr/bin/env pypix

import unittest

class TestUsdRiUtilities(unittest.TestCase):

  def test_RmanConversions(self):
      from pxr.UsdRi import *

      # Note that we have the old names as the first elements in
      # the list, our conversion test relies on this fact. This is only
      # really relevant in the case of Rman's '1' value, in which we have
      # some ambiguity(it could be cornersPlus1, cornersPlus2, or cornersOnly)
      # since we don't currently express the propogateCorners argument through
      # the system.
      faceVaryingConversionTable = [  
        (["bilinear",      "all"], 0),
        (["edgeAndCorner", "cornersPlus1", "cornersPlus2", "cornersOnly"], 1),
        (["edgeOnly",      "none"], 2),
        (["alwaysSharp",   "boundaries"], 3)]

      for tokens, rmanValue in faceVaryingConversionTable:
        # Check all tokens, old and new
        for token in tokens:
          # Convert to renderman values
          self.assertEqual(
            ConvertToRManFaceVaryingLinearInterpolation(token), rmanValue)

        # Convert from renderman values
        # Note that we only map to the new tokens.
        self.assertEqual(
          ConvertFromRManFaceVaryingLinearInterpolation(rmanValue), tokens[1])

if __name__ == "__main__":
  unittest.main()
