#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
"""
This script generates C++ classes and supporting Python code for USD Schemata.
It is driven by a USD layer (schema.usda) that defines the schema classes.

This USD layer must meet the following preconditions in order for code to be
generated that will compile and work with USD Core successfully:
    
    * Must specify the libraryName as layer metadata.
    * Schema typenames must be unique across all libraries.
    * Attribute names and tokens must be camelCased valid identifiers.
    * usd/schema.usda must exist in the LayerStack, not necessarily as a 
        directly subLayer.
"""

import sys, os, re, inspect
from argparse import ArgumentParser
from collections import namedtuple

from jinja2 import Environment, FileSystemLoader
from jinja2.exceptions import TemplateSyntaxError

from pxr import Sdf, Usd, Tf

#------------------------------------------------------------------------------#
# Parsed Objects                                                               #
#------------------------------------------------------------------------------#

def _SanitizeDoc(doc, leader):
    """Cleanup the doc string in several ways:
      * Convert None to empty string
      * Replace new line chars with doxygen comments
      * Strip leading white space per line
    """
    if doc is None:
        return ''
    
    return leader.join([line.lstrip() for line in doc.split('\n')])


def _ListOpToList(listOp):
    """Return either the explicitItems or the addedItems if listOp
    """
    if not listOp:
        return [] 

    return listOp.explicitItems if listOp.explicitItems else listOp.addedItems


def _GetDefiningLayerAndPrim(stage, schemaName):
    """ Searches the stage LayerStack for a prim whose name is equal to
        schemaName.
    """
    # SchemaBase is not actually defined in the core schema file, but this
    # behavior causes the code generator to produce correct C++ inheritance.
    if schemaName == 'SchemaBase':
        return (stage.GetLayerStack()[-1], None)
    
    else:
        for layer in stage.GetLayerStack():
            for sdfPrim in layer.rootPrims:
                if sdfPrim.name == schemaName:
                    return (layer, sdfPrim)

    raise Exception("Could not find the defining layer for schema: %s" % schemaName)
                    

def _GetLibMetadata(layer):
    """ Return a dictionary of library-specific data found in layer."""
    
    globalPrim = layer.GetPrimAtPath('/GLOBAL')
    if not globalPrim:
        raise Exception("Code generation requires a \"/GLOBAL\" prim with "
            "customData to define at least libraryName and libraryPath. "
            "GLOBAL prim not found.")
    
    if not globalPrim.customData:
        raise Exception("customData is either empty or not defined on /GLOBAL "
            "prim. At least \"libraryName\" and \"libraryPath\" entries in "
            "customData are required for code generation.")
    
    # Return a copy of customData to avoid accessing an invalid map proxy during
    # template rendering.
    return dict(globalPrim.customData)


def _GetLibName(layer):
    """ Return the libraryName defined in layer."""
    
    libData = _GetLibMetadata(layer)
    if 'libraryName' not in libData:
        raise Exception("Code generation requires that \"libraryName\" be defined "
            "in customData on /GLOBAL prim.")

    return libData['libraryName']
    

def _GetLibPath(layer):
    """ Return the libraryPath defined in layer."""

    libData = _GetLibMetadata(layer)
    if 'libraryPath' not in libData:
        raise Exception("Code generation requires that \"libraryPath\" be "
            "defined in customData on /GLOBAL prim.  The format for "
            "libraryPath is \"path/to/lib\".")

    return libData['libraryPath']


def _GetLibPrefix(layer):
    """ Return the libraryPrefix defined in layer. If not defined, fall
    back to ProperCase(libraryName). Used to prefix all class names"""
    
    return _GetLibMetadata(layer).get('libraryPrefix', 
        _ProperCase(_GetLibName(layer)))


def _GetTokensPrefix(layer):
    """ Return the tokensPrefix defined in layer."""

    return _GetLibMetadata(layer).get('tokensPrefix', 
        _GetLibPrefix(layer))

    
def _UpperCase(aString):
    return aString.upper()


