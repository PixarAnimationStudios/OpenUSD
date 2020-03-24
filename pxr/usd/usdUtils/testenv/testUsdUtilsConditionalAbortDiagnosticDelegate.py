#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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
from __future__ import print_function
from pxr import Tf, UsdUtils

import argparse

def delegateSetup(stringIncFilders, codePathIncFilters, stringExcFilters,
        codePathExcFilters):
    incFilter = UsdUtils.ConditionalAbortDiagnosticDelegateErrorFilters()
    excFilter = UsdUtils.ConditionalAbortDiagnosticDelegateErrorFilters()
    if stringIncFilders:
        incFilter.SetStringFilters(stringIncFilders)
    if codePathIncFilters:
        incFilter.SetCodePathFilters(codePathIncFilters)
    if stringExcFilters:
        excFilter.SetStringFilters(stringExcFilters)
    if codePathExcFilters:
        excFilter.SetCodePathFilters(codePathExcFilters)
    return UsdUtils.ConditionalAbortDiagnosticDelegate(incFilter, excFilter)


def testCase1():
    print("Case1: Abort all string errors")
    delegate = delegateSetup(["*"], None, [""], None)
    try:
        Tf.RaiseCodingError("")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase2():
    print("Case2: Ignore all string errors")
    delegate = delegateSetup(["*"], None, ["*"], None)
    try:
        Tf.RaiseCodingError("")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase3():
    print("Case3: Empty lists of error filters")
    delegate = delegateSetup(None, None, None, None)
    try:
        Tf.RaiseCodingError("")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase4():
    print("Case4: Empty lists of inclusion filters for string errors")
    delegate = delegateSetup(None, None, ["blah"], None)
    try:
        Tf.RaiseCodingError("")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase5():
    print("Case5: Empty lists of exclude filters for string errors, include "
          "list does not contain any error")
    delegate = delegateSetup(["blah"], None, None, None)
    try:
        Tf.RaiseCodingError("error string not included")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase5a():
    print("Case5a: Empty lists of exclude filters for string errors, include "
          "list does contain error")
    delegate = delegateSetup(["*included*"], None, None, None)
    try:
        Tf.RaiseCodingError("error string included")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase6():
    print("Case6: Exclude some but abort on at least one string error")
    delegate = delegateSetup(["*abort*"], None, ["*not*"], None);
    try:
        Tf.RaiseCodingError("does not abort")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("not abort")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("aborting on this!!")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase6a():
    print("Case6a: All string errors consumed by exclude list without using"
          "glob")
    delegate = delegateSetup(["*"], None, ["*not*"], None);
    try:
        Tf.RaiseCodingError("does not abort")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("lets not abort")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("abort not please")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase7():
    print("Case7: Abort all codePath errors")
    delegate = delegateSetup(None, ["*"], None, [""])
    try:
        Tf.RaiseCodingError("")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase8():
    print("Case8: Ignore all codePath errors")
    delegate = delegateSetup(None, ["*"], None, ["*"])
    try:
        Tf.RaiseCodingError("")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase9():
    print("Case9: Empty lists of inclusion filters for codePath errors")
    delegate = delegateSetup(None, None, ["blah"], None)
    try:
        Tf.RaiseCodingError("")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase10():
    print("Case10: Empty lists of exclude filters for codePath errors, "
          "include list does not contain any error")
    delegate = delegateSetup(None, ["blah"], None, None)
    try:
        Tf.RaiseCodingError("codePath not included")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase10a():
    print("Case10a: Empty lists of exclude filters for codePath errors, "
          "include list does contain error")
    delegate = delegateSetup(None, ["*test*"], None, None)
    try:
        Tf.RaiseCodingError("codePath included")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase11():
    print("Case11: Exclude some but abort on at least one codePath error")
    # Note this is done using string exclude filters
    delegate = delegateSetup(None, ["*test*"], ["*not*"], None)
    try:
        Tf.RaiseCodingError("does not abort")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("not abort")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("aborting on this!!")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase11a():
    print("Case11a: All string errors consumed by exclude list without using "
          "glob")
    delegate = delegateSetup(None, ["ConditionalAbort"], None, ["*test*"])
    try:
        Tf.RaiseCodingError("does not abort")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("lets not abort")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("abort not please")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase12():
    print("Case12: include string error but exclude from path - does not abort")
    delegate = delegateSetup(["*"], None, None, ["*test*"])
    try:
        Tf.RaiseCodingError("does not abort - nothing in pxr is aborted")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("lets not abort - nothing in pxr is aborted")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("abort not please - nothing in pxr is aborted")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase12a():
    print("Case12a: include string error but exclude from path - aborts")
    delegate = delegateSetup(["please"], None, None, ["*usdImaging*"])
    try:
        Tf.RaiseCodingError("does not abort - not in usdImaging")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("lets not abort - not in usdImaging")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("abort please - not in usdImaging")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase13():
    print("Case13: include codePath error but exclude from string - "
          "does not abort")
    delegate = delegateSetup(None, ["*pxr*"], ["*not*"], None)
    try:
        Tf.RaiseCodingError("does not abort - word not excluded")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("lets not abort - word not excluded")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("abort not please - word not excluded")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase13a():
    print("Case13a: include codePath error but exclude from string - aborts")
    delegate = delegateSetup(None, ["*test*"], ["*not*"], None)
    try:
        Tf.RaiseCodingError("does not abort - word not excluded")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("lets not abort - word not excluded")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)
    try:
        Tf.RaiseCodingError("abort please - word excluded")
    except Tf.ErrorException as e:
        Tf.RepostErrors(e)

