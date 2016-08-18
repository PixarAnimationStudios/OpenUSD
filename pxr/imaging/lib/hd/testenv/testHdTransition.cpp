#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/unitTestGLDrawing.h"
#include "pxr/imaging/hd/unitTestHelper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <boost/shared_ptr.hpp>

static float g_time = 0.0f;

class My_TestGLDrawing : public Hd_UnitTestGLDrawing {
public:
    My_TestGLDrawing() : _refineLevel(0), _nextCommand(0) {
        SetCameraRotate(60.0f, 0.0f);
        SetCameraTranslate(GfVec3f(0, 0, -20.0f-1.7320508f*2.0f));
    }

    // Hd_UnitTestGLDrawing overrides
    virtual void InitTest();
    virtual void DrawTest();
    virtual void OffscreenTest();

    virtual void KeyRelease(int key);

protected:
    virtual void ParseArgs(int argc, char *argv[]);

    class Cmd {
    public:
        /// returns true when need to pause/draw
        virtual bool Run(Hd_TestDriver *driver) = 0;
    };

    class DrawCmd : public Cmd {
    public:
        virtual bool Run(Hd_TestDriver *driver) {
            return true;
        }
    };

    class ChangeReprCmd : public Cmd {
    public:
        ChangeReprCmd(const std::string &reprName) : _repr(reprName) {}
        virtual bool Run(Hd_TestDriver *driver) {
            std::cerr << "Set repr to " << _repr.GetText() << "\n";
            driver->SetRepr(_repr);
            return false;
        }
    private:
        TfToken _repr;
    };

    class ChangeRefineLevelCmd : public Cmd {
    public:
        ChangeRefineLevelCmd(int level) : _level(level) {}
        virtual bool Run(Hd_TestDriver *driver) {
            std::cerr << "Set refine level to " << _level << "\n";
            driver->GetDelegate().SetRefineLevel(_level);
            return false;
        }
    private:
        int _level;
    };

    class AddClipPlaneCmd : public Cmd {
    public:
        AddClipPlaneCmd(GfVec4d const &clipPlane) : _clipPlane(clipPlane) {}
        virtual bool Run(Hd_TestDriver *driver) {
            std::cerr << "Add clip plane " << _clipPlane << "\n";
            HdRenderPassState::ClipPlanesVector clipPlanes =
                driver->GetRenderPassState()->GetClipPlanes();
            clipPlanes.push_back(_clipPlane);
            driver->GetRenderPassState()->SetClipPlanes(clipPlanes);
            return false;
        }
    private:
        GfVec4d _clipPlane;
    };

    class AnimateCmd : public Cmd {
    public:
        AnimateCmd(const std::string &name) : _id(name) {}
        virtual bool Run(Hd_TestDriver *driver) {
            std::cerr << "Animate " << _id << "\n";
            driver->GetDelegate().UpdatePositions(_id, g_time);
            return false;
        }
    private:
        SdfPath _id;
    };

private:
    bool next();

    Hd_TestDriver* _driver;

    TfToken _reprName;
    int _refineLevel;
    std::string _outputFilePrefix;
    std::vector<boost::shared_ptr<Cmd> > _commands;
    int _nextCommand;
};

////////////////////////////////////////////////////////////

GLuint vao;

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";

    _driver = new Hd_TestDriver(_reprName);
    Hd_UnitTestDelegate &delegate = _driver->GetDelegate();
    delegate.SetRefineLevel(_refineLevel);

    GfVec3f center(0);
    center = delegate.PopulateBasicTestSet();

    // center camera
    SetCameraTranslate(GetCameraTranslate() - center);

    // XXX: Setup a VAO, the current drawing engine will not yet do this.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);
}

void
My_TestGLDrawing::DrawTest()
{
    GLfloat clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    GLfloat clearDepth[1] = { 1.0f };
    glClearBufferfv(GL_DEPTH, 0, clearDepth);

    int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    // camera
    _driver->SetCamera(viewMatrix, projMatrix, GfVec4d(0, 0, width, height));

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(vao);

    _driver->Draw();

    glBindVertexArray(0);
}

void
My_TestGLDrawing::OffscreenTest()
{
    if (not TF_VERIFY(not _outputFilePrefix.empty())) return;

    DrawTest();

    int count = 0;
    while(next()) {
        DrawTest();

        std::stringstream filename;
        filename << _outputFilePrefix
                 << "_"
                 << count++
                 << ".png";
        WriteToFile("color", filename.str());
    }
}

void
My_TestGLDrawing::KeyRelease(int key)
{
    next();
}

bool
My_TestGLDrawing::next()
{
    g_time += 1.0f;

    while (_nextCommand < _commands.size()) {
        if (_commands[_nextCommand++]->Run(_driver)) return true;
    }
    return false;
}

/* virtual */
void
My_TestGLDrawing::ParseArgs(int argc, char *argv[])
{
    for (int i=0; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--outputFilePrefix") {
            _outputFilePrefix = argv[++i];
        } else if (arg == "--repr") {
            _reprName = TfToken(argv[++i]);
        } else if (arg == "--refineLevel") {
            _refineLevel = atoi(argv[++i]);
        } else if (arg == "--cmd" or arg == "-c") {
            //
            // csv commands
            //    draw               force draw (or save the image)
            //    repr=<repr>        switch to <repr>
            //    refineLevel=<int>  set global refinement level
            //    animate=<id>       move the positions of <id> mesh
            //    clipPlane=x:y:z:w  add clip plane
            //
            //  example:
            //   --cmd draw,repr=smoothHull,refineLevel=2,repr=refined,animate=/cube0
            //
            std::string cmds(argv[++i]);
            std::stringstream cmdStream(cmds);
            std::string field;
            while(std::getline(cmdStream, field, ',')) {
                size_t n = field.find('=');
                std::string cmd = field.substr(0, n);
                std::string arg = n == (std::string::npos) ? "" : field.substr(n+1);
                if (cmd == "draw") {
                    _commands.push_back(
                        boost::shared_ptr<DrawCmd>(
                            new DrawCmd()));
                } else if (cmd == "repr") {
                    _commands.push_back(
                        boost::shared_ptr<ChangeReprCmd>(
                            new ChangeReprCmd(TfToken(arg))));
                } else if (cmd == "refineLevel") {
                    _commands.push_back(
                        boost::shared_ptr<ChangeRefineLevelCmd>(
                            new ChangeRefineLevelCmd(atoi(arg.c_str()))));
                } else if (cmd == "clipPlane") {
                    float cp[4];
                    sscanf(arg.c_str(), "%f:%f:%f:%f",
                           &cp[0], &cp[1], &cp[2], &cp[3]);
                    _commands.push_back(
                        boost::shared_ptr<AddClipPlaneCmd>(
                            new AddClipPlaneCmd(GfVec4d(GfVec4f(cp)))));
                } else if (cmd == "animate") {
                    _commands.push_back(
                        boost::shared_ptr<AnimateCmd>(
                            new AnimateCmd(arg)));
                } else {
                    std::cerr << "Unknown command : " << cmd << "\n";
                    return;
                }
            }
        }
    }
}

void
BasicTest(int argc, char *argv[])
{
    My_TestGLDrawing driver;
    driver.RunTest(argc, argv);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    BasicTest(argc, argv);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