def _LowerCase(aString):
    return aString.lower()


def _ProperCase(aString):
    """Returns the given string (camelCase or ProperCase) in ProperCase,
        stripping out any non-alphanumeric characters.
    """
    if len(aString) > 1:
        return ''.join([s[0].upper() + s[1:] for s in re.split(r'\W+', aString)])
    else:
        return aString.upper()


def _CamelCase(aString):
    """Returns the given string (camelCase or ProperCase) in camelCase,
        stripping out any non-alphanumeric characters.
    """
    if len(aString) > 1:
        pcase = _ProperCase(aString)
        return pcase[0].lower() + pcase[1:]
    else:
        return aString.lower()

    
Token = namedtuple('Token', ['id', 'value', 'desc'])

class PropInfo(object):
    def __init__(self, sdfProp):
        # Allow user to specify custom naming through customData metadata.
        self.customData = dict(sdfProp.customData)

        self.name       = _CamelCase(sdfProp.name)
        self.apiName    = self.customData.get('apiName', self.name)
        self.rawName    = sdfProp.name
        self.doc        = _SanitizeDoc(sdfProp.documentation, '\n    /// ')
        self.custom     = sdfProp.custom

class RelInfo(PropInfo):
    def __init__(self, sdfProp):
        super(RelInfo, self).__init__(sdfProp)

# Map an Sdf.ValueTypeName.XXX object to the 'XXX' token string -- we use this
# to go from sdf attribute types to their symbolic tokens, for example:
# Sdf.ValueTypeNames.UInt -> 'UInt'.
valueTypeNameToStr = dict(
    [(getattr(Sdf.ValueTypeNames, n), n)
     for n in dir(Sdf.ValueTypeNames)
     if isinstance(getattr(Sdf.ValueTypeNames, n), Sdf.ValueTypeName)])

class AttrInfo(PropInfo):
    def __init__(self, sdfProp):
        super(AttrInfo, self).__init__(sdfProp)
        self.allowedTokens = sdfProp.GetInfo('allowedTokens')
        
        self.variability = str(sdfProp.variability).replace('Sdf.', 'Sdf')
        self.fallback = sdfProp.default

        self.cppType = sdfProp.typeName.type.typeName
        # XXX: not sure why, but std::string maps to string, perhaps a
        # result of calling this from Python. Remap it to std::string here
        # manually.
        if self.cppType == 'string':
            self.cppType = 'std::string'

        self.usdType = "SdfValueTypeNames->%s" % (
            valueTypeNameToStr[sdfProp.typeName])
        
        self.details = [('C++ Type', self.cppType),
                        ('Usd Type', self.usdType),
                        ('Variability', self.variability),
                        ('Fallback Value', 'No Fallback'
                         if self.fallback is None else str(self.fallback))]
        if self.allowedTokens:
            self.details.append(('\\ref ' + \
                _GetTokensPrefix(sdfProp.layer) + \
                'Tokens "Allowed Values"', str(self.allowedTokens)))

def _ExtractNames(sdfPrim, customData):
    usdPrimTypeName = sdfPrim.path.name
    className = customData.get('className', _ProperCase(usdPrimTypeName))
    cppClassName = _GetLibPrefix(sdfPrim.layer) + className
    baseFileName = customData.get('fileName', _CamelCase(className))
    
    return usdPrimTypeName, className, cppClassName, baseFileName


