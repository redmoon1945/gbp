# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/gbp_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/gbp_autogen.dir/ParseCache.txt"
  "gbp_autogen"
  )
endif()
