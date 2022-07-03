#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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

# Script to create the symlinks for the usdUdimsSymlinked.usda test case

import os

# (dst, src) pairs of symlinks to create
symlinksToCreate = [
    ("symlink_1001.jpg", "image_a.jpg"),
    ("symlink_1002.jpg", "image_b.jpg"),
    ("symlink_1003.jpg", "image_c.jpg"),
    ("symlink_1004.jpg", "image_b.jpg"),
    ("symlink_1005.jpg", "image_b.jpg"),
    ("symlink_1006.jpg", "image_b.jpg"),
    ("symlink_1008.jpg", "image_c.jpg"),
    ("symlink_1009.jpg", "image_b.jpg"),
    ("symlink_1010.jpg", "image_c.jpg"),
    ("symlink_1011.jpg", "image_b.jpg"),
    ("symlink_1012.jpg", "image_b.jpg"),
    ("symlink_1013.jpg", "image_c.jpg"),
]

for (dst, src) in symlinksToCreate:
    if os.path.exists(dst):
        os.remove(dst)

    os.symlink(src, dst)
