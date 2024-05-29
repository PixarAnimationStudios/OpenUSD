#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
[
    dict(
        SCHEMA_NAME = 'ALL_SCHEMAS',
        LIBRARY_PATH = 'pxr/imaging/hdar',
    ),        
    
    #--------------------------------------------------------------------------
    # hdar/system
    dict(
        SCHEMA_NAME = 'System',
        DOC = '''The {{ SCHEMA_CLASS_NAME }} specifies a container that will 
                 hold "system" data that is relevant to asset resolution.''',
        SCHEMA_TOKEN = 'assetResolution',
        LOCATOR_PREFIX = 'HdSystemSchema::GetDefaultLocator()',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('resolverContext', T_RESOLVERCONTEXT, {}),
        ],
        IMPL_SCHEMA_INCLUDES = ['pxr/imaging/hd/systemSchema'],
    ),
]
