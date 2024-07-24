#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
[
    dict(
        SCHEMA_NAME = 'ALL_SCHEMAS',
        LIBRARY_PATH = 'pxr/usdImaging/usdImaging',
    ),        

    #--------------------------------------------------------------------------
    # usdImaging/usdPrimInfo
    dict(
        SCHEMA_NAME = 'UsdPrimInfo',
        SCHEMA_TOKEN = '__usdPrimInfo',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('niPrototypePath', T_PATH, dict(ADD_LOCATOR=True)),
            ('isNiPrototype', T_BOOL, {}),
            ('specifier', T_TOKEN, {}),
            ('piPropagatedPrototypes', T_CONTAINER, {}),
            ('isLoaded', T_BOOL, {}),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('specifier', ['def', 'over', '(class_, "class")']),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/model - corresponds to UsdModelAPI
    dict(
        SCHEMA_NAME = 'Model',
        SCHEMA_TOKEN = 'model',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('modelPath', T_PATH, {}),
            ('assetIdentifier', T_ASSETPATH, {}),
            ('assetName', T_STRING, {}),
            ('assetVersion', T_STRING, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/geomModel - corresponds to UsdGeomModelAPI
    dict(
        SCHEMA_NAME = 'GeomModel',
        SCHEMA_TOKEN = 'geomModel',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('drawMode', T_TOKEN, dict(ADD_LOCATOR=True)),
            ('applyDrawMode', T_BOOL, {}),
            ('drawModeColor', T_VEC3F, {}),
            ('cardGeometry', T_TOKEN, {}),
            ('cardTextureXPos', T_ASSETPATH, {}),
            ('cardTextureYPos', T_ASSETPATH, {}),
            ('cardTextureZPos', T_ASSETPATH, {}),
            ('cardTextureXNeg', T_ASSETPATH, {}),
            ('cardTextureYNeg', T_ASSETPATH, {}),
            ('cardTextureZNeg', T_ASSETPATH, {}),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('drawMode', [
                '(default_, "default")',
                'origin',
                'bounds',
                'cards',
                'inherited']),
            ('cardGeometry', [
                'cross',
                'box',
                'fromTexture']),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/directMaterialBinding - corresponds to UsdShadeMaterialBindingAPI::DirectBinding
    dict(
        SCHEMA_NAME = 'DirectMaterialBinding',
        SCHEMA_TOKEN = 'directMaterialBinding',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('materialPath', T_PATH, {}),
            ('bindingStrength', T_TOKEN, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/DirectMaterialBindings - corresponds to UsdShadeMaterialBindingAPI::DirectBinding
    dict(
        SCHEMA_NAME = 'DirectMaterialBindings',
        SCHEMA_TOKEN = 'directMaterialBindings',
        EXTRA_TOKENS = [
            '(allPurpose, "")',
        ],
        ADD_DEFAULT_LOCATOR = True,
        GENERIC_BUILD_RETAINED = True,
    ),

    #--------------------------------------------------------------------------
    # usdImaging/collectionMaterialBinding - corresponds to UsdShadeMaterialBindingAPI::CollectionBinding
    dict(
        SCHEMA_NAME = 'CollectionMaterialBinding',
        SCHEMA_TOKEN = 'collectionMaterialBinding',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('collectionPath', T_PATH, {}),
            ('materialPath', T_PATH, {}),
            ('bindingStrength', T_TOKEN, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/collectionMaterialBindings - corresponds to UsdShadeMaterialBindingAPI::CollectionBinding
    dict(
        SCHEMA_NAME = 'CollectionMaterialBindings',
        SCHEMA_TOKEN = 'collectionMaterialBindings',
        ADD_DEFAULT_LOCATOR = True,
        EXTRA_TOKENS = [
            '(allPurpose, "")',
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/usdImagingRenderSettings
    dict(
        SCHEMA_NAME = 'UsdRenderSettings',
        SCHEMA_TOKEN = '__usdRenderSettings',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR=True)),
            # UsdRenderSettingsBase
            ('resolution', T_VEC2I, {}),
            ('pixelAspectRatio', T_FLOAT, {}),
            ('aspectRatioConformPolicy', T_TOKEN, {}),
            ('dataWindowNDC', T_VEC4F, {}), # XXX T_RANGE2F
            ('disableMotionBlur', T_BOOL, {}),
            # note: instantaneousShutter is deprecated in favor of 
            # disableMotionBlur, so we skip it.
            ('disableDepthOfField', T_BOOL, {}),
            ('camera', T_PATH, {}),

            # UsdRenderSettings
            ('includedPurposes', T_TOKENARRAY, {}),
            ('materialBindingPurposes', T_TOKENARRAY, {}),
            ('renderingColorSpace', T_TOKEN, {}),
            ('products', T_PATHARRAY, {}),

            # note: namespacedSettings isn't in the USD schema.
            ('namespacedSettings', T_CONTAINER, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/usdImagingRenderProduct
    dict(
        SCHEMA_NAME = 'UsdRenderProduct',
        SCHEMA_TOKEN = '__usdRenderProduct',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            # UsdRenderSettingsBase
            ('resolution', T_VEC2I, {}),
            ('pixelAspectRatio', T_FLOAT, {}),
            ('aspectRatioConformPolicy', T_TOKEN, {}),
            ('dataWindowNDC', T_VEC4F, {}), # XXX T_RANGE2F
            ('disableMotionBlur', T_BOOL, {}),
            # note: instantaneousShutter is deprecated in favor of 
            # disableMotionBlur, so we skip it.
            ('disableDepthOfField', T_BOOL, {}),
            ('camera', T_PATH, {}),

            # UsdRenderProduct
            ('productType', T_TOKEN, {}),
            ('productName', T_TOKEN, {}),
            ('orderedVars', T_PATHARRAY, {}),

            # note: namespacedSettings isn't in the USD schema.
            ('namespacedSettings', T_CONTAINER, dict(ADD_LOCATOR=True)),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/usdImagingRenderVar
    dict(
        SCHEMA_NAME = 'UsdRenderVar',
        SCHEMA_TOKEN = '__usdRenderVar',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            # UsdRenderProduct
            ('dataType', T_TOKEN, {}),
            ('sourceName', T_STRING, {}),
            ('sourceType', T_TOKEN, {}),

            # note: namespacedSettings isn't in the USD schema.
            ('namespacedSettings', T_CONTAINER, dict(ADD_LOCATOR=True)),
        ],
    ),
]