class ClassInfo(object):
    def __init__(self, usdPrim, sdfPrim):
        # First validate proper class naming...
        if (sdfPrim.typeName != sdfPrim.path.name and
            sdfPrim.typeName != ''):
            raise Exception("Code generation requires that every instantiable "
                            "class's name must match its declared type "
                            "('%s' and '%s' do not match.)" % 
                            (sdfPrim.typeName, sdfPrim.path.name))
        
        # NOTE: usdPrim should ONLY be used for querying information regarding
        # the class's parent in order to avoid duplicating class members during
        # code generation.
        inherits = usdPrim.GetMetadata('inheritPaths') 
        inheritsList = _ListOpToList(inherits)

        # Allow user to specify custom naming through customData metadata.
        self.customData = dict(sdfPrim.customData)

        # For accumulation of AttrInfo objects
        self.attrs = {}
        self.rels = {}
        self.attrOrder = []
        self.relOrder = []
        self.tokens = set()

        # Important names
        (self.usdPrimTypeName,
         self.className,
         self.cppClassName,
         self.baseFileName) = _ExtractNames(sdfPrim, self.customData)

        # Class Parent's Info
        parentClass = inheritsList[0].name if inheritsList else 'SchemaBase'
        (parentLayer,
         parentPrim) = _GetDefiningLayerAndPrim(usdPrim.GetStage(), parentClass)
        self.parentLibPath = _GetLibPath(parentLayer)
        parentCustomData = {}
        if parentPrim is not None:
            parentCustomData = dict(parentPrim.customData)
            (parentUsdName, parentClassName,
             self.parentCppClassName, self.parentBaseFileName) = \
             _ExtractNames(parentPrim, parentCustomData)
        else:
            self.parentCppClassName = "UsdSchemaBase"
            self.parentBaseFileName = "schemaBase"

        # Extra Class Metadata
        self.doc = _SanitizeDoc(sdfPrim.documentation, '\n/// ')
        self.typeName = sdfPrim.typeName
        self.extraIncludes = self.customData.get('extraIncludes', None)

        # Do not to inherit the type name of parent classes.
        if inherits:
            for path in inherits.addedOrExplicitItems:
                parentTypeName = parentLayer.GetPrimAtPath(path).typeName
                if parentTypeName == self.typeName:
                    self.typeName = ''

        self.isConcrete = 'false' if not self.typeName else 'true'


    def GetHeaderFile(self):
        return self.baseFileName + '.h'

    def GetParentHeaderFile(self):
        return self.parentBaseFileName + '.h'

    def GetCppFile(self):
        return self.baseFileName + '.cpp'

    def GetWrapFile(self):
        return 'wrap' + self.className + '.cpp'


#------------------------------------------------------------------------------#
# USD PARSER                                                                   #
#------------------------------------------------------------------------------#

def ParseUsd(usdFilePath):
    sdfLayer = Sdf.Layer.FindOrOpen(usdFilePath)
    stage = Usd.Stage.Open(sdfLayer)
    classes = []

    # PARSE CLASSES
    for sdfPrim in sdfLayer.rootPrims:
        if sdfPrim.name == "Typed" or sdfPrim.specifier != Sdf.SpecifierClass:
            continue

        usdPrim = stage.GetPrimAtPath(sdfPrim.path)
        classInfo = ClassInfo(usdPrim, sdfPrim)
        classes.append(classInfo)
        #
        # We don't want to use the composed property names here because we only
        # want the local properties declared directly on the class, which the
        # "properties" metadata field provides.
        #
        if sdfPrim.properties:
            attrApiNames = []
            relApiNames = []
            for sdfProp in sdfPrim.properties:
                
                # Attribute
                usdAttr = usdPrim.GetAttribute(sdfProp.name)
                if usdAttr:
                    attrInfo = AttrInfo(sdfProp)

                    # Assert unique attribute names
                    if attrInfo.name in classInfo.attrs: 
                        raise Exception(
                            'Schema Attribute names must be unique, '
                            'irrespective of namespacing. '
                            'Duplicate name encountered: %s.%s' %
                            (classInfo.usdPrimTypeName, attrInfo.name))
                    elif attrInfo.apiName in attrApiNames:
                        raise Exception(
                            'Schema Attribute API names must be unique. '
                            'Duplicate apiName encountered: %s.%s' %
                            (classInfo.usdPrimTypeName, attrInfo.apiName))
                    else:
                        attrApiNames.append(attrInfo.apiName)
                        classInfo.attrs[attrInfo.name] = attrInfo
                        classInfo.attrOrder.append(attrInfo.name)
                
                # Relationship
                else:
                    relInfo = RelInfo(sdfProp)
                    
                    # Assert unique relationship names
                    if relInfo.name in classInfo.rels: 
                        raise Exception(
                            'Schema Relationship names must be unique, '
                            'irrespective of namespacing. '
                            'Duplicate name encountered: %s.%s' %
                            (classInfo.usdPrimTypeName, relInfo.name))
                    elif relInfo.apiName in relApiNames:
                        raise Exception(
                            'Schema Relationship API names must be unique. '
                            'Duplicate apiName encountered: %s.%s' %
                            (classInfo.usdPrimTypeName, relInfo.apiName))
                    else:
                        relApiNames.append(relInfo.apiName)
                        classInfo.rels[relInfo.name] = relInfo
                        classInfo.relOrder.append(relInfo.name)

    return (_GetLibName(sdfLayer),
            _GetLibPath(sdfLayer),
            _GetLibPrefix(sdfLayer),
            _GetTokensPrefix(sdfLayer),
            _GetLibMetadata(sdfLayer).get('libraryTokens', {}),
            classes)


