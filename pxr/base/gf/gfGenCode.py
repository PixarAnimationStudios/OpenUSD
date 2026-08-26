#!/usr/bin/env python
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

##############################################################################
# Code generation script for the various flavors of GfVec, GfMatrix, GfRange,
# GfQuat, GfDualQuat classes.
#
# Run this script manually to update the source code that's checked in.  Run
# with --validate to compare what would be generated with the existing code.  If
# it differs, this script will print a diff and error out.
#

import os, sys, itertools
from argparse import ArgumentParser

from jinja2 import Environment, FileSystemLoader
from jinja2.exceptions import TemplateError, TemplateSyntaxError

# Write filePath if its content differs from \p content.  If unchanged, print a
# message indicating that.  If filePath is not writable, print a diff.
def _WriteFile(filePath, content, verbose=True):
    import difflib
    # If file currently exists and content is unchanged, do nothing.
    existingContent = '\n'
    content = (content + '\n'
               if content and not content.endswith('\n') else content)
    if os.path.exists(filePath):
        with open(filePath, 'r') as fp:
            existingContent = fp.read()
        if existingContent == content:
            if verbose:
                print('\tunchanged %s' % filePath)
            return
    # Otherwise attempt to write to file.
    try:
        with open(filePath, 'w') as curfile:
            curfile.write(content)
            if verbose:
                print('\t    wrote %s' % filePath)
    except IOError as ioe:
        print('\t', ioe)
        print('Diff:')
        print('\n'.join(difflib.unified_diff(existingContent.split('\n'),
                                             content.split('\n'))))

def IsFloatingPoint(t):
    return t in ['double', 'float', 'GfHalf']

def RankScalar(s):
    # Return a numeric rank for a scalar type.  We allow implicit conversions to
    # scalars of greater rank.
    return dict([(t, n) for n, t in
                 enumerate(['int', 'GfHalf', 'float', 'double'])])[s]

def AllowImplicitConversion(src, dst):
    # Somewhat questionably, we always allow ints to implicitly convert to
    # floating point types.
    return RankScalar(src) <= RankScalar(dst)

def Component(i):
    # Return the name of the i'th vector component, or the empty string if
    # there is no name for it.  Only vectors (dimension <= 4) name their
    # components; this is unused by the other templates.
    return 'xyzw'[i] if i < 4 else ''

def MakeListFn(defaultN):
    def List(fmt, sep=', ', num=None):
        num = num if num is not None else defaultN
        return sep.join([fmt % {'i': i, 'c': Component(i)} for i in range(num)])
    return List

def MakeMatrixFn(defaultN):
    def Matrix(fmt, sep=', ', indent=0, diagFmt=None, num=None):
        num = num if num is not None else defaultN
        diagFmt = diagFmt if diagFmt is not None else fmt

        def GetFmt(row, col):
            return diagFmt if row == col else fmt

        strs = [(sep.join([GetFmt(i,j) % {'i':i,'j':j} for j in range(num)])) \
                    for i in range(num)]
        indentStr = (' ' * indent)
        lineSep = sep + ('\n' + indentStr if '\n' not in sep else '')
        result = lineSep.join(strs)
        return result
    return Matrix

def GenerateFromTemplates(env, templates, suffix, outputPath, verbose=True):
    for tmpl in templates:
        tmplName = tmpl % '.template'

        try:
            _WriteFile(os.path.join(outputPath, tmpl % suffix),
                env.get_template(tmplName).render(), verbose)
        except TemplateSyntaxError as err:
            print('Syntax Error: {0.name}:{0.lineno}: {0.message}'.format(err),
                  file=sys.stderr)
        except TemplateError as err:
            print('Template Error: {}: {}'.format(err, tmplName),
                  file=sys.stderr)

def ScalarSuffix(scl):
    if scl == 'GfHalf':
        return 'h'
    else:
        return scl[0]

def VecName(dim, scl):
    return 'GfVec%s%s' % (dim, ScalarSuffix(scl))

def Eps(scl):
    return '0.001' if scl == 'GfHalf' else 'GF_MIN_VECTOR_LENGTH'