def testCase14():
    print("Case14: test TfWarning filtering - aborts")
    delegate = delegateSetup(None, ["*test*"], ["*not*"], None)
    Tf.Warn("does not abort - word not excluded")
    Tf.Warn("lets not abort - word not excluded")
    Tf.Warn("abort please - word excluded")

def testCase14a():
    print("Case14a: test TfWarning filtering - no aborts")
    delegate = delegateSetup(None, ["*test*"], ["*not*"], None)
    Tf.Warn("does not abort - word not excluded")
    Tf.Warn("lets not abort - word not excluded")
    Tf.Warn("abort not please - word not excluded")

def testCase15():
    print("Case15: test Fatal ALWAYS aborts")
    delegate = delegateSetup(None, ["*test*"], ["*not*"], None)
    Tf.Fatal("Aborts, even though word excluded")

def testCase16():
    print("Case16: test Status NEVER aborts")
    delegate = delegateSetup(None, ["*test*"], ["*not*"], None)
    Tf.Status("Does not abort, even though word not excluded")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()
    
    case1 = subparsers.add_parser('case1', help = 'abort all string error')
    case1.set_defaults(func=testCase1)
    case2 = subparsers.add_parser('case2', help = 'Ignore all string errors')
    case2.set_defaults(func=testCase2)
    case3 = subparsers.add_parser('case3', help = 'empty lists of strings')
    case3.set_defaults(func=testCase3)
    case4 = subparsers.add_parser('case4', help = 'empty include list of ' \
                                        'strings')
    case4.set_defaults(func=testCase4)
    case5 = subparsers.add_parser('case5', help = 'empty exclude list of ' \
                                        'strings - results in abort')
    case5.set_defaults(func=testCase5)
    case5a = subparsers.add_parser('case5a', help = 'empty exclude list of ' \
                                        'strings - does not result in abort')
    case5a.set_defaults(func=testCase5a)
    case6 = subparsers.add_parser('case6', help = 'Abort on at least one, ' \
                                        'exclude rest')
    case6.set_defaults(func=testCase6)
    case6a = subparsers.add_parser('case6a', help = 'No Glob, all string ' \
                                        'errors consumed in exclude list ')
    case6a.set_defaults(func=testCase6a)

    case7 = subparsers.add_parser('case7', help = 'abort all codepath error')
    case7.set_defaults(func=testCase7)
    case8 = subparsers.add_parser('case8', help = 'Ignore all codepath errors')
    case8.set_defaults(func=testCase8)
    case9 = subparsers.add_parser('case9', help = 'empty include list of ' \
                                        'codepaths')
    case9.set_defaults(func=testCase9)
    case10 = subparsers.add_parser('case10', help = 'empty exclude list of ' \
                                        'codepaths - results in abort')
    case10.set_defaults(func=testCase10)
    case10a = subparsers.add_parser('case10a', help = 'empty exclude list of ' \
                                        'codepaths - does not result in abort')
    case10a.set_defaults(func=testCase10a)
    case11 = subparsers.add_parser('case11', help = 'abort some codepaths ' \
                                        'errors')
    case11.set_defaults(func=testCase11)
    case11a = subparsers.add_parser('case11a', help = 'No Glob, all codepath ' \
                                        'errors consumed in exclude list')
    case11a.set_defaults(func=testCase11a)

    case12 = subparsers.add_parser('case12', help = 'include on string but ' \
                                        'excludes from codepath - no abort')
    case12.set_defaults(func=testCase12)
    case12a = subparsers.add_parser('case12a', help = 'include on string but ' \
                                        'excludes from codepath - aborts')
    case12a.set_defaults(func=testCase12a)
    case13 = subparsers.add_parser('case13', help = 'include on codepath but ' \
                                        'excludes from string - no abort')
    case13.set_defaults(func=testCase13)
    case13a = subparsers.add_parser('case13a', help = 'include on codepath ' \
                                        'but excludes from string - aborts')
    case13a.set_defaults(func=testCase13a)

    case14 = subparsers.add_parser('case14', help = 'test TfWarning filtering' \
                                        ' - aborts')
    case14.set_defaults(func=testCase14)
    case14a = subparsers.add_parser('case14a', help = 'test TfWarning ' \
                                        'filtering - no abort')
    case14a.set_defaults(func=testCase14a)

    case15 = subparsers.add_parser('case15', help = 'test Fatal ALWAYS aborts')
    case15.set_defaults(func=testCase15)

    case16 = subparsers.add_parser('case16', help = 'test status NEVER aborts')
    case16.set_defaults(func=testCase16)

    args = parser.parse_args()
    args.func()
    