#------------------------------------------------------------------------------#
# CODE GENERATOR                                                               #
#------------------------------------------------------------------------------#

def _WriteFile(filePath, content):
    import difflib
    # If file currently exists and content is unchanged, do nothing.
    existingContent = '\n'
    content = (content + '\n'
               if content and not content.endswith('\n') else content)
    if os.path.exists(filePath):
        existingContent = open(filePath, 'r').read()
        if existingContent == content:
            print '\tunchanged %s' % filePath
            return
    # Otherwise attempt to write to file.
    try:
        with open(filePath, 'w') as curfile:
            curfile.write(content)
            print '\t    wrote %s' % filePath
    except IOError as ioe:
        print '\t', ioe
        print 'Diff:'
        print '\n'.join(difflib.unified_diff(existingContent.split('\n'),
                                             content.split('\n')))

def _ExtractCustomCode(filePath, default=None):
    defaultTxt = default if default else ''
    
    if not os.path.exists(filePath):
        return defaultTxt

    try:
        with open(filePath, 'r') as src:
            existing = src.read()
            parts = existing.split('// --(BEGIN CUSTOM CODE)--\n')
            if len(parts) != 2 or not parts[1].strip():
                return defaultTxt
            return parts[1]
                
    except Exception as e:
        print e
        return defaultTxt


def _AddToken(tokenDict, tokenId, val, desc):
    # if token is a reserved word in either language, append with underscore
    reserved = set(['class', 'default', 'def', 'case', 'switch', 'break',
                    'if', 'else', 'struct', 'template'])
    if tokenId in reserved:
        tokenId = tokenId + '_'
    if tokenId in tokenDict:
        token = tokenDict[tokenId]

        # Assert one-to-one mapping of token.id -> token.value
        if token.value != val:
            raise Exception(
             'Token identifiers must map to exactly one token value. '
             'One-to-Many mapping encountered: %s maps to \"%s\" and \"%s\"'
             % (token.id, token.value, val))
        
        # Update Description
        tokenDict[tokenId] = token._replace(
            desc=desc + ', ' + token.desc)
    
    else:
        tokenDict[tokenId] = Token(tokenId, val, desc)


