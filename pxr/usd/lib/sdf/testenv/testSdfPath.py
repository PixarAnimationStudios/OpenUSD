#!/pxrpythonsubst

import sys
from Mentor.Runtime import *

from pxr.Sdf.Test.testUtils_Sdf import *
from pxr import Sdf, Tf

# Assert failures end test
SetAssertMode(MTR_EXIT_TEST)

# ========================================================================
# Test SdfPath creation and pathString
# ========================================================================

print '\nTest creating bad paths: warnings expected'

# XXX: Here are a couple bad paths that are 
# currently allowed...  add these to the test cases when they are properly 
# disallowed
#  ../../
#  .rel[targ][targ].attr
#  .attr[1, 2, 3].attr

badPaths = '''
    DD/DDD.&ddf$
    DD[]/DDD
    DD[]/DDD.bar
    foo.prop/bar
    /foo.prop/bar.blah
    /foo.prop/bar.blah
    /foo//bar
    /foo/.bar
    /foo..bar
    /foo.bar.baz
    /.foo
    </foo.bar
    </Foo/Bar/>
    /Foo/Bar/
    /Foo.bar[targ]/Bar
    /Foo.bar[targ].foo.foo
    123
    123test
    /Foo:Bar
    /Foo.bar.mapper[/Targ.attr].arg:name:space
    '''.split()
for badPath in badPaths:
    AssertTrue(Sdf.Path(badPath).isEmpty)
    AssertEqual(Sdf.Path(badPath), Sdf.Path())
    AssertEqual(Sdf.Path(badPath), Sdf.Path.emptyPath)
    AssertFalse(Sdf.Path.IsValidPathString(badPath))
print '\tPassed'

# Test lessthan
Assert(Sdf.Path('aaa') < Sdf.Path('aab'))
Assert(not Sdf.Path('aaa') < Sdf.Path())
Assert(Sdf.Path('/') < Sdf.Path('/a'))

# XXX test path from elements ['.prop'] when we have that wrapped?

# ========================================================================
# Test SdfPath other queries
# ========================================================================

print '\nTest scenepath queries'
testPathStrings = [
    "/Foo/Bar.baz",
    "Foo",
    "Foo/Bar",
    "Foo.bar",
    "Foo/Bar.bar",
    ".bar",
    "/Some/Kinda/Long/Path/Just/To/Make/Sure",
    "Some/Kinda/Long/Path/Just/To/Make/Sure.property",
    "../Some/Kinda/Long/Path/Just/To/Make/Sure",
    "../../Some/Kinda/Long/Path/Just/To/Make/Sure.property",
    "/Foo/Bar.baz[targ].boom",
    "Foo.bar[targ].boom",
    ".bar[targ].boom",
    "Foo.bar[targ.attr].boom",
    "/A/B/C.rel3[/Blah].attr3",
    "A/B.rel2[/A/B/C.rel3[/Blah].attr3].attr2",
    "/A.rel1[/A/B.rel2[/A/B/C.rel3[/Blah].attr3].attr2].attr1"
]
testAbsolute = [True, False, False, False, False, False, True, False, False, False, True, False, False, False, True, False, True]
testProperty = [True, False, False, True, True, True, False, True, False, True, True, True, True, True, True, True, True]
testElements = [
    ["Foo", "Bar", ".baz"],
    ["Foo"],
    ["Foo", "Bar"],
    ["Foo", ".bar"],
    ["Foo", "Bar", ".bar"],
    [".bar"],
    ["Some", "Kinda", "Long", "Path", "Just", "To", "Make", "Sure"],
    ["Some", "Kinda", "Long", "Path", "Just", "To", "Make", "Sure", ".property"],
    ["..", "Some", "Kinda", "Long", "Path", "Just", "To", "Make", "Sure"],
    ["..", "..", "Some", "Kinda", "Long", "Path", "Just", "To", "Make", "Sure", ".property"],
    ["Foo", "Bar", ".baz", "[targ]", ".boom"],
    ["Foo", ".bar", "[targ]", ".boom"],
    [".bar", "[targ]", ".boom"],
    ["Foo", ".bar", "[targ.attr]", ".boom"],
    ["A", "B", "C", ".rel3", "[/Blah]", ".attr3"],
    ["A", "B", ".rel2", "[/A/B/C.rel3[/Blah].attr3]", ".attr2"],
    ["A", ".rel1", "[/A/B.rel2[/A/B/C.rel3[/Blah].attr3].attr2]", ".attr1"]
]

# Test IsAbsolutePath and IsPropertyPath
def BasicTest(path, elements):
    AssertEqual(path.IsAbsolutePath(), testAbsolute[testIndex])
    AssertEqual(path.IsPropertyPath(), testProperty[testIndex])
    prefixes = PathElemsToPrefixes(path.IsAbsolutePath(), elements)
    AssertEqual(path.GetPrefixes(), prefixes)
    AssertEqual(path, eval(repr(path)))
    AssertTrue(Sdf.Path.IsValidPathString(str(path)))

