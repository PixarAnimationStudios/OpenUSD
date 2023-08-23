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
# cdParser.py
#
# The XML parsing routines for convertDoxygen, including the logic
# to traverse the XML and ask the writer plugin to format the
# various doc strings.
#

import xml.sax.saxutils
import xml.sax.handler
import glob
import fnmatch
import xml.etree.ElementTree

from .cdUtils import *
from .cdDocElement import *

class XMLNode:
    """
    Rrepresent a single node in the XML tree.
    """
    __slots__ = ("parent", "name", "attrs", "text", "childNodes")

    def __init__(self, parent, name, attrs, text):
        self.parent = parent
        self.name = name
        self.attrs = attrs
        self.text = text
        self.childNodes = []

    def __repr__(self) -> str:
        return "XMLNode(%s, %s, ...)" % (self.name, self.attrs.items())

    def addChildNode(self, node):
        """Append the specifed node to the children of this node."""
        self.childNodes.append(node)

    def isText(self):
        """Return True if the current node contains text data."""
        return self.name == '#text' and len(self.text)

    def getAttrValue(self, attrname, defVal=None):
        """Return the value of the named attribute in this xml node"""
        if self.attrs == None:
            return defVal
        for k, v in self.attrs.items():
            if k.lower() == attrname.lower():
                return v
        return defVal

    def findNode(self, nodeName):
        """Find the named node underneath this one."""
        for child in self.childNodes:
            if child.name == nodeName:
                return child
        return None

    def getText(self, nodeName=None):
        """Gather the text under this node, or the named child node."""
        node = self
        if nodeName:
            node = self.findNode(nodeName)
            if not node:
                return ''
        tlist = []
        for child in node.childNodes:
            if child.isText():
                tlist.append(child.text)
            else:
                tlist.append(child.getText())
        return ' '.join(tlist).strip()

    def getKind(self):
        """Return the value of the 'kind' attribute for this node."""
        return self.getAttrValue('kind')

    def getProt(self):
        """Return the value of the 'prot' attribute for this node."""
        return self.getAttrValue('prot')

    def getLocation(self):
        """Return a (lineno,filename) tuple for the location of this node."""
        locNode = self.findNode('location')
        if not locNode is None:
            return (locNode.getAttrValue('line'), locNode.getAttrValue('file'))
        return ('', '')


class XMLParser(xml.sax.handler.ContentHandler):
    """
    Custom parser class used to parse the Doxygen XML
    """

    def __init__(self):
        self.__curNode = None
        self.__rootNodes = []
        self.__curDepth = -1
        self.__textNode = []

    def startNode(self, name, attrs, text):
        self.__curNode = XMLNode(self.__curNode, name, attrs, text)
        if self.__curNode.parent == None:
            self.__rootNodes.append(self.__curNode)

    def endNode(self, name):
        if self.__curNode.parent != None:
            self.__curNode.parent.addChildNode(self.__curNode)
        self.__curNode = self.__curNode.parent

    def startElement(self, name, attrs):
        # flush out any text for the current node before starting a new one
        if self.__curDepth >= 0 and self.__textNode[self.__curDepth]:
            self.startNode('#text', None, self.__textNode[self.__curDepth])
            self.endNode('#text')
            self.__textNode[-1] = ""

        # now create a new node and bump the depth
        self.__curDepth += 1
        self.__textNode.append("")

        # finally, start the new node
        self.startNode(name, attrs, None)

    def endElement(self, name):
        # flush out any text for the current node before closing it
        if self.__textNode[self.__curDepth]:
            self.startNode('#text', None, self.__textNode[self.__curDepth])
            self.endNode('#text')
            
        # and bump down the current depth
        self.__curDepth -= 1
        del self.__textNode[-1]

        # finally, end the current node
        self.endNode(name)

    def characters(self, ch):
        # need to handle the fact that the parser may call characters() multiple times
        # for a single text node, so we need to accumulate the chars and then emit them
        # at the right times during the startElement() and endElement() calls. Pheww!
        ch = ch.strip(' \v\r\t\n')
        if ch:
            self.__textNode[self.__curDepth] += ch
            
    def getRoots(self):
        return self.__rootNodes