def GatherTokens(classes, libName, libTokens):
    tokenDict = {}

    # Add tokens from all classes to the token set
    for cls in classes:
        for attr in cls.attrs.values():

            # Add Attribute Names to token set
            cls.tokens.add(attr.name)
            _AddToken(tokenDict, attr.name, attr.rawName, cls.cppClassName)

            
            # Add default value (if token type) to token set
            if attr.usdType == 'SdfValueTypeNames->Token' and attr.fallback:
                fallbackName = _CamelCase(attr.fallback)
                desc = 'Default value for %s::Get%sAttr()' % \
                       (cls.cppClassName, _ProperCase(attr.name))
                cls.tokens.add(fallbackName)
                _AddToken(tokenDict, fallbackName, attr.fallback, desc)
            
            # Add Allowed Tokens for this attribute to token set
            if attr.allowedTokens:
                for val in attr.allowedTokens:
                    tokenId = _CamelCase(val)
                    desc = 'Possible value for %s::Get%sAttr()' % \
                           (cls.cppClassName, _ProperCase(attr.name))
                    cls.tokens.add(tokenId)
                    _AddToken(tokenDict, tokenId, val, desc)
                    
        # Add Relationship Names to token set
        for rel in cls.rels.values():
            cls.tokens.add(rel.name)
            _AddToken(tokenDict, rel.name, rel.rawName, cls.cppClassName)
            
    # Add library-wide tokens to token set
    for token, tokenInfo in libTokens.iteritems():
        _AddToken(tokenDict, token, tokenInfo.get("value", token), _SanitizeDoc(tokenInfo.get("doc",
            "Special token for the %s library." % libName), ' '))

    return sorted(tokenDict.values(), key=lambda token: token.id.lower())


def GenerateCode(codeGenPath, tokenData, classes, env):
    #
    # Load Templates
    #
    print 'Loading Templates'
    try:
        headerTemplate = env.get_template('schemaClass.h')
        sourceTemplate = env.get_template('schemaClass.cpp')
        wrapTemplate = env.get_template('wrapSchemaClass.cpp')
        tokensHTemplate = env.get_template('tokens.h')
        tokensCppTemplate = env.get_template('tokens.cpp')
        tokensWrapTemplate = env.get_template('wrapTokens.cpp')
        plugInfoTemplate = env.get_template('plugInfo.json')
    
    except TemplateSyntaxError as tse:
        print '\t', tse,
        print 'Aborting GenerateCode...'
        return
    
    if tokenData:
        print 'Writing Schema Tokens:'
        # tokens.h
        _WriteFile(os.path.join(codeGenPath, 'tokens.h'),
                   tokensHTemplate.render(tokens=tokenData))
        # tokens.cpp
        _WriteFile(os.path.join(codeGenPath, 'tokens.cpp'),
                   tokensCppTemplate.render())
        # wrapTokens.cpp
        _WriteFile(os.path.join(codeGenPath, 'wrapTokens.cpp'),
                   tokensWrapTemplate.render(tokens=tokenData))

    #
    # Generate Schema Class Files
    #
    print 'Generating Classes:'

            
    for cls in classes:
        hasTokenAttrs = any(
            [cls.attrs[attr].usdType == 'SdfValueTypeNames->Token' for attr in cls.attrs])

        # header file
        clsHFilePath = os.path.join(codeGenPath, cls.GetHeaderFile())
        customCode = _ExtractCustomCode(clsHFilePath, default='};\n\n#endif\n')
        _WriteFile(clsHFilePath,
                   headerTemplate.render(
                       cls=cls, hasTokenAttrs=hasTokenAttrs) + customCode)

        # source file
        clsCppFilePath = os.path.join(codeGenPath, cls.GetCppFile())
        customCode = _ExtractCustomCode(clsCppFilePath)
        _WriteFile(clsCppFilePath, 
                   sourceTemplate.render(cls=cls) + customCode)
        
        # wrap file
        clsWrapFilePath = os.path.join(codeGenPath, cls.GetWrapFile())
        customCode = _ExtractCustomCode(clsWrapFilePath, default=
                                        '\nWRAP_CUSTOM {\n}\n')
        _WriteFile(clsWrapFilePath,
                   wrapTemplate.render(cls=cls) + customCode)

    #
    # Generate plugInfo.json.
    #
    if classes:
        import json
        # read existing plugInfo file, strip comments.
        plugInfoFile = os.path.join(codeGenPath, 'plugInfo.json')
        if os.path.isfile(plugInfoFile):
            infoLines = open(plugInfoFile).readlines()
            infoLines = [l for l in infoLines
                         if not l.strip().startswith('#')]
            # parse as json.
            try:
                info = json.loads(''.join(infoLines))
            except ValueError as ve:
                print '\t', ve, 'reading', plugInfoFile
        else:
            # use plugInfo.json template as starting point for new files,
            try:
                info = json.loads(plugInfoTemplate.render())
            except ValueError as ve:
                print '\t', ve, 'from template', plugInfoTemplate.filename

        # pull the types dictionary.
        if 'Plugins' in info:
            for pluginData in info.get('Plugins', {}):
                if pluginData.get('Name') == env.globals['libraryName']:
                    types = (pluginData
                             .setdefault('Info', {})
                             .setdefault('Types', {}))
                    break
            else:
                print '\t', 'Could not find plugin metadata section for ', \
                    env.globals['libraryName']
        else:
            types = info.setdefault('Types', {})
        # remove auto-generated types.
        for tname in types.keys():
            if types[tname].get('autoGenerated', False):
                del types[tname]
        # generate types entries.
        for cls in classes:
            clsDict = {'bases': [cls.parentCppClassName],
                       'autoGenerated': True }
            # add alias for concrete types.
            if cls.isConcrete == 'true':
                clsDict['alias'] = {'UsdSchemaBase': cls.usdPrimTypeName}
            types[cls.cppClassName] = clsDict
        # write plugInfo file back out.
        content = ((
            "# Portions of this file auto-generated by %s.\n"
            "# Edits will survive regeneration except for comments and\n"
            "# changes to types with autoGenerated=true.\n"
            % os.path.basename(os.path.splitext(sys.argv[0])[0])) + 
                   json.dumps(info, indent=4, sort_keys=True))
        _WriteFile(plugInfoFile, content)


