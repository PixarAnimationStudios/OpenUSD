file(TO_NATIVE_PATH  ${infile} INFILE)
file(TO_NATIVE_PATH  ${outfile} OUTFILE)

file(READ "${INFILE}" _tmp_file_content)
file(WRITE "${OUTFILE}" "\#line 1 ${infile}\n")
file(APPEND "${OUTFILE}" "${_tmp_file_content}")
