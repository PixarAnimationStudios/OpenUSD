#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
Screenshot utilities for headless noodles tests.

Provides image saving, validation, comparison, and encoding
for use with the headless test harness.
"""

import base64


def save_png(image, path):
    """Save a QImage to disk as PNG.

    Args:
        image: QImage
        path: str file path

    Returns:
        bool: True if saved successfully
    """
    return image.save(path, "PNG")


def image_is_nonempty(image):
    """Check that a QImage has valid dimensions and is not uniform.

    Samples pixels across the image and checks that at least two
    distinct colors appear. This catches both all-black (GL init
    failure) and all-same-color (clear-only, no content rendered).

    Args:
        image: QImage

    Returns:
        bool: True if the image has visible content
    """
    if image.isNull() or image.width() == 0 or image.height() == 0:
        return False

    w, h = image.width(), image.height()

    # Sample a grid of pixels across the image
    step_x = max(1, w // 10)
    step_y = max(1, h // 10)
    first_color = None

    for y in range(0, h, step_y):
        for x in range(0, w, step_x):
            color = image.pixelColor(x, y)
            rgb = (color.red(), color.green(), color.blue())
            if first_color is None:
                first_color = rgb
            elif rgb != first_color:
                # Found at least two distinct colors — image has content
                return True

    return False


def to_base64(image):
    """Encode a QImage as a base64 PNG string.

    Args:
        image: QImage

    Returns:
        str: base64-encoded PNG data
    """
    from pxr.Usdviewq.qt import QtCore

    buffer = QtCore.QBuffer()
    buffer.open(QtCore.QIODevice.OpenModeFlag.WriteOnly)
    image.save(buffer, "PNG")
    png_bytes = buffer.data().data()
    return base64.b64encode(png_bytes).decode("ascii")


def images_match(img_a, img_b, tolerance=0.05):
    """Compare two QImages with pixel-level tolerance.

    Args:
        img_a: QImage
        img_b: QImage
        tolerance: float, fraction of pixels allowed to differ (0.0 - 1.0)

    Returns:
        bool: True if images match within tolerance
    """
    if img_a.size() != img_b.size():
        return False

    w, h = img_a.width(), img_a.height()
    total_pixels = w * h
    if total_pixels == 0:
        return True

    diff_count = 0
    # Sample ~10,000 pixels on a uniform grid for performance.
    # step = sqrt(total_pixels / 10000) gives a grid that covers the
    # image evenly while keeping comparison fast.
    step = max(1, int((total_pixels / 10000) ** 0.5))
    sampled = 0

    for y in range(0, h, step):
        for x in range(0, w, step):
            sampled += 1
            ca = img_a.pixelColor(x, y)
            cb = img_b.pixelColor(x, y)
            if (
                abs(ca.red() - cb.red()) > 5
                or abs(ca.green() - cb.green()) > 5
                or abs(ca.blue() - cb.blue()) > 5
            ):
                diff_count += 1

    if sampled == 0:
        return True

    diff_fraction = diff_count / sampled
    return diff_fraction <= tolerance