def MakeSwizzles(dim):
    # Return the set of multi-component swizzles for a vector of dimension
    # 'dim': every permutation, without repeats, of length 2 through dim.  That
    # is 2, 12 and 60 names for dimensions 2, 3 and 4 respectively.
    #
    # Single-component swizzles are not included -- those are the vector's
    # named components ('x', 'y', ...), which are plain data members.
    #
    # Repeated components ('xx', 'yxy') are deliberately excluded.  Including
    # them would mean 336 names for a 4-dimensional vector rather than 60,
    # which measurably slows compilation and inflates debug information for
    # every translation unit that uses a vector.
    #
    # Each entry is a dict describing one swizzle:
    #   NAME    the swizzle, and the accessor's name, e.g. 'xzy'
    #   ARGS    the components in order, ready for a constructor call, 'x, z, y'
    #   RETDIM  the dimension of the result, e.g. 3
    #   IDXS    the component indexes, e.g. [0, 2, 1]
    ret = []
    for swizLen in range(2, dim+1):
        for p in itertools.permutations(range(dim), swizLen):
            ret.append(dict(NAME=''.join([Component(n) for n in p]),
                            ARGS=', '.join([Component(n) for n in p]),
                            RETDIM=len(p),
                            IDXS=list(p)))
    return ret

########################################################################
# GfVec
def GetVecSpecs():
    scalarTypes = ['double', 'float', 'GfHalf', 'int']
    dimensions = [2, 3, 4]
    vecSpecs = sorted(
        [dict(SCL=scl,
              DIM=dim,
              SUFFIX=str(dim) + ScalarSuffix(scl),
              VEC=VecName(dim, scl),
              EPS=Eps(scl),
              LIST=MakeListFn(dim),
              VECNAME=VecName,
              SCALAR_SUFFIX=ScalarSuffix,
              SCALARS=scalarTypes,
              SWIZZLES=MakeSwizzles(dim))
         for scl, dim in itertools.product(scalarTypes, dimensions)],
        key=lambda d: RankScalar(d['SCL']))

    return dict(templates=['vec%s.h', 'vec%s.cpp', 'wrapVec%s.cpp'],
                specs=vecSpecs)

########################################################################
# GfRange
def GetRangeSpecs():
    def RngName(dim, scl):
        return 'GfRange%s%s' % (dim, ScalarSuffix(scl))

    def MinMaxType(dim, scl):
        return scl if dim == 1 else VecName(dim, scl)

    def MinMaxParm(dim, scl):
        t = MinMaxType(dim, scl)
        return t + ' ' if dim == 1 else 'const %s &' % t

    scalarTypes = ['double', 'float']
    dimensions = [1, 2, 3]
    rngSpecs = sorted(
        [dict(SCL=scl,
              MINMAX=MinMaxType(dim, scl),
              MINMAXPARM=MinMaxParm(dim, scl),
              DIM=dim,
              SUFFIX=str(dim) + ScalarSuffix(scl),
              RNG=RngName(dim, scl),
              RNGNAME=RngName,
              SCALARS=scalarTypes,
              LIST=MakeListFn(dim))
         for scl, dim in itertools.product(scalarTypes, dimensions)],
        key=lambda d: RankScalar(d['SCL']))

    return dict(templates=['range%s.h', 'range%s.cpp', 'wrapRange%s.cpp'],
                specs=rngSpecs)
    
########################################################################
# GfQuat
def GetQuatSpecs():
    def QuatName(scl):
        return 'GfQuat%s' % ScalarSuffix(scl)

    scalarTypes = ['double', 'float', 'GfHalf']
    quatSpecs = sorted(
        [dict(SCL=scl,
              SUFFIX=ScalarSuffix(scl),
              QUAT=QuatName(scl),
              QUATNAME=QuatName,
              SCALAR_SUFFIX=ScalarSuffix,
              SCALARS=scalarTypes,
              LIST=MakeListFn(4))
         for scl in scalarTypes],
        key=lambda d: RankScalar(d['SCL']))

    return dict(templates=['quat%s.h', 'quat%s.cpp', 'wrapQuat%s.cpp'],
                specs=quatSpecs)

