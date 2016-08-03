#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>

#include <boost/python.hpp>

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
TestGlfBindingMap()
{
    GlfGLContext::MakeCurrent(GlfGLContext::GetSharedGLContext());

    bool ret = true;
    std::cout <<  "*** Running test: GlfBindingMap\n\n";

    std::string file = _FindDataFile("testGlfBindingMap.testenv/test.glslfx");

    GlfGLSLFX src(file);

    if (not src.IsValid()) {
        std::cout << "invalid source\n";
        return false;
    }

    GLuint program = glCreateProgram();

    std::string header = "#version 410\n";

    std::string vsSource = src.GetVertexSource();
    if (vsSource.empty()) {
        std::cout << "no vertex shader specified\n";
        return false;
    }

    vsSource = header + vsSource;
    const char * vsBuffer = vsSource.c_str();

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vsBuffer, 0);

    GLint vCompileStatus = 0;
    glCompileShader(vShader);
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &vCompileStatus);
    if (vCompileStatus != GL_TRUE) {
        std::cout << "vertex shader compile failed\n";
        return false;
    }
    glAttachShader(program, vShader);

    std::string fsSource = src.GetFragmentSource();
    if (fsSource.empty()) {
        std::cout << "no fragment shader specified\n";
        return false;
    }

    fsSource = header + fsSource;
    const char * fsBuffer = fsSource.c_str();

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fsBuffer, 0);

    GLint fCompileStatus = 0;
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &fCompileStatus);
    if (fCompileStatus != GL_TRUE) {
        std::cout << "fragment shader compile failed\n";
        return false;
    }
    glAttachShader(program, fShader);

    glLinkProgram(program);

    GlfBindingMap bindingMap;
    bindingMap.AddCustomBindings(program);

    bindingMap.Debug();

    return ret;
}
int
main(int argc, char **argv)
{
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();

    bool passed = TestGlfBindingMap();

    if (passed) {
        std::cout << "Passed\n";
        exit(0);
    } else {
        std::cout << "Failed\n";
        exit(1);
    }
 
}
