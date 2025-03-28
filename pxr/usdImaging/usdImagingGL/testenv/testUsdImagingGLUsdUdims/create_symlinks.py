#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
