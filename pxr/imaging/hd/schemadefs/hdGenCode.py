#!/usr/bin/env python
from __future__ import print_function

from jinja2 import Environment, FileSystemLoader, Template
import re
import os
import sys
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

#------------------------------------------------------------------------------

bin_path = os.path.dirname(os.path.abspath(sys.argv[0]))
hd_path = os.path.dirname(bin_path)

defaultSchemadefsDir = os.path.join(hd_path, 'schemadefs')

print(defaultSchemadefsDir)

env = Environment(loader=FileSystemLoader(defaultSchemadefsDir))
env.filters['capitalizeFirst'] = CapitalizeFirstLetter
env.filters['snake'] = PathCamelCaseToSnakeCase
env.filters['underlyingDataSource'] = ToUnderlyingDataSource

headerTemplate = env.get_template("schema.template.h")
implTemplate = env.get_template("schema.template.cpp")

#------------------------------------------------------------------------------

def ProcessEntry(dstDir, **args):
    fields = dict(args)

    this_library_path = 'pxr/imaging/hd'

    library_path = fields.setdefault('LIBRARY_PATH', this_library_path)
    include_path = fields.setdefault('INCLUDE_PATH', library_path)
    header_guard = fields.setdefault('HEADER_GUARD', library_path)
    library_name = library_path.split('/')[-1]
    fields.setdefault('LIBRARY_API', '%s_API' % library_name.upper())

    if fields.get('IS_LOCAL'):
        baseName = fields['FILE_NAME']
    else:
        baseName = os.path.normpath(
            os.path.join(
                os.path.relpath(library_path, this_library_path),
                fields['FILE_NAME']))

    headerName = baseName + '.h'
    implName = baseName + '.cpp'

    if 'HEADER_TEMPLATE_NAME' in fields:
        localHeaderTemplate = env.get_template(fields['HEADER_TEMPLATE_NAME'])
    else:
        localHeaderTemplate = headerTemplate

    with open(os.path.join(dstDir, headerName), 'w') as f:
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

    entries = []

    for defFilePath in ('schemadefs/defs.py', 'hdSchema.py'):
        if os.path.isfile(defFilePath):
            v = eval(open(defFilePath).read())
            if isinstance(v, list):
                for e in v:
                    e['IS_LOCAL'] = (defFilePath == 'hdSchema.py')
                entries.extend(v)
            else:
                raise TypeError(
                    "got unexpected result type from eval('{}'): "
                    "expected dict or list, found {}".format(defFilePath, type(v)))

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
