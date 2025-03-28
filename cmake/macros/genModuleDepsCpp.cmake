#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Parse library list
separate_arguments(libraryList NATIVE_COMMAND ${libraries})

# Format library list
foreach(library ${libraryList})
    list(APPEND reqLibs "\t\tTfToken(\"${library}\")")
endforeach()
list(JOIN reqLibs ",\n" reqLibs)

# Read in template file and generate moduleDeps.cpp
file(READ "${sourceDir}/cmake/macros/moduleDeps.cpp.in" fileTemplate)
string(CONFIGURE "${fileTemplate}" fileContents)
file(WRITE ${outfile} ${fileContents})