#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
[
    dict(
        SCHEMA_NAME = 'ALL_SCHEMAS',
        LIBRARY_PATH = 'pxr/usdImaging/usdRiPxrImaging',
    ),

    #--------------------------------------------------------------------------
    # projection
    dict(
        SCHEMA_NAME = 'Projection',
        SCHEMA_TOKEN = 'projection',
        SCHEMA_INCLUDES = ['pxr/imaging/hd/materialNodeSchema'],
        MEMBERS = [
            ('resource', 'HdMaterialNodeSchema', dict(ADD_LOCATOR = True)),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),
]
