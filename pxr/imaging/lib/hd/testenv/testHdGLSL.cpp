#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/base/tf/errorMark.h"

#include <boost/python.hpp>

#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

template <typename T>
class SSBO {
public:
    SSBO(size_t numElements, int binding) : _buffer(0), _numElements(numElements) {
        glGenBuffers(1, &_buffer);
        std::vector<T> data(numElements);
        for (size_t i = 0; i < numElements; ++i) {
            data[i] = T(i);  // fill as [0.0, 1.0, 2.0, ... ]
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, numElements*sizeof(T),
                     &data[0], GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, _buffer);
    }
    ~SSBO() {
        if (_buffer) glDeleteBuffers(1, &_buffer);
    }

    // retrieve result
    std::vector<T> Get() const {
        std::vector<T> data(_numElements);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffer);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                           _numElements*sizeof(float), &data[0]);
        return data;
    }

    void Dump() const {
        std::vector<T> data = Get();
        for (size_t i = 0; i < data.size(); ++i) {
            std::cout << data[i] << ", ";
        }
        std::cout << "\n";
    }

private:
    GLuint _buffer;
    size_t _numElements;
};

class GLSLUnitTest {
public:
    GLSLUnitTest(const char *glslfxfile, const char *name) :
        _program(0),
        _resultBuffer(1, 0),
        _floatBuffer(1*64, 1),  // 64 floats
        _vec4Buffer(4*16, 2),   // 16 vectors
        _mat4Buffer(16*4, 3) {  // 4 matrices

        GlfGLSLFX glslfx(glslfxfile);

        _program = glCreateProgram();

        GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

        std::string source = glslfx.GetSource(TfToken(name));
        const char *shaderSource[3] = { "#version 430\n",
                                        "#define MAT4 mat4\n",
                                        source.c_str() };
        glShaderSource(shader, sizeof(shaderSource)/sizeof(const char*), shaderSource, NULL);
        glCompileShader(shader);

        glAttachShader(_program, shader);
        glDeleteShader(shader);

        glProgramParameteri(_program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
        glLinkProgram(_program);

        GLint status;
        glGetProgramiv(_program, GL_LINK_STATUS, &status);
        if (!status) {
            std::cout << "Link Error:\n";
            GLint infoLength = 0;
            glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &infoLength);
            if (infoLength > 0) {
                char *infoLog = new char[infoLength];;
                glGetProgramInfoLog(_program, infoLength, NULL, infoLog);
                printf("%s\n", infoLog);
                delete[] infoLog;

                glDeleteProgram(_program);
                _program = 0;
            }
        }
    }

    ~GLSLUnitTest() {
        if (_program) glDeleteProgram(_program);
    }

    bool Run() {
        glUseProgram(_program);

        glDispatchCompute(1, 1, 1);

        if (_resultBuffer.Get()[0]) return true;

        // fail
        std::cout << "GLSL test fails.\n";
        std::cout << "float buffer: ";
        _floatBuffer.Dump();
        std::cout << "vec4 buffer: ";
        _vec4Buffer.Dump();
        std::cout << "mat4 buffer: ";
        _mat4Buffer.Dump();


        return false;
    }

    void DumpBin() {
        GLint size;
        GLsizei len;
        GLenum format;
        glGetProgramiv(_program, GL_PROGRAM_BINARY_LENGTH, &size);
        std::vector<unsigned char> bin(size);
        glGetProgramBinary(_program, size, &len, &format, &bin[0]);
        FILE *fp = fopen("program.bin", "wb");
        fwrite(&bin[0], size, 1, fp);
        fclose(fp);
    }

private:
    GLuint        _program;
    SSBO<int>     _resultBuffer;
    SSBO<float>   _floatBuffer;
    SSBO<float>   _vec4Buffer;
    SSBO<float>   _mat4Buffer;
};

// copied from testSdGetObjectAtPath.cpp
static std::string
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

int main(int argc, char **argv)
{
    TfErrorMark mark;

    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <glslfx> <entry>\n";
        return 1;
    }

    std::string filepath = _FindDataFile(argv[1]);

    GLSLUnitTest test(filepath.c_str(), argv[2]);
    TF_VERIFY(test.Run());

    test.DumpBin();

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
