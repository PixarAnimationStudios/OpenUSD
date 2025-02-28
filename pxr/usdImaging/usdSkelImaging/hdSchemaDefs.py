#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
[
    dict(
        SCHEMA_NAME = 'ALL_SCHEMAS',
        LIBRARY_PATH = 'pxr/usdImaging/usdSkelImaging',
    ),

    #--------------------------------------------------------------------------
    # usdSkelImaging/skeleton
    dict(
        SCHEMA_NAME = 'Skeleton',
        SCHEMA_TOKEN = 'skeleton',
        ADD_DEFAULT_LOCATOR = True,
        DOC = '''Corresponds to UsdSkelSkeleton.''',
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),

            ('joints', T_TOKENARRAY,
             dict(DOC='''
                    Determines topology of skeleton.''')),
            ('jointNames', T_TOKENARRAY,
             dict(DOC='''
                    Here for completeness but ignored for posing the geometry.
                    See Skeleton.jointNames in usdSkel/schema for uses.''')),
            ('bindTransforms', T_MATRIXARRAY, {}),
            ('restTransforms', T_MATRIXARRAY,
             dict(DOC='''
                    These are local rest transforms.''')),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdSkelImaging/animation
    dict(
        SCHEMA_NAME = 'Animation',
        SCHEMA_TOKEN = 'skelAnimation',
        ADD_DEFAULT_LOCATOR = True,
        DOC = '''Corresponds to UsdSkelAnimation.''',
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),

            ('joints', T_TOKENARRAY, {}),
            ('translations', T_VEC3FARRAY, {}),
            ('rotations', T_QUATFARRAY, {}),
            ('scales', T_VEC3HARRAY, {}),

            ('blendShapes', T_TOKENARRAY, {}),
            ('blendShapeWeights', T_FLOATARRAY, {})
        ],
    ),


    #--------------------------------------------------------------------------
    # usdSkelImaging/binding
    dict(
        SCHEMA_NAME = 'Binding',
        SCHEMA_TOKEN = 'skelBinding',
        ADD_DEFAULT_LOCATOR = True,
        DOC = '''Corresponds to UsdSkelBindingAPI.''',
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),

            ('animationSource', T_PATH,
             dict(DOC = '''
                  Note that in UsdSkel, the animation comes from the
                  animationSource on the Skeleton (which also carries the
                  topology of the skeleton as joints).
                  This animationSource is either authored on the Skeleton
                  directly (and UsdSkelBindingAPI is applied) or inherited
                  from an ancestor of Skeleton (that has UsdSkelBindingAPI
                  applied).''')),

            ('skeleton', T_PATH, {}),
            ('joints', T_TOKENARRAY, {}),

            ('blendShapes', T_TOKENARRAY,
             dict(DOC = '''
                  This is not inherited according to the UsdSkel spec.''')),

            ('blendShapeTargets', T_PATHARRAY,
             dict(DOC = '''
                  This is not inherited according to the UsdSkel spec.''')),
        ],
        EXTRA_TOKENS = [
            '(skinningMethodPrimvar, "skel:skinningMethod")',
            '(geomBindTransformPrimvar, "skel:geomBindTransform")',
            '(jointIndicesPrimvar, "skel:jointIndices")',
            '(jointWeightsPrimvar, "skel:jointWeights")'
        ],
    ),

    #--------------------------------------------------------------------------
    # usdSkelImaging/inbetweenShape
    dict(
        SCHEMA_NAME = 'InbetweenShape',
        DOC = '''Corresponds to UsdSkelInbetweenShape.
        Each instance corresponds to a group of attributes on a UsdSkelBlendShape
        that share a prefix inbetweens:NAME.''',
        MEMBERS = [
            ('weight', T_FLOAT,
             dict(DOC = '''
                    Corresponds to Usd attribute metadata''')),
            ('offsets', T_VEC3FARRAY, {}),
            ('normalOffsets', T_VEC3FARRAY, {}),
        ],
    ),


    #--------------------------------------------------------------------------
    # usdSkelImaging/blendShape
    dict(
        SCHEMA_NAME = 'BlendShape',
        SCHEMA_TOKEN = 'skelBlendShape',
        ADD_DEFAULT_LOCATOR = True,
        DOC = '''Corresponds to UsdSkelBlendShape.''',
        SCHEMA_INCLUDES = [
            '{{LIBRARY_PATH}}/schemaTypeDefs'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),

            ('offsets', T_VEC3FARRAY, {}),
            ('normalOffsets', T_VEC3FARRAY, {}),
            ('pointIndices', T_INTARRAY, {}),

            ('inbetweenShapes', 'UsdSkelImagingInbetweenShapeContainerSchema', {})
        ],
    ),
]
