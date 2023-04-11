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
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
#
# cdWriterDocstring.py
#
# Writer plugin to create Python docstring documentation from Docstring
# XML comments. This module must define a 'Writer' class, with the
# following methods:
#
#   Writer.getDocString() - format the Doxygen XML to a string. This
#      method is called by the Parser object during the traverse() call
#
#   Writer.generate() - write the list of all docs to an output file.
#      This is called once the entire file has been traversed.
#

import sys
import textwrap
import types
import re

from .cdUtils import *

class Writer:
    """
    Manage the formatting of Python docstrings and output file generation.
    """

    # This dictionary holds all 'python path' : 'docstring' pairs for
    # properties until all objects have been processed.  This is so
    # we can combine property docstrings for getters/setters.
    propertyTable = {}

    # Default constructor that assumes we're processing a single module
    def __init__(self):
        #
        # Parse the extra arguments required by this plugin
        #
        project = GetArgValue(['--package', '-p'])
        package = GetArgValue(['--module', '-m'])

        if not project:
            Error("Required option --package not specified")
        if not package:
            Error("Required option --module not specified")

        # Import the python module that these docs pertain to
        if not Import("from " +project+ " import " +package+ " as " +package):
            Error("Could not import %s" % (package))

        self.module = eval(package)
        self.prefix = self.module.__name__.split('.')[-1]
        self.seenPaths = {}

    # Constructor that takes package and module name, used when processing 
    # a list of modules
    def __init__(self, packageName, moduleName):

        # Import the python module 
        if not Import("from " +packageName+ " import " +moduleName+ " as " +moduleName):
            Error("Could not import %s" % (moduleName))

        self.module = eval(moduleName)
        self.prefix = self.module.__name__.split('.')[-1]
        self.seenPaths = {}
        self.propertyTable = {}

    # Unload an imported module. Used when processing multiple modules at
    # once, to avoid matching entities on already-processed modules.
    def unloadModule(self, moduleName):
        self.module = None
        del moduleName
        #del sys.modules[moduleName]

    #
    # Routines to take an XML tree describing an API doc and
    # return a formatted string - called during Parser.traverse()
    #

    def getDocString(self, node):
        """
        Public API call to convert an XML tree into a docstring.
        """
        docstring = self.__convertNode(node)
        docstring = self.__wordWrapDocString(docstring.split('\n'))

        if docstring:
            summary = re.sub(r'\s+', ' ', docstring[:40].strip())
            Debug("Returning docstring: %s..." % summary)

        return docstring
    
    def getDocTags(self, node):
        """
        Public API to retrieve tags from within an XML node's assorted docstrings.
        Reserved for future use, currently no special doc tags are used
        """
        return list()

    def __convertNode(self, node, sep=' '):
        """
        return a string representation of the current XML node and
        all of its children, separating each section of text with
        the sep character.
        """
        tlist = []
        # translate the various doxygen xml nodes into an appropriate
        # representation for the python docstring
        for child in node.childNodes:
            if child.isText():
                tlist.append(child.text)
            elif (child.name == "para"):
                tlist.append("PARA")
                tlist.append(self.__convertNode(child,sep))
            elif (child.name == "listitem"):
                nodetext = self.__convertNode(child, sep)
                if nodetext.startswith("PARA"):
                    nodetext = nodetext[len("PARA"):].strip()
                tlist.append("NEWLINE   - " + nodetext + "NEWLINE")
            elif (child.name == "emphasis"):
                nodetext = self.__convertNode(child, sep)
                # put trailing ':' outside of the emphasis block, for prop docs
                if nodetext.endswith(":"):
                    tlist.append("*" + nodetext[:-1] + "*" + nodetext[-1])
                else:
                    tlist.append("*" + nodetext + "*")
            elif (child.name == "bold"):
                nodetext = self.__convertNode(child, sep)
                tlist.append("B{" + nodetext + "}")
            elif (child.name == "computeroutput"):
                nodetext = self.__convertNode(child, sep)
                tlist.append("C{" + nodetext + "}")
            elif (child.name == "heading"):
                nodetext = self.__convertNode(child, sep)
                tlist.append(nodetext + "NEWLINE" + "=" * len(nodetext))
            elif (child.name == "sect1"):
                # we use the <sect1> tag (\section comment) to describe
                # component attributes and relationships. Output a tag
                # that we can pick up in epydoc and format appropriately.
                nodetext = self.__convertNode(child, sep)

                # if the section has an explicit name then use that,
                # otherwise use default section names
                secid = child.getAttrValue("id")
                secname = child.getText("title")
                if secname.find("attr_inherited_") >= 0:
                    secname = "Inherited Prim Attributes"
                elif secname.find("attr_") >= 0:
                    secname = "Prim Attributes"
                elif secname.find("rel_inherited_") >= 0:
                    secname = "Inherited Prim Relationships"
                elif secname.find("rel_") >= 0:
                    secname = "Prim Relationships"
                elif secname.find("pycode") >= 0:
                    secname = "Python Code Example"
                elif secid.find("cppcode") >= 0:
                    continue  # ignore C++ code sections in python docs

                title = "PARA" + secname + "NEWLINE" + "=" * len(secname)
                tlist.append("%sNEWLINE%s" % (title, nodetext))
            elif (child.name == "title"):
                # already processed in the sect1 tag
                pass
            elif (child.name == "programlisting"):
                nodetext = self.__convertNode(child, sep)
                tlist.append("::PARA" + nodetext + "NEWLINE")
            elif (child.name == "codeline"):
                # don't separate entries in the code section with spaces
                sep = ''
                nodetext = self.__convertNode(child, sep)
                tlist.append("CODE_START" + nodetext + "CODE_END")
            elif (child.name == "sp"):
                # spaces in a code section are represented with a custom
                # token so that we don't try to word wrap code sections
                tlist.append("SPACER")
                sep = ''
            else:
                tlist.append(self.__convertNode(child, sep))

        # fixup any whitespace issues between punctuation and words
        result = sep.join(tlist).strip()
        result = re.sub(r'([0-9A-Za-z*][\)\}>]?) +([,.?;\}\)>])([^A-Za-z])', r'\1\2\3', result)
        result = re.sub(r'([0-9A-Za-z*][\)\}>]?) +([,.?;\}\)>])$', r'\1\2', result)
        result = re.sub(r'( \() ([A-Z][A-Za-z0-9 ]*\))', r'\1\2', result)
        result = re.sub(r'([A-Za-z])&([A-Za-z])', r'\1 & \2', result)
        return result

    def __wordWrapDocString(self, lines):

        # support the PARA and NEWLINE tokens that we inserted above
        newlines = []
        for x in lines:
            x = re.sub("PARA\s*::", "::", x)  # don't para break before ::
            #x = re.sub("::", "NEWLINE::", x)  # but do put :: on a new line
            # Original line above, KRC's hack below
            x = re.sub("::\s*$", "NEWLINE::", x)  # No, but do NOT put a :: on a new line willy nilly!
                                                  # Only if the :: is at the end of a line.  

            for y in x.split('PARA'):
                for z in y.strip().split("NEWLINE"):
                    newlines.append(z)
                # don't add extra newlines if they're not necessary
                if y != '':
                    newlines.append("")

        # support the reStructuredText list items and ensure that
        # following lines of a list item are indented at the same
        # level. We word wrap the text, but don't break long words
        # as we don't want to word wrap code sections.
        textWrapper = textwrap.TextWrapper(width=70, break_long_words=False)
        newlines = list(map(textWrapper.fill, newlines))
        lines = []
        inlistitem = False
        for curline in newlines:
            # the textwrap.fill call adds \n's at line breaks
            for line in curline.split('\n'):
                if line.startswith("   - "):
                    lines.append(line)
                    inlistitem = True
                elif line.strip() == "":
                    lines.append("")
                    inlistitem = False
                elif inlistitem:
                    lines.append("     " + line.strip())
                else:
                    lines.append(line)

        # support code blocks as reStructuredText preformatted
        # sections. These are indented and prefixed with "::"
        fulltest = re.sub(r'CODE_START(.*)\n(.*)CODE_END',
                          r'CODE_START\1 \2CODE_END',
                          '\n'.join(lines))
        newlines = []
        for line in fulltest.split('\n'):
            line = re.sub('SPACER', ' ', line)
            line = line.replace('CODE_START', '  ')
            line = line.replace('CODE_END', '\n')
            newlines.append(line)
           
        # return as a single string (and compress blank lines)
        ret = '\n'.join(newlines)
        ret = re.sub(r'\n\n+', '\n\n', ret)
        return ret

    #
    # Routines to take the generated docstrings and to write these
    # all out to the output __DOC.py file
    #

    def generate(self, output_file, docElements):
        """Build the output file contents and write it to the output file."""
        # build the list of lines to output to the file
        bodylines = []
        self.seenPaths = {}
        for docElem in docElements:
            bodylines += self.__generate_r([docElem])

        # Now add all the property docstrings we've accumulated
        for proppath, desc in self.propertyTable.items():
            bodylines.append(desc)

        # Remove re-definitions of a top-level module free method
        # if a "real" top-level definition exists
        for key in self.seenPaths.keys():
            found = [jumped for jumped, path, desc in self.seenPaths[key]]
            if True in found and False in found:
                [bodylines.remove(desc) for jumped, path, desc in self.seenPaths[key] if jumped]

        # format the lines to be written to the __DOC.py file
        lines = []
        lines.append("def Execute(result):")
        for line in bodylines:
            lines.append('   ' + line)
        if len(lines) == 1:
            lines.append("    pass")

        # output the lines to disk...
        try:
            logfile = open(output_file, 'w')
            logfile.write('\n'.join(lines))
            logfile.close()
        except:
            Error("Could not write to file: %s" % output_file)

        return True

    def __generate_r(self, docElem):
        """Recursive continuation of generate()."""
        ret = []
        for childName, childObjectList in docElem[-1].children.items():

            # Work out the possible Python name(s) for this C++ object
            # Note that some C++ names have both potential corresponding
            # python method and property names.
            (pyobj, pypath, proppyobj, proppypath, jumped) \
                = self.__getPythonObjectAndPath(docElem, childObjectList)

            if pyobj is not None:
                # get the full docstring for this element - this could be
                # None if __getDocString() returned None.
                desc = self.__getOutputFormat(pypath, pyobj, childObjectList)
                if desc:
                    ret.append(desc)
                    found = self.seenPaths.setdefault(pypath, [])
                    found.append( (jumped, pypath, desc) )
            elif proppyobj is not None:
                # look for the docstring for the property name if we didn't find a
                # docstring for the literal name above
                desc = self.__getOutputFormat(proppypath, proppyobj, childObjectList)
                if desc:
                    if proppypath in self.propertyTable:
                        # Partial docstring already exists for this property so we need
                        # to add in the new description rather than overwriting everything.
                        oldOutput = self.propertyTable[proppypath]
                        oldDescStart = oldOutput.find('"""')
                        oldDescEnd = oldOutput.find('"""', oldDescStart + 3)
                        newDescStart = desc.find('"""')
                        newDescEnd = desc.find('"""', newDescStart + 3)
                        if oldDescStart > -1 and oldDescEnd > -1 and \
                            newDescStart > -1 and newDescEnd > -1:
                            oldDesc = oldOutput[oldDescStart+3:oldDescEnd]
                            newDesc = desc[newDescStart+3:newDescEnd]
                            # We want the getter's docstring first, since it lists the type
                            if newDesc.find("type :") != -1:
                                newDesc = newDesc + "-"*70 + oldDesc
                            else:
                                newDesc = oldDesc + "-"*70 + newDesc
                            newOutput = oldOutput.replace(oldDesc, newDesc)
                            self.propertyTable[proppypath] = newOutput
                        else:
                            Debug("unrecognized property docstring: %s" % desc)
                    else:
                        # add new python property with docstring to the property dictionary
                        self.propertyTable[proppypath] = desc

            # recurse through all of this element's children too
            for child in childObjectList:
                ret += self.__generate_r(docElem + [child])
        return ret

    def __pathGenerator(self, parentPath, overloads):
        ret = []
        pret = [] # this is used for a potential python property path
        pret2 = []
        path = parentPath + overloads[:1]

        for i in range(1, len(path)):
            name = path[i].name
            if i != 0 and path[i].isFunction and path[i-1].isClass:
                # constructor?
                if name == path[i-1].name:
                    name = '__init__'
                # property?
                lowername = name.lower()
                if lowername.startswith("get") or lowername.startswith("set"):
                    if len(name) > 3:
                        # getter/setter
                        pname = name[3].lower() + name[4:]
                        pret = ret[:]
                        pret.append(pname)
                elif lowername.startswith("is"):
                    if len(name) > 2:
                        # boolean property
                        
                        # Unfortunately boolean properties might retain a leading "is"
                        # but might not, so we need to check both.

                        # remove leading "Is" and make first letter lowercase
                        pname = name[2].lower() + name[3:]
                        pret = ret[:]
                        pret.append(pname)
                        
                        # leave leading "is" but make sure the 'i' is lowercase
                        pname2 = name[0].lower() + name[1:]
                        pret2 = ret[:]
                        pret2.append(pname2)
            
            if name.startswith(self.prefix) and name != self.prefix:
                name = name[len(self.prefix):]
            ret.append(name)

        return (ret, pret, pret2)

    def __getPythonObject(self, path):
        """Returns the python object corresponding to the provided path."""
        obj = None
        try:
            if hasattr(self.module, path[0]):
                obj = getattr(self.module, path[0])
                # stop searching if we're at the module level
                if len(path) > 1:
                    for elem in path[1:]:
                        obj = getattr(obj, elem)
        except AttributeError as e:
            obj = None
        except IndexError as e:
            obj = None
        
        return obj

    def __getPythonObjectByPath(self, path):
        """Returns a tuple containing the python object corresponding to the 
        provided path and the path itself. It returns the path just in case we 
        had to modify shorten the path list because the object we're looking for
        is wrapped to a different hierarchy level than the corresponding C++
        object."""
        jumped = False
        if len(path) == 0:
            return (None, path, False)

        # Make sure that this object actually exists. If it doesn't, look 
        # at module-level definitions.
        obj = self.__getPythonObject(path)
        if not obj: 
            mpath = path[-1:] # look in the top of the module
            testobj = self.__getPythonObject(mpath)
            # When we look one level up, we're only looking for instance or 
            # or module-level free methods
            if isinstance(testobj, types.MethodType) or \
                isinstance(testobj, types.FunctionType):
                obj = testobj
                path = mpath
                jumped = True
                
        return (obj, path, jumped)

    def __getPythonObjectAndPath(self, parentPath, overloads):
        """Return the full Python path for a module/class/method.
        The first 2 items in the tuple are the verbatim python object and
        corresponding path, if it exists.  The second 2 items in the tuple
        are the property version of the object and its corresponding path,
        if it exists."""
        # do we have a module?
        if overloads[0].isModule():
            return (self.module, self.module.__name__, None, None, False)

        # otherwise we have a class, method, or property
        (pypath, ppypath1, ppypath2) = self.__pathGenerator(parentPath, overloads)

        (obj, pypath, jumped) = self.__getPythonObjectByPath(pypath)

        # check for the property by either possible name (since there
        # are two possible naming conventions for boolean properties)
        ppypath = ppypath1
        (pobj, ppypath1, pjumped) = self.__getPythonObjectByPath(ppypath1)
        if not pobj:
            # we didn't find an object with the first name--let's try the other
            (pobj, ppypath2, pjumped) = self.__getPythonObjectByPath(ppypath2)
            ppypath = ppypath2

        return (obj, '.'.join(pypath), pobj, '.'.join(ppypath), jumped)

    def __convertTypeName(self, cppName):
        """Convert a C++ type name into a Python type name."""
        # get rid of const, volatile, &, *.
        ret = cppName
        ret = ret.replace('const', '')
        ret = ret.replace('volatile ', '')
        ret = ret.replace('&', '')
        ret = ret.replace('*', '')
        ret = ret.replace('std::', '')
        ret = ret.replace('boost::', '')
        ret = ret.replace('vector', 'sequence')
        ret = ret.replace('::', '.')
        ret = ret.strip()
        if ret.startswith(self.prefix):
            ret = ret[len(self.prefix):]
        return ret.strip()

    def __convertCppSyntax(self, line):
        """Convert C++ terminology into Python terminology."""
        ret = line
        ret = ret.replace('NULL', 'None')
        ret = ret.replace('library', 'module')
        ret = ret.replace('libraries', 'modules')

        return ret
            
    def __getSignatureString(self, pyname, pyobj, doxy):
        """Describe the signature for a single method call."""
        if doxy.isFunction():
            cnt = 1;
            pnames = []
            for ptype, pname in doxy.params:
                if len(pname):
                    pnames.append(pname)
                else:
                    pnames.append('arg%s' % cnt)
                cnt += 1
            sig = '('+', '.join(pnames)+')'
            retType = self.__convertTypeName(doxy.returnType)
            if len(retType) and retType != 'void':
                sig += ' -> ' + retType
            return pyname + sig + '\n'
        return None

    def __getSignatureDescription(self, pyname, pyobj, doxy):
        """Return the description of each argument in a method call."""
        if doxy.isFunction():
            cnt = 0
            lines = []
            for ptype, pname in doxy.params:
                cnt += 1
                if not len(pname):
                    pname = 'arg%s' % cnt
                lines.append(' : '.join((pname, self.__convertTypeName(ptype))))
            return lines
        return None

    def __getShortDescription(self, pyname, pyobj, doxy):
        """Return the top-level description of the class/method."""
        ret = []
        if type(pyobj) == property:
            # Try to parse out the type for this property from the
            # corresponding C++ getter.
            sig = self.__getSignatureString(pyname, pyobj, doxy)
            typeindex = sig.find("->")
            if typeindex > -1:
                pytype = sig[typeindex+3:]
                ret.append("type : " + pytype)
        elif doxy.isFunction():
            ret.append(self.__getSignatureString(pyname, pyobj, doxy))
        return ret

    def __getDocumentation(self, pyname, pyobj, doxy):
        """Return the actual (brief and details) doc string."""
        lines = []
        if doxy.doc['brief']:
            lines.append(doxy.doc['brief'])
        if doxy.doc['detailed']:
            lines.append(doxy.doc['detailed'])
        newLines = []
        for line in lines:
            newLines.append(self.__convertCppSyntax(line))
        return newLines

    def __getFullDoc(self, pyname, pyobj, doxy):
        """Return the complete class/method description for output."""

        # opt-out for pyobj's that contain the notinpython element
        if ATTR_NOT_IN_PYTHON in doxy.doc['tags']:
            return ''
        # make the doxy element static if it is tagged as such
        if ATTR_STATIC_METHOD in doxy.doc['tags']:
            doxy.static = 'yes'
        
        lines = self.__getShortDescription(pyname, pyobj, doxy)
        if doxy.isFunction() and type(pyobj) != property:
            lines += self.__getSignatureDescription(pyname, pyobj, doxy)
            lines.append('')
        lines += self.__getDocumentation(pyname, pyobj, doxy)
        lines.append('')
        return lines

    def __getOutputFormat(self, pypath, pyobj, overloads):
        """Return the line that installs the docstring into the namespace."""

        # is there an existing python docstring? we don't want to overwrite
        # this because it may be custom authored in the C++ wrap files.
        # However, we always override the module doc string for now...
        if hasattr(pyobj, '__doc__') and pyobj.__doc__ is not None:
            doc = pyobj.__doc__.strip()
            if len(doc) > 0 and not doc.startswith("C++ signature:") \
               and not overloads[0].isModule():
                Debug("Docstring exists for %s - skipping" % pypath)
                return None

        # get the full docstring that we want to output
        lines = []
        pyname = pypath.split('.')[-1]
        docString = ''
        
        if len(overloads) == 1:
            lines += self.__getFullDoc(pyname, pyobj, overloads[0])
            if overloads[0].static == 'yes':
                docString = LABEL_STATIC # set the return type to static
        else:
            for doxy in overloads:
                if doxy.static == 'yes':
                    docString = LABEL_STATIC # set the return type to static
                    
                desc = self.__getFullDoc(pyname, pyobj, doxy)
                if lines and desc:
                    lines.append('-'*70)
                if desc:
                    lines += desc    
        docString += '\n'.join(lines)

        # work out the attribute to set to install this docstring
        words = pypath.split('.')
        cls = words[0]
        func = '.'.join(words[1:])
        if func: func = '.' + func

        setterString = "result"
        if isinstance(pyobj, types.MethodType) and hasattr(pyobj, 'im_func'):
            setterString += "[\"%s\"]%s.im_func.func_doc" % (cls, func)
        elif isinstance(pyobj, types.ModuleType):
            setterString += "[\"__doc__\"]"
        elif hasattr(pyobj, 'func_doc'):
            setterString += "[\"%s\"]%s.func_doc" % (cls, func)
        else:
            setterString += "[\"%s\"]%s.__doc__" % (cls, func)
        
        setterString += ' = ' + '"""%s"""'

        # Cannot reset property docstrings, so create a new property in
        # its place with the new docstring.
        if type(pyobj) == property and len(docString) > 0:
            setterString = "result[\"%s\"]%s" % (cls, func) +\
                           ' = property(result["%s"]%s.fget, ' % (cls, func) +\
                           'result["%s"]%s.fset, ' % (cls, func) +\
                           'result["%s"]%s.fdel, ' % (cls, func) +'"""%s""")'
        elif type(pyobj) == property and len(docString) == 0:
            #Warn("Property %s does not have a docstring" % ".".join(words[-2:]))
            return None
                
        return setterString % docString
