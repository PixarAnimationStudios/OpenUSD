#include "pxr/imaging/glf/glew.h"

#include "pxr/usdImaging/usdImaging/defaultTaskDelegate.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/glf/testGLContext.h"

#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/simpleLightBypassTask.h"

#include <QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    // prepare GL context
    GlfTestGLContext::RegisterGLContextCallbacks();
    GlfGlewInit();
    GlfSharedGLContextScopeHolder sharedContext;

    HdRenderIndexSharedPtr renderIndex(new HdRenderIndex);

    UsdImaging_DefaultTaskDelegate delegate(renderIndex,
                                            SdfPath("delegate"));

    UsdImagingEngine::RenderParams params;

    HdTaskSharedPtrVector tasks = delegate.GetRenderTasks(params);

    // Only HdxRenderTask, HdxSelectionTask
    TF_VERIFY(tasks.size() == 2);
    TF_VERIFY(boost::dynamic_pointer_cast<HdxRenderTask>(tasks[0]));
    TF_VERIFY(boost::dynamic_pointer_cast<HdxSelectionTask>(tasks[1]));

    GlfSimpleLightingContextRefPtr lightingContext
        = GlfSimpleLightingContext::New();
    delegate.SetLightingState(lightingContext);

    // HdxSimpleLightTask, HdxRenderTask, HdxSelectionTask
    tasks = delegate.GetRenderTasks(params);
    TF_VERIFY(tasks.size() == 3);
    TF_VERIFY(boost::dynamic_pointer_cast<HdxSimpleLightTask>(tasks[0]));
    TF_VERIFY(boost::dynamic_pointer_cast<HdxRenderTask>(tasks[1]));
    TF_VERIFY(boost::dynamic_pointer_cast<HdxSelectionTask>(tasks[2]));

    delegate.SetBypassedLightingState(lightingContext);

    // HdxSimpleLightBypassTask, HdxRenderTask, HdxSelectionTask
    tasks = delegate.GetRenderTasks(params);
    TF_VERIFY(tasks.size() == 3);
    TF_VERIFY(boost::dynamic_pointer_cast<HdxSimpleLightBypassTask>(tasks[0]));
    TF_VERIFY(boost::dynamic_pointer_cast<HdxRenderTask>(tasks[1]));
    TF_VERIFY(boost::dynamic_pointer_cast<HdxSelectionTask>(tasks[2]));



    std::cout << "OK" << std::endl;
}
