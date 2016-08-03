#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>

#include <boost/python.hpp>

using std::string;
using std::vector;
using std::string;

// copied from testSdGetObjectAtPath.cpp
static
std::string
_FindDataFile(const std::string& file)
{
    static std::once_flag importOnce;
    std::call_once(importOnce, [](){
        const std::string importFindDataFile = "from Mentor.Runtime import *";
        if (TfPyRunSimpleString(importFindDataFile) != 0) {
            TF_FATAL_ERROR("ERROR: Could not import FindDataFile");
        }
    });

    const std::string findDataFile =
        TfStringPrintf("FindDataFile(\'%s\')", file.c_str());

    using namespace boost::python;
    const object resultObj(TfPyRunString(findDataFile, Py_eval_input));
    const extract<std::string> dataFileObj(resultObj);

    if (not dataFileObj.check()) {
        TF_FATAL_ERROR("ERROR: Could not extract result of FindDataFile");
        return std::string();
    }

    return dataFileObj();
}

static bool
TestGlfGLSLFX()
{
    bool ret = true;
    std::cout <<  "*** Running test: GlfGLSLFX\n\n";

    vector<string> filePaths;
    filePaths.push_back(_FindDataFile("testGlfGLSLFX.testenv/modulate.glslfx"));
    filePaths.push_back(_FindDataFile("testGlfGLSLFX.testenv/replace.glslfx"));

    for (int i = 0; i < filePaths.size(); i++) {

        string file = filePaths[i];
        GlfGLSLFX src(file);

        if (not src.IsValid()) {
            std::cout << "invalid source\n";
            ret = false;
            break;
        }

        if (src.GetFragmentSource().empty()) {
            std::cout << "no fragment specified\n";
            ret = false;
            break;
        }
    }

    // stream input
    std::stringstream ss;
    ss << "-- glslfx version 0.1\n"
       << "#import $TOOLS/glos/shaders/simpleLightingShader.glslfx\n"
       << "-- configuration\n"
       << "{\n"
       << " \"techniques\": {\n"
       << "   \"default\": {\n"
       << "     \"vertexShader\": {\n"
       << "       \"source\": [ \"SimpleLighting.Lighting\", \"SimpleLighting.Vertex\" ]\n"
       << "     },\n"
       << "     \"fragmentShader\": {\n"
       << "       \"source\": [ \"SimpleLighting.Lighting\", \"SimpleLighting.Fragment\" ]\n"
       << "     }\n"
       << "   }\n"
       << " }\n"
       << "}\n";
    {
        GlfGLSLFX src(ss);
        TF_VERIFY(not src.GetVertexSource().empty());
        TF_VERIFY(not src.GetFragmentSource().empty());
    }

    return ret;
}
int
main(int argc, char **argv)
{
    TfErrorMark mark;
    bool passed = TestGlfGLSLFX();

    if (passed && mark.IsClean()) {
        std::cout << "Passed\n";
        exit(0);
    } else {
        std::cout << "Failed\n";
        exit(1);
    }
 
}
