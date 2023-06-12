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
# cdDocElement.py
#
# Provides the DocElement class, which represents a single API documentation
# instance, such as the documentation for a single class, method, or function.
#
# The Parser class is responsible for building up a list of these objects.
#

from .cdUtils import Warn

class DocElement:
    """
    Describe the documentation for a single class, method, function, etc.

    This data structure supports a hierarchy of doc elements, such that
    methods of a class are children of that class node. To see what this
    hierarchy looks like, you could print it out using printDocElementTree().

    Also, each child is actually a list of DocElements, where each entry
    in that list represents an alternative calling signature for the
    same function, i.e., in C++ parlance, the overloaded methods.
    """

    def __init__(self, name, kind, prot, doc, location):
        self.name = name                     # the name of this class/method
        self.kind = kind                     # e.g., function, class, etc.
        self.prot = prot                     # e.g., public, private, protected
        self.doc = doc                       # the actual doc string
        self.location = location             # (lineno, filename) tuple
        self.children = {}                   # children of this doc element
        self.const = None                    # is this is a const method?
        self.virt = None                     # is this is a virtual method?
        self.explicit = None                 # is this an explicit cnstr?
        self.static = None                   # is this a static method?
        self.inline = None                   # is this an inlined method?
        self.returnType = None               # return type of a method/function
        self.argsString = None               # arguments for this method/func
        self.definition = None               # full C++ definition for method
        self.params = None                   # name and type of each parameter

    def isFunction(self):
        """Is this doc element a function?"""
        return self.kind == 'function'

    def isClass(self):
        """Is this doc element a class?"""
        return self.kind == 'class'

    def isInnerClass(self):
        """Is this doc element an inner class?"""
        return self.kind == 'innerclass'

    def isModule(self):
        """Is this doc element a module or package?"""
        return self.kind == 'module'

    def isEnum(self):
        """Is this doc element the name of an enum type?"""
        return self.kind == 'enum'

    def isRoot(self):
        """Is this doc element the root of the doxygen XML tree?"""
        return self.kind == 'root'

    def addChildren(self, children):
        """Adds the list of nodes as children of this node."""
        for child in children:
            self.__addChild(child)

    def removeChildrenWithName(self, name):
        """Remove a named child of this node."""
        if name in self.children:
            del(self.children[name])

    def replaceInnerClass(self, innerClassName, obj):
        """Replace any named inner classes with the provided child node."""
        for childName, childList in list(self.children.items()):
            if childName == innerClassName and len(childList) == 1 and childList[0].isInnerClass():
                del(self.children[childName])
                self.__addChild(obj)
                return
        Warn('could not find innerclass %s in %s' % (innerClassName,self.name))

    def __addChild(self, child):
        if child.name in self.children:
            firstOverload = self.children[child.name][0]
            # only allow overloaded functions
            if firstOverload.isFunction() and child.isFunction():
                self.children[child.name].append(child)
            else:
                k1 = self.children[child.name][0].kind
                k2 = child.kind
                if (k1 == 'innerclass' and k2 == 'class') or \
                   (k2 == 'innerclass' and k1 == 'class'):
                    # we get an innerclass of ourselves in header files
                    # so just ignore it.
                    pass
                else:
                    Warn('overload mismatch: expected functions, got %s and %s' % \
                          (self.children[child.name][0].kind, child.kind))
        else:
            self.children[child.name] = [child]


def printDocElementTree(docList, tabs=0):
    """Dump out a list of DocElement nodes, and all their children."""
    indent = " " * (tabs*2)
    if not isinstance(docList, type([])):
        docList = [docList]
    for doc in docList:
        print("%s%s (%s, %s)" % (indent, doc.name, doc.kind, doc.prot))
        for childName, childList in list(doc.children.items()):
            printDocElementTree(childList, tabs + 1)
            

