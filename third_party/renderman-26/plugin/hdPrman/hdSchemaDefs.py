#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
[
    dict(
        SCHEMA_NAME = 'ALL_SCHEMAS',
        LIBRARY_PATH = 'ext/rmanpkg/plugin/renderman/plugin/hdPrman',
        INCLUDE_PATH = 'hdPrman',
        VERSION_GUARD_CONST_GETTER = True
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyParam
    dict(
        SCHEMA_NAME = 'RileyParam',
        MEMBERS = [
            ('value', T_SAMPLED, {}),
            ('role', T_TOKEN,
             dict(DOC = '''
             Disambiguates what call to RtParamList::SetFOO to use.

             Can take values from HdPrimvarRoleTokens and
             HdPrmanRileyAdditionalRoleTokens.''')),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyParamList
    dict(
        DOC = '''
        Schema to describe RtParamList.

        To obtain an RtParamList from this schema, we call the appropriate
        RtParamList::SetFOO(name, value) for each name in the container params.

        The schema {{LIBRARY_NAME}}RileyParamSchema determines what SetFOO
        method is called with what value. To resolve what SetFOO method to use,
        the type of the VtValue from the value data source is used as well as
        the role token if necessary. The value is coming from the value data
        source.
        ''',
        SCHEMA_NAME = 'RileyParamList',
        SCHEMA_INCLUDES = ['hdPrman/rileySchemaTypeDefs'],
        MEMBERS = [
            ('params', 'HdPrmanRileyParamContainerSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyPrimvar
    dict(
        SCHEMA_NAME = 'RileyPrimvar',
        MEMBERS = [
            ('value', T_SAMPLED, {}),
            ('detailType', T_TOKEN, {}),
            ('role', T_TOKEN,
             dict(DOC = '''
             Disambiguates what call to RtPrimvarList::SetFOO to use.

             Can take values from HdPrimvarRoleTokens and
             HdPrmanRileyAdditionalRoleTokens.''')),
        ],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('detailType', ['constant',
                            'uniform',
                            'vertex',
                            'varying',
                            'facevarying',
                            'reference']),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyPrimvarList
    dict(
        DOC = '''
        Schema to describe RtPrimvarList.

        To obtain an RtPrimvarList we pass numUniform, numVertex,
        numVarying, numFaceVarying to the constructor, call
        RtPrimvarList::SetTimes if "P" (for points) is among the params and has
        samples and finally call the appropriate
        RtPrimvarList::SetFOO[Detail](
            name, value[, detailType[, sampleTimeIndex]]).

        The {{LIBRARY_NAME}}RileyPrimvarSchema determines what SetFOO[Detail]
        method is valled with what value, detailType or sampleTimeIndex.

        For many data types or when detailType is constant, the behavior is
        the same as for the {{LIBRARY_NAME}}RileyParamSchema.

        Otherweise, we call SetFooDetail using {{LIBRARY_NAME}}RileyParamSchema
        similar as we did for {{LIBRARY_NAME}}RileyParamSchema but consuming
        the detailType data source as well. The value data source is only
        sampled at time 0.

        The "P" param is treated specially. It is the only param for which take
        time samples (from the sampled value data source) and always corresponds
        to a call to RtPrimvarList::SetPointDetail with detailType = vertex.
        ''',
        SCHEMA_NAME = 'RileyPrimvarList',
        SCHEMA_INCLUDES = ['hdPrman/rileySchemaTypeDefs'],
        MEMBERS = [
            ('numUniform', T_SIZET, {}),
            ('numVertex', T_SIZET, {}),
            ('numVarying', T_SIZET, {}),
            ('numFaceVarying', T_SIZET, {}),
            ('params', 'HdPrmanRileyPrimvarContainerSchema', {}),
        ],

        EXTRA_TOKENS = [
            'P',
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyShadingNode
    dict(
        SCHEMA_NAME = 'RileyShadingNode',
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('type', T_TOKEN, {}),
            ('name', T_TOKEN, {}),
            ('handle', T_TOKEN, {}),
            ('params', 'HdPrmanRileyParamListSchema', {}),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('type', ['pattern',
                      'bxdf',
                      'integrator',
                      'light',
                      'lightFilter',
                      'projection',
                      'displacement',
                      'sampleFilter',
                      'displayFilter']),
        ],
    ),

    #--------------------------------------------------------------------------
    #
    #
    # Schema's corresponding to riley globals.
    #
    #
    # hdPrman/rileyGlobals
    dict(
        SCHEMA_NAME = 'RileyGlobals',
        SCHEMA_TOKEN = 'rileyGlobals',
        ADD_DEFAULT_LOCATOR = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],

        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('options', 'HdPrmanRileyParamListSchema', {})
        ],
    ),

    #--------------------------------------------------------------------------
    #
    #
    # Schema's corresponding to riley prims.
    #
    #

    #--------------------------------------------------------------------------
    # hdPrman/rileyCamera
    dict(
        SCHEMA_NAME = 'RileyCamera',
        SCHEMA_TOKEN = 'rileyCamera',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyShadingNodeSchema',
                           'hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('name', T_TOKEN,
             dict(RILEY_CONVERTER='HdPrman_RileyUniqueString',
                  RILEY_NO_MODIFY=True)),
            ('projection', 'HdPrmanRileyShadingNodeSchema', {}),
            ('xform', T_MATRIX, {}),
            ('properties', 'HdPrmanRileyParamListSchema', {}),
        ],
        EXTRA_TOKENS = [
            'nearClip',
            'farClip',
            'focusregion',
            'dofaspect',
            'apertureNSides',
            'apertureAngle',
            'apertureRoundness',
            'apertureDensity',
            'shutterOpenTime',
            'shutterCloseTime',
            'shutteropening',
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyClippingPlane
    dict(
        SCHEMA_NAME = 'RileyClippingPlane',
        SCHEMA_TOKEN = 'rileyClippingPlane',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        NO_RILEY_USER_ID = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('xform', T_MATRIX, {}),
            ('params', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyCoordinateSystem
    dict(
        SCHEMA_NAME = 'RileyCoordinateSystem',
        SCHEMA_TOKEN = 'rileyCoordinateSystem',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        ADD_RILEY_ID_LIST = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('xform', T_MATRIX, {}),
            ('attributes', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyDisplacement
    dict(
        SCHEMA_NAME = 'RileyDisplacement',
        SCHEMA_TOKEN = 'rileyDisplacement',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileySchemaTypeDefs',
                           'hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('displacement', 'HdPrmanRileyShadingNodeVectorSchema', {}),
            ('attributes', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyDisplay
    dict(
        SCHEMA_NAME = 'RileyDisplay',
        SCHEMA_TOKEN = 'rileyDisplay',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('renderTarget', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyRenderTarget")),
            ('name', T_TOKEN, {}),
            ('driver', T_TOKEN, {}),
            ('renderOutputs', T_PATHARRAY,
             dict(RILEY_RELATIONSHIP_TARGET="RileyRenderOutput")),
            ('driverParams', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),


    #--------------------------------------------------------------------------
    # hdPrman/rileyDisplayFilter
    dict(
        SCHEMA_NAME = 'RileyDisplayFilter',
        SCHEMA_TOKEN = 'rileyDisplayFilter',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileySchemaTypeDefs',
                           'hdPrman/rileyParamListSchema'],
        ADD_RILEY_ID_LIST = True,
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('displayFilter', 'HdPrmanRileyShadingNodeVectorSchema', {}),
            ('attributes', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyGeometryInstance
    dict(
        SCHEMA_NAME = 'RileyGeometryInstance',
        SCHEMA_TOKEN = 'RileyGeometryInstance',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('groupPrototype', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyGeometryPrototype",
                  RILEY_NO_MODIFY=True)),
            ('geoPrototype', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyGeometryPrototype",
                  RILEY_NO_MODIFY=True)),
            ('material', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyMaterial")),
            ('coordsys', T_PATHARRAY,
             dict(RILEY_RELATIONSHIP_TARGET="RileyCoordinateSystem")),
            ('xform', T_MATRIX, {}),
            ('attributes', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyGeometryPrototype
    dict(
        SCHEMA_NAME = 'RileyGeometryPrototype',
        SCHEMA_TOKEN = 'RileyGeometryPrototype',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyPrimvarListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('type', T_TOKEN,
             dict(RILEY_NO_MODIFY=True)),
            ('displacement', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyDisplacement")),
            ('primvars', 'HdPrmanRileyPrimvarListSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyIntegrator
    dict(
        SCHEMA_NAME = 'RileyIntegrator',
        SCHEMA_TOKEN = 'rileyIntegrator',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyShadingNodeSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('integratorNode', 'HdPrmanRileyShadingNodeSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyLightInstance
    dict(
        SCHEMA_NAME = 'RileyLightInstance',
        SCHEMA_TOKEN = 'RileyLightInstance',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('groupPrototype', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyGeometryPrototype",
                  RILEY_NO_MODIFY=True)),
            ('geoPrototype', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyGeometryPrototype",
                  RILEY_NO_MODIFY=True)),
            ('material', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyMaterial")),
            ('lightShader', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyLightShader")),
            ('coordsys', T_PATHARRAY,
             dict(RILEY_RELATIONSHIP_TARGET="RileyCoordinateSystem")),
            ('xform', T_MATRIX, {}),
            ('attributes', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyLightShader
    dict(
        SCHEMA_NAME = 'RileyLightShader',
        SCHEMA_TOKEN = 'rileyLightShader',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileySchemaTypeDefs'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('light', 'HdPrmanRileyShadingNodeVectorSchema', {}),
            ('lightFilter', 'HdPrmanRileyShadingNodeVectorSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyMaterial
    dict(
        SCHEMA_NAME = 'RileyMaterial',
        SCHEMA_TOKEN = 'rileyMaterial',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileySchemaTypeDefs',
                           'hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('bxdf', 'HdPrmanRileyShadingNodeVectorSchema', {}),
            ('attributes', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyRenderOutput
    dict(
        SCHEMA_NAME = 'RileyRenderOutput',
        SCHEMA_TOKEN = 'rileyRenderOutput',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        ADD_RILEY_ID_LIST = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('name', T_TOKEN, {}),
            ('type', T_TOKEN,
             dict(RILEY_CONVERTER = "HdPrman_RileyRenderOutputType")),
            ('source', T_TOKEN, {}),
            ('accumulationRule', T_TOKEN, {}),
            ('filter', T_TOKEN, {}),
            ('filterSize', T_VEC2F,
             dict(RILEY_CONVERTER = "HdPrman_RileyFilterSize")),
            ('relativePixelVariance', T_FLOAT,
             dict(FALLBACK_VALUE = "1.0f")),
            ('params', 'HdPrmanRileyParamListSchema', {}),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('type', ['(float_, "float")', 'integer', 'color', 'vector']),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyRenderTarget
    dict(
        SCHEMA_NAME = 'RileyRenderTarget',
        SCHEMA_TOKEN = 'rileyRenderTarget',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('renderOutputs', T_PATHARRAY,
             dict(RILEY_RELATIONSHIP_TARGET="RileyRenderOutput")),
            ('extent', T_VEC3I,
             dict(RILEY_CONVERTER = "HdPrman_RileyExtent")),
            ('filterMode', T_TOKEN, {}),
            ('pixelVariance', T_FLOAT,
             dict(FALLBACK_VALUE = "1.0f")),
            ('params', 'HdPrmanRileyParamListSchema', {}),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('filterMode', ['importance', 'weighted']),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyRenderView
    dict(
        SCHEMA_NAME = 'RileyRenderView',
        SCHEMA_TOKEN = 'rileyRenderView',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileyParamListSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('renderTarget', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyRenderTarget")),
            ('camera', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyCamera")),
            ('integrator', T_PATH,
             dict(RILEY_RELATIONSHIP_TARGET="RileyIntegrator")),
            ('displayFilters', T_PATHARRAY,
             dict(RILEY_RELATIONSHIP_TARGET="RileyDisplayFilter")),
            ('sampleFilters', T_PATHARRAY,
             dict(RILEY_RELATIONSHIP_TARGET="RileySampleFilter")),
            ('params', 'HdPrmanRileyParamListSchema', {}),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('filterMode', ['importance', 'weighted']),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileySampleFilter
    dict(
        SCHEMA_NAME = 'RileySampleFilter',
        SCHEMA_TOKEN = 'rileySampleFilter',
        ADD_DEFAULT_LOCATOR = True,
        GENERATE_RILEY_PRIM = True,
        SCHEMA_INCLUDES = ['hdPrman/rileySchemaTypeDefs',
                           'hdPrman/rileyParamListSchema'],
        ADD_RILEY_ID_LIST = True,
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            ('sampleFilter', 'HdPrmanRileyShadingNodeVectorSchema', {}),
            ('attributes', 'HdPrmanRileyParamListSchema', {}),
        ],
    ),
]