testPaths = list()
for testIndex in range(len(testPathStrings)):
    string = testPathStrings[testIndex]

    # Test path
    testPaths.append(Sdf.Path(string))
    BasicTest(testPaths[-1], testElements[testIndex])

    # If path is a property then try it with a namespaced name.
    if testProperty[testIndex]:
        testPaths.append(Sdf.Path(string + ':this:has:namespaces'))
        elements = list(testElements[testIndex])
        elements[-1] += ':this:has:namespaces'
        BasicTest(testPaths[-1], elements)

print '\tPassed'

# ========================================================================
# Test SdfPath hashing
# ========================================================================
dict = {}
for i in enumerate(testPaths):
    dict[i[1]] = i[0]
for i in enumerate(testPaths):
    AssertEqual(dict[i[1]], i[0])

# ========================================================================
# Test SdfPath <-> string conversion
# ========================================================================

AssertEqual(Sdf.Path('foo'), 'foo')
AssertEqual('foo', Sdf.Path('foo'))

AssertNotEqual(Sdf.Path('foo'), 'bar')
AssertNotEqual('bar', Sdf.Path('foo'))

# Test repr w/ quotes
pathWithQuotes = Sdf.Path("b'\"")
AssertEqual( eval(repr(pathWithQuotes)), pathWithQuotes )

# ========================================================================
# Test SdfPath -> bool conversion
# ========================================================================

# Empty paths should evaluate to false, other paths should evaluate to true.
Assert(Sdf.Path('/foo.bar[baz]'))
Assert(Sdf.Path('/foo.bar'))
Assert(Sdf.Path('/'))
Assert(Sdf.Path('.'))
Assert(not Sdf.Path())

# For now, creating a path with Sdf.Path('') emits a warning and returns 
# the empty path.
warnings = []
def _HandleWarning(notice, sender):
    warnings.append(notice.warning)

listener = Tf.Notice.RegisterGlobally(Tf.DiagnosticNotice.IssuedWarning,
                                      _HandleWarning)
with ExpectedWarnings(1):
    Assert(not Sdf.Path(''))

AssertEqual(len(warnings), 1);

# ========================================================================
# Test converting relative paths to absolute paths
# ========================================================================
print "Test converting relative paths to absolute paths"

anchor = Sdf.Path("/A/B/E/F/G")
relPath = Sdf.Path("../../../C/D")
AssertEqual(relPath.MakeAbsolutePath( anchor ), "/A/B/C/D")

# Try too many ".."s for the base path
ExpectedErrorBegin(3)
AssertEqual(relPath.MakeAbsolutePath( "/A" ), Sdf.Path.emptyPath)
ExpectedErrorEnd()

relPath = Sdf.Path("../../../..") 
AssertEqual(relPath.MakeAbsolutePath( anchor ), "/A")

relPath = Sdf.Path("../../.radius")
AssertEqual(relPath.MakeAbsolutePath( anchor ), "/A/B/E.radius")

relPath = Sdf.Path("..")
ExpectedErrorBegin(1)
AssertEqual(relPath.MakeAbsolutePath( Sdf.Path( "../../A/B") ), Sdf.Path.emptyPath)
ExpectedErrorEnd()

# test passing a property path as the anchor
ExpectedErrorBegin(1)
AssertEqual(relPath.MakeAbsolutePath( Sdf.Path( "/A/B.radius") ), Sdf.Path.emptyPath)
ExpectedErrorEnd()

# test on an absolute path
AssertEqual(anchor.MakeAbsolutePath( anchor ), anchor)
print '\tPassed'

# ========================================================================
# Test converting absolute paths to relative paths
# ========================================================================
print "Test converting absolute paths to relative paths"

anchor = Sdf.Path("/A/B/E/F/G")
absPath = Sdf.Path("/A/B/C/D")
AssertEqual(absPath.MakeRelativePath( anchor ), "../../../C/D")

absPath = Sdf.Path("/H/I/J")
AssertEqual(absPath.MakeRelativePath( anchor ), "../../../../../H/I/J")

absPath = Sdf.Path("/A/B/E/F/G/H/I/J.radius")
AssertEqual(absPath.MakeRelativePath( anchor ), "H/I/J.radius")

anchor = Sdf.Path("/A/B")
absPath = Sdf.Path("/A.radius")
AssertEqual( absPath.MakeRelativePath( anchor ), "../.radius" )

ExpectedErrorBegin(1)
AssertEqual(absPath.MakeRelativePath( Sdf.Path("H/I") ), "")
ExpectedErrorEnd()

# test passing a property path as the anchor
ExpectedErrorBegin(1)
AssertEqual(absPath.MakeRelativePath( Sdf.Path( "/A/B.radius") ), "")
ExpectedErrorEnd()

