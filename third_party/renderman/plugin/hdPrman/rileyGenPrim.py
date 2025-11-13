#!/usr/bin/env python
#
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from jinja2 import Environment, FileSystemLoader, Template, pass_context
import re
import os
import sys
import textwrap
from argparse import ArgumentParser

#------------------------------------------------------------------------------

# DATA SOURCE TYPE ALIASES
T_CONTAINER = "HdContainerDataSource"
T_INT = "HdIntDataSource"
T_INTARRAY = "HdIntArrayDataSource"
T_FLOAT = "HdFloatDataSource"
T_FLOATARRAY = "HdFloatArrayDataSource"
T_DOUBLE = "HdDoubleDataSource"
T_DOUBLEARRAY = "HdDoubleArrayDataSource"
T_BOOL = "HdBoolDataSource"
T_BOOLARRAY = "HdBoolArrayDataSource"
T_TOKEN = "HdTokenDataSource"
T_TOKENARRAY = "HdTokenArrayDataSource"
T_PATH = "HdPathDataSource"
T_PATHARRAY = "HdPathArrayDataSource"
T_ASSETPATH = "HdAssetPathDataSource"
T_VEC2I = "HdVec2iDataSource"
T_VEC2F = "HdVec2fDataSource"
T_VEC2D = "HdVec2dDataSource"
T_VEC2FARRAY = "HdVec2fArrayDataSource"
T_VEC2DARRAY = "HdVec2dArrayDataSource"
T_VEC3I = "HdVec3iDataSource"
T_VEC3F = "HdVec3fDataSource"
T_VEC3D = "HdVec3dDataSource"
T_VEC3FARRAY = "HdVec3fArrayDataSource"
T_VEC3DARRAY = "HdVec3dArrayDataSource"
T_VEC4I = "HdVec4iDataSource"
T_VEC4F = "HdVec4fDataSource"
T_VEC4D = "HdVec4dDataSource"
T_VEC4DARRAY = "HdVec4dArrayDataSource"
T_MATRIX = "HdMatrixDataSource"
T_MATRIXARRAY = "HdMatrixArrayDataSource"
T_LOCATOR = "HdLocatorDataSource"
T_FORMAT = "HdFormatDataSource"
T_SAMPLED = "HdSampledDataSource"
T_TUPLE = "HdTupleTypeDataSource"
T_SIZET = "HdSizetDataSource"
T_BASE = "HdDataSourceBase"
T_STRING = "HdStringDataSource"
T_VECTOR = "HdVectorDataSource"
T_RESOLVERCONTEXT = "HdResolverContextDataSource"

#------------------------------------------------------------------------------
# Filters

def CapitalizeFirstLetter(s):
    return s[0].upper() + s[1:]

def UncapitalizeFirstLetter(s):
    return s[0].lower() + s[1:]

def CamelCaseToSnakeCase(name):
    nameTokens = []
    for s in re.compile(r'([A-Z])').split(name):
        if not s:
            continue
        if nameTokens and nameTokens[-1].isupper():
            nameTokens[-1] = nameTokens[-1] + s
        else:
            nameTokens.append(s)

    return '_'.join(nameTokens).upper()

def PathCamelCaseToSnakeCase(path):
    s = '_'.join(CamelCaseToSnakeCase(name)
                 for name in path.split('/'))
    return s.replace('.', '_')

def ToUnderlyingDataSource(dataSourceTypeOrSchemaName):
    if dataSourceTypeOrSchemaName.endswith('Schema'):
        if dataSourceTypeOrSchemaName.endswith('VectorSchema'):
            return 'HdVectorDataSource'
        else:
            return 'HdContainerDataSource'
    else:
        return dataSourceTypeOrSchemaName

def ToComment(text, indent = 0):
    result = ''

    for paragraph in re.split(r'\n\s*\n', text, re.DOTALL):
        for line in textwrap.wrap(' '.join(paragraph.split()),
                                  width = 75 - indent):
            result += indent * ' ' + '// ' + line + '\n'
        result += indent * ' ' + '//' + '\n'

    return result

def ToTokenName(text : str):
    """
    Given a string starting with '(' such as '(class_, "class")', returns 'class_'.
    
    Otherwise, just returns the input.
    """

    if not text.startswith('('):
        return text
    if not text.endswith(')'):
        raise RuntimeError(
            'Expected token specifier that start with "(" to end with ")". '
            'Got "%s".' % text)
    parts = text[1:-1].split(',')
    if not len(parts) == 2:
        raise RuntimeError(
            'Expected token specifier to have two parts. Got "%s".' % text)
    return parts[0].strip()

