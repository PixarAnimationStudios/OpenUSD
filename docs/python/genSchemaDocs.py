#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""
This script generates Sphinx documentation (in "Myst" Markdown form)
for USD schemas with user documentation content. It takes as input 
a schema's "schemaUserDoc.usda" (and path) and produces markdown content
with the schema class and property docs, along with any overview documentation
(if provided by the schema).
"""

import sys
import os
from argparse import ArgumentParser
import re
import shutil
import datetime
from collections import namedtuple

# Need to set the environment variable for disabling the schema registry's
# loading of schema type prim definitions before importing any pxr libraries,
# otherwise this setting won't take. We disable this to make sure we don't try
# to load generatedSchema.usda files (which usdGenSchema generates) during
# our processing of schema definitions, to ensure this script is really only
# looking at our loaded schema.usda / schemaUserDoc.usda information.
os.environ["USD_DISABLE_PRIM_DEFINITIONS_FOR_USDGENSCHEMA"] = "1"
from pxr import Ar, Plug, Sdf, Usd, Vt, Tf

#------------------------------------------------------------------------------#
# Tokens                                                                       #
#------------------------------------------------------------------------------#

# Name of script, e.g. "genSchemaDocs"
PROGRAM_NAME = os.path.splitext(os.path.basename(sys.argv[0]))[0]

# Custom metadata tokens for user doc
USERDOC_BRIEF = "userDocBrief"
USERDOC_FULL = "userDoc"

SCHEMAUSERDOC_FILENAME = "schemaUserDoc.usda"
SCHEMA_FILENAME = "schema.usda"
SCHEMAUSERDOC_DIRNAME = "userDoc"

#------------------------------------------------------------------------------#
# Helper classes                                                               #
#------------------------------------------------------------------------------#

class _Printer():
    """
    Object used for printing. This gives us a way to control output with regards 
    to program arguments such as --quiet.
    """
    def __init__(self, quiet=False):
        self._quiet = quiet

    def __PrintImpl(self, stream, *args):
        if len(args):
            print(*args, file=stream)
        else:
            print(file=stream)

    def __call__(self, *args):
        if not self._quiet:
            self.__PrintImpl(sys.stdout, *args)

    def Err(self, *args):
        self.__PrintImpl(sys.stderr, *args)

    def SetQuiet(self, quiet):
        self._quiet = quiet

Print = _Printer()

# Named tuple to hold data needed to document a property
_PropertyData = namedtuple("_PropertyData",
    "name className doc valueType defaultValue")

class _SchemaDoc:
    """
    Convenience data class for schema class userDoc
    """
    def __init__(self, schemaDomain, schemaName, userDoc):
        self.schemaDomain = schemaDomain
        self.schemaName = schemaName
        self.userDoc = userDoc
        self.propertyDoc = {}
        self.resourceFiles = []
        resourceFiles = _GetResourceFilesFromMarkdownString(userDoc)
        self.resourceFiles.extend(resourceFiles)
        self.markdownWriter = _SchemaDocMarkdownGenerator(self.schemaDomain,
            self.schemaName)
        self.inheritsList = []

    def SetInheritsList(self, inheritsClassList):
        """
        Set the inherits list for this schema
        """
        self.inheritsList = list(inheritsClassList)

    def AddPropertyDoc(self, className, propertyName, propertyDoc, propertyType,
        defaultValue):
        """
        Add the userDoc for a property. className refers to the class where
        the property was defined, and helps track inherited properties.
        """
        if className not in self.propertyDoc:
            self.propertyDoc[className] = {}
        self.propertyDoc[className][propertyName] = _PropertyData(propertyName,
            className, propertyDoc, propertyType, defaultValue)
        # Extract image/resource files in propertyDoc, if any
        resourceFiles = _GetResourceFilesFromMarkdownString(propertyDoc)
        self.resourceFiles.extend(resourceFiles)

    def WriteMystMarkdownFile(self, filepath):
        """
        Write schema class and property user doc to "schema.md" Myst markdown file
        Returns path+filename to written file
        """
        # For now, writing as basic markdown. If we need additional features
        # (e.g. tables, etc), consider using https://python-markdown.github.io/
        filename = os.path.join(os.path.abspath(filepath), self.schemaName + ".md")
        try:
            with open(filename, 'w') as file:
                self.markdownWriter.WriteHeader(file)
                self.markdownWriter.WriteSchema(file, self.userDoc)
                if len(self.propertyDoc) > 0:
                    self.markdownWriter.WritePropertiesTitle(file)
                    # List non-inherited properties first (if any)
                    if self.schemaName in self.propertyDoc:
                        nonInheritedProps = self.propertyDoc[self.schemaName]
                        for prop in sorted(nonInheritedProps):
                            self.markdownWriter.WriteProperty(file, nonInheritedProps[prop])
                    # Followed by list of inherited properties (again, if any)
                    if len(self.propertyDoc) > 1 or (len(self.propertyDoc) == 1 and
                        self.schemaName not in self.propertyDoc):
                        # Add in inherits hierarchy order
                        for inheritsClass in self.inheritsList:
                            if inheritsClass in self.propertyDoc:
                                self.markdownWriter.WriteInheritedPropertiesTitle(
                                    file, inheritsClass)
                                for prop in sorted(self.propertyDoc[inheritsClass]):
                                    self.markdownWriter.WriteProperty(
                                        file, self.propertyDoc[inheritsClass][prop])
                        # Add any "unknown" at the end?
                        if "unknown" in self.propertyDoc:
                            self.markdownWriter.WriteInheritedPropertiesTitle(
                                file, "unknown")
                            for prop in sorted(self.propertyDoc["unknown"]):
                                self.markdownWriter.WriteProperty(
                                    file, self.propertyDoc["unknown"][prop])
        except IOError as openException:
            Print.Err("Failed to write to " + filename + ": " + str(openException))
        return filename

    def GetResourceFiles(self):
        """
        Returns a list of discovered resource files, such as images, referenced
        in class or property userdoc
        """
        return self.resourceFiles


class _SchemaDocMarkdownGenerator:
    """
    Utility class for writing data from a _SchemaDoc to a Myst Markdown file
    """
    def __init__(self, schemaDomain, schemaName):
        self.schemaDomain = schemaDomain
        self.schemaName = schemaName

    def __WriteSphinxLabel(self, file, labelString):
        """
        Writes a Sphinx/RST-style label to the file, which for Myst Markdown
        is a line on its own of the format "(labelString)="
        Note that in most circumstances you need to place the label _before_
        a header for Sphinx to use it (as an anchor)
        """
        label = "(" + labelString + ")="
        file.write("\n" + label + "\n")

    def WriteHeader(self, file):
        """
        Write "header" markdown content (non-rendered file metadata)
        """
        # Write "comments"
        workStr = ("% WARNING: THIS FILE IS GENERATED BY " + PROGRAM_NAME +
            ". DO NOT EDIT.\n")
        file.write(workStr)
        workStr = "% Generated: " + datetime.datetime.now().strftime(
            "%I:%M%p on %B %d, %Y") + "\n\n"
        file.write(workStr)

    def WriteSchema(self, file, schemaUserDoc):
        """
        Write markdown content for schema class (but not its properties)
        """
        # Add a label for the schema for other files to link/refer to
        # This needs to come before the header
        #workStr = self.schemaDomain + "_" + self.schemaName
        workStr = self.schemaName
        self.__WriteSphinxLabel(file, workStr)
        # Write main title of page, which is the schema name
        workStr = "# " + self.schemaName + "\n"
        file.write(workStr)
        workStr = "\n" + schemaUserDoc + "\n"
        file.write(workStr)
        # Add an in-page TOC for convenience
        workStr = "```{contents}\n"
        file.write(workStr)
        workStr = ":depth: 2\n"
        file.write(workStr)
        workStr = ":local:\n"
        file.write(workStr)
        workStr = ":backlinks: none\n"
        file.write(workStr)
        workStr = "```\n"
        file.write(workStr)

    def WritePropertiesTitle(self, file):
        """
        Write markdown for properties section title
        """
        workStr = self.schemaName + "_properties"
        self.__WriteSphinxLabel(file, workStr)
        workStr = "\n" + "## Properties" + "\n"
        file.write(workStr)

    def WriteInheritedPropertiesTitle(self, file, parentClass):
        """
        Write markdown for inherited properties section title
        Adds Sphinx reference to parentClass
        """
        workStr = self.schemaName + "_inheritedproperties_" + parentClass
        self.__WriteSphinxLabel(file, workStr)
        workStr = "\n" + "## Inherited Properties ("
        workStr += "{ref}`" + parentClass + "`"
        workStr += ")" + "\n"
        file.write(workStr)

    def WriteProperty(self, file, propData):
        """
        Write markdown content for a schema property
        """
        # Add a label for the property for linking/referring to
        # note that this must come before the header for the property
        workStr = self.schemaName + "_" + propData.name
        self.__WriteSphinxLabel(file, workStr)
        # Write a header for the property
        workStr = "\n### " + propData.name + "\n"
        file.write(workStr)
        # Check if valueType is "rel" and note that the property is
        # really a relationship
        if propData.valueType == "rel":
            workStr = ("\n**USD type**: `" + str(propData.valueType) +
             "` (relationship)\n")
        else:
            workStr = ("\n**USD type**: `" + str(propData.valueType) + "`\n")
        file.write(workStr)
        if propData.defaultValue is not None:
            workStr = ("\n**Fallback value**: `" + str(propData.defaultValue) +
             "`\n")
            file.write(workStr)
        workStr = "\n" + propData.doc + "\n"
        file.write(workStr)

#------------------------------------------------------------------------------#
# Helper functions                                                             #
#------------------------------------------------------------------------------#

def _GetResourceFilesFromMarkdownString(markdown):
    """
    Scans markdown string for any referenced local resource files (e.g. images)
    and returns list of any files found. Used to determine what image files 
    might need to get copied over to output dir
    """
    imageFileList = []
    # Look for markdown-style image links of the format: ![link text](image link)
    # via this regex: !\[[^\]]*\]\((.*?)\s*\)
    imageMatches = re.findall(r'!\[[^\]]*\]\((.*?)\s*\)', markdown)
    for match in imageMatches:
        # We're only looking for local image files, so skip any URLs
        if not match.startswith("http"):
            imageFileList.append(match)
    # We ignore HTML-style image links, as these most likely point to URLs,
    # not local files
    # Eventually we will also look for other possible resources:
    # - Videos: through <video> HTML tag:
    #   <video src="path/to/video.mp4" width="320" height="240" controls></video>
    # - But ignore embedded URL videos like
    #   [![alt text](https://img.youtube.com/vi/video-id/0.jpg)](https://www.youtube.com/watch?v=video-id)
    return imageFileList


def _GetArgs():
    """
    Get the cmdline args
    """
    usage = "Generate Sphinx user docs from schema USD files."
    parser = ArgumentParser(description=usage)
    # We can take either the schemaUserDoc.usda or a dir that contains it
    # or even a schema.usda if schema author has decided to embed customData
    # docstrings in schema.usda instead of schemaUserDoc.usda.
    parser.add_argument('schemaUserDocPath',
        nargs='?',
        type=str,
        default="./" + SCHEMAUSERDOC_FILENAME,
        help='The source USD file where schema user doc is authored, or '
        'directory where the USD file and supporting content files are located. '
        '[Default: %(default)s]')
    parser.add_argument('sphinxOutputPath',
        nargs='?',
        type=str,
        default='.',
        help='The target directory where the markdown should be generated. '
        '[Default: %(default)s]')
    parser.add_argument('-t', '--sphinxTocFile',
        action='store_true',
        help='Optionally generate example Sphinx table-of-contents file.')
    parser.add_argument('-q', '--quiet',
        action='store_true',
        help='Do not output text during execution.')
    args = parser.parse_args()
    # Sanity check args
    if not os.path.exists(args.schemaUserDocPath):
        Print.Err('Usage Error: First positional argument must be a USD '
        'schemaUserDoc file, or a directory that contains a schemaUserDoc file.')
        parser.print_help()
        sys.exit(1)
    Print.SetQuiet(args.quiet)
    return args


def _ProcessGuides(userDocsPath):
    """
    Process any markdown "guides" included with schemaUserDoc.usda, if any. 
    Currently these are expected to live in the same dir as schemaUserDoc.usda
    Returns list of guide files and a separate list of any associated resource 
    files, such as image files. 
    """
    # Use a set to avoid duplicate files
    guideFiles = set()
    resourceFiles = set()
    # Currently we just look for guides in the top-level userDocs dir, but
    # if we eventually need to allow for a subdir hierarchy, wrap the following
    # in os.walk
    files = os.listdir(userDocsPath)
    # Filter for only markdown (and RST?) files
    for file in files:
        fileAndPath = os.path.join(userDocsPath,file)
        if not os.path.isfile(fileAndPath):
            continue
        (shortname, extension) = os.path.splitext(file)
        if extension == ".md":
            # Scan file for any local image files
            with open(fileAndPath, 'r') as checkedFile:
                fileContent = checkedFile.read()
                imageFiles = _GetResourceFilesFromMarkdownString(fileContent)
                for imageFile in imageFiles:
                    resourceFiles.add(imageFile)
            guideFiles.add(file)
    return list(guideFiles), list(resourceFiles)


def _ProcessSchemaDocs(schemaFile):
    """
    Process schema user doc custom metadata fields, creating Myst markdown
    files as needed.
    """
    # Load the schemaUserDoc.usda or schema.usda and compose into a stage
    sdfLayer = Sdf.Layer.FindOrOpen(schemaFile)
    stage = Usd.Stage.Open(sdfLayer)
    # Get the domain from the global prim
    globalPrim = stage.GetPrimAtPath("/GLOBAL")
    domain = ""
    domainTitle = ""
    if globalPrim is not None:
        if "libraryName" in globalPrim.GetCustomData():
            domain = globalPrim.GetCustomData()["libraryName"]
        if "userDocTitle" in globalPrim.GetCustomData():
            domainTitle = globalPrim.GetCustomData()["userDocTitle"]
    if domain == "":
        Print.Err("No schema domain (libraryName) found in: " + schemaFile)
    # Traverse root layer schema classes, extract the userDoc for each
    # schema and schema property
    classList = []
    for sdfPrim in sdfLayer.rootPrims:
        cls = stage.GetPrimAtPath(sdfPrim.path)
        # Ignore GLOBAL
        schemaName = cls.GetName()
        if schemaName == "GLOBAL":
            continue
        # Get schema(class) userDoc from prim customData
        schemaCustomData = cls.GetCustomData()
        if USERDOC_FULL in schemaCustomData:
            workSchema = _SchemaDoc(domain, schemaName,
                schemaCustomData[USERDOC_FULL])
        else:
            Print("Warning: No user doc found for schema class: " + schemaName)
            workSchema = _SchemaDoc(domain, schemaName, "")
        # Get inheritance stack
        inheritsClassList = []
        inheritsList = cls.GetInherits().GetAllDirectInherits()
        for inheritsPath in inheritsList:
            inheritsClassList.append(inheritsPath.name)
        workSchema.SetInheritsList(inheritsClassList)
        # Get properties data
        # Use UsdClass.GetProperties to get inherited/composed properties as well
        for clsProp in cls.GetProperties():
            propName = clsProp.GetBaseName()
            # Get the property USD type and schema fallback value if any
            if isinstance(clsProp, Usd.Attribute):
                propTypeName = clsProp.GetTypeName()
                # Because we're parsing the schema definitions themselves, we
                # can't check the schema registry and instead have to inspect
                # the UsdProperty values directly
                if clsProp.HasAuthoredValue():
                    propDefault = clsProp.Get()
                else:
                    propDefault = None
            elif isinstance(clsProp, Usd.Relationship):
                propTypeName = "rel"
                # Relationships in schemas won't have any "default" targets
                propDefault = None
            else:
                # Skip properties that are not attributes or relationships
                Print.Err(f"Property {propName} is neither attribute nor "
                 "relationship: " + clsProp.GetName())
                continue
            # We will get composed (inherited) properties as well, and
            # need to track this so that later doc generation will list these
            # properties separately.
            propStack = clsProp.GetPropertyStack()
            # Use the last spec in the property stack to get the parent class
            if len(propStack) > 0:
                propSpec = propStack[-1]
                propClassName = propSpec.path.GetAbsoluteRootOrPrimPath().name
                # Check to see if this property is only defined in an over,
                # in case the property was renamed or deleted in the parent
                # schema class
                if len(propStack) == 1:
                    primSpec = propSpec.owner
                    # Make sure we have a primSpec and not a relationshipSpec
                    if isinstance(primSpec, Sdf.PrimSpec):
                        if primSpec.specifier == Sdf.SpecifierOver:
                            Print(f"Warning: property {propName} defined in "
                                f"over but not in schema class {schemaName}. "
                                "Property may have been renamed or removed.")
            else:
                # Treat missing property stacks as errors, skip
                Print.Err("No property stack found for " + propName)
                continue
            # Get property userDoc from property customData
            propCustomData = clsProp.GetCustomData()
            if USERDOC_FULL in propCustomData:
                workSchema.AddPropertyDoc(propClassName, propName,
                    propCustomData[USERDOC_FULL], propTypeName, propDefault)
            else:
                Print("Warning: No user doc found for property " + propName +
                    " in schema class: " + schemaName)
                workSchema.AddPropertyDoc(propClassName, propName,
                    "", propTypeName, propDefault)
        classList.append(workSchema)
    return domain, domainTitle, classList


def _GenerateSphinxTOC(fileList, outputFile, domain, domainTitle):
    """
    Generate a Sphinx TOC file (RST format) with the generated files.
    This file can be included in a Sphinx build to pull in all doc files
    for the schema.
    """
    try:
        with open(outputFile, 'w') as file:
            workStr = (".. WARNING: THIS FILE IS GENERATED BY " + PROGRAM_NAME +
                ".  DO NOT EDIT.\n")
            file.write(workStr)
            workStr = ".. TOC for " + domain + "\n"
            file.write(workStr)
            workStr = ".. Generated: " + datetime.datetime.now().strftime(
                "%I:%M%p on %B %d, %Y") + "\n\n"
            file.write(workStr)
            # Create a RST-style H1 header with domain title and name
            if domainTitle is not None and len(domainTitle) > 0:
                workTitle = f"{domainTitle} ({domain})"
            else:
                workTitle = domain
            workStr = "#"
            workStr = workStr.replace("#", "#" * len(workTitle)) + "\n"
            file.write(workStr)
            file.write(workTitle + "\n")
            file.write(workStr)
            workStr = "\n"
            file.write(workStr)
            # Add the TOC entries in the order listed in fileList
            workStr = ".. toctree::\n"
            file.write(workStr)
            for generatedFile in fileList:
                workStr = "   " + os.path.basename(generatedFile) + "\n"
                file.write(workStr)
    except IOError as openException:
        Print.Err("Failed to write to " + outputFile + ": " + str(openException))

def _InitializeResolver():
    """
    Copied from usdGenSchema.py

    Initialize the resolver so that search paths pointing to schema.usda
    files are resolved to the directories where those files are installed
    """

    # Force the use of the ArDefaultResolver so we can take advantage
    # of its search path functionality.
    Ar.SetPreferredResolver('ArDefaultResolver')

    # Figure out where all the plugins that provide schemas are located
    # and add their resource directories to the search path prefix list.
    resourcePaths = set()
    pr = Plug.Registry()
    for t in pr.GetAllDerivedTypes('UsdSchemaBase'):
        plugin = pr.GetPluginForType(t)
        if plugin:
            resourcePaths.add(plugin.resourcePath)

    # The sorting shouldn't matter here, but we do it for consistency
    # across runs.
    Ar.DefaultResolver.SetDefaultSearchPath(sorted(list(resourcePaths)))

#------------------------------------------------------------------------------#
# main                                                                         #
#------------------------------------------------------------------------------#

if __name__ == '__main__':
    #
    # Parse Command-line
    #
    args = _GetArgs()

    #
    # Set resolver to find inherited schemas
    #
    _InitializeResolver()

    #
    # Gather and parse schema docs and guides
    #
    guidesList = []
    classList = []
    resourceList = []
    if os.path.isdir(args.schemaUserDocPath):
        # Look for schemaUserDoc.usda in dir
        schemaFile = os.path.join(os.path.abspath(args.schemaUserDocPath),
            SCHEMAUSERDOC_FILENAME)
        if os.path.exists(schemaFile):
            domain, domainTitle, classList = _ProcessSchemaDocs(schemaFile)
        else:
            schemaFile = os.path.join(os.path.abspath(args.schemaUserDocPath),
                SCHEMA_FILENAME)
            if os.path.exists(schemaFile):
                domain, domainTitle, classList = _ProcessSchemaDocs(schemaFile)
            else:
                Print.Err("Usage Error: No " + SCHEMAUSERDOC_FILENAME + " or " +
                    SCHEMA_FILENAME +
                    " found in provided directory: " + args.schemaUserDocPath)
                sys.exit(1)
        # Look for any Markdown files and process accordingly
        guidesList, resourceList = _ProcessGuides(args.schemaUserDocPath)
    else:
        # For now, if dir is not provided, assume that we only want to
        # process the schemaUserDoc.usda or schema.usda file.
        domain, domainTitle, classList = _ProcessSchemaDocs(args.schemaUserDocPath)

    #
    # Write Sphinx output
    #
    writtenContentFiles = []
    writtenResourceFiles = []
    writtenSchemaGeneratedFiles = []
    # Create a dir with the schema domain name (e.g. "usdVol")
    outputDir = os.path.join(os.path.abspath(args.sphinxOutputPath), domain)
    if not os.path.isdir(outputDir):
        try:
            os.makedirs(outputDir, exist_ok=True)
        except OSError as error:
            Print.Err(f"Output directory {outputDir} cannot be created")
            sys.exit(1)
    # Copy guides and supporting files (first)
    for file in guidesList:
        srcFile = os.path.join(args.schemaUserDocPath, file)
        destFile = os.path.join(outputDir,file)
        shutil.copy2(srcFile, destFile)
        writtenContentFiles.append(destFile)
    for file in resourceList:
        srcFile = os.path.join(args.schemaUserDocPath, file)
        destFile = os.path.join(outputDir,file)
        shutil.copy2(srcFile, destFile)
        writtenResourceFiles.append(destFile)

    # Copy schema files
    for schemaClass in sorted(classList, key=lambda x: x.schemaName):
        outputFile = schemaClass.WriteMystMarkdownFile(outputDir)
        writtenSchemaGeneratedFiles.append(outputFile)
        schemaResourceFiles = schemaClass.GetResourceFiles()
        for file in schemaResourceFiles:
            srcFile = os.path.join(args.schemaUserDocPath, file)
            destFile = os.path.join(outputDir,file)
            shutil.copy2(srcFile, destFile)
        writtenResourceFiles.extend(schemaResourceFiles)
    writtenContentFiles.extend(writtenSchemaGeneratedFiles)

    # Generate TOC
    if args.sphinxTocFile:
        tocFile = os.path.join(outputDir, domain + "_toc.rst")
        _GenerateSphinxTOC(writtenContentFiles, tocFile, domain, domainTitle)