print '\tPassed'

# ========================================================================
# Test converting sub-optimal relative paths to optimal relative paths
# ========================================================================
print "Test converting sub-optimal relative paths to optimal relative paths"

anchor = Sdf.Path("/A/B/C")
relPath = Sdf.Path("../../B/C/D")
AssertEqual(relPath.MakeRelativePath( anchor ), "D")

relPath = Sdf.Path("../../../A")
AssertEqual(relPath.MakeRelativePath( anchor ), "../..")
print '\tPassed'

# ========================================================================
# Test GetPrimPath
# ========================================================================
print "Test GetPrimPath"

primPath = Sdf.Path("/A/B/C").GetPrimPath()
AssertEqual(primPath, Sdf.Path("/A/B/C"))

primPath = Sdf.Path("/A/B/C.foo").GetPrimPath()
AssertEqual(primPath, Sdf.Path("/A/B/C"))
primPath = Sdf.Path("/A/B/C.foo:bar:baz").GetPrimPath()
AssertEqual(primPath, Sdf.Path("/A/B/C"))

primPath = Sdf.Path("/A/B/C.foo[target].bar").GetPrimPath()
AssertEqual(primPath, Sdf.Path("/A/B/C"))
primPath = Sdf.Path("/A/B/C.foo[target].bar:baz").GetPrimPath()
AssertEqual(primPath, Sdf.Path("/A/B/C"))

primPath = Sdf.Path("A/B/C.foo[target].bar").GetPrimPath()
AssertEqual(primPath, Sdf.Path("A/B/C"))
primPath = Sdf.Path("A/B/C.foo[target].bar:baz").GetPrimPath()
AssertEqual(primPath, Sdf.Path("A/B/C"))

primPath = Sdf.Path("../C.foo").GetPrimPath()
AssertEqual(primPath, Sdf.Path("../C"))
primPath = Sdf.Path("../C.foo:bar:baz").GetPrimPath()
AssertEqual(primPath, Sdf.Path("../C"))

primPath = Sdf.Path("../.foo[target].bar").GetPrimPath()
AssertEqual(primPath, Sdf.Path(".."))
primPath = Sdf.Path("../.foo[target].bar:baz").GetPrimPath()
AssertEqual(primPath, Sdf.Path(".."))

print '\tPassed'

# ========================================================================
# Test HasPrefix and ReplacePrefix
# ========================================================================
print "Test hasPrefix and replacePrefix"

# Test HasPrefix
AssertFalse( Sdf.Path.emptyPath.HasPrefix('A') )
AssertFalse( Sdf.Path('A').HasPrefix( Sdf.Path.emptyPath ) )
aPath = Sdf.Path("/Chars/Buzz_1/LArm.FB")
AssertEqual( aPath.HasPrefix( "/Chars/Buzz_1" ), True )
AssertEqual( aPath.HasPrefix( "Buzz_1" ), False )

# Replace aPath's prefix and get a new path
bPath = aPath.ReplacePrefix( "/Chars/Buzz_1", "/Chars/Buzz_2" )
AssertEqual( bPath, Sdf.Path("/Chars/Buzz_2/LArm.FB") )

# Specify a bogus prefix to replace and get an empty path
cPath = bPath.ReplacePrefix("/BadPrefix/Buzz_2", "/ReleasedChars/Buzz_2")
AssertEqual( cPath, bPath )

# This formerly crashed due to a reference counting bug.
p = Sdf.Path('/A/B.a[/C/D.a[/E/F.a]].a')
p.ReplacePrefix('/E/F', '/X')
p.ReplacePrefix('/E/F', '/X')
p.ReplacePrefix('/E/F', '/X')
p.ReplacePrefix('/E/F', '/X')

# This formerly failed to replace due to an early out if the element count
# was not longer than the element count of the prefix path we were replacing.
p = Sdf.Path('/A.a[/B/C/D/E]') # Element count 3 [A, a, [/B/C/D/E] ]
result = p.ReplacePrefix('/B/C/D/E', # Element count 4
                         '/B/C/D/F')
AssertEqual(result, Sdf.Path('/A.a[/B/C/D/F]'))

# Test replacing target paths.
p = Sdf.Path('/A/B.a[/C/D.a[/A/F.a]].a')
assert (p.ReplacePrefix('/A', '/_', fixTargetPaths=False)
        == Sdf.Path('/_/B.a[/C/D.a[/A/F.a]].a'))
assert (p.ReplacePrefix('/A', '/_', fixTargetPaths=True)
        == Sdf.Path('/_/B.a[/C/D.a[/_/F.a]].a'))

# Test invalid prefix replacements.
with RequiredException(Tf.ErrorException):
    Sdf.Path('/A{x=y}B').ReplacePrefix('/A', '/A{x=z}')