@pass_context
def Expand(context, text):
    return Template(text).render(**context)

#------------------------------------------------------------------------------

bin_path = os.path.dirname(os.path.abspath(sys.argv[0]))

defaultSchemadefsDir = os.path.join(bin_path, 'codegenTemplates')

env = Environment(loader=FileSystemLoader(defaultSchemadefsDir))
env.filters['capitalizeFirst'] = CapitalizeFirstLetter
env.filters['uncapitalizeFirst'] = UncapitalizeFirstLetter
env.filters['snake'] = PathCamelCaseToSnakeCase
env.filters['underlyingDataSource'] = ToUnderlyingDataSource
env.filters['makeComment'] = ToComment
env.filters['tokenName'] = ToTokenName
env.filters['expand'] = Expand

headerTemplate = env.get_template("rileyPrim.h")
implTemplate = env.get_template("rileyPrim.cpp")

#------------------------------------------------------------------------------

customCodeRegEx = r'// --\(BEGIN CUSTOM CODE: (?P<word>[\w ]+)\)--\n(.*)\n// --\(END CUSTOM CODE: (?P=word)\)--'

def ExtractCustomCode(path):
    if not os.path.isfile(path):
        return {}
    with open(path, 'r') as f:
        contents = f.read()
    return dict((key, text) for key, text in re.findall(customCodeRegEx, contents, re.DOTALL)
                if text.strip())

def ApplyUnderlayToMembers(members):
    global_member_opt_dict = {}
    for name, type_name, opt_dict in members:
        if name == 'ALL_MEMBERS':
            global_member_opt_dict = opt_dict
            break

    return [ (name, type_name, global_member_opt_dict | opt_dict)
             for name, type_name, opt_dict in members
             if name != 'ALL_MEMBERS' ]

def ProcessEntry(dstDir, **args):
    fields = dict(args)

    library_path = fields['LIBRARY_PATH']
    include_path = fields.setdefault('INCLUDE_PATH', library_path)
    header_guard = fields.setdefault('HEADER_GUARD', library_path)
    library_name = fields.setdefault('LIBRARY_NAME', CapitalizeFirstLetter(library_path.split('/')[-1]))
    fields.setdefault('LIBRARY_API', '%s_API' % library_name.upper())

    schema_name = fields['SCHEMA_NAME']
    fields.setdefault('SCHEMA_CLASS_NAME', library_name + schema_name + 'Schema')

    fields.setdefault('RILEY_TYPE', schema_name.removeprefix('Riley'))
    
    baseName = fields.setdefault('FILE_NAME', UncapitalizeFirstLetter(schema_name) + 'Prim')

    headerName = baseName + '.h'
    headerPath = os.path.join(dstDir, headerName)
    implName = baseName + '.cpp'
    implPath = os.path.join(dstDir, implName)

    fields['CUSTOM_CODE_HEADER'] = ExtractCustomCode(headerPath)
    fields['CUSTOM_CODE_IMPL'] = ExtractCustomCode(implPath)

    if 'MEMBERS' in fields:
        fields['MEMBERS'] = ApplyUnderlayToMembers(fields['MEMBERS'])

    fields['FORWARD_DECLS'] = sorted(set(
        opt_dict['RILEY_RELATIONSHIP_TARGET']
        for name, type_name, opt_dict in fields['MEMBERS']
        if 'RILEY_RELATIONSHIP_TARGET' in opt_dict))

    for name, type_name, opt_dict in fields['MEMBERS']:
        if 'RILEY_RELATIONSHIP_TARGET' in opt_dict:
            opt_dict.setdefault('RILEY_NEEDS_OBSERVER', True)
            if type_name == T_PATHARRAY:
                opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyIdList<HdPrman_' + opt_dict['RILEY_RELATIONSHIP_TARGET'] + 'Prim>')
            else:
                opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyId<HdPrman_' + opt_dict['RILEY_RELATIONSHIP_TARGET'] + 'Prim>')
        elif 'RILEY_IS_NAME' in opt_dict:
            opt_dict.setdefault('RILEY_CONVERTER', '')
        elif type_name == T_FLOAT:
            opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyFloat')
        elif type_name == T_TOKEN:
            opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyString')
            opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyParamList')
        elif type_name == T_MATRIX:
            opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyTransform')
            opt_dict.setdefault('RILEY_NEEDS_SHUTTER_INTERVAL', True)
        elif type_name == 'HdPrmanRileyShadingNodeSchema':
            opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyShadingNode')
        elif type_name == 'HdPrmanRileyShadingNodeVectorSchema':
            opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyShadingNetwork')
        elif type_name == 'HdPrmanRileyParamListSchema':
            opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyParamList')
        elif type_name == 'HdPrmanRileyPrimvarListSchema':
            opt_dict.setdefault('RILEY_CONVERTER', 'HdPrman_RileyPrimvarList')
            opt_dict.setdefault('RILEY_NEEDS_SHUTTER_INTERVAL', True)

    if 'HEADER_TEMPLATE_NAME' in fields:
        localHeaderTemplate = env.get_template(fields['HEADER_TEMPLATE_NAME'])
    else:
        localHeaderTemplate = headerTemplate

    with open(headerPath, 'w') as f:
        f.write(localHeaderTemplate.render(**fields))

    print ('wrote:', headerName)

    if 'IMPL_TEMPLATE_NAME' in fields:
        localImplTemplate = env.get_template(fields['IMPL_TEMPLATE_NAME'])
    else:
        localImplTemplate = implTemplate

    with open(os.path.join(dstDir, implName), 'w') as f:
        f.write(localImplTemplate.render(**fields))

    print ('wrote:', implName)

