[
    #--------------------------------------------------------------------------
    # dependency
    dict(
        FILE_NAME = 'dependencySchema',
        SCHEMA_NAME = 'HdDependency',
        MEMBERS = [
            ('dependedOnPrimPath', T_PATH),
            ('dependedOnDataSourceLocator', T_LOCATOR),
            ('affectedDataSourceLocator', T_LOCATOR),
        ],
    ),

    #--------------------------------------------------------------------------
    # dependencies
    dict(
        FILE_NAME = 'dependenciesSchema',
        OTHER_SCHEMA_INCLUDES = ['dependency'],
        SCHEMA_NAME = 'HdDependencies',
        SCHEMA_TOKEN = "__dependencies",
        HEADER_TEMPLATE_NAME = "dependenciesSchema.template.h",
        IMPL_TEMPLATE_NAME = "dependenciesSchema.template.cpp",
        DEFAULT_LOCATOR = ["__dependencies"],
    ),

    #--------------------------------------------------------------------------
    # subdivisionTags
    dict(
        FILE_NAME = 'subdivisionTagsSchema',
        SCHEMA_NAME = 'HdSubdivisionTags',
        SCHEMA_TOKEN = 'subdivisionTags',
        IMPL_OTHER_SCHEMA_INCLUDES = ['mesh'],
        MEMBERS = [
            ('faceVaryingLinearInterpolation', T_TOKEN),
            ('interpolateBoundary', T_TOKEN),
            ('triangleSubdivisionRule', T_TOKEN),
            ('cornerIndices', T_INTARRAY),
            ('cornerSharpnesses', T_FLOATARRAY),
            ('creaseIndices', T_INTARRAY),
            ('creaseLengths', T_INTARRAY),
            ('creaseSharpnesses', T_FLOATARRAY),
        ],

        DEFAULT_LOCATOR = [
            ('HdMesh', 'mesh'),
            'subdivisionTags'
        ],
    ),

    #--------------------------------------------------------------------------
    # geomSubset
    dict(
        FILE_NAME = 'geomSubsetSchema',
        SCHEMA_NAME = 'HdGeomSubset',
        MEMBERS = [
            ('type', T_TOKEN),
            ('indices', T_INTARRAY),
        ],

        EXTRA_TOKENS = [
            'typeFaceSet',
            'typePointSet',
            'typeCurveSet',
        ],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('Type', ['typeFaceSet', 'typePointSet', 'typeCurveSet']),
        ],
    ),

    #--------------------------------------------------------------------------
    # geomSubsets
    dict(
        FILE_NAME = 'geomSubsetsSchema',
        SCHEMA_NAME = 'HdGeomSubsets',
        SCHEMA_TOKEN = 'geomSubsets',
        OTHER_SCHEMA_INCLUDES = ['geomSubset'],
        HEADER_TEMPLATE_NAME = "geomSubsetsSchema.template.h",
        IMPL_TEMPLATE_NAME = "geomSubsetsSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    # mesh
    dict(
        FILE_NAME = 'meshSchema',
        SCHEMA_NAME = 'HdMesh',
        SCHEMA_TOKEN = 'mesh',
        OTHER_SCHEMA_INCLUDES =
            ['meshTopology', 'subdivisionTags', 'geomSubsets'],
        
        MEMBERS = [
            ('topology', 'HdMeshTopologySchema'),
            ('subdivisionScheme', T_TOKEN),
            ('subdivisionTags', 'HdSubdivisionTagsSchema'),
            ('geomSubsets', 'HdGeomSubsetsSchema'),
            ('doubleSided', T_BOOL),
        ],

        DEFAULT_LOCATOR = ['mesh'],

        STATIC_LOCATOR_ACCESSORS = [
            ('Topology', ['mesh', 'topology']),
            ('GeomSubsets', ['mesh', 'geomSubsets']),
            ('DoubleSided', ['mesh', 'doubleSided']),
            ('SubdivisionTags', ['mesh', 'subdivisionTags']),
            ('SubdivisionScheme', ['mesh', 'subdivisionScheme']),
        ]
    ),

    #--------------------------------------------------------------------------
    # meshTopology
    dict(
        FILE_NAME = 'meshTopologySchema',
        SCHEMA_NAME = 'HdMeshTopology',
        SCHEMA_TOKEN = 'topology',
        OTHER_SCHEMA_INCLUDES = ['subdivisionTags'],
        MEMBERS = [
            ('faceVertexCounts', T_INTARRAY),
            ('faceVertexIndices', T_INTARRAY),
            ('holeIndices', T_INTARRAY),
            ('orientation', T_TOKEN),
        ],
        EXTRA_TOKENS = [
            'leftHanded',
            'rightHanded',
        ],
        IMPL_OTHER_SCHEMA_INCLUDES = ['mesh'],
        DEFAULT_LOCATOR = [('HdMesh', 'mesh'), 'topology'],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('Orientation', ['leftHanded', 'rightHanded']),
        ],
    ),

    #--------------------------------------------------------------------------
    # nurbsPatch
    dict(
        FILE_NAME = 'nurbsPatchSchema',
        SCHEMA_NAME = 'HdNurbsPatch',
        SCHEMA_TOKEN = 'nurbsPatch',
        OTHER_SCHEMA_INCLUDES =
            ['nurbsPatchTrimCurve'],
        MEMBERS = [
            ('uVertexCount', T_INT),
            ('vVertexCount', T_INT),
            ('uOrder', T_INT),
            ('vOrder', T_INT),
            ('uKnots', T_DOUBLEARRAY),
            ('vKnots', T_DOUBLEARRAY),
            ('uForm', T_TOKEN),
            ('vForm', T_TOKEN),
            ('uRange', T_VEC2D),
            ('vRange', T_VEC2D),
            ('trimCurve', 'HdNurbsPatchTrimCurveSchema'),
            ('orientation', T_TOKEN),
            ('doubleSided', T_BOOL),
        ],
        
        DEFAULT_LOCATOR = ['nurbsPatch'],

        EXTRA_TOKENS = [
            'open',
            'closed',
            'periodic',
            'leftHanded',
            'rightHanded'
        ],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('Form', ['open', 'closed', 'periodic']),
        ],
    ),

    #--------------------------------------------------------------------------
    # nurbsPatchTrimCurve
    dict(
        FILE_NAME = 'nurbsPatchTrimCurveSchema',
        SCHEMA_NAME = 'HdNurbsPatchTrimCurve',
        SCHEMA_TOKEN = 'trimCurve',
        MEMBERS = [
            ('counts', T_INTARRAY),
            ('orders', T_INTARRAY),
            ('vertexCounts', T_INTARRAY),
            ('knots', T_DOUBLEARRAY),
            ('ranges', T_VEC2DARRAY),
            ('points', T_VEC3DARRAY),
        ],
        IMPL_OTHER_SCHEMA_INCLUDES = ['nurbsPatch'],
        DEFAULT_LOCATOR = [('HdNurbsPatch', 'nurbsPatch'), 'trimCurve'],
    ),

    #--------------------------------------------------------------------------
    # basisCurves
    dict(
        FILE_NAME = 'basisCurvesSchema',
        SCHEMA_NAME = 'HdBasisCurves',
        SCHEMA_TOKEN = 'basisCurves',
        OTHER_SCHEMA_INCLUDES =
            ['basisCurvesTopology',  'geomSubsets'],
        
        MEMBERS = [
            ('topology', 'HdBasisCurvesTopologySchema'),
            ('geomSubsets', 'HdGeomSubsetsSchema'),
        ],

        DEFAULT_LOCATOR = ['basisCurves'],

        STATIC_LOCATOR_ACCESSORS = [
            ('Topology', ['basisCurves', 'topology']),
            ('GeomSubsets', ['basisCurves', 'geomSubsets']),
        ]

    ),

    #--------------------------------------------------------------------------
    # basisCurvesTopology
    dict(
        FILE_NAME = 'basisCurvesTopologySchema',
        SCHEMA_NAME = 'HdBasisCurvesTopology',
        SCHEMA_TOKEN = 'topology',
        IMPL_OTHER_SCHEMA_INCLUDES = ['basisCurves'],
        DEFAULT_LOCATOR = [
            ('HdBasisCurves', 'basisCurves'),
            'topology'],
        MEMBERS = [
            ('curveVertexCounts', T_INTARRAY),
            ('curveIndices', T_INTARRAY),
            ('basis', T_TOKEN),
            ('type', T_TOKEN),
            ('wrap', T_TOKEN),
        ],
    ),

    #--------------------------------------------------------------------------
    # nurbsCurves
    dict(
        FILE_NAME = 'nurbsCurvesSchema',
        SCHEMA_NAME = 'HdNurbsCurves',
        SCHEMA_TOKEN = 'nurbsCurves',
        MEMBERS = [
            ('curveVertexCounts', T_INTARRAY),
            ('order', T_INTARRAY),
            ('knots', T_DOUBLEARRAY),
            ('ranges', T_VEC2DARRAY),
        ],

        DEFAULT_LOCATOR = ['nurbsCurves']
    ),

    #--------------------------------------------------------------------------
    # primvar
    dict(
        FILE_NAME = 'primvarSchema',
        SCHEMA_NAME = 'HdPrimvar',
        MEMBERS = [
            ('primvarValue', T_SAMPLED),
            ('indexedPrimvarValue', T_SAMPLED),
            ('indices', T_INTARRAY),
            ('interpolation', T_TOKEN),
            ('role', T_TOKEN),
        ],
        EXTRA_TOKENS = [
            'constant', # interpolations
            'uniform',
            'varying',
            'vertex',
            'faceVarying',
            'instance',
            'point',    # roles matching sdf types, but with Hd's capitalization
            'normal',
            'vector',
            'color',
            'pointIndex',
            'edgeIndex',
            'faceIndex',
            'textureCoordinate',
            'transform',
        ],

        ACCESSOR_COMMENTS = dict(
            primvarValue = '''
                If the primvar does not have indices, GetPrimvarValue() and
                GetIndexedPrimvarValue() will return the same thing.
                If the primvar does has indices, GetPrimvarValue() will return the 
                flattened value, while GetIndexedPrimvarValue() will return the
                unflattened value.
                ''',

        ),

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('Interpolation', [
                'constant',
                'uniform',
                'varying',
                'vertex',
                'faceVarying',
                'instance',
                ]),
            ('Role', [
                'point',
                'normal',
                'vector',
                'color',
                'pointIndex',
                'edgeIndex',
                'faceIndex',
                'textureCoordinate',
                ]),
        ],
        HEADER_TEMPLATE_NAME = "primvarSchema.template.h",
        IMPL_TEMPLATE_NAME = "primvarSchema.template.cpp",

        ACCCESSOR_IMPL_OVERRIDES =      # optional for custom Get method 
        dict (   
            primvarValue = 'primvarSchemaGetValue.template.cpp',
            indexedPrimvarValue = 'primvarSchemaGetIndexedValue.template.cpp',
        ),

    ),

    #--------------------------------------------------------------------------
    # primvars
    dict(
        FILE_NAME = 'primvarsSchema',
        SCHEMA_NAME = 'HdPrimvars',
        SCHEMA_TOKEN = 'primvars',
        DEFAULT_LOCATOR = ['primvars'],
        STATIC_LOCATOR_ACCESSORS = [
            ('Points', ['primvars', 'points']),
            ('Normals', ['primvars', 'normals']),
            ('Widths', ['primvars', 'widths']),
        ],
        EXTRA_TOKENS = [
            'points',
            'normals',
            'widths',
        ],
        HEADER_TEMPLATE_NAME = "primvarsSchema.template.h",
        IMPL_TEMPLATE_NAME = "primvarsSchema.template.cpp",

        OTHER_SCHEMA_INCLUDES = ['primvar'],
    ),

    #--------------------------------------------------------------------------
    # xform
    dict(
        FILE_NAME = 'xformSchema',
        SCHEMA_NAME = 'HdXform',
        SCHEMA_TOKEN = 'xform',
        MEMBERS = [
            ('matrix', T_MATRIX),
            ('resetXformStack', T_BOOL),
        ],
        ACCESSOR_COMMENTS = dict(
            resetXformStack = '''
                The "resetXformStack" flag tells consumers that this transform
                doesn't inherit from the parent prim's transform.
                ''',

        ),

        DEFAULT_LOCATOR = ['xform'],
    ),

    #--------------------------------------------------------------------------
    # visibility
    dict(
        FILE_NAME = 'visibilitySchema',
        SCHEMA_NAME = 'HdVisibility',
        SCHEMA_TOKEN = 'visibility',
        MEMBERS = [
            ('visibility', T_BOOL),
        ],
        DEFAULT_LOCATOR = ['visibility'],
    ),

    #--------------------------------------------------------------------------
    # purpose
    dict(
        FILE_NAME = 'purposeSchema',
        SCHEMA_NAME = 'HdPurpose',
        SCHEMA_TOKEN = 'purpose',
        MEMBERS = [
            ('purpose', T_TOKEN),
        ],
        DEFAULT_LOCATOR = ['purpose'],
    ),

    #--------------------------------------------------------------------------
    # extent
    dict(
        FILE_NAME = 'extentSchema',
        SCHEMA_NAME = 'HdExtent',
        SCHEMA_TOKEN = 'extent',
        MEMBERS = [
            ('min', T_VEC3D),
            ('max', T_VEC3D),
            
        ],
        DEFAULT_LOCATOR = ['extent'],
    ),

    #--------------------------------------------------------------------------
    # materialNode
    dict(
        FILE_NAME = 'materialNodeSchema',
        SCHEMA_NAME = 'HdMaterialNode',
        MEMBERS = [
            ('parameters', T_CONTAINER),
            ('inputConnections', T_CONTAINER),
            ('nodeIdentifier', T_TOKEN),
            ('renderContextNodeIdentifiers', T_CONTAINER),
            ('nodeTypeInfo', T_CONTAINER)
        ],
        ACCESSOR_COMMENTS = dict(
            nodeIdentifier = '''
                This identifies the shader the node represents. The
                renderContextNodeIdentifier container can store alternative
                values for this. A consumer which is interested in a specific
                render context should check for that token within
                renderContextNodeIdentifiers and fall back on this value in its
                absence.
            ''',
            renderContextNodeIdentifiers = '''
                A shading node can hold a nodeIdentifier value for multiple
                render contexts at once. This allows multiple renderer target
                representations to coexist in the same renderable scene. The
                contents of this container are alternate possible values for
                nodeIdentifier.
                A consumer which is interested in a specific
                render context should check for that token within
                this container and fall back on nodeIdentifier in its
                absence.
                ''',
            nodeTypeInfo = '''
                Rather than having an identifier, a shader can be specified
                by other information.
                ''',
        ),
    ),

    #--------------------------------------------------------------------------
    # materialNodeParameter
    dict(
        FILE_NAME = 'materialNodeParameterSchema',
        SCHEMA_NAME = 'HdMaterialNodeParameter',
        MEMBERS = [
            ('value', T_SAMPLED),
            # Parameter Metadata
            ('colorSpace', T_TOKEN),
        ],
    ),

    #--------------------------------------------------------------------------
    # materialConnection
    dict(
        FILE_NAME = 'materialConnectionSchema',
        SCHEMA_NAME = 'HdMaterialConnection',
        MEMBERS = [
            ('upstreamNodePath', T_TOKEN),
            ('upstreamNodeOutputName', T_TOKEN),
        ],
    ),

    #--------------------------------------------------------------------------
    # materialNetwork
    dict(
        FILE_NAME = 'materialNetworkSchema',
        SCHEMA_NAME = 'HdMaterialNetwork',
        MEMBERS = [
            ('nodes', T_CONTAINER),
            ('terminals', T_CONTAINER),
        ],
    ),

    #--------------------------------------------------------------------------
    # material
    dict(
        FILE_NAME = 'materialSchema',
        SCHEMA_NAME = 'HdMaterial',
        SCHEMA_TOKEN = 'material',
        EXTRA_TOKENS = [
            '(universalRenderContext, "")',
        ],
        DEFAULT_LOCATOR = ['material'],

        GENERIC_BUILD_RETAINED = True,
        HEADER_TEMPLATE_NAME = "materialSchema.template.h",
        IMPL_TEMPLATE_NAME = "materialSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    # materialBinding
    dict(
        FILE_NAME = 'materialBindingSchema',
        SCHEMA_NAME = 'HdMaterialBinding',
        MEMBERS = [
            ('path', T_PATH),
            ('bindingStrength', T_TOKEN),
        ],
    ),

    #--------------------------------------------------------------------------
    # materialBindings
    dict(
        FILE_NAME = 'materialBindingsSchema',
        SCHEMA_NAME = 'HdMaterialBindings',
        SCHEMA_TOKEN = 'materialBindings',
        EXTRA_TOKENS = [
            '(allPurpose, "")',
        ],
        DEFAULT_LOCATOR = ['materialBindings'],
        GENERIC_BUILD_RETAINED = True,
        HEADER_TEMPLATE_NAME = "materialBindingsSchema.template.h",
        IMPL_TEMPLATE_NAME = "materialBindingsSchema.template.cpp",
        OTHER_SCHEMA_INCLUDES = ['materialBinding'],
    ),

    #--------------------------------------------------------------------------
    # volumeFieldBinding
    dict(
        FILE_NAME = 'volumeFieldBindingSchema',
        SCHEMA_NAME = 'HdVolumeFieldBinding',
        SCHEMA_TOKEN = 'volumeFieldBinding',
        DEFAULT_LOCATOR = ['volumeFieldBinding'],
        GENERIC_BUILD_RETAINED = True,
        HEADER_TEMPLATE_NAME = "volumeFieldBindingSchema.template.h",
        IMPL_TEMPLATE_NAME = "volumeFieldBindingSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    # coordSysBinding
    dict(
        FILE_NAME = 'coordSysBindingSchema',
        SCHEMA_NAME = 'HdCoordSysBinding',
        SCHEMA_TOKEN = 'coordSysBinding',
        DEFAULT_LOCATOR = ['coordSysBinding'],
        GENERIC_BUILD_RETAINED = True,
        HEADER_TEMPLATE_NAME = "coordSysBindingSchema.template.h",
        IMPL_TEMPLATE_NAME = "coordSysBindingSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    # coordSys
    dict(
        FILE_NAME = 'coordSysSchema',
        SCHEMA_NAME = 'HdCoordSys',
        SCHEMA_TOKEN = 'coordSys',
        MEMBERS = [
            ('name', T_TOKEN),
        ],
        DEFAULT_LOCATOR = ['coordSys'],
    ),

    #--------------------------------------------------------------------------
    # instancedBy
    dict(
        FILE_NAME = 'instancedBySchema',
        SCHEMA_NAME = 'HdInstancedBy',
        SCHEMA_TOKEN = 'instancedBy',
        DEFAULT_LOCATOR = ['instancedBy'],
        MEMBERS = [
            ('paths', T_PATHARRAY),
            ('prototypeRoots', T_PATHARRAY),
        ],
        HEADER_TEMPLATE_NAME = "instancedBySchema.template.h",
    ),

    #--------------------------------------------------------------------------
    # instance
    dict(
        FILE_NAME = 'instanceSchema',
        SCHEMA_NAME = 'HdInstance',
        SCHEMA_TOKEN = 'instance',
        DEFAULT_LOCATOR = ['instance'],
        MEMBERS = [
            ('instancer', T_PATH),
            ('prototypeIndex', T_INT),
            ('instanceIndex', T_INT),
        ],
        ACCESSOR_COMMENTS = dict(
            instancer = '''Path to instancer for which a (sub-)entry was added to
                           its instancerTopology's instanceIndices during
                           instance aggregation to account for this instance.
                           Note that instanceIndices is nested, that is a vector
                           data source containing integer arrays, one for each
                           prototype the instancer is instancing. Thus, we need
                           two indices to identify the entry: prototypeIndex is the
                           outer index and instanceIndex the inner index.''',
            prototypeIndex = '''Index into vector data source at
                             instancer's instancerTopology's instanceIndices
                             to find entry corresponding to this instance.''',
            instanceIndex = '''Index into int array within the vector data source at
                             instancer's instancerTopology's instanceIndices
                             to find entry corresponding to this instance.''',
        ),
        HEADER_TEMPLATE_NAME = "instanceSchema.template.h",
    ),

    #--------------------------------------------------------------------------
    # instancerTopology
    dict(
        FILE_NAME = 'instancerTopologySchema',
        SCHEMA_NAME = 'HdInstancerTopology',
        SCHEMA_TOKEN = 'instancerTopology',
        DEFAULT_LOCATOR = ['instancerTopology'],
        MEMBERS = [
            ('prototypes', T_PATHARRAY),
            ('instanceIndices', 'HdIntArrayVectorSchema'),
            ('mask', T_BOOLARRAY),
            ('instanceLocations', T_PATHARRAY),

        ],
        HEADER_TEMPLATE_NAME = "instancerTopologySchema.template.h",
        IMPL_TEMPLATE_NAME = "instancerTopologySchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    # legacyDisplayStyle
    dict(
        FILE_NAME = 'legacyDisplayStyleSchema',
        SCHEMA_NAME = 'HdLegacyDisplayStyle',
        SCHEMA_TOKEN = 'displayStyle',
        DEFAULT_LOCATOR = ['displayStyle'],
        MEMBERS = [
            ('refineLevel', T_INT),
            ('flatShadingEnabled', T_BOOL),
            ('displacementEnabled', T_BOOL),
            ('occludedSelectionShowsThrough', T_BOOL),
            ('pointsShadingEnabled', T_BOOL),
            ('materialIsFinal', T_BOOL),
            ('shadingStyle', T_TOKEN),
            ('reprSelector', T_TOKENARRAY),
            ('cullStyle', T_TOKEN),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('ReprSelector', ['displayStyle', 'reprSelector']),
            ('CullStyle', ['displayStyle', 'cullStyle']),
        ],
    ),

    #--------------------------------------------------------------------------
    # light
    dict(
        FILE_NAME = 'lightSchema',
        SCHEMA_NAME = 'HdLight',
        SCHEMA_TOKEN = 'light',
        DEFAULT_LOCATOR = ['light'],
    ),

    #--------------------------------------------------------------------------
    # imageShader
    dict(
        FILE_NAME = 'imageShaderSchema',
        SCHEMA_NAME = 'HdImageShader',
        SCHEMA_TOKEN = 'imageShader',
        DEFAULT_LOCATOR = ['imageShader'],
        MEMBERS = [
            ('enabled', T_BOOL),
            ('priority', T_INT),
            ('filePath', T_ASSETPATH),
            ('constants', T_CONTAINER),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('Enabled', ['imageShader', 'enabled']),
            ('Priority', ['imageShader', 'priority']),
            ('FilePath', ['imageShader', 'filePath']),
            ('Constants', ['imageShader', 'constants']),
        ],
    ),

    #--------------------------------------------------------------------------
    # renderBuffer
    dict(
        FILE_NAME = 'renderBufferSchema',
        SCHEMA_NAME = 'HdRenderBuffer',
        SCHEMA_TOKEN = 'renderBuffer',
        MEMBERS = [
            ('dimensions', T_VEC3I),
            ('format', T_FORMAT),
            ('multiSampled', T_BOOL),
        ],
        DEFAULT_LOCATOR = ['renderBuffer'],
    ),

    #--------------------------------------------------------------------------
    # renderSettings
    dict(
        FILE_NAME = 'renderSettingsSchema',
        SCHEMA_NAME = 'HdRenderSettings',
        SCHEMA_TOKEN = 'renderSettings',
        DEFAULT_LOCATOR = ['renderSettings'],
        MEMBERS = [
            ('namespacedSettings', T_CONTAINER),
            ('active', T_BOOL),
            ('renderProducts', 'HdRenderProductVectorSchema'),
            ('includedPurposes', T_TOKENARRAY),
            ('materialBindingPurposes', T_TOKENARRAY),
            ('renderingColorSpace', T_TOKEN),
            ('shutterInterval', T_VEC2D)
        ],
        ACCESSOR_COMMENTS = dict(
            shutterInterval = '''
            Frame-relative time interval representing the sampling window for 
            data relevant to motion blur. Renderers can use this interval when
            querying time-sampled data (e.g., xforms, points, velocities, ...)
            to simulate motion blur effects.
            
            Note: This closely relates to the (frame-relative) shutter interval
                  of a camera specified via shutter open and close times and is
                  expected to span the union of the shutter intervals of cameras
                  used in generating the render artifacts.'''
        ),
        STATIC_LOCATOR_ACCESSORS = [
            ('NamespacedSettings', ['renderSettings', 'namespacedSettings']),
            ('Active', ['renderSettings', 'active']),
            ('RenderProducts', ['renderSettings', 'renderProducts']),
            ('IncludedPurposes', ['renderSettings', 'includedPurposes']),
            ('MaterialBindingPurposes', ['renderSettings', 'materialBindingPurposes']),
            ('RenderingColorSpace', ['renderSettings', 'renderingColorSpace']),
            ('ShutterInterval', ['renderSettings', 'shutterInterval']),
        ],
        
    ),

    #--------------------------------------------------------------------------
    # renderProduct
    dict(
        FILE_NAME = 'renderProductSchema',
        SCHEMA_NAME = 'HdRenderProduct',
        SCHEMA_TOKEN = 'renderProduct',
        DEFAULT_LOCATOR = ['renderProduct'],
        MEMBERS = [
            ['path', T_PATH], # scene prim path
            ('type', T_TOKEN),
            ('name', T_TOKEN), # output name
            ('resolution', T_VEC2I),
            ('renderVars', 'HdRenderVarVectorSchema'),
            # Camera & Framing
            ('cameraPrim', T_PATH),
            ('pixelAspectRatio', T_FLOAT),
            ('aspectRatioConformPolicy', T_TOKEN),
            ('apertureSize', T_VEC2F),
            ('dataWindowNDC', T_VEC4F), # XXX T_RANGE2F
            # Product specific overrides
            ('disableMotionBlur', T_BOOL),
            ('namespacedSettings', T_CONTAINER),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('Resolution', ['renderProduct', 'resolution']),
            ('RenderVars', ['renderProduct', 'renderVars']),
            ('NamespacedSettings', ['renderProduct', 'namespacedSettings']),
            # XXX Add remaining as and when necessary.
        ],
    ),

    #--------------------------------------------------------------------------
    # renderVar
    dict(
        FILE_NAME = 'renderVarSchema',
        SCHEMA_NAME = 'HdRenderVar',
        SCHEMA_TOKEN = 'renderVar',
        DEFAULT_LOCATOR = ['renderVar'],
        MEMBERS = [
            ['path', T_PATH], # scene prim path
            ('dataType', T_TOKEN),
            ('sourceName', T_TOKEN),
            ('sourceType', T_TOKEN),
            # Var specific overrides
            ('namespacedSettings', T_CONTAINER),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('NamespacedSettings', ['renderVar', 'namespacedSettings']),
            # XXX Add remaining as and when necessary.
        ],
    ),

    #--------------------------------------------------------------------------
    # integrator
    dict(
        FILE_NAME = 'integratorSchema',
        SCHEMA_NAME = 'HdIntegrator',
        SCHEMA_TOKEN = 'integrator',
        OTHER_SCHEMA_INCLUDES = ['materialNode'],
        MEMBERS = [
            ('resource', 'HdMaterialNodeSchema'),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('Resource', ['integrator', 'resource']),
        ],
        DEFAULT_LOCATOR = ['integrator'],
    ),

    #--------------------------------------------------------------------------
    # sampleFilter
    dict(
        FILE_NAME = 'sampleFilterSchema',
        SCHEMA_NAME = 'HdSampleFilter',
        SCHEMA_TOKEN = 'sampleFilter',
        OTHER_SCHEMA_INCLUDES = ['materialNode'],
        MEMBERS = [
            ('resource', 'HdMaterialNodeSchema'),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('Resource', ['sampleFilter', 'resource']),
        ],
        DEFAULT_LOCATOR = ['sampleFilter'],
    ),

    #--------------------------------------------------------------------------
    # displayFilter
    dict(
        FILE_NAME = 'displayFilterSchema',
        SCHEMA_NAME = 'HdDisplayFilter',
        SCHEMA_TOKEN = 'displayFilter',
        OTHER_SCHEMA_INCLUDES = ['materialNode'],
        MEMBERS = [
            ('resource', 'HdMaterialNodeSchema'),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('Resource', ['displayFilter', 'resource']),
        ],
        DEFAULT_LOCATOR = ['displayFilter'],
    ),

    #--------------------------------------------------------------------------
    # system
    dict(
        FILE_NAME = 'systemSchema',
        SCHEMA_NAME = 'HdSystem',
        SCHEMA_TOKEN = 'system',
        DEFAULT_LOCATOR = ['system'],
        HEADER_TEMPLATE_NAME = "systemSchema.template.h",
        IMPL_TEMPLATE_NAME = "systemSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    # volumeField
    dict(
        FILE_NAME = 'volumeFieldSchema',
        SCHEMA_NAME = 'HdVolumeField',
        SCHEMA_TOKEN = 'volumeField',
        DEFAULT_LOCATOR = ['volumeField'],
        MEMBERS = [
            ('filePath', T_ASSETPATH),
            ('fieldName', T_TOKEN),
            ('fieldIndex', T_INT),
            ('fieldDataType', T_TOKEN),
            ('vectorDataRoleHint', T_TOKEN),
        ],
    ),

    #--------------------------------------------------------------------------
    # camera
    dict(
        FILE_NAME = 'cameraSchema',
        SCHEMA_NAME = 'HdCamera',
        SCHEMA_TOKEN = 'camera',
        OTHER_SCHEMA_INCLUDES = ['splitDiopter', 'lensDistortion'],
        DEFAULT_LOCATOR = ['camera'],
        MEMBERS = [
            ('projection', T_TOKEN),
            ('horizontalAperture', T_FLOAT),
            ('verticalAperture', T_FLOAT),
            ('horizontalApertureOffset', T_FLOAT),
            ('verticalApertureOffset', T_FLOAT),
            ('focalLength', T_FLOAT),
            ('clippingRange', T_VEC2F),
            ('clippingPlanes', T_VEC4DARRAY),
            ('fStop', T_FLOAT),
            ('focusDistance', T_FLOAT),
            ('shutterOpen', T_DOUBLE),
            ('shutterClose', T_DOUBLE),
            ('exposure', T_FLOAT),
            ('focusOn', T_BOOL),
            ('dofAspect', T_FLOAT),
            ('splitDiopter', 'HdSplitDiopterSchema'),
            ('lensDistortion', 'HdLensDistortionSchema')
        ],
        EXTRA_TOKENS = [
            'perspective',
            'orthographic',
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('Projection', ['perspective', 'orthographic']),
        ],
    ),

    #--------------------------------------------------------------------------
    # splitDiopter
    dict(
        FILE_NAME = 'splitDiopterSchema',
        SCHEMA_NAME = 'HdSplitDiopter',
        SCHEMA_TOKEN = 'splitDiopter',
        MEMBERS = [
            ('count', T_INT),
            ('angle', T_FLOAT),
            ('offset1', T_FLOAT),
            ('width1', T_FLOAT),
            ('focusDistance1', T_FLOAT),
            ('offset2', T_FLOAT),
            ('width2', T_FLOAT),
            ('focusDistance2', T_FLOAT),
        ],
        IMPL_OTHER_SCHEMA_INCLUDES = ['camera'],
        DEFAULT_LOCATOR = [('HdCamera', 'camera'), 'splitDiopter'],
    ),

    #--------------------------------------------------------------------------
    # lensDistortion
    dict(
        FILE_NAME = 'lensDistortionSchema',
        SCHEMA_NAME = 'HdLensDistortion',
        SCHEMA_TOKEN = 'lensDistortion',
        MEMBERS = [
            ('type', T_TOKEN),
            ('k1', T_FLOAT),
            ('k2', T_FLOAT),
            ('center', T_VEC2F),
            ('anaSq', T_FLOAT),
            ('asym', T_VEC2F),
            ('scale', T_FLOAT),
            ('ior', T_FLOAT),
        ],
        EXTRA_TOKENS = [
            'standard',
            'fisheye',
        ],
        IMPL_OTHER_SCHEMA_INCLUDES = ['camera'],
        DEFAULT_LOCATOR = [('HdCamera', 'camera'), 'lensDistortion'],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('Type', ['standard', 'fisheye']),
        ],
    ),

    #--------------------------------------------------------------------------
    # categories
    dict(
        FILE_NAME = 'categoriesSchema',
        SCHEMA_NAME = 'HdCategories',
        SCHEMA_TOKEN = 'categories',
        DEFAULT_LOCATOR = ['categories'],

        HEADER_TEMPLATE_NAME = "categoriesSchema.template.h",
        IMPL_TEMPLATE_NAME = "categoriesSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    # instanceCategories
    dict(
        FILE_NAME = 'instanceCategoriesSchema',
        SCHEMA_NAME = 'HdInstanceCategories',
        SCHEMA_TOKEN = 'instanceCategories',
        DEFAULT_LOCATOR = ['instanceCategories'],
        MEMBERS = [
            ('categoriesValues', 'HdVectorDataSource'),
        ],
    ),

    #--------------------------------------------------------------------------
    # extcomputation_primvar
    dict(
        FILE_NAME = 'extComputationPrimvarSchema',
        SCHEMA_NAME = 'HdExtComputationPrimvar',
        OTHER_SCHEMA_INCLUDES = ['primvar'],
        MEMBERS = [
            ('interpolation', T_TOKEN),
            ('role', T_TOKEN),
            ('sourceComputation', T_PATH),
            ('sourceComputationOutputName', T_TOKEN),
            ('valueType', T_TUPLE),
        ],
        HEADER_TEMPLATE_NAME = "extComputationPrimvarSchema.template.h",
    ),

    #--------------------------------------------------------------------------
    # extcomputation_primvars
    dict(
        FILE_NAME = 'extComputationPrimvarsSchema',
        SCHEMA_NAME = 'HdExtComputationPrimvars',
        SCHEMA_TOKEN = 'extComputationPrimvars',
        DEFAULT_LOCATOR = ['extComputationPrimvars'],
        OTHER_SCHEMA_INCLUDES = ['extComputationPrimvar'],

        HEADER_TEMPLATE_NAME = "extComputationPrimvarsSchema.template.h",
        IMPL_TEMPLATE_NAME = "extComputationPrimvarsSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    # extcomputation_inputComputation
    dict(
        FILE_NAME = 'extComputationInputComputationSchema',
        SCHEMA_NAME = 'HdExtComputationInputComputation',
        MEMBERS = [
            ('name', T_TOKEN),
            ('sourceComputation', T_PATH),
            ('sourceComputationOutputName', T_TOKEN),
        ],
    ),

    #--------------------------------------------------------------------------
    # extcomputation_output
    dict(
        FILE_NAME = 'extComputationOutputSchema',
        SCHEMA_NAME = 'HdExtComputationOutput',
        MEMBERS = [
            ('name', T_TOKEN),
            ('valueType', T_TUPLE),
        ],
    ),

    #--------------------------------------------------------------------------
    # extcomputation
    dict(
        FILE_NAME = 'extComputationSchema',
        SCHEMA_NAME = 'HdExtComputation',
        SCHEMA_TOKEN = 'extComputation',
        DEFAULT_LOCATOR = ['extComputation'],
        MEMBERS = [
            ('inputValues', T_CONTAINER),
            ('inputComputations', T_VECTOR),
            ('outputs', T_VECTOR),
            ('glslKernel', T_STRING),
            ('cpuCallback', T_BASE),
            ('dispatchCount', T_SIZET),
            ('elementCount', T_SIZET),
        ],

        STATIC_LOCATOR_ACCESSORS = [
            ('InputValues', ['extComputation', 'inputValues']),
            ('InputComputations', ['extComputation', 'inputComputations']),
            ('Outputs', ['extComputation', 'outputs']),
            ('DispatchCount', ['extComputation', 'dispatchCount']),
            ('ElementCount', ['extComputation', 'elementCount']),
            ('GlslKernel', ['extComputation', 'glslKernel']),
        ],
    ),
    
    #--------------------------------------------------------------------------
    # cube
    dict(
        FILE_NAME = 'cubeSchema',
        SCHEMA_NAME = 'HdCube',
        SCHEMA_TOKEN = 'cube',
        DEFAULT_LOCATOR = ['cube'],
        MEMBERS = [
            ('size', T_DOUBLE)
        ],
    ),

    #--------------------------------------------------------------------------
    # sphere
    dict(
        FILE_NAME = 'sphereSchema',
        SCHEMA_NAME = 'HdSphere',
        SCHEMA_TOKEN = 'sphere',
        DEFAULT_LOCATOR = ['sphere'],
        MEMBERS = [
            ('radius', T_DOUBLE)
        ],
    ),

    #--------------------------------------------------------------------------
    # cone
    dict(
        FILE_NAME = 'coneSchema',
        SCHEMA_NAME = 'HdCone',
        SCHEMA_TOKEN = 'cone',
        DEFAULT_LOCATOR = ['cone'],
        MEMBERS = [
            ('height', T_DOUBLE),
            ('radius', T_DOUBLE),
            ('axis', T_TOKEN),
        ],

        EXTRA_TOKENS = [
            'X',
            'Y',
            'Z',
        ],
    ),

    #--------------------------------------------------------------------------
    # cylinder
    dict(
        FILE_NAME = 'cylinderSchema',
        SCHEMA_NAME = 'HdCylinder',
        SCHEMA_TOKEN = 'cylinder',
        DEFAULT_LOCATOR = ['cylinder'],
        MEMBERS = [
            ('height', T_DOUBLE),
            # deprecated in favor of radiusTop and radiusBottom
            ('radius', T_DOUBLE), 
            ('radiusTop', T_DOUBLE),
            ('radiusBottom', T_DOUBLE),
            ('axis', T_TOKEN),
        ],

        EXTRA_TOKENS = [
            'X',
            'Y',
            'Z',
        ],
    ),

    #--------------------------------------------------------------------------
    # capsule
    dict(
        FILE_NAME = 'capsuleSchema',
        SCHEMA_NAME = 'HdCapsule',
        SCHEMA_TOKEN = 'capsule',
        DEFAULT_LOCATOR = ['capsule'],
        MEMBERS = [
            ('height', T_DOUBLE),
            # deprecated in favor of radiusTop and radiusBottom
            ('radius', T_DOUBLE), 
            ('radiusTop', T_DOUBLE),
            ('radiusBottom', T_DOUBLE),
            ('axis', T_TOKEN),
        ],

        EXTRA_TOKENS = [
            'X',
            'Y',
            'Z',
        ],

    ),

    #--------------------------------------------------------------------------
    # primOrigin
    dict(
        FILE_NAME = 'primOriginSchema',
        SCHEMA_NAME = 'HdPrimOrigin',
        SCHEMA_TOKEN = 'primOrigin',
        DEFAULT_LOCATOR = ['primOrigin'],
        STATIC_LOCATOR_ACCESSORS = [
            ('ScenePath', ['primOrigin', 'scenePath']),
        ],
        EXTRA_TOKENS = [
            'scenePath',
        ],
        HEADER_TEMPLATE_NAME = "primOriginSchema.template.h",
        IMPL_TEMPLATE_NAME = "primOriginSchema.template.cpp"
    ),

    #--------------------------------------------------------------------------
    # selection
    dict(
        FILE_NAME = 'selectionSchema',
        SCHEMA_NAME = 'HdSelection',
        MEMBERS = [
            ('fullySelected', T_BOOL),
            ('nestedInstanceIndices', 'HdInstanceIndicesVectorSchema'),
        ],
        ACCESSOR_COMMENTS = dict(
            nestedInstanceIndices = '''Starting with the outer most, list for each
                                       nesting level of instancing what instances
                                       are selected.''',
        ),
    ),

    dict(
        FILE_NAME = 'instanceIndicesSchema',
        SCHEMA_NAME = 'HdInstanceIndices',
        MEMBERS = [
            ('instancer', T_PATH),
            ('prototypeIndex', T_INT),
            ('instanceIndices', T_INTARRAY),
        ],
    ),

    #--------------------------------------------------------------------------
    # sceneGlobals
    dict(
        FILE_NAME = 'sceneGlobalsSchema',
        SCHEMA_NAME = 'HdSceneGlobals',
        SCHEMA_TOKEN = 'sceneGlobals',
        DEFAULT_LOCATOR = ['sceneGlobals'],
        MEMBERS = [
            ('activeRenderSettingsPrim', T_PATH)
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('ActiveRenderSettingsPrim', ['sceneGlobals', 'activeRenderSettingsPrim']),
        ],
        HEADER_TEMPLATE_NAME = "sceneGlobalsSchema.template.h",
        IMPL_TEMPLATE_NAME = "sceneGlobalsSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    #--------------------------------------------------------------------------
    #
    # hdar
    #
    #

    #--------------------------------------------------------------------------
    # hdar/system
    dict(
        # these eventually go into hdar so we provide the library path here
        LIBRARY_PATH = 'pxr/imaging/hdar',
        FILE_NAME = 'systemSchema',
        SCHEMA_NAME = 'HdarSystem',
        SCHEMA_TOKEN = 'assetResolution',
        DEFAULT_LOCATOR = [('HdSystem', 'system'), 'assetResolution'],
        MEMBERS = [
            ('resolverContext', T_RESOLVERCONTEXT),
        ],
        IMPL_OTHER_SCHEMA_INCLUDES = ['system'],
        HEADER_TEMPLATE_NAME = "hdar_systemSchema.template.h",
        IMPL_TEMPLATE_NAME = "hdar_systemSchema.template.cpp",
    ),

    #--------------------------------------------------------------------------
    #--------------------------------------------------------------------------
    #
    # usdImaging
    #
    #

    #--------------------------------------------------------------------------
    # usdImaging/usdPrimInfo
    dict(
        LIBRARY_PATH = 'pxr/usdImaging/usdImaging',
        FILE_NAME = 'usdPrimInfoSchema',
        SCHEMA_NAME = 'UsdImagingUsdPrimInfo',
        SCHEMA_TOKEN = '__usdPrimInfo',
        DEFAULT_LOCATOR = ['__usdPrimInfo'],
        MEMBERS = [
            ('niPrototypePath', T_PATH),
            ('isNiPrototype', T_BOOL),
            ('specifier', T_TOKEN),
            ('piPropagatedPrototypes', T_CONTAINER),
            ('isLoaded', T_BOOL),
        ],
        EXTRA_TOKENS = [
            'def',
            'over',
            '(class_, "class")'
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('NiPrototypePath', ['__usdPrimInfo', 'niPrototypePath']),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/model - corresponds to UsdModelAPI
    dict(
        LIBRARY_PATH = 'pxr/usdImaging/usdImaging',
        FILE_NAME = 'modelSchema',
        SCHEMA_NAME = 'UsdImagingModel',
        SCHEMA_TOKEN = 'model',
        DEFAULT_LOCATOR = ['model'],
        MEMBERS = [
            ('modelPath', T_PATH),
            ('kind', T_TOKEN),
            ('assetIdentifier', T_ASSETPATH),
            ('assetName', T_STRING),
            ('assetVersion', T_STRING),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/geomModel - corresponds to UsdGeomModelAPI
    dict(
        LIBRARY_PATH = 'pxr/usdImaging/usdImaging',
        FILE_NAME = 'geomModelSchema',
        SCHEMA_NAME = 'UsdImagingGeomModel',
        SCHEMA_TOKEN = 'geomModel',
        DEFAULT_LOCATOR = ['geomModel'],
        MEMBERS = [
            ('drawMode', T_TOKEN),
            ('applyDrawMode', T_BOOL),
            ('drawModeColor', T_VEC3F),
            ('cardGeometry', T_TOKEN),
            ('cardTextureXPos', T_ASSETPATH),
            ('cardTextureYPos', T_ASSETPATH),
            ('cardTextureZPos', T_ASSETPATH),
            ('cardTextureXNeg', T_ASSETPATH),
            ('cardTextureYNeg', T_ASSETPATH),
            ('cardTextureZNeg', T_ASSETPATH),
        ],
        EXTRA_TOKENS = [
            'inherited',
            'origin',
            'bounds',
            'cards',
            '(default_, "default")',
            'cross',
            'box',
            'fromTexture'
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('DrawMode', ['geomModel', 'drawMode']),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/usdImagingRenderSettings
    dict(
        LIBRARY_PATH = 'pxr/usdImaging/usdImaging',
        FILE_NAME = 'usdRenderSettingsSchema',
        SCHEMA_NAME = 'UsdImagingUsdRenderSettings',
        SCHEMA_TOKEN = '__usdRenderSettings',
        DEFAULT_LOCATOR = ['__usdRenderSettings'],
        MEMBERS = [
            # UsdRenderSettingsBase
            ('resolution', T_VEC2I),
            ('pixelAspectRatio', T_FLOAT),
            ('aspectRatioConformPolicy', T_TOKEN),
            ('dataWindowNDC', T_VEC4F), # XXX T_RANGE2F
            ('disableMotionBlur', T_BOOL),
            # note: instantaneousShutter is deprecated in favor of 
            # disableMotionBlur, so we skip it.
            ('camera', T_PATH),

            # UsdRenderSettings
            ('includedPurposes', T_TOKENARRAY),
            ('materialBindingPurposes', T_TOKENARRAY),
            ('renderingColorSpace', T_TOKEN),
            ('products', T_PATHARRAY),

            # note: namespacedSettings isn't in the USD schema.
            ('namespacedSettings', T_CONTAINER),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('Resolution', ['__usdRenderSettings', 'resolution']),
            ('PixelAspectRatio', ['__usdRenderSettings', 'pixelAspectRatio']),
            ('AspectRatioConformPolicy', ['__usdRenderSettings', 'aspectRatioConformPolicy']),
            ('DataWindowNDC', ['__usdRenderSettings', 'dataWindowNDC']),
            ('DisableMotionBlur', ['__usdRenderSettings', 'disableMotionBlur']),
            ('Camera', ['__usdRenderSettings', 'camera']),
            ('IncludedPurposes', ['__usdRenderSettings', 'includedPurposes']),
            ('MaterialBindingPurposes', ['__usdRenderSettings', 'materialBindingPurposes']),
            ('RenderingColorSpace', ['__usdRenderSettings', 'renderingColorSpace']),
            ('Products', ['__usdRenderSettings', 'products']),
            ('NamespacedSettings', ['__usdRenderSettings', 'namespacedSettings']),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/usdImagingRenderProduct
    dict(
        LIBRARY_PATH = 'pxr/usdImaging/usdImaging',
        FILE_NAME = 'usdRenderProductSchema',
        SCHEMA_NAME = 'UsdImagingUsdRenderProduct',
        SCHEMA_TOKEN = '__usdRenderProduct',
        DEFAULT_LOCATOR = ['__usdRenderProduct'],
        MEMBERS = [
            # UsdRenderSettingsBase
            ('resolution', T_VEC2I),
            ('pixelAspectRatio', T_FLOAT),
            ('aspectRatioConformPolicy', T_TOKEN),
            ('dataWindowNDC', T_VEC4F), # XXX T_RANGE2F
            ('disableMotionBlur', T_BOOL),
            # note: instantaneousShutter is deprecated in favor of 
            # disableMotionBlur, so we skip it.
            ('camera', T_PATH),

            # UsdRenderProduct
            ('productType', T_TOKEN),
            ('productName', T_TOKEN),
            ('orderedVars', T_PATHARRAY),

            # note: namespacedSettings isn't in the USD schema.
            ('namespacedSettings', T_CONTAINER),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('NamespacedSettings', ['__usdRenderProduct', 'namespacedSettings']),
        ],
    ),

    #--------------------------------------------------------------------------
    # usdImaging/usdImagingRenderVar
    dict(
        LIBRARY_PATH = 'pxr/usdImaging/usdImaging',
        FILE_NAME = 'usdRenderVarSchema',
        SCHEMA_NAME = 'UsdImagingUsdRenderVar',
        SCHEMA_TOKEN = '__usdRenderVar',
        DEFAULT_LOCATOR = ['__usdRenderVar'],
        MEMBERS = [
            # UsdRenderProduct
            ('dataType', T_TOKEN),
            ('sourceName', T_STRING),
            ('sourceType', T_TOKEN),

            # note: namespacedSettings isn't in the USD schema.
            ('namespacedSettings', T_CONTAINER),
        ],
        STATIC_LOCATOR_ACCESSORS = [
            ('NamespacedSettings', ['__usdRenderVar', 'namespacedSettings']),
        ],
    ),
]
