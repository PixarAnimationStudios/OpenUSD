#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
file(READ ${infile} _tmp_file_content)
file(WRITE ${outfile} "\#line 1 \"${infile}\"\n")
file(APPEND ${outfile} "${_tmp_file_content}")
