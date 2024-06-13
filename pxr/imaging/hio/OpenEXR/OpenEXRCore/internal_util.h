/*
** SPDX-License-Identifier: BSD-3-Clause
** Copyright Contributors to the OpenEXR Project.
*/

#ifndef OPENEXR_PRIVATE_UTIL_H
#define OPENEXR_PRIVATE_UTIL_H

#include <stdint.h>

static inline int
compute_sampled_height (int height, int y_sampling, int start_y)
{
    int nlines;

    if (y_sampling <= 1) return height;

    if (height == 1)
        nlines = (start_y % y_sampling) == 0 ? 1 : 0;
    else
    {
        int start, end;

        /* computed the number of times y % ysampling == 0, by
         * computing interval based on first and last time that occurs
         * on the given range
         */
        start = start_y % y_sampling;
        if (start != 0)
            start = start_y + (y_sampling - start);
        else
            start = start_y;
        end = start_y + height - 1;
        end -= (end < 0) ? (-end % y_sampling) : (end % y_sampling);

        if (start > end)
            nlines = 0;
        else
            nlines = (end - start) / y_sampling + 1;
    }

    return nlines;
}

static inline int
compute_sampled_width (int width, int x_sampling, int start_x)
{
    /*
     * we require that the start_x % x_sampling == 0 and for tiled images (and for deep),
     * x_sampling must be 1, so this can simplify the math compared to the y case
     * where when we are reading scanline images, we always are reading the entire
     * width. If this changes, can look like the above call for the lines, but
     * for now can be simpler math
     */
    if (x_sampling <= 1) return width;

    return (width == 1) ? 1 : (width / x_sampling);
}

#endif /* OPENEXR_PRIVATE_UTIL_H */
