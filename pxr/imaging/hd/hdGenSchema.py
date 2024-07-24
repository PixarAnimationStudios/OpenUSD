#!/pxrpythonsubst
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
T_PATHEXPRESSION = "HdPathExpressionDataSource"
T_ASSETPATH = "HdAssetPathDataSource"
T_VEC2I = "HdVec2iDataSource"
T_VEC2F = "HdVec2fDataSource"
T_VEC2D = "HdVec2dDataSource"
T_VEC2FARRAY = "HdVec2fArrayDataSource"
T_VEC2DARRAY = "HdVec2dArrayDataSource"
T_VEC3I = "HdVec3iDataSource"
T_VEC3IARRAY = "HdVec3iArrayDataSource"
T_VEC3F = "HdVec3fDataSource"
T_VEC3D = "HdVec3dDataSource"
T_VEC3FARRAY = "HdVec3fArrayDataSource"
T_VEC3DARRAY = "HdVec3dArrayDataSource"
T_VEC4I = "HdVec4iDataSource"
T_VEC4IARRAY = "HdVec4iArrayDataSource"
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

def AddFilters(env):
    env.filters['capitalizeFirst'] = CapitalizeFirstLetter
    env.filters['snake'] = PathCamelCaseToSnakeCase
    env.filters['underlyingDataSource'] = ToUnderlyingDataSource
    env.filters['makeComment'] = ToComment
    env.filters['tokenName'] = ToTokenName
    env.filters['expand'] = Expand

def GetTemplates():
    templatesDir = 'codegenTemplates'

    bin_path = os.path.dirname(os.path.abspath(sys.argv[0]))
    templatesPath = os.path.join(bin_path, templatesDir)

    if not os.path.isdir(templatesPath):
        from pxr import Plug
        hd_path = os.path.abspath(
            Plug.Registry().GetPluginWithName('hd').resourcePath)
        templatesPath = os.path.join(hd_path, templatesDir)

    env = Environment(loader=FileSystemLoader(templatesPath))
    AddFilters(env)
    
    return { 'HEADER' : env.get_template("schemaClass.h"),
             'IMPL' : env.get_template("schemaClass.cpp") }

#------------------------------------------------------------------------------

customCodeRegEx = r'// --\(BEGIN CUSTOM CODE: (?P<word>[\w ]+)\)--\n(.*)\n// --\(END CUSTOM CODE: (?P=word)\)--'

def ExtractCustomCode(path):
    if not os.path.isfile(path):
        return {}
    with open(path, 'r') as f:
        contents = f.read()
    return dict(
        (key, text)
        for key, text in re.findall(customCodeRegEx, contents, re.DOTALL)
        # Drop empty custom code section.
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

def ExpandEntry(srcDir, fields):
    library_path = fields['LIBRARY_PATH']
    include_path = fields.setdefault('INCLUDE_PATH', library_path)
    header_guard = fields.setdefault('HEADER_GUARD', library_path)
    library_name = fields.setdefault('LIBRARY_NAME', CapitalizeFirstLetter(library_path.split('/')[-1]))
    fields.setdefault('LIBRARY_API', '%s_API' % library_name.upper())

    schema_name = fields['SCHEMA_NAME']
    fields.setdefault('SCHEMA_CLASS_NAME', library_name + schema_name + 'Schema')

    baseName = fields.setdefault('FILE_NAME', UncapitalizeFirstLetter(schema_name) + 'Schema')

    fields.setdefault('HEADER_NAME', baseName + '.h')
    fields.setdefault('IMPL_NAME', baseName + '.cpp')

    fields['CUSTOM_CODE_HEADER'] = ExtractCustomCode(
        os.path.join(srcDir, fields['HEADER_NAME']))
    fields['CUSTOM_CODE_IMPL'] = ExtractCustomCode(
        os.path.join(srcDir, fields['IMPL_NAME']))

    if 'MEMBERS' in fields:
        fields['MEMBERS'] = ApplyUnderlayToMembers(fields['MEMBERS'])

def WriteEntry(dstDir, fields, templates):
    headerName = fields['HEADER_NAME']
    with open(os.path.join(dstDir, headerName), 'w') as f:
        f.write(templates['HEADER'].render(**fields))
    print('wrote:', headerName)

    implName = fields['IMPL_NAME']
    with open(os.path.join(dstDir, implName), 'w') as f:
        f.write(templates['IMPL'].render(**fields))
    print('wrote:', implName)

def FilterEntriesByNames(entries, names):
    names_set = set(names)

    entries = [ entry
                for entry in entries
                if entry['SCHEMA_NAME'] in names_set ]
    if len(entries) != len(names):
        existing_names = set(entry['SCHEMA_NAME'] for entry in entries)
        for name in sorted(names_set - existing_names):
            print("Warning: No schema with name %s!!!" % name)

    return entries

def ApplyUnderlayToEntries(entries):
    global_entry = {}
    for entry in entries:
        if entry['SCHEMA_NAME'] == 'ALL_SCHEMAS':
            global_entry = entry
            break

    return [ global_entry | entry
             for entry in entries 
             if entry['SCHEMA_NAME'] != 'ALL_SCHEMAS' ]

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
    ap.add_argument('--srcDir', default=os.curdir)
    ap.add_argument('--dstDir', default=os.curdir)
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--list', action='store_true')
    ap.add_argument('--names')
    ap.add_argument('--validate', action='store_true')

    args = ap.parse_args()

    if args.names is None:
        names = None
    else:
        names = args.names.split(',')

    if names is None and not args.all and not args.list:
        ap.print_help()
        sys.exit(-1)

    entries = eval(open(args.schemaFile).read())
    if not isinstance(entries, list):
        raise TypeError(
            "got unexpected result type from eval('{}'): "
            "expected dict or list, found {}".format(args.schemaFile, type(v)))

    entries = ApplyUnderlayToEntries(entries)

    if names is not None:
        entries = FilterEntriesByNames(entries, names)

    templates = GetTemplates()

    if args.list:
        for entry in entries:
            print(entry.get('SCHEMA_NAME'))
    elif args.validate:
        import tempfile
        with tempfile.TemporaryDirectory() as tmpdir:
            for entry in entries:
                ExpandEntry(args.srcDir, entry)
                WriteEntry(tmpdir, entry, templates)
            ValidateFiles(args.srcDir, tmpdir)
    else:
        for entry in entries:
            ExpandEntry(args.srcDir, entry)
            WriteEntry(args.dstDir, entry, templates)
