#
# Copyright 2023 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
[
    dict(
        SCHEMA_NAME = 'ALL_SCHEMAS',
        LIBRARY_PATH = 'ext/rmanpkg/25.0/plugin/renderman/plugin/hdPrman',
        INCLUDE_PATH = 'hdPrman'
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyRenderOutput
    dict(
        SCHEMA_NAME = 'RileyRenderOutput',
        SCHEMA_TOKEN = 'rileyRenderOutput',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR= True)),
            ('name', T_TOKEN, {}),
            ('type', T_TOKEN, {}),
            ('source', T_TOKEN, {}),
            ('accumulationRule', T_TOKEN, {}),
            ('filter', T_TOKEN, {}),
            ('filterSize', T_VEC2F, {}),
            ('relativePixelVariance', T_FLOAT, {}),
            ('params', T_CONTAINER, {}),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('Type', ['(float_, "float")', 'integer', 'color', 'vector']),
        ],
    ),

    #--------------------------------------------------------------------------
    # hdPrman/rileyRenderTarget
    dict(
        SCHEMA_NAME = 'RileyRenderTarget',
        SCHEMA_TOKEN = 'rileyRenderTarget',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR= True)),
            ('renderOutputs', T_PATHARRAY, {}),
            ('extent', T_VEC3I, {}),
            ('filterMode', T_TOKEN, {}),
            ('pixelVariance', T_FLOAT, {}),
            ('params', T_CONTAINER, {}),
        ],
        STATIC_TOKEN_DATASOURCE_BUILDERS = [
            ('FilterMode', ['importance', 'weighted']),
        ],
    ),

    # hdPrman/rileyCoordinateSystem
    dict(
        SCHEMA_NAME = 'RileyCoordinateSystem',
        SCHEMA_TOKEN = 'rileyCoordinateSystem',
        ADD_DEFAULT_LOCATOR = True,
        MEMBERS = [
            ('ALL_MEMBERS', '', dict(ADD_LOCATOR= True)),
            ('xform', T_MATRIX, {}),
            ('attributes', T_CONTAINER, {}),
        ],
    ),
]
