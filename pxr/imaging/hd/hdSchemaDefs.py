#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
[
    dict(
        SCHEMA_NAME = 'ALL_SCHEMAS',
        LIBRARY_PATH = 'pxr/imaging/hd'
    ),

    #--------------------------------------------------------------------------
    # dependency
    dict(
        SCHEMA_NAME = 'Dependency',
        MEMBERS = [
            ('dependedOnPrimPath', T_PATH, {}),
            ('dependedOnDataSourceLocator', T_LOCATOR, {}),
            ('affectedDataSourceLocator', T_LOCATOR, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # dependencies
    dict(
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/dependencySchema'],
        SCHEMA_NAME = 'Dependencies',
        SCHEMA_TOKEN = '__dependencies',
        GENERIC_MEMBER = ('dependency', 'HdDependencySchema',
                          # We have custom code to get the data, so do not
                          # provide a GETTER.
                          # But we do need to have GENERIC_MEMBER to not
                          # provide an empty Builder().
                          dict(GETTER=False)),
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # subdivisionTags
    dict(
        SCHEMA_NAME = 'SubdivisionTags',
        SCHEMA_TOKEN = 'subdivisionTags',
        IMPL_SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/meshSchema'],
        MEMBERS = [
            ('faceVaryingLinearInterpolation', T_TOKEN, {}),
            ('interpolateBoundary', T_TOKEN, {}),
            ('triangleSubdivisionRule', T_TOKEN, {}),
            ('cornerIndices', T_INTARRAY, {}),
            ('cornerSharpnesses', T_FLOATARRAY, {}),
            ('creaseIndices', T_INTARRAY, {}),
            ('creaseLengths', T_INTARRAY, {}),
            ('creaseSharpnesses', T_FLOATARRAY, {}),
        ],

        LOCATOR_PREFIX = 'HdMeshSchema::GetDefaultLocator()',
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # geomSubset
    dict(
        SCHEMA_NAME = 'GeomSubset',
        SCHEMA_TOKEN = 'geomSubset',
        MEMBERS = [
            ('type', T_TOKEN, {}),
            ('indices', T_INTARRAY, {}),
        ],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('type', ['typeFaceSet', 'typePointSet', 'typeCurveSet']),
        ],
        
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # mesh
    dict(
        SCHEMA_NAME = 'Mesh',
        SCHEMA_TOKEN = 'mesh',
        SCHEMA_INCLUDES =
            ['{{LIBRARY_PATH}}/meshTopologySchema',
             '{{LIBRARY_PATH}}/subdivisionTagsSchema'],
        
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR = True)),
            ('topology', 'HdMeshTopologySchema', {}),
            ('subdivisionScheme', T_TOKEN, {}),
            ('subdivisionTags', 'HdSubdivisionTagsSchema', {}),
            ('doubleSided', T_BOOL, {}),
        ],

        ADD_DEFAULT_LOCATOR = True,

    ),

    #--------------------------------------------------------------------------
    # meshTopology
    dict(
        SCHEMA_NAME = 'MeshTopology',
        SCHEMA_TOKEN = 'topology',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/subdivisionTagsSchema'],
        MEMBERS = [
            ('faceVertexCounts', T_INTARRAY, {}),
            ('faceVertexIndices', T_INTARRAY, {}),
            ('holeIndices', T_INTARRAY, {}),
            ('orientation', T_TOKEN, {}),
        ],
        IMPL_SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/meshSchema'],
        LOCATOR_PREFIX = 'HdMeshSchema::GetDefaultLocator()',
        ADD_DEFAULT_LOCATOR = True,

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('orientation', ['leftHanded', 'rightHanded']),
        ],
    ),

    #--------------------------------------------------------------------------
    # tetMesh
    dict(
        SCHEMA_NAME = 'TetMesh',
        SCHEMA_TOKEN = 'tetMesh',
        SCHEMA_INCLUDES =
            ['{{LIBRARY_PATH}}/tetMeshTopologySchema'],
        
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR = True)),
            ('topology', 'HdTetMeshTopologySchema', {}),
            ('doubleSided', T_BOOL, {}),
        ],

        ADD_DEFAULT_LOCATOR = True,

    ),
    #--------------------------------------------------------------------------
    # tetMeshTopology
    dict(
        SCHEMA_NAME = 'TetMeshTopology',
        SCHEMA_TOKEN = 'topology',
        MEMBERS = [
            ('tetVertexIndices', T_VEC4IARRAY, dict(ADD_LOCATOR = True)),
            ('surfaceFaceVertexIndices', T_VEC3IARRAY, dict(ADD_LOCATOR = True)),
            ('orientation', T_TOKEN, {}),
        ],
        IMPL_SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/tetMeshSchema'],
        LOCATOR_PREFIX = 'HdTetMeshSchema::GetDefaultLocator()',
        ADD_DEFAULT_LOCATOR = True,

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('orientation', ['leftHanded', 'rightHanded']),
        ],
    ),

    #--------------------------------------------------------------------------
    # nurbsPatch
    dict(
        SCHEMA_NAME = 'NurbsPatch',
        SCHEMA_TOKEN = 'nurbsPatch',
        SCHEMA_INCLUDES =
            ['{{LIBRARY_PATH}}/nurbsPatchTrimCurveSchema'],
        MEMBERS = [
            ('uVertexCount', T_INT, {}),
            ('vVertexCount', T_INT, {}),
            ('uOrder', T_INT, {}),
            ('vOrder', T_INT, {}),
            ('uKnots', T_DOUBLEARRAY, {}),
            ('vKnots', T_DOUBLEARRAY, {}),
            ('uForm', T_TOKEN, {}),
            ('vForm', T_TOKEN, {}),
            ('uRange', T_VEC2D, {}),
            ('vRange', T_VEC2D, {}),
            ('trimCurve', 'HdNurbsPatchTrimCurveSchema', {}),
            ('orientation', T_TOKEN, {}),
            ('doubleSided', T_BOOL, {}),
        ],
        
        ADD_DEFAULT_LOCATOR = True,

        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('form', ['open', 'closed', 'periodic']),
            ('orientation', ['leftHanded', 'rightHanded']),
        ],
    ),

    #--------------------------------------------------------------------------
    # nurbsPatchTrimCurve
    dict(
        SCHEMA_NAME = 'NurbsPatchTrimCurve',
        SCHEMA_TOKEN = 'trimCurve',
        MEMBERS = [
            ('counts', T_INTARRAY, {}),
            ('orders', T_INTARRAY, {}),
            ('vertexCounts', T_INTARRAY, {}),
            ('knots', T_DOUBLEARRAY, {}),
            ('ranges', T_VEC2DARRAY, {}),
            ('points', T_VEC3DARRAY, {}),
        ],
        IMPL_SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/nurbsPatchSchema'],
        LOCATOR_PREFIX = 'HdNurbsPatchSchema::GetDefaultLocator()',
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # basisCurves
    dict(
        SCHEMA_NAME = 'BasisCurves',
        SCHEMA_TOKEN = 'basisCurves',
        SCHEMA_INCLUDES =
            ['{{LIBRARY_PATH}}/basisCurvesTopologySchema'],

        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR = True)),
            ('topology', 'HdBasisCurvesTopologySchema', {}),
        ],

        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # basisCurvesTopology
    dict(
        SCHEMA_NAME = 'BasisCurvesTopology',
        SCHEMA_TOKEN = 'topology',
        IMPL_SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/basisCurvesSchema'],
        LOCATOR_PREFIX = 'HdBasisCurvesSchema::GetDefaultLocator()',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('curveVertexCounts', T_INTARRAY, {}),
            ('curveIndices', T_INTARRAY, {}),
            ('basis', T_TOKEN, {}),
            ('type', T_TOKEN, {}),
            ('wrap', T_TOKEN, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # nurbsCurves
    dict(
        SCHEMA_NAME = 'NurbsCurves',
        SCHEMA_TOKEN = 'nurbsCurves',
        MEMBERS = [
            ('curveVertexCounts', T_INTARRAY, {}),
            ('order', T_INTARRAY, {}),
            ('knots', T_DOUBLEARRAY, {}),
            ('ranges', T_VEC2DARRAY, {}),
        ],

        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # primvar
    dict(
        SCHEMA_NAME = 'Primvar',
        MEMBERS = [
            ('primvarValue', T_SAMPLED,
             dict(DOC = '''
                If the primvar does not have indices, GetPrimvarValue() and
                GetIndexedPrimvarValue() will return the same thing.
                If the primvar does has indices, GetPrimvarValue() will return the 
                flattened value, while GetIndexedPrimvarValue() will return the
                unflattened value.''',
                  # We provide a custom getter for this.
                  GETTER = False)),
            ('indexedPrimvarValue', T_SAMPLED,
                  # We provide a custom getter for this.
             dict(GETTER = False)),
            ('indices', T_INTARRAY, {}),
            ('interpolation', T_TOKEN, {}),
            ('role', T_TOKEN, {}),
        ],
        EXTRA_TOKENS = [
            'transform',
        ],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('interpolation', [
                'constant',
                'uniform',
                'varying',
                'vertex',
                'faceVarying',
                'instance',
                ]),
            ('role', [
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
    ),

    #--------------------------------------------------------------------------
    # primvars
    dict(
        SCHEMA_NAME = 'Primvars',
        SCHEMA_TOKEN = 'primvars',
        ADD_DEFAULT_LOCATOR = True,
        GENERIC_MEMBER = ('primvar', 'HdPrimvarSchema', {}),
        MEMBERS = [
            ('ALL_MEMBERS', 'HdPrimvarSchema',
             # We want locators but no Getters for these primvars.
             dict(GETTER = False,
                  ADD_LOCATOR = True)),
            ('points', 'HdPrimvarSchema', {}),
            ('normals', 'HdPrimvarSchema', {}),
            ('widths', 'HdPrimvarSchema', {}),
        ],

        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/primvarSchema'],
    ),

    #--------------------------------------------------------------------------
    # xform
    dict(
        SCHEMA_NAME = 'Xform',
        SCHEMA_TOKEN = 'xform',
        MEMBERS = [
            ('matrix', T_MATRIX, {}),
            ('resetXformStack', T_BOOL,
             dict(DOC = '''
                The "resetXformStack" flag tells consumers that this transform
                doesn't inherit from the parent prim's transform.''')),
        ],

        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # visibility
    dict(
        SCHEMA_NAME = 'Visibility',
        SCHEMA_TOKEN = 'visibility',
        MEMBERS = [
            ('visibility', T_BOOL, {}),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # purpose
    dict(
        SCHEMA_NAME = 'Purpose',
        SCHEMA_TOKEN = 'purpose',
        MEMBERS = [
            ('purpose', T_TOKEN, {}),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # extent
    dict(
        SCHEMA_NAME = 'Extent',
        SCHEMA_TOKEN = 'extent',
        MEMBERS = [
            ('min', T_VEC3D, {}),
            ('max', T_VEC3D, {}),
            
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # materialNode
    dict(
        SCHEMA_NAME = 'MaterialNode',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/schemaTypeDefs'],
        MEMBERS = [
            ('parameters', 'HdMaterialNodeParameterContainerSchema', {}),
            ('inputConnections', 'HdMaterialConnectionVectorContainerSchema', {}),
            ('nodeIdentifier', T_TOKEN,
             dict(DOC = '''
                This identifies the shader the node represents. The
                renderContextNodeIdentifier container can store alternative
                values for this. A consumer which is interested in a specific
                render context should check for that token within
                renderContextNodeIdentifiers and fall back on this value in its
                absence.''')),
            ('renderContextNodeIdentifiers', T_CONTAINER,
             dict(DOC = '''
                A shading node can hold a nodeIdentifier value for multiple
                render contexts at once. This allows multiple renderer target
                representations to coexist in the same renderable scene. The
                contents of this container are alternate possible values for
                nodeIdentifier.
                A consumer which is interested in a specific
                render context should check for that token within
                this container and fall back on nodeIdentifier in its
                absence.''')),
            ('nodeTypeInfo', T_CONTAINER,
             dict(DOC = '''
                Rather than having an identifier, a shader can be specified
                by other information.''')),
        ],
    ),

    #--------------------------------------------------------------------------
    # materialNodeParameter
    dict(
        SCHEMA_NAME = 'MaterialNodeParameter',
        MEMBERS = [
            ('value', T_SAMPLED, {}),
            # Parameter Metadata
            ('colorSpace', T_TOKEN, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # materialConnection
    dict(
        SCHEMA_NAME = 'MaterialConnection',
        MEMBERS = [
            ('upstreamNodePath', T_TOKEN, {}),
            ('upstreamNodeOutputName', T_TOKEN, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # materialInterfaceMapping
    dict(
        SCHEMA_NAME = 'MaterialInterfaceMapping',
        MEMBERS = [
            ('nodePath', T_TOKEN, {}),
            ('inputName', T_TOKEN, {})
        ],
    ),

    #--------------------------------------------------------------------------
    # materialNetwork
    dict(
        SCHEMA_NAME = 'MaterialNetwork',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/schemaTypeDefs'],
        MEMBERS = [
            ('nodes', 'HdMaterialNodeContainerSchema', {}),
            ('terminals', 'HdMaterialConnectionContainerSchema', {}),
            ('interfaceMappings',
                'HdMaterialInterfaceMappingsContainerSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # material
    dict(
        SCHEMA_NAME = 'Material',
        SCHEMA_TOKEN = 'material',
        EXTRA_TOKENS = [
            '(universalRenderContext, "")',
        ],
        ADD_DEFAULT_LOCATOR = True,

        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/materialNetworkSchema'],

        GENERIC_MEMBER = (
            'materialNetwork', 'HdMaterialNetworkSchema',
            # We provide a custom getter for the material network
            # to fallback to the universalRenderContext.
            dict(GETTER = False))
    ),

    #--------------------------------------------------------------------------
    # materialBinding
    dict(
        SCHEMA_NAME = 'MaterialBinding',
        MEMBERS = [
            ('path', T_PATH, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # materialBindings
    dict(
        SCHEMA_NAME = 'MaterialBindings',
        SCHEMA_TOKEN = 'materialBindings',
        EXTRA_TOKENS = [
            '(allPurpose, "")',
        ],
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/materialBindingSchema'],
        GENERIC_MEMBER = (
            'materialBinding', 'HdMaterialBindingSchema',
            # We provide a custom getter falling back to the allPurpose.
            dict(GETTER = False)),
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # volumeFieldBinding
    dict(
        SCHEMA_NAME = 'VolumeFieldBinding',
        SCHEMA_TOKEN = 'volumeFieldBinding',
        ADD_DEFAULT_LOCATOR = True,
        GENERIC_MEMBER = ('volumeFieldBinding', T_PATH, {}),
    ),

    #--------------------------------------------------------------------------
    # coordSysBinding
    dict(
        SCHEMA_NAME = 'CoordSysBinding',
        SCHEMA_TOKEN = 'coordSysBinding',
        ADD_DEFAULT_LOCATOR = True,
        GENERIC_MEMBER = ('coordSysBinding', T_PATH, {}),
    ),

    #--------------------------------------------------------------------------
    # coordSys
    dict(
        SCHEMA_NAME = 'CoordSys',
        SCHEMA_TOKEN = 'coordSys',
        MEMBERS = [
            ('name', T_TOKEN, {}),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # instancedBy
    dict(
        SCHEMA_NAME = 'InstancedBy',
        DOC = '''A schema marking a prim as instanced by another prim.

                 Many renderers need to know not what prototypes an instancer has, but
                 rather what instancers a prototype has; this is encoded in
                 "instancedBy".  A prim is "instancedBy" /Instancer if /Instancer has
                 a prototype path that's a parent of the prim.  A complicating exception is
                 if /A instances /A/B, which instances /A/B/C, we don't consider /A to be
                 instancing /A/B/C directly; this is to support nested explicit instancing
                 of things like leaves/trees/forests.

                 This value is computed based on the instancer topology of instancer prims in
                 the scene.

                 Note: if multiple instancers reference a prototype, it's possible for
                 instancedBy to contain multiple entries.  Some renderers may be able to
                 read this directly, but some may need to duplicate prims with an op so that
                 each prim has a single instancer, depending on how the renderer exposes
                 instancing.''',
        SCHEMA_TOKEN = 'instancedBy',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('paths', T_PATHARRAY, {}),
            ('prototypeRoots', T_PATHARRAY, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # instance
    dict(
        SCHEMA_NAME = 'Instance',
        SCHEMA_TOKEN = 'instance',
        ADD_DEFAULT_LOCATOR = True,
        DOC = '''This schema can be considered the opposite of
                 instancerTopology's "instanceLocations".  When the scene
                 coalesces scene prims into multiple instances of a single
                 prototype, it inserts "instance" prims at the site of
                 de-duplication.  The instancer prim added to manage the
                 prototype uses "instanceLocations" to point back to all of
                 these instance prims.

                 The instance prims aren't directly useful for rendering but
                 can be useful for scene processing and data aggregation.''',
        MEMBERS = [
            ('instancer', T_PATH,
             dict(DOC = '''
                Path to instancer for which a (sub-)entry was added to
                its instancerTopology's instanceIndices during
                instance aggregation to account for this instance.
                Note that instanceIndices is nested, that is a vector
                data source containing integer arrays, one for each
                prototype the instancer is instancing. Thus, we need
                two indices to identify the entry: prototypeIndex is the
                outer index and instanceIndex the inner index.''')),
            ('prototypeIndex', T_INT,
             dict(DOC = '''
                Index into vector data source at
                instancer's instancerTopology's instanceIndices
                to find entry corresponding to this instance.''')),
            ('instanceIndex', T_INT,
             dict(DOC = '''
                Index into int array within the vector data source at
                instancer's instancerTopology's instanceIndices
                to find entry corresponding to this instance.''')),
        ],
    ),

    #--------------------------------------------------------------------------
    # instancerTopology
    dict(
        SCHEMA_NAME = 'InstancerTopology',
        DOC = '''Since the instancing schema is complicated:

                 An instancer is a prim at a certain scenegraph location that causes other
                 prims to be duplicated.  The instancer can also hold instance-varying data
                 like constant primvars or material relationships.

                 The important things an instancer has is:

                 1.) Instancer topology, describing how exactly the prims are duplicated;

                 2.) Instance-rate data, meaning data that varies per instance, such as
                     primvars or material bindings.

                 If an instancer causes prims "/A" and "/B" to be duplicated, we encode that
                 by setting prototypes = ["/A", "/B"].  Note that "/A" and "/B" can be
                 subtrees, not direct gprims.  instanceIndices encodes both multiplicity
                 and position in arrays of instance-rate data, per prototype path; if
                 instanceIndices = { [0,2], [1] }, then we draw /A twice (with instance
                 primvar indices 0 and 2); and /B once (with instance primvar index 1).
                 Mask is an auxiliary parameter that can be used to deactivate certain
                 instances; mask = [true, true, false] would disable the
                 second copy of "/A".  An empty mask array is the same as all-true.

                 Scenes generally specify instancing in one of two ways:

                 1.) Explicit instancing: prim /Instancer wants to draw its subtree at
                     an array of locations.  This is a data expansion form.

                 2.) Implicit instancing: prims /X and /Y are marked as being identical,
                     and scene load replaces them with a single prim and an instancer.
                     This is a data coalescing form.

                 For implicit instancing, we want to know the original paths of /X and /Y,
                 for doing things like resolving inheritance.  This is encoded in the
                 "instanceLocations" path, while the prototype prims (e.g. /_Prototype/Cube,
                 the deduplicated version of /X/Cube and /Y/Cube) is encoded in the
                 "prototypes" path.

                 For explicit instancing, the "instanceLocations" attribute is meaningless
                 and should be left null.''',
        SCHEMA_TOKEN = 'instancerTopology',
        ADD_DEFAULT_LOCATOR = True,
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/schemaTypeDefs'],
        MEMBERS = [
            ('prototypes', T_PATHARRAY, {}),
            ('instanceIndices', 'HdIntArrayVectorSchema', {}),
            ('mask', T_BOOLARRAY, {}),
            ('instanceLocations', T_PATHARRAY, {}),

        ],
    ),

    #--------------------------------------------------------------------------
    # legacyDisplayStyle
    dict(
        SCHEMA_NAME = 'LegacyDisplayStyle',
        SCHEMA_TOKEN = 'displayStyle',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('refineLevel', T_INT, {}),
            ('flatShadingEnabled', T_BOOL, {}),
            ('displacementEnabled', T_BOOL, {}),
            ('occludedSelectionShowsThrough', T_BOOL, {}),
            ('pointsShadingEnabled', T_BOOL, {}),
            ('materialIsFinal', T_BOOL, {}),
            ('shadingStyle', T_TOKEN, {}),
            ('reprSelector', T_TOKENARRAY,
             dict(ADD_LOCATOR = True)),
            ('cullStyle', T_TOKEN,
             dict(ADD_LOCATOR = True)),
        ],
    ),

    #--------------------------------------------------------------------------
    # light
    dict(
        SCHEMA_NAME = 'Light',
        SCHEMA_TOKEN = 'light',
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # imageShader
    dict(
        SCHEMA_NAME = 'ImageShader',
        SCHEMA_TOKEN = 'imageShader',
        ADD_DEFAULT_LOCATOR = True,
        SCHEMA_INCLUDES = [
            '{{LIBRARY_PATH}}/schemaTypeDefs',
            '{{LIBRARY_PATH}}/materialNetworkSchema'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR = True)),
            ('enabled', T_BOOL, {}),
            ('priority', T_INT, {}),
            ('filePath', T_STRING, {}),
            ('constants', 'HdSampledDataSourceContainerSchema', {}),
            ('materialNetwork', 'HdMaterialNetworkSchema', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # renderBuffer
    dict(
        SCHEMA_NAME = 'RenderBuffer',
        SCHEMA_TOKEN = 'renderBuffer',
        MEMBERS = [
            ('dimensions', T_VEC3I, {}),
            ('format', T_FORMAT, {}),
            ('multiSampled', T_BOOL, {}),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # renderPass
    dict(
        SCHEMA_NAME = 'RenderPass',
        SCHEMA_TOKEN = 'renderPass',
        ADD_DEFAULT_LOCATOR = True,
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/schemaTypeDefs'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR = True)),
            ('passType', T_TOKEN, {}),
            ('renderSource', T_PATH, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # renderSettings
    dict(
        SCHEMA_NAME = 'RenderSettings',
        SCHEMA_TOKEN = 'renderSettings',
        ADD_DEFAULT_LOCATOR = True,
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/schemaTypeDefs'],
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR = True)),
            ('namespacedSettings', T_CONTAINER, {}),
            ('active', T_BOOL, {}),
            ('renderProducts', 'HdRenderProductVectorSchema', {}),
            ('includedPurposes', T_TOKENARRAY, {}),
            ('materialBindingPurposes', T_TOKENARRAY, {}),
            ('renderingColorSpace', T_TOKEN, {}),
            ('shutterInterval', T_VEC2D,
             dict(DOC = '''
                Frame-relative time interval representing the sampling window for 
                data relevant to motion blur. Renderers can use this interval when
                querying time-sampled data (e.g., xforms, points, velocities, ...)
                to simulate motion blur effects.
            
                Note: This closely relates to the (frame-relative) shutter
                      interval of a camera specified via shutter open and close
                      times and is expected to span the union of the shutter
                      intervals of cameras used in generating the render
                      artifacts.''')),
        ],
    ),

    #--------------------------------------------------------------------------
    # renderProduct
    dict(
        SCHEMA_NAME = 'RenderProduct',
        SCHEMA_TOKEN = 'renderProduct',
        ADD_DEFAULT_LOCATOR = True,
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/schemaTypeDefs'],
        MEMBERS = [
            ('path', T_PATH, {}), # scene prim path
            ('type', T_TOKEN, {}),
            ('name', T_TOKEN, {}), # output name
            ('resolution', T_VEC2I, dict(ADD_LOCATOR = True)),
            ('renderVars', 'HdRenderVarVectorSchema', dict(ADD_LOCATOR = True)),
            # Camera & Framing
            ('cameraPrim', T_PATH, {}),
            ('pixelAspectRatio', T_FLOAT, {}),
            ('aspectRatioConformPolicy', T_TOKEN, {}),
            ('apertureSize', T_VEC2F, {}),
            ('dataWindowNDC', T_VEC4F, {}), # XXX T_RANGE2F
            # Product specific overrides
            ('disableMotionBlur', T_BOOL, {}),
            ('disableDepthOfField', T_BOOL, {}),
            ('namespacedSettings', T_CONTAINER, dict(ADD_LOCATOR = True)),
        ],
    ),

    #--------------------------------------------------------------------------
    # renderVar
    dict(
        SCHEMA_NAME = 'RenderVar',
        SCHEMA_TOKEN = 'renderVar',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('path', T_PATH, {}), # scene prim path
            ('dataType', T_TOKEN, {}),
            ('sourceName', T_TOKEN, {}),
            ('sourceType', T_TOKEN, {}),
            # Var specific overrides
            ('namespacedSettings', T_CONTAINER, dict(ADD_LOCATOR = True)),
        ],
    ),

    #--------------------------------------------------------------------------
    # integrator
    dict(
        SCHEMA_NAME = 'Integrator',
        SCHEMA_TOKEN = 'integrator',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/materialNodeSchema'],
        MEMBERS = [
            ('resource', 'HdMaterialNodeSchema', dict(ADD_LOCATOR = True)),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # sampleFilter
    dict(
        SCHEMA_NAME = 'SampleFilter',
        SCHEMA_TOKEN = 'sampleFilter',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/materialNodeSchema'],
        MEMBERS = [
            ('resource', 'HdMaterialNodeSchema', dict(ADD_LOCATOR = True)),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # displayFilter
    dict(
        SCHEMA_NAME = 'DisplayFilter',
        SCHEMA_TOKEN = 'displayFilter',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/materialNodeSchema'],
        MEMBERS = [
            ('resource', 'HdMaterialNodeSchema', dict(ADD_LOCATOR = True)),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # system
    dict(
        SCHEMA_NAME = 'System',
        DOC = '''The {{ SCHEMA_CLASS_NAME }} specifies a container that will hold
                 "system" data.  Each piece of system data is identified by a key
                 within the container.  A piece of system data is evaluated at a
                 given location by walking up the namespace looking for a system 
                 container that contains the corresponding key.''',
        SCHEMA_TOKEN = 'system',
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # volumeField
    dict(
        SCHEMA_NAME = 'VolumeField',
        SCHEMA_TOKEN = 'volumeField',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('filePath', T_ASSETPATH, {}),
            ('fieldName', T_TOKEN, {}),
            ('fieldIndex', T_INT, {}),
            ('fieldDataType', T_TOKEN, {}),
            ('vectorDataRoleHint', T_TOKEN, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # camera
    dict(
        SCHEMA_NAME = 'Camera',
        SCHEMA_TOKEN = 'camera',
        SCHEMA_INCLUDES = [
            '{{LIBRARY_PATH}}/schemaTypeDefs',
            '{{LIBRARY_PATH}}/splitDiopterSchema',
            '{{LIBRARY_PATH}}/lensDistortionSchema'],
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('projection', T_TOKEN, {}),
            ('horizontalAperture', T_FLOAT, {}),
            ('verticalAperture', T_FLOAT, {}),
            ('horizontalApertureOffset', T_FLOAT, {}),
            ('verticalApertureOffset', T_FLOAT, {}),
            ('focalLength', T_FLOAT, {}),
            ('clippingRange', T_VEC2F, {}),
            ('clippingPlanes', T_VEC4DARRAY, {}),
            ('fStop', T_FLOAT, {}),
            ('focusDistance', T_FLOAT, {}),
            ('shutterOpen', T_DOUBLE, dict(ADD_LOCATOR = True)),
            ('shutterClose', T_DOUBLE, dict(ADD_LOCATOR = True)),
            ('exposure', T_FLOAT, {}),
            ('focusOn', T_BOOL, {}),
            ('dofAspect', T_FLOAT, {}),
            ('splitDiopter', 'HdSplitDiopterSchema', {}),
            ('lensDistortion', 'HdLensDistortionSchema', {}),
            ('namespacedProperties', 'HdSampledDataSourceContainerContainerSchema', dict(ADD_LOCATOR = True)),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('projection', ['perspective', 'orthographic']),
        ],
    ),

    #--------------------------------------------------------------------------
    # splitDiopter
    dict(
        SCHEMA_NAME = 'SplitDiopter',
        SCHEMA_TOKEN = 'splitDiopter',
        MEMBERS = [
            ('count', T_INT, {}),
            ('angle', T_FLOAT, {}),
            ('offset1', T_FLOAT, {}),
            ('width1', T_FLOAT, {}),
            ('focusDistance1', T_FLOAT, {}),
            ('offset2', T_FLOAT, {}),
            ('width2', T_FLOAT, {}),
            ('focusDistance2', T_FLOAT, {}),
        ],
        IMPL_SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/cameraSchema'],
        LOCATOR_PREFIX = 'HdCameraSchema::GetDefaultLocator()',
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # lensDistortion
    dict(
        SCHEMA_NAME = 'LensDistortion',
        SCHEMA_TOKEN = 'lensDistortion',
        MEMBERS = [
            ('type', T_TOKEN, {}),
            ('k1', T_FLOAT, {}),
            ('k2', T_FLOAT, {}),
            ('center', T_VEC2F, {}),
            ('anaSq', T_FLOAT, {}),
            ('asym', T_VEC2F, {}),
            ('scale', T_FLOAT, {}),
            ('ior', T_FLOAT, {}),
        ],
        IMPL_SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/cameraSchema'],
        LOCATOR_PREFIX = 'HdCameraSchema::GetDefaultLocator()',
        ADD_DEFAULT_LOCATOR = True,

        STATIC_TOKEN_DATASOURCE_BUILDERS = [ # optional for shared token ds's
            ('type', ['standard', 'fisheye']),
        ],
    ),

    #--------------------------------------------------------------------------
    # categories
    dict(
        SCHEMA_NAME = 'Categories',
        SCHEMA_TOKEN = 'categories',
        ADD_DEFAULT_LOCATOR = True,
    ),

    #--------------------------------------------------------------------------
    # instanceCategories
    dict(
        SCHEMA_NAME = 'InstanceCategories',
        SCHEMA_TOKEN = 'instanceCategories',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('categoriesValues', 'HdVectorDataSource', {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # extcomputation_primvar
    dict(
        SCHEMA_NAME = 'ExtComputationPrimvar',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/primvarSchema'],
        MEMBERS = [
            ('interpolation', T_TOKEN, {}),
            ('role', T_TOKEN, {}),
            ('sourceComputation', T_PATH, {}),
            ('sourceComputationOutputName', T_TOKEN, {}),
            ('valueType', T_TUPLE, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # extcomputation_primvars
    dict(
        SCHEMA_NAME = 'ExtComputationPrimvars',
        SCHEMA_TOKEN = 'extComputationPrimvars',
        ADD_DEFAULT_LOCATOR = True,
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/extComputationPrimvarSchema'],
    ),

    #--------------------------------------------------------------------------
    # extcomputation_inputComputation
    dict(
        SCHEMA_NAME = 'ExtComputationInputComputation',
        MEMBERS = [
            ('name', T_TOKEN, {}),
            ('sourceComputation', T_PATH, {}),
            ('sourceComputationOutputName', T_TOKEN, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # extcomputation_output
    dict(
        SCHEMA_NAME = 'ExtComputationOutput',
        MEMBERS = [
            ('name', T_TOKEN, {}),
            ('valueType', T_TUPLE, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # extcomputation
    dict(
        SCHEMA_NAME = 'ExtComputation',
        SCHEMA_TOKEN = 'extComputation',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR = True)),
            # inputValues should be a vector schema of typed data sources
            ('inputValues', T_CONTAINER, {}),
            ('inputComputations', T_VECTOR, {}),
            ('outputs', T_VECTOR, {}),
            ('glslKernel', T_STRING, {}),
            ('cpuCallback', T_BASE, {}),
            ('dispatchCount', T_SIZET, {}),
            ('elementCount', T_SIZET, {}),
        ],
    ),
    
    #--------------------------------------------------------------------------
    # cube
    dict(
        SCHEMA_NAME = 'Cube',
        SCHEMA_TOKEN = 'cube',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('size', T_DOUBLE, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # sphere
    dict(
        SCHEMA_NAME = 'Sphere',
        SCHEMA_TOKEN = 'sphere',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('radius', T_DOUBLE, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # cone
    dict(
        SCHEMA_NAME = 'Cone',
        SCHEMA_TOKEN = 'cone',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('height', T_DOUBLE, {}),
            ('radius', T_DOUBLE, {}),
            ('axis', T_TOKEN, {}),
        ],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('axis', ['X', 'Y', 'Z']),
        ],
    ),

    #--------------------------------------------------------------------------
    # cylinder
    dict(
        SCHEMA_NAME = 'Cylinder',
        SCHEMA_TOKEN = 'cylinder',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('height', T_DOUBLE, {}),
            # deprecated in favor of radiusTop and radiusBottom
            ('radius', T_DOUBLE, {}), 
            ('radiusTop', T_DOUBLE, {}),
            ('radiusBottom', T_DOUBLE, {}),
            ('axis', T_TOKEN, {}),
        ],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('axis', ['X', 'Y', 'Z']),
        ],
    ),

    #--------------------------------------------------------------------------
    # capsule
    dict(
        SCHEMA_NAME = 'Capsule',
        SCHEMA_TOKEN = 'capsule',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('height', T_DOUBLE, {}),
            # deprecated in favor of radiusTop and radiusBottom
            ('radius', T_DOUBLE, {}), 
            ('radiusTop', T_DOUBLE, {}),
            ('radiusBottom', T_DOUBLE, {}),
            ('axis', T_TOKEN, {}),
        ],

        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('axis', ['X', 'Y', 'Z']),
        ],

    ),

    #--------------------------------------------------------------------------
    # primOrigin
    dict(
        SCHEMA_NAME = 'PrimOrigin',
        SCHEMA_TOKEN = 'primOrigin',
        ADD_DEFAULT_LOCATOR = True,
        STATIC_LOCATOR_ACCESSORS = [
            ('scenePath', ['primOrigin', 'scenePath']),
        ],
        EXTRA_TOKENS = [
            'scenePath',
        ],
    ),

    #--------------------------------------------------------------------------
    # selection
    dict(
        SCHEMA_NAME = 'Selection',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/schemaTypeDefs'],
        MEMBERS = [
            ('fullySelected', T_BOOL, {}),
            ('nestedInstanceIndices', 'HdInstanceIndicesVectorSchema',
             dict(DOC = '''
                Starting with the outer most, list for each nesting
                level of instancing what instances are selected.''')),
        ],
    ),

    #--------------------------------------------------------------------------
    # instanceIndices
    dict(
        SCHEMA_NAME = 'InstanceIndices',
        MEMBERS = [
            ('instancer', T_PATH, {}),
            ('prototypeIndex', T_INT, {}),
            ('instanceIndices', T_INTARRAY, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # sceneGlobals
    dict(
        SCHEMA_NAME = 'SceneGlobals',
        DOC = '''The {{ SCHEMA_CLASS_NAME }} encapsulates "global" state to orchestrate a
                 render. It currently houses the active render settings
                 and pass prim paths that describe the information
                 necessary to generate images from a single invocation
                 of a renderer, and the active time sample range and current  
                 frame number that may be relevant to downstream scene indices 
                 (e.g. procedural evaluation).

                 We shall use the convention of a container data source at the root prim
                 of the scene index that is populated with this global state.
                 The renderer and downstream scene indices can query it to configure their
                 behavior as necessary.''',
        SCHEMA_TOKEN = 'sceneGlobals',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR = True)),
            ('activeRenderPassPrim', T_PATH, {}),
            ('activeRenderSettingsPrim', T_PATH, {}),
            ('startTimeCode', T_DOUBLE, {}),
            ('endTimeCode', T_DOUBLE, {}),
            ('currentFrame', T_DOUBLE, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # renderCapabilities
    dict(
        SCHEMA_NAME = 'RenderCapabilities',
        MEMBERS = [
            ('motionBlur', T_BOOL, {}),
        ],
    ),

    #--------------------------------------------------------------------------
    # collections
    dict(
        SCHEMA_NAME = 'Collections',
        DOC = '''The {{ SCHEMA_CLASS_NAME }} specifies a wrapper container
                 for collection entries with the key being the collection name.
              ''',
        SCHEMA_TOKEN = 'collections',
        SCHEMA_INCLUDES = ['{{LIBRARY_PATH}}/collectionSchema'],
        GENERIC_MEMBER = (
            'collection', 'HdCollectionSchema', {}),
        ADD_DEFAULT_LOCATOR = True,
    ),

    # collection
    dict(
        SCHEMA_NAME = 'Collection',
        SCHEMA_TOKEN = 'collection',
        MEMBERS = [
            ('membershipExpression', T_PATHEXPRESSION, {}),
        ],
        ADD_DEFAULT_LOCATOR = True,
    ),
    #--------------------------------------------------------------------------
]