########################################################################
# GfDualQuat
def GetDualQuatSpecs():
    def QuatName(scl):
        return 'GfQuat%s' % ScalarSuffix(scl)
    def DualQuatName(scl):
        return 'GfDualQuat%s' % ScalarSuffix(scl)

    scalarTypes = ['double', 'float', 'GfHalf']
    dualQuatSpecs = sorted(
        [dict(SCL=scl,
              SUFFIX=ScalarSuffix(scl),
              QUAT=QuatName(scl),
              QUATNAME=QuatName,
              DUALQUAT=DualQuatName(scl),
              DUALQUATNAME=DualQuatName,
              SCALAR_SUFFIX=ScalarSuffix,
              SCALARS=scalarTypes,
              LIST=MakeListFn(4))
         for scl in scalarTypes],
        key=lambda d: RankScalar(d['SCL']))

    return dict(templates=['dualQuat%s.h', 'dualQuat%s.cpp', 'wrapDualQuat%s.cpp'],
                specs=dualQuatSpecs)

########################################################################
# GfMatrix
def GetMatrixSpecs(dim):
    def MatrixName(dim, scl):
        return 'GfMatrix%s%s' % (dim, ScalarSuffix(scl))

    scalarTypes = ['double', 'float']
    dimensions = [dim]

    matrixSpecs = sorted(
        [dict(SCL=scl,
              DIM=i,
              FILESUFFIX=ScalarSuffix(scl),
              SUFFIX=str(i) + ScalarSuffix(scl),
              MAT=MatrixName(i, scl),
              LIST=MakeListFn(i),
              MATRIX=MakeMatrixFn(i),
              MATNAME=MatrixName,
              SCALARS=scalarTypes)
         for scl, i in itertools.product(scalarTypes, dimensions)],
        key=lambda d: RankScalar(d['SCL']))

    return dict(templates=['matrix%s%%s.h' % dim,
                           'matrix%s%%s.cpp' % dim,
                           'wrapMatrix%s%%s.cpp' % dim ], specs=matrixSpecs)

def GetMatrix2Specs():
    return GetMatrixSpecs(dim = 2)

def GetMatrix3Specs():
    return GetMatrixSpecs(dim = 3)

def GetMatrix4Specs():
    return GetMatrixSpecs(dim = 4)

# Check that each file in dstDir matches the corresponding file in srcDir.
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

if __name__ == '__main__':
    ap = ArgumentParser(
        description='Generate source code for GfVec, GfRange, GfQuat, GfDualQuat.')
    ap.add_argument('--validate', action='store_true')
    ap.add_argument('--dstDir', default=os.curdir)
    ap.add_argument('--srcDir', default=os.curdir)
    args = ap.parse_args()

    stdEnv = dict(UPPER=str.upper, LOWER=str.lower,
                  ALLOW_IMPLICIT_CONVERSION=AllowImplicitConversion,
                  IS_FLOATING_POINT=IsFloatingPoint)

    if args.validate:
        # Make a temporary directory for results.
        import tempfile
        args.dstDir = tempfile.mkdtemp()

    try:
        for s in [GetVecSpecs(), GetRangeSpecs(),
                  GetQuatSpecs(), GetDualQuatSpecs(),
                  GetMatrix2Specs(), GetMatrix3Specs(), GetMatrix4Specs()]:
            env = Environment(loader=FileSystemLoader(args.srcDir),
                              trim_blocks=True)
            env.globals.update(**stdEnv)
            templates, specs = s['templates'], s['specs']
            for spec in specs:
                env.globals.update(**spec)
                GenerateFromTemplates(
                    env, templates, spec.get('FILESUFFIX', spec['SUFFIX']),
                    args.dstDir, verbose=not args.validate)

        if args.validate:
            ValidateFiles(args.srcDir, args.dstDir)

    finally:
        # Remove the temporary directory.
        if args.validate:
            import shutil
            shutil.rmtree(args.dstDir)