def ProcessEntries(dstDir, entries, names):
    for entry in entries:
        if names and entry.get('SCHEMA_NAME') not in names:
            continue
        ProcessEntry(dstDir, **entry)

#------------------------------------------------------------------------------

def ValidateFiles(srcDir, dstDir):
    import difflib
    missing = []
    diffs = []
    for dstFile in [os.path.join(dstDir, f) for f in os.listdir(dstDir)
                    if os.path.isfile(os.path.join(dstDir, f))]:
        srcFile = os.path.join(srcDir, os.path.basename(dstFile))
        if not os.path.isfile(srcFile):
            missing.append(srcFile)
            continue
        dstContent, srcContent = open(dstFile).read(), open(srcFile).read()
        if dstContent != srcContent:
            diff = '\n'.join(difflib.unified_diff(
                srcContent.split('\n'),
                dstContent.split('\n'),
                'Source ' + os.path.basename(srcFile),
                'Generated ' + os.path.basename(dstFile)))
            diffs.append(diff)
            continue

    if missing or diffs:
        msg = []
        if missing:
            msg.append('*** Missing Generated Files: ' + ', '.join(missing))
        if diffs:
            msg.append('*** Differing Generated Files:\n' + '\n'.join(diffs))
        raise RuntimeError('\n' + '\n'.join(msg))
    else:
        print ('validation succeeded')


#------------------------------------------------------------------------------

if __name__ == '__main__':
    ap = ArgumentParser(
        description='Generate source code schemas.\n'
            'Use either --all or --names NAME1,NAME2')
    ap.add_argument('--schemaFile', default='hdSchemaDefs.py')
    ap.add_argument('--dstDir', default=os.curdir)
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--list', action='store_true')
    ap.add_argument('--names')
    ap.add_argument('--validate', action='store_true')

    args = ap.parse_args()

    names = []
    if isinstance(args.names, str):
        names.extend(args.names.split(','))

    if not names and not args.all and not args.list:
        ap.print_help()
        sys.exit(-1)

    entries = eval(open(args.schemaFile).read())
    if not isinstance(entries, list):
        raise TypeError(
            "got unexpected result type from eval('{}'): "
            "expected dict or list, found {}".format(args.schemaFile, type(v)))

    global_entry = {}
    for entry in entries:
        if entry['SCHEMA_NAME'] == 'ALL_SCHEMAS':
            global_entry = entry
            break

    entries = [ global_entry | entry
                for entry in entries 
                if entry['SCHEMA_NAME'] != 'ALL_SCHEMAS' and entry.get('GENERATE_RILEY_PRIM') ]

    if args.list:
        for entry in entries:
            print(entry.get('SCHEMA_NAME'))
        sys.exit(0)

    if args.validate:
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            ProcessEntries(tmpdir, entries, names)
            ValidateFiles(os.curdir, tmpdir)
    else:
        ProcessEntries(args.dstDir, entries, names)
