[
    #--------------------------------------------------------------------------
    # hdPrman/rileyRenderOutput
    dict(
        LIBRARY_PATH = 'ext/rmanpkg/25.0/plugin/renderman/plugin/hdPrman',
        INCLUDE_PATH = 'hdPrman',
        FILE_NAME = 'rileyRenderOutputSchema',
        SCHEMA_NAME = 'HdPrmanRileyRenderOutput',
        SCHEMA_TOKEN = 'rileyRenderOutput',
        DEFAULT_LOCATOR = ['rileyRenderOutput'],
        MEMBERS = [
            ('name', T_TOKEN),
            ('type', T_TOKEN),
            ('source', T_TOKEN),
            ('accumulationRule', T_TOKEN),
            ('filter', T_TOKEN),
            ('filterSize', T_VEC2F),
            ('relativePixelVariance', T_FLOAT),
            ('params', T_CONTAINER),
        ],
        EXTRA_TOKENS = [
            '(typeFloat, "float")',
            '(typeInteger, "integer")',
            '(typeColor, "color")',
            '(typeVector, "vector")',
        ],
        ADD_LOCATOR_FOR_EACH_MEMBER = True,
    ),
]