def _MakeFlattenedRegistryLayer(filePath):
    # We flatten the schema classes to 'eliminate' composition when querying
    # the schema registry.
    stage = Usd.Stage.Open(filePath)

    # Certain qualities on builtin properties are not overridable in scene
    # description.  For example, builtin attributes' types always come from the
    # definition registry, never from scene description.  That's problematic in
    # this case where we're trying to *establish* the definition registry.
    # Since the classes in schema.usda use the real prim type names
    # (e.g. 'Mesh') Usd will pick up these built-in qualities from the existing
    # prim definition.  To side-step this, we temporarily override the prim type
    # names in the session layer so they do not pick up these special
    # nonoverridable qualities when we flatten.  Then after the fact we repair
    # all the type names.

    def mangle(typeName):
        return '__MANGLED_TO_AVOID_BUILTINS__' + typeName
    def demangle(typeName):
        return typeName.replace('__MANGLED_TO_AVOID_BUILTINS__', '')

    # Mangle the typeNames.
    with Usd.EditContext(stage, editTarget=stage.GetSessionLayer()):
        for cls in stage.GetPseudoRoot().GetAllChildren():
            if cls.GetTypeName():
                cls.SetTypeName(mangle(cls.GetTypeName()))

    flatLayer = stage.Flatten(addSourceFileComment=False)

    # Demangle the typeNames.
    for cls in flatLayer.rootPrims:
        if cls.typeName:
            cls.typeName = demangle(cls.typeName)

    # In order to prevent derived classes from inheriting base class
    # documentation metadata, we must manually replace docs here.
    for layer in stage.GetLayerStack():
        for cls in layer.rootPrims:
            flatCls = flatLayer.GetPrimAtPath(cls.path)
            if cls.HasInfo('documentation'):
                flatCls.SetInfo('documentation', cls.documentation)
            else:
                flatCls.ClearInfo('documentation')

    return flatLayer
    

