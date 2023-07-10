
set(BINDINGS_NAME "jsBindings")

function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

message(STATUS "SOURCE_DIR: ${SOURCE_DIR}")

set(ES6_MODULE_FILES
  ${CMAKE_BINARY_DIR}/extras/usd/js_bindings/${BINDINGS_NAME}.js
  ${SOURCE_DIR}/js/usd-module-appendix.js
)
file(WRITE ${CMAKE_BINARY_DIR}/extras/usd/js_bindings/usd.js "")

foreach(ES6_MODULE_FILE ${ES6_MODULE_FILES})
  cat(${ES6_MODULE_FILE} ${CMAKE_BINARY_DIR}/extras/usd/js_bindings/usd.js)
endforeach()
