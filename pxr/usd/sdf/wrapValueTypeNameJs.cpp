#include "pxr/usd/sdf/valueTypeName.h"
#include <emscripten/bind.h>
using namespace emscripten;


EMSCRIPTEN_BINDINGS(ValueTypeName) {
    class_<pxr::SdfValueTypeName>("ValueTypeName")
    ;
}
