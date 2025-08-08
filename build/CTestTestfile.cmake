# CMake generated Testfile for 
# Source directory: C:/Users/mvton/GBLator_Project_Folder/GBLator
# Build directory: C:/Users/mvton/GBLator_Project_Folder/GBLator/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(gblator_unit_tests "C:/Users/mvton/GBLator_Project_Folder/GBLator/build/Debug/gblator_tests.exe")
  set_tests_properties(gblator_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/mvton/GBLator_Project_Folder/GBLator/CMakeLists.txt;25;add_test;C:/Users/mvton/GBLator_Project_Folder/GBLator/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(gblator_unit_tests "C:/Users/mvton/GBLator_Project_Folder/GBLator/build/Release/gblator_tests.exe")
  set_tests_properties(gblator_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/mvton/GBLator_Project_Folder/GBLator/CMakeLists.txt;25;add_test;C:/Users/mvton/GBLator_Project_Folder/GBLator/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(gblator_unit_tests "C:/Users/mvton/GBLator_Project_Folder/GBLator/build/MinSizeRel/gblator_tests.exe")
  set_tests_properties(gblator_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/mvton/GBLator_Project_Folder/GBLator/CMakeLists.txt;25;add_test;C:/Users/mvton/GBLator_Project_Folder/GBLator/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(gblator_unit_tests "C:/Users/mvton/GBLator_Project_Folder/GBLator/build/RelWithDebInfo/gblator_tests.exe")
  set_tests_properties(gblator_unit_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/mvton/GBLator_Project_Folder/GBLator/CMakeLists.txt;25;add_test;C:/Users/mvton/GBLator_Project_Folder/GBLator/CMakeLists.txt;0;")
else()
  add_test(gblator_unit_tests NOT_AVAILABLE)
endif()