def GenerateRegistry(codeGenPath, filePath, classes, env):

    # Get the flattened layer to work with.
    flatLayer = _MakeFlattenedRegistryLayer(filePath)

    # Delete GLOBAL prim and other schema prim customData so it doesn't pollute
    # the schema registry.  Also remove any definitions we included from
    # lower-level schema modules.  We hop back up to the UsdStage API to do
    # so because it is more convenient for these kinds of operations.
    flatStage = Usd.Stage.Open(flatLayer)
    pathsToDelete = []
    primsToKeep = set(cls.usdPrimTypeName for cls in classes)
    if not flatStage.RemovePrim('/GLOBAL'):
        print "WARNING: Could not remove GLOBAL prim."
    for p in flatStage.GetPseudoRoot().GetAllChildren():
        p.ClearCustomData()
        for myproperty in p.GetAuthoredProperties():
            myproperty.ClearCustomData()
        if p.GetName() not in primsToKeep:
            pathsToDelete.append(p.GetPath())
    for p in pathsToDelete:
        flatStage.RemovePrim(p)
        
    # Set layer's comment to indicate that the file is generated.
    flatLayer.comment = 'WARNING: THIS FILE IS GENERATED.  DO NOT EDIT.'

    #
    # Generate Schematics
    #
    print 'Generating Schematics:'
    layerSource = flatLayer.ExportToString()

    # Remove doxygen tags from schema registry docs.
    # ExportToString escapes '\' again, so take that into account.
    layerSource = layerSource.replace(r'\\em ', '')
    layerSource = layerSource.replace(r'\\li', '-')
    layerSource = re.sub(r'\\+ref [^\s]+ ', '', layerSource)
    layerSource = re.sub(r'\\+section [^\s]+ ', '', layerSource)

    _WriteFile(os.path.join(codeGenPath, 'generatedSchema.usda'), layerSource)

if __name__ == '__main__':
    #
    # Parse Command-line
    #
    parser = ArgumentParser(description='Generate C++ schema class code from a '
        'USD file.')
    parser.add_argument('schemaPath',
        nargs='?',
        type=str,
        default='./schema.usda',
        help='The source USD file where schema classes are defined. '
        '[Default: %(default)s]')
    parser.add_argument('codeGenPath',
        nargs='?',
        type=str,
        default='.',
        help='The target directory where the code should be generated. '
        '[Default: %(default)s]')

    defaultTemplateDir = os.path.join(
        os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))),
        'codegenTemplates')

    parser.add_argument('-t', '--templates',
        dest='templatePath',
        type=str,
        default=defaultTemplateDir,
        help='The directory of the schema class templates. '
        '[Default: %(default)s')

    args = parser.parse_args()
    codeGenPath = os.path.abspath(args.codeGenPath)
    schemaPath = os.path.abspath(args.schemaPath)
    templatePath = os.path.abspath(args.templatePath)

    #
    # Error Checking
    #
    if not os.path.isfile(schemaPath):
        print 'Usage Error: First positional argument must be a USD schema file.'
        parser.print_help()
        sys.exit(1)
    if not os.path.isdir(codeGenPath):
        print 'Usage Error: Second positional argument must be a directory to contain generated code.'
        parser.print_help()
        sys.exit(1)
    if not os.path.isdir(templatePath):
        print 'Usage Error: templatePath argument must be the path to the codgenTemplates.'
        parser.print_help()
        sys.exit(1)

    try:
        
        #
        # Gather Schema Class information
        #
        libName, \
        libPath, \
        libPrefix, \
        tokensPrefix, \
        libTokens, \
        classes = ParseUsd(schemaPath)
        tokenData = GatherTokens(classes, libName, libTokens)
        print 'Processing schema classes:' 
        print ', '.join(map(lambda self: self.usdPrimTypeName, classes))

        #
        # Generate Code from Templates
        #
        j2_env = Environment(loader=FileSystemLoader(templatePath),
                             trim_blocks=True)
        j2_env.globals.update(Camel=_CamelCase,
                              Proper=_ProperCase,
                              Upper=_UpperCase,
                              Lower=_LowerCase,
                              libraryName=libName,
                              libraryPath=libPath,
                              libraryPrefix=libPrefix,
                              tokensPrefix=tokensPrefix)
        GenerateCode(codeGenPath, tokenData, classes, j2_env)
        GenerateRegistry(codeGenPath, schemaPath, classes, j2_env)
    
    except Exception as e:
        print "ERROR: ", str(e)
        sys.exit(1)