# ========================================================================
# Test RemoveCommonSuffix
# ========================================================================
print "Test RemoveCommonSuffix"

aPath = Sdf.Path('/A/B/C')
bPath = Sdf.Path('/X/Y/Z')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/A/B/C'))
AssertEqual(r2, Sdf.Path('/X/Y/Z'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/A/B/C'))
AssertEqual(r2, Sdf.Path('/X/Y/Z'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('X/Y/Z')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A/B/C'))
AssertEqual(r2, Sdf.Path('X/Y/Z'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A/B/C'))
AssertEqual(r2, Sdf.Path('X/Y/Z'))

aPath = Sdf.Path('/A/B/C')
bPath = Sdf.Path('/X/Y/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/A/B'))
AssertEqual(r2, Sdf.Path('/X/Y'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/A/B'))
AssertEqual(r2, Sdf.Path('/X/Y'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('X/Y/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('X/Y'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('X/Y'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('/X/Y/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('/X/Y'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('/X/Y'))

aPath = Sdf.Path('/A/B/C')
bPath = Sdf.Path('/X/B/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/A'))
AssertEqual(r2, Sdf.Path('/X'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/A'))
AssertEqual(r2, Sdf.Path('/X'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('X/B/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A'))
AssertEqual(r2, Sdf.Path('X'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A'))
AssertEqual(r2, Sdf.Path('X'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('/X/B/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A'))
AssertEqual(r2, Sdf.Path('/X'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A'))
AssertEqual(r2, Sdf.Path('/X'))

aPath = Sdf.Path('/A/B/C')
bPath = Sdf.Path('/A/B/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/'))
AssertEqual(r2, Sdf.Path('/'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/A'))
AssertEqual(r2, Sdf.Path('/A'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('A/B/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('.'))
AssertEqual(r2, Sdf.Path('.'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A'))
AssertEqual(r2, Sdf.Path('A'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('/A/B/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('.'))
AssertEqual(r2, Sdf.Path('/'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A'))
AssertEqual(r2, Sdf.Path('/A'))

aPath = Sdf.Path('/A/B/C')
bPath = Sdf.Path('/X/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/A/B'))
AssertEqual(r2, Sdf.Path('/X'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/A/B'))
AssertEqual(r2, Sdf.Path('/X'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('X/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('X'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('X'))

aPath = Sdf.Path('/A/B/C')
bPath = Sdf.Path('/B/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/A'))
AssertEqual(r2, Sdf.Path('/'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/A/B'))
AssertEqual(r2, Sdf.Path('/B'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('B/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A'))
AssertEqual(r2, Sdf.Path('.'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('B'))

aPath = Sdf.Path('/A/B/C')
bPath = Sdf.Path('/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/A/B'))
AssertEqual(r2, Sdf.Path('/'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/A/B/C'))
AssertEqual(r2, Sdf.Path('/C'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('.'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A/B/C'))
AssertEqual(r2, Sdf.Path('C'))

aPath = Sdf.Path('A/B/C')
bPath = Sdf.Path('/C')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('A/B'))
AssertEqual(r2, Sdf.Path('/'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('A/B/C'))
AssertEqual(r2, Sdf.Path('/C'))

aPath = Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
bPath = Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz].intensity')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet]'))
AssertEqual(r2, Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz]'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet]'))
AssertEqual(r2, Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz]'))

aPath = Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
bPath = Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz].intensity')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz/Helmet]'))
AssertEqual(r2, Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz]'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz/Helmet]'))
AssertEqual(r2, Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz]'))

aPath = Sdf.Path('/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
bPath = Sdf.Path('/Lights2/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('/Lights'))
AssertEqual(r2, Sdf.Path('/Lights2'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('/Lights'))
AssertEqual(r2, Sdf.Path('/Lights2'))

aPath = Sdf.Path('Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
bPath = Sdf.Path('Lights2/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity')
(r1, r2) = aPath.RemoveCommonSuffix(bPath)
AssertEqual(r1, Sdf.Path('Lights'))
AssertEqual(r2, Sdf.Path('Lights2'))
(r1, r2) = aPath.RemoveCommonSuffix(bPath, stopAtRootPrim=True)
AssertEqual(r1, Sdf.Path('Lights'))
AssertEqual(r2, Sdf.Path('Lights2'))

# ========================================================================
# Test GetTargetPath
# ========================================================================
print "Test targetPath"

aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity")
AssertEqual( aPath.targetPath, Sdf.Path("/Chars/Buzz/Helmet") )

aPath = Sdf.Path("/Lights/Lkey.shinesOn[../../Buzz/Helmet].intensity")
AssertEqual( aPath.targetPath, Sdf.Path("../../Buzz/Helmet") )

aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity")
AssertEqual( aPath.targetPath, Sdf.Path("/Chars/Buzz/Helmet") )

aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet.blah].intensity")
AssertEqual( aPath.targetPath, Sdf.Path("/Chars/Buzz/Helmet.blah") )

# No target
aPath = Sdf.Path.emptyPath
AssertEqual( aPath.targetPath, Sdf.Path.emptyPath )


# ========================================================================
# Test GetAllTargetPathsRecursively
# ========================================================================
print "Test GetAllTargetPathsRecursively"

aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity")
AssertEqual( aPath.GetAllTargetPathsRecursively(), 
             [Sdf.Path("/Chars/Buzz/Helmet")] )

aPath = Sdf.Path("/Lights/Lkey.shinesOn[../../Buzz/Helmet].intensity")
AssertEqual( aPath.GetAllTargetPathsRecursively(),
             [Sdf.Path("../../Buzz/Helmet")] )

aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet].intensity")
AssertEqual( aPath.GetAllTargetPathsRecursively(),
             [Sdf.Path("/Chars/Buzz/Helmet")] )

aPath = Sdf.Path("/Lights/Lkey.shinesOn[/Chars/Buzz/Helmet.blah].intensity")
AssertEqual( aPath.GetAllTargetPathsRecursively(),
             [Sdf.Path("/Chars/Buzz/Helmet.blah")] )

aPath = Sdf.Path('/A/B.a[/C/D.a[/E/F.a]].a')
AssertEqual(aPath.GetAllTargetPathsRecursively(),
            [Sdf.Path(x) for x in ['/C/D.a[/E/F.a]', '/E/F.a']])

aPath = Sdf.Path('/A/B.a[/C/D.a[/E/F.a]].a[/A/B.a[/C/D.a]]')
AssertEqual(aPath.GetAllTargetPathsRecursively(),
            [Sdf.Path(x) for x in ['/A/B.a[/C/D.a]', '/C/D.a', 
                                  '/C/D.a[/E/F.a]', '/E/F.a']])

aPath = Sdf.Path('/No/Target/Paths')
AssertEqual(aPath.GetAllTargetPathsRecursively(), [])


# =======================================================================
# Test AppendChild
# =======================================================================
print "Test appendChild"

aPath = Sdf.Path("/foo")
AssertEqual( aPath.AppendChild("bar"), Sdf.Path("/foo/bar") )
aPath = Sdf.Path("foo")
AssertEqual( aPath.AppendChild("bar"), Sdf.Path("foo/bar") )

aPath = Sdf.Path("/foo.prop")
ExpectedErrorBegin(1)
AssertEqual( aPath.AppendChild("bar"), Sdf.Path.emptyPath )
ExpectedErrorEnd()

# =======================================================================
# Test AppendProperty
# =======================================================================
print "Test appendProperty"

aPath = Sdf.Path("/foo")
AssertEqual( aPath.AppendProperty("prop"), Sdf.Path("/foo.prop") )
AssertEqual( aPath.AppendProperty("prop:foo:bar"), Sdf.Path("/foo.prop:foo:bar") )

aPath = Sdf.Path("/foo.prop")
ExpectedErrorBegin(2)
AssertEqual( aPath.AppendProperty("prop2"), Sdf.Path.emptyPath )
AssertEqual( aPath.AppendProperty("prop2:foo:bar"), Sdf.Path.emptyPath )
ExpectedErrorEnd()

# =======================================================================
# Test AppendPath
# =======================================================================
print "Test AppendPath"

# append to empty path -> empty path
AssertException("Sdf.Path().AppendPath( Sdf.Path() )", Tf.ErrorException)
AssertException("Sdf.Path().AppendPath( Sdf.Path('A') )", Tf.ErrorException)

# append to root/prim path
assert Sdf.Path('/').AppendPath( Sdf.Path('A') ) == Sdf.Path('/A')
assert Sdf.Path('/A').AppendPath( Sdf.Path('B') ) == Sdf.Path('/A/B')

# append empty to root/prim path -> no change
AssertException("Sdf.Path('/').AppendPath( Sdf.Path() )", Tf.ErrorException)
AssertException("Sdf.Path('/A').AppendPath( Sdf.Path() )", Tf.ErrorException)

# =======================================================================
# Test AppendTarget
# =======================================================================
print "Test appendTarget"

aPath = Sdf.Path("/foo.rel")
AssertEqual( aPath.AppendTarget("/Bar/Baz"), Sdf.Path("/foo.rel[/Bar/Baz]") )

aPath = Sdf.Path("/foo")
ExpectedErrorBegin(1)
AssertEqual( aPath.AppendTarget("/Bar/Baz"), Sdf.Path.emptyPath )
ExpectedErrorEnd()

# =======================================================================
# Test AppendRelationalAttribute
# =======================================================================
print "Test appendRelationalAttribute"

aPath = Sdf.Path("/foo.rel[/Bar/Baz]")
AssertEqual( aPath.AppendRelationalAttribute("attr"), Sdf.Path("/foo.rel[/Bar/Baz].attr") )
AssertEqual( aPath.AppendRelationalAttribute("attr:foo:bar"), Sdf.Path("/foo.rel[/Bar/Baz].attr:foo:bar") )

aPath = Sdf.Path("/foo")
ExpectedErrorBegin(2)
AssertEqual( aPath.AppendRelationalAttribute("attr"), Sdf.Path.emptyPath )
AssertEqual( aPath.AppendRelationalAttribute("attr:foo:bar"), Sdf.Path.emptyPath )
ExpectedErrorEnd()

# =======================================================================
# Test GetParentPath and GetName
# =======================================================================
print "Test parentPath, name, and replaceName"

AssertEqual(Sdf.Path("/foo/bar/baz").GetParentPath(), Sdf.Path("/foo/bar"))
AssertEqual(Sdf.Path("/foo").GetParentPath(), Sdf.Path("/"))
AssertEqual(Sdf.Path.emptyPath.GetParentPath(), Sdf.Path.emptyPath)
AssertEqual(Sdf.Path("/foo/bar/baz.prop").GetParentPath(), Sdf.Path("/foo/bar/baz"))
AssertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr").GetParentPath(), Sdf.Path("/foo/bar/baz.rel[/targ]"))
AssertEqual(Sdf.Path("/foo/bar/baz.rel[/targ]").GetParentPath(), Sdf.Path("/foo/bar/baz.rel"))
AssertEqual(Sdf.Path("../../..").GetParentPath(), Sdf.Path("../../../.."))
AssertEqual(Sdf.Path("..").GetParentPath(), Sdf.Path("../.."))
AssertEqual(Sdf.Path("../../../.prop").GetParentPath(), Sdf.Path("../../.."))
AssertEqual(Sdf.Path("../../../.rel[/targ]").GetParentPath(), Sdf.Path("../../../.rel"))
AssertEqual(Sdf.Path("../../../.rel[/targ].attr").GetParentPath(), Sdf.Path("../../../.rel[/targ]"))
AssertEqual(Sdf.Path("foo/bar/baz").GetParentPath(), Sdf.Path("foo/bar"))
AssertEqual(Sdf.Path("foo").GetParentPath(), Sdf.Path("."))
AssertEqual(Sdf.Path("foo/bar/baz.prop").GetParentPath(), Sdf.Path("foo/bar/baz"))
AssertEqual(Sdf.Path("foo/bar/baz.rel[/targ]").GetParentPath(), Sdf.Path("foo/bar/baz.rel"))
AssertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr").GetParentPath(), Sdf.Path("foo/bar/baz.rel[/targ]"))

AssertEqual(Sdf.Path("/foo/bar/baz").name, "baz")
AssertEqual(Sdf.Path("/foo").name, "foo")
AssertEqual(Sdf.Path.emptyPath.name, "")
AssertEqual(Sdf.Path("/foo/bar/baz.prop").name, "prop")
AssertEqual(Sdf.Path("/foo/bar/baz.prop:argle:bargle").name, "prop:argle:bargle")
AssertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr").name, "attr")
AssertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr:argle:bargle").name, "attr:argle:bargle")
AssertEqual(Sdf.Path("../../..").name, "..")
AssertEqual(Sdf.Path("../../.prop").name, "prop")
AssertEqual(Sdf.Path("../../.prop:argle:bargle").name, "prop:argle:bargle")
AssertEqual(Sdf.Path("../../.rel[/targ].attr").name, "attr")
AssertEqual(Sdf.Path("../../.rel[/targ].attr:argle:bargle").name, "attr:argle:bargle")
AssertEqual(Sdf.Path("foo/bar/baz").name, "baz")
AssertEqual(Sdf.Path("foo").name, "foo")
AssertEqual(Sdf.Path("foo/bar/baz.prop").name, "prop")
AssertEqual(Sdf.Path("foo/bar/baz.prop:argle:bargle").name, "prop:argle:bargle")
AssertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr").name, "attr")
AssertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr:argle:bargle").name, "attr:argle:bargle")

AssertEqual(Sdf.Path("/foo/bar/baz").ReplaceName('foo'), Sdf.Path("/foo/bar/foo"))
AssertEqual(Sdf.Path("/foo").ReplaceName('bar'), Sdf.Path("/bar"))
AssertEqual(Sdf.Path("/foo/bar/baz.prop").ReplaceName('attr'),
            Sdf.Path("/foo/bar/baz.attr"))
AssertEqual(Sdf.Path("/foo/bar/baz.prop").ReplaceName('attr:argle:bargle'),
            Sdf.Path("/foo/bar/baz.attr:argle:bargle"))
AssertEqual(Sdf.Path("/foo/bar/baz.prop:argle:bargle").ReplaceName('attr'),
            Sdf.Path("/foo/bar/baz.attr"))
AssertEqual(Sdf.Path("/foo/bar/baz.prop:argle:bargle").ReplaceName('attr:foo:fa:raw'),
            Sdf.Path("/foo/bar/baz.attr:foo:fa:raw"))
AssertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr").ReplaceName('prop'),
            Sdf.Path("/foo/bar/baz.rel[/targ].prop"))
AssertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr").ReplaceName('prop:argle:bargle'),
            Sdf.Path("/foo/bar/baz.rel[/targ].prop:argle:bargle"))
AssertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr:argle:bargle").ReplaceName('prop'),
            Sdf.Path("/foo/bar/baz.rel[/targ].prop"))
AssertEqual(Sdf.Path("/foo/bar/baz.rel[/targ].attr:argle:bargle").ReplaceName('prop:foo:fa:raw'),
            Sdf.Path("/foo/bar/baz.rel[/targ].prop:foo:fa:raw"))
AssertEqual(Sdf.Path("../../..").ReplaceName('foo'), Sdf.Path("../../../../foo"))
AssertEqual(Sdf.Path("..").ReplaceName('foo'), Sdf.Path("../../foo"))
AssertEqual(Sdf.Path("../../../.prop").ReplaceName('attr'),
            Sdf.Path("../../../.attr"))
AssertEqual(Sdf.Path("../../../.prop").ReplaceName('attr:argle:bargle'),
            Sdf.Path("../../../.attr:argle:bargle"))
AssertEqual(Sdf.Path("../../../.rel[/targ].attr").ReplaceName('prop'),
            Sdf.Path("../../../.rel[/targ].prop"))
AssertEqual(Sdf.Path("../../../.rel[/targ].attr").ReplaceName('prop:argle:bargle'),
            Sdf.Path("../../../.rel[/targ].prop:argle:bargle"))
AssertEqual(Sdf.Path("foo/bar/baz").ReplaceName('foo'), Sdf.Path("foo/bar/foo"))
AssertEqual(Sdf.Path("foo").ReplaceName('bar'), Sdf.Path("bar"))
AssertEqual(Sdf.Path("foo/bar/baz.prop").ReplaceName('attr'),
            Sdf.Path("foo/bar/baz.attr"))
AssertEqual(Sdf.Path("foo/bar/baz.prop").ReplaceName('attr:argle:bargle'),
            Sdf.Path("foo/bar/baz.attr:argle:bargle"))
AssertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr").ReplaceName('prop'), 
            Sdf.Path("foo/bar/baz.rel[/targ].prop"))
AssertEqual(Sdf.Path("foo/bar/baz.rel[/targ].attr").ReplaceName('prop:argle:bargle'), 
            Sdf.Path("foo/bar/baz.rel[/targ].prop:argle:bargle"))

AssertException("Sdf.Path('/foo/bar[target]').ReplaceName('xxx')",
                Tf.ErrorException);

# =======================================================================
# Test GetConciseRelativePaths
# =======================================================================
print "Test GetConciseRelativePaths"

aPath = Sdf.Path("/foo/bar")
bPath = Sdf.Path("/foo/baz")
cPath = Sdf.Path("/foo")

# a typical assortment of paths
AssertEqual( Sdf.Path.GetConciseRelativePaths([aPath,bPath,cPath]),
               [Sdf.Path("bar"), Sdf.Path("baz"), Sdf.Path("/foo")] )

# test some property paths
dPath = Sdf.Path("/foo/bar.a")
ePath = Sdf.Path("/foo/bar.b")

AssertEqual( Sdf.Path.GetConciseRelativePaths([dPath, ePath]),
                [Sdf.Path("bar.a"), Sdf.Path("bar.b")] )

fPath = Sdf.Path("/baz/bar")

AssertEqual( Sdf.Path.GetConciseRelativePaths([aPath, fPath]),
             [Sdf.Path("/foo/bar"), Sdf.Path("/baz/bar")] )

# now give it two identical paths
AssertEqual( Sdf.Path.GetConciseRelativePaths([aPath, aPath]),
             [Sdf.Path("bar"), Sdf.Path("bar")] )

# give it root paths
gPath = Sdf.Path("/bar")
AssertEqual( Sdf.Path.GetConciseRelativePaths([cPath, gPath]),
             [Sdf.Path("/foo"), Sdf.Path("/bar")] )

# now give it a relative path as an argument
ExpectedErrorBegin(1)
AssertEqual( Sdf.Path.GetConciseRelativePaths([Sdf.Path("a")]),
             [Sdf.Path("a")] )
ExpectedErrorEnd()


# =======================================================================
# Test RemoveDescendentPaths
# =======================================================================

print "Test RemoveDescendentPaths"

paths = [Sdf.Path(x) for x in
         ['/a/b/c', '/q', '/a/b/c/d/e/f/g', '/r/s/t', '/a/b', 
          '/q/r/s/t', '/x/y', '/a/b/d']]

expected = [Sdf.Path(x) for x in ['/a/b', '/q', '/x/y', '/r/s/t']]

result = Sdf.Path.RemoveDescendentPaths(paths)

# ensure result is unique, then compare independent of order.
AssertEqual(len(result), len(set(result)))
AssertEqual(set(result), set(expected))


# =======================================================================
# Test RemoveAncestorPaths
# =======================================================================

print "Test RemoveAncestorPaths"

paths = [Sdf.Path(x) for x in
         ['/a/b/c', '/q', '/a/b/c/d/e/f/g', '/r/s/t', '/a/b', 
          '/q/r/s/t', '/x/y', '/a/b/d']]

expected = [Sdf.Path(x) for x in 
            ['/a/b/c/d/e/f/g', '/a/b/d', '/q/r/s/t', '/r/s/t', '/x/y']]

result = Sdf.Path.RemoveAncestorPaths(paths)

# ensure result is unique, then compare independent of order.
AssertEqual(len(result), len(set(result)))
AssertEqual(set(result), set(expected))

# ========================================================================
# Test FindPrefixedRange and FindLongestPrefix
# ========================================================================

def testFindPrefixedRangeAndFindLongestPrefix():
    print "Test FindPrefixedRange and FindLongestPrefix"

    import random, time
    rgen = random.Random()
    seed = int(time.time())
    rgen.seed(seed)
    print 'random seed', seed

    letters = [chr(x) for x in range(ord('a'), ord('d')+1)]
    maxLen = 8

    paths = []
    for i in range(300):
        elems = [rgen.choice(letters) for i in range(rgen.randint(1, maxLen))]
        paths.append(Sdf.Path('/' + '/'.join(elems)))
    paths.append(Sdf.Path('/'))

    tests = []
    for i in range(300):
        elems = [rgen.choice(letters) for i in range(rgen.randint(1, maxLen))]
        tests.append(Sdf.Path('/' + '/'.join(elems)))
    tests.append(Sdf.Path('/'))

    paths.sort()
    tests.sort()

    # print '== paths', '='*64
    # print '\n'.join(map(str, paths))

    # print '== tests', '='*64
    # print '\n'.join(map(str, tests))

    def testFindPrefixedRange(p, paths):
        sl = Sdf.Path.FindPrefixedRange(paths, p)
        #print p, '>>', ', '.join([str(x) for x in paths[sl]])
        assert all([path.HasPrefix(p) for path in paths[sl]])
        others = list(paths)
        del others[sl]
        assert not any([path.HasPrefix(p) for path in others[sl]])

    def testFindLongestPrefix(p, paths):
        lp = Sdf.Path.FindLongestPrefix(paths, p)
        # should always have some prefix since '/' is in paths.
        assert p.HasPrefix(lp)
        # manually find longest prefix
        bruteLongest = Sdf.Path('/')
        for x in paths:
            if (p.HasPrefix(x) and 
                x.pathElementCount > bruteLongest.pathElementCount):
                bruteLongest = x
        # bruteLongest should match.
        #print 'path:', p, 'lp:', lp, 'bruteLongest:', bruteLongest
        assert lp == bruteLongest, ('lp (%s) != bruteLongest (%s)' % 
                                    (lp, bruteLongest))

    for testp in tests:
        testFindPrefixedRange(testp, paths)
        testFindLongestPrefix(testp, paths)

    # Do a few simple cases directly.
    paths = map(Sdf.Path, ['/a', '/a/b/c/d', '/b/a', '/b/c/d/e'])
    flp = Sdf.Path.FindLongestPrefix
    assert flp(paths, '/x') == None
    assert flp(paths, '/a') == Sdf.Path('/a')
    assert flp(paths, '/a/a/a') == Sdf.Path('/a')
    assert flp(paths, '/a/c/d/e/f') == Sdf.Path('/a')
    assert flp(paths, '/a/b/c/d') == Sdf.Path('/a/b/c/d')
    assert flp(paths, '/a/b/c/d/a/b/c/d') == Sdf.Path('/a/b/c/d')
    assert flp(paths, '/a/b/c/e/f/g') == Sdf.Path('/a')
    assert flp(paths, '/b') == None
    assert flp(paths, '/b/a/b/c/d/e') == Sdf.Path('/b/a')
    assert flp(paths, '/b/c/d/e/f/g') == Sdf.Path('/b/c/d/e')
    assert flp(paths, '/b/c/d/e') == Sdf.Path('/b/c/d/e')
    assert flp(paths, '/b/c/x/y/z') == None

testFindPrefixedRangeAndFindLongestPrefix()

Sdf._DumpPathStats()

print '\tPassed'

print 'Test SUCCEEDED'