class Parser:
    """
    Parse a Doxygen XML file and create a collection of DocElement objects.
    """

    def __init__(self):
        self.writer = None
        self.rootNode = None
        self.docElements = None

    #
    # Routines to parse the Doxygen XML format
    #

    def parse(self, xml_file):
        """Parse the input XML file into a tree of XMLNodes."""

        Debug("Attempting to parse file: '%s'" % xml_file)
        
        try:
            parser = xml.sax.make_parser()
            parser.setFeature(xml.sax.handler.feature_namespaces, 0)
            xmlparser = XMLParser()
            parser.setContentHandler(xmlparser)
            parser.parse(open(xml_file))
            self.rootNode = xmlparser.getRoots()
            return True
        except Exception:
            return False

    def parseDoxygenIndexFile(self, doxygen_index_file):
        """Parse a set of files as listed in a doxygen-generated index.xml"""

        Debug("Attempting to parse Doxygen index file: '%s'" % doxygen_index_file)

        # Get the list of "compound" XML files to parse from index.xml
        # Expected index.xml format:
        # <doxygenindex ...>
        #     <compound refid="entity_ref_name" kind="dox_type">
        #         ...various child elements like <name>, <member>...
        #     </compound>
        # </doxygenindex>
        entity_file_list = list()
        index_dir = os.path.dirname(doxygen_index_file)
        with open(doxygen_index_file, "rb") as content:
            tree = xml.etree.ElementTree.parse(content)
            compound_element_list = tree.findall('compound')
            for compound_element in compound_element_list:
                # possible dox_types to skip (where kind=<dox_type>):
                # - "page" (a .dox page)
                # - "dir" (a subdirectory)
                # - "file" (a source file copy)
                kind = compound_element.get('kind')
                if (kind == "page"):
                    continue
                if (kind == "dir"):
                    continue
                # we need to keep kind == "file" because this holds info on functions
                refid = compound_element.get('refid')
                # Individual entity XML generated XML files are <refid>.xml
                entity_file_name = refid + ".xml"
                # pre-pend path to index file to individual XML files
                entity_file_name = os.path.join(index_dir, entity_file_name)
                entity_file_list.append(entity_file_name)
        if len(entity_file_list) <= 0:
            Error("No entity XML files found in %s" % doxygen_index_file)
            return False
        # Parse each entity XML file
        try:
            parser = xml.sax.make_parser()
            parser.setFeature(xml.sax.handler.feature_namespaces, 0)
            xmlparser = XMLParser()
            parser.setContentHandler(xmlparser)
            for file in entity_file_list:
                Debug("Attempting to parse file: '%s'" % file)
                with open(file, mode="r") as content:
                    parser.parse(content)
            self.rootNode = xmlparser.getRoots()
            return True
        except Exception:
            return False

    #
    # Routines to traverse the Doxygen XML tree and produce a tree of
    # DocElement nodes to describe each documentation element
    #

    def traverse(self, writerClass):
        """Traverse the XML tree and builds DocElements for each item."""
        
        # ensure we have a class to create our doc strings for us
        if not writerClass:
            Error("traverse: No writer class was specified.")

        # traverse the XML tree and generate DocElements
        self.writer = writerClass
        self.docElements = []
        for node in self.rootNode:
            obj = self.__traverse_r(node)
            self.__resolveInnerClassRefs(obj)
            self.docElements += obj

        return self.docElements

    def __traverse_r(self, xmlNode):
        """Recursive continuation of the traverse() method."""
        resultList = []
        
        # get the doc element for this node (may be None)
        obj = self.__createDocElement(xmlNode)
        if obj is not None:
            resultList = [obj]
            
        # recursely decent through all children nodes
        for childNode in xmlNode.childNodes:
            childObjs = self.__traverse_r(childNode)
            if obj is not None:
                obj.addChildren(childObjs)
            else:
                resultList += childObjs

        return resultList

    def __resolveInnerClassRefs(self, objlist):
        # Walk the tree and accumulate a map of innerclass refs with their
        # parents.  Then walk the tree again finding classes with the
        # matching names.  For each one found, modify the name and inject it
        # into the parent.
        innerClassRefs = {}
        for o in objlist:
            refs = self.__findInnerClassRefs(o)
            self.__resolveInnerClassRefs_r(None, o, refs)
            
    def __findInnerClassRefs(self, obj):
        # returns a dictionary mapping from inner class names to the parents
        # that want them in this subtree.
        ret = {}
        for childName, childObjectList in obj.children.items():
            for child in childObjectList:
                if child.isInnerClass():
                    # this obj wants an innerclass of childName
                    ret[childName] = obj
                else:
                    ret.update(self.__findInnerClassRefs(child))
        return ret

    def __resolveInnerClassRefs_r(self, parent, obj, refs):
        if not parent is None and obj.isClass():
            if obj.name in refs:
                refname = obj.name
                # strip off leading names to leave class name.
                obj.name = obj.name.split('::')[-1]
                refs[refname].replaceInnerClass(refname, obj)
                parent.removeChildrenWithName(refname)
                # erase this guy from refs?  should be only one out there.
        for childName, childObjectList in list(obj.children.items()):
            for childObj in childObjectList:
                self.__resolveInnerClassRefs_r(obj, childObj, refs)

    #
    # Routines to gather the documentation, ask the writer plugin to format
    # the doc strings, and creates DocElement structures for each instance.
    #

    def __getDocStringFromWriter(self, node, nodeName):
        """Call the Writer plugin to format the docstring for this node."""
        
        docstring = ''
        tags = []
        nodeUnder = node.findNode(nodeName)
        if not nodeUnder is None:
            docstring = self.writer.getDocString(nodeUnder)
            tags = self.writer.getDocTags(nodeUnder)
        return docstring, tags

    def __getAllDocStrings(self, node, nodeName):
        """Ask the Writer plugin to fill in all of the doc strings."""
        Debug("Calling Writer plugin on node '%s'" % nodeName)
        ret = {}
        ret['brief'], tags0 = self.__getDocStringFromWriter(node, 'briefdescription')
        ret['detailed'], tags1 = self.__getDocStringFromWriter(node, 'detaileddescription')
        ret['inbody'], tags2 = self.__getDocStringFromWriter(node, 'inbodydescription')
        ret['tags'] = tags0 + tags1 + tags2
        return ret

    def __getAllParams(self, node):
        """Get the name and type of each parameter of a function."""
        params = []
        for child in node.childNodes:
            if child.name == 'param':
                pname = child.getText('declname')
                ptype = child.getText('type')
                pdefault = child.getText('defval') or None
                params.append(Param(ptype, pname, pdefault))
        return params

    def __createDocElement(self, node):
        """Create a DocElement object for the current node."""
        ret = None
        if node.name == 'doxygen':
            #
            # Found the doxygen root node
            #
            kind = 'root'
            name = node.name
            prot = ''
            doc = self.__getAllDocStrings(node, name)
            location = node.getLocation()
            ret = DocElement(name, kind, prot, doc, location)
        elif node.name == 'innerclass':
            #
            # Found an innerclass
            #
            kind = 'innerclass'
            name = node.getText()
            prot = ''
            doc = self.__getAllDocStrings(node, name)
            location = node.getLocation()
            if location != ('', ''):
                # These elements shadow class elements of the same name, but they 
                # lack any valuable information, and thus create empty docstrings.
                ret = DocElement(name, kind, prot, doc, location)
        elif node.name == 'compounddef':
            kind = node.getKind()
            if kind == 'class' or kind == 'struct':
                #
                # Found a class
                #
                kind = 'class'
                name = node.getText('compoundname')
                if len(name):
                    prot = node.getProt()
                    doc = self.__getAllDocStrings(node, name)
                    location = node.getLocation()
                    ret = DocElement(name, kind, prot, doc, location)
            elif kind == 'page':
                name = node.getText('compoundname')
                if name == "index":
                    #
                    # Found a module description
                    #
                    kind = 'module'
                    prot = ''
                    doc = self.__getAllDocStrings(node, name)
                    location = node.getLocation()
                    ret = DocElement(name, kind, prot, doc, location)
        elif node.name == 'memberdef':
            kind = node.getKind()
            if kind == 'function':
                #
                # Found a function/method
                #
                name = node.getText('name')
                if len(name):
                    prot = node.getProt()
                    doc = self.__getAllDocStrings(node, name)
                    location = node.getLocation()
                    ret = DocElement(name, kind, prot, doc, location)
                    ret.const = node.getAttrValue('const')
                    ret.virt = node.getAttrValue('virt')
                    ret.explicit = node.getAttrValue('explicit')
                    ret.static = node.getAttrValue('static')
                    ret.inline = node.getAttrValue('inline')
                    ret.returnType = node.getText('type')
                    ret.argsString = node.getText('argsstring')
                    ret.definition = node.getText('definition')
                    ret.params = self.__getAllParams(node)
                          
            elif kind == 'enum':
                name = node.getText('name')
                if len(name): 
                    #
                    # Found an enumerated type
                    #
                    kind = 'enum'
                    prot = ''
                    doc = self.__getAllDocStrings(node, name)
                    location = node.getLocation()
                    ret = DocElement(name, kind, prot, doc, location)

        return ret



