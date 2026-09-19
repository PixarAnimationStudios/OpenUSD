# Copyright 2026 Pixar
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
"""
Regression test for build_usd.py's OpenImageIO/OpenColorIO dependency
ordering (see GitHub issue #4219).

build_usd.py is a top-level script (it runs argparse and exits on import if
a C++ compiler is not installed), so it can't be imported directly in a
sandboxed test environment. This test instead re-implements, in isolation,
the exact requiredDependencies-building predicate from the "buildImaging"
block of build_usd.py so the fix's boolean logic can be verified without a
compiler.
"""

import unittest

ZLIB, OPENCOLORIO, JPEG, TIFF, PNG, OPENEXR, OPENIMAGEIO = range(7)


def required_dependencies(build_ocio, build_oiio):
    """Mirrors the fixed predicate in build_usd.py's buildImaging block."""
    deps = []
    if build_ocio or build_oiio:
        deps += [ZLIB, OPENCOLORIO]
    if build_oiio:
        deps += [ZLIB, JPEG, TIFF, PNG, OPENEXR, OPENIMAGEIO]
    return deps


class TestOiioRequiresOcio(unittest.TestCase):
    def test_oiio_without_explicit_ocio_still_requires_opencolorio(self):
        # This is exactly the scenario in issue #4219: --openimageio without
        # --opencolorio. Before the fix, OPENCOLORIO was absent here even
        # though OpenImageIO 3.x links against it.
        deps = required_dependencies(build_ocio=False, build_oiio=True)
        self.assertIn(OPENCOLORIO, deps)
        self.assertIn(OPENIMAGEIO, deps)

    def test_explicit_ocio_still_requires_opencolorio(self):
        deps = required_dependencies(build_ocio=True, build_oiio=False)
        self.assertIn(OPENCOLORIO, deps)
        self.assertNotIn(OPENIMAGEIO, deps)

    def test_neither_requested_no_imaging_deps(self):
        deps = required_dependencies(build_ocio=False, build_oiio=False)
        self.assertNotIn(OPENCOLORIO, deps)
        self.assertNotIn(OPENIMAGEIO, deps)

    def test_opencolorio_is_added_before_openimageio(self):
        # OIIO links against OCIO, so OCIO must appear first in the build
        # order for a fresh install (matches the existing code comment).
        deps = required_dependencies(build_ocio=False, build_oiio=True)
        self.assertLess(deps.index(OPENCOLORIO), deps.index(OPENIMAGEIO))


if __name__ == "__main__":
    unittest.main()
