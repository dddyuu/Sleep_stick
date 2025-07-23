# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "Amplifier\\Amplifier_autogen"
  "Amplifier\\CMakeFiles\\Amplifier_autogen.dir\\AutogenUsed.txt"
  "Amplifier\\CMakeFiles\\Amplifier_autogen.dir\\ParseCache.txt"
  "Amplifier\\filter\\CMakeFiles\\filter_autogen.dir\\AutogenUsed.txt"
  "Amplifier\\filter\\CMakeFiles\\filter_autogen.dir\\ParseCache.txt"
  "Amplifier\\filter\\filter_autogen"
  "Amplifier\\filter\\test\\CMakeFiles\\filter_test_autogen.dir\\AutogenUsed.txt"
  "Amplifier\\filter\\test\\CMakeFiles\\filter_test_autogen.dir\\ParseCache.txt"
  "Amplifier\\filter\\test\\filter_test_autogen"
  "Amplifier\\test\\Amplifier_test_autogen"
  "Amplifier\\test\\CMakeFiles\\Amplifier_test_autogen.dir\\AutogenUsed.txt"
  "Amplifier\\test\\CMakeFiles\\Amplifier_test_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\DataOperation_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\DataOperation_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\DataOperation_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\DataOperation_test_autogen.dir\\ParseCache.txt"
  "DataOperation_autogen"
  "DataOperation_test_autogen"
  "FileStorage\\CMakeFiles\\FileStorage_autogen.dir\\AutogenUsed.txt"
  "FileStorage\\CMakeFiles\\FileStorage_autogen.dir\\ParseCache.txt"
  "FileStorage\\FileStorage_autogen"
  "FileStorage\\MAT\\CMakeFiles\\MAT_autogen.dir\\AutogenUsed.txt"
  "FileStorage\\MAT\\CMakeFiles\\MAT_autogen.dir\\ParseCache.txt"
  "FileStorage\\MAT\\MAT_autogen"
  "FileStorage\\MAT\\test\\CMakeFiles\\mat_test_autogen.dir\\AutogenUsed.txt"
  "FileStorage\\MAT\\test\\CMakeFiles\\mat_test_autogen.dir\\ParseCache.txt"
  "FileStorage\\MAT\\test\\mat_test_autogen"
  "FileStorage\\test\\CMakeFiles\\FileStorage_lib_autogen.dir\\AutogenUsed.txt"
  "FileStorage\\test\\CMakeFiles\\FileStorage_lib_autogen.dir\\ParseCache.txt"
  "FileStorage\\test\\FileStorage_lib_autogen"
  )
endif()
