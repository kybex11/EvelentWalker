# CMake generated Testfile for 
# Source directory: E:/evelent-core/EvelentWalker/EvelentWalker/cpp
# Build directory: E:/evelent-core/EvelentWalker/EvelentWalker/cpp/build-gui
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[evw_tests]=] "E:/evelent-core/EvelentWalker/EvelentWalker/cpp/build-gui/Debug/evw_tests.exe")
  set_tests_properties([=[evw_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "E:/evelent-core/EvelentWalker/EvelentWalker/cpp/CMakeLists.txt;62;add_test;E:/evelent-core/EvelentWalker/EvelentWalker/cpp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[evw_tests]=] "E:/evelent-core/EvelentWalker/EvelentWalker/cpp/build-gui/Release/evw_tests.exe")
  set_tests_properties([=[evw_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "E:/evelent-core/EvelentWalker/EvelentWalker/cpp/CMakeLists.txt;62;add_test;E:/evelent-core/EvelentWalker/EvelentWalker/cpp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[evw_tests]=] "E:/evelent-core/EvelentWalker/EvelentWalker/cpp/build-gui/MinSizeRel/evw_tests.exe")
  set_tests_properties([=[evw_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "E:/evelent-core/EvelentWalker/EvelentWalker/cpp/CMakeLists.txt;62;add_test;E:/evelent-core/EvelentWalker/EvelentWalker/cpp/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[evw_tests]=] "E:/evelent-core/EvelentWalker/EvelentWalker/cpp/build-gui/RelWithDebInfo/evw_tests.exe")
  set_tests_properties([=[evw_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "E:/evelent-core/EvelentWalker/EvelentWalker/cpp/CMakeLists.txt;62;add_test;E:/evelent-core/EvelentWalker/EvelentWalker/cpp/CMakeLists.txt;0;")
else()
  add_test([=[evw_tests]=] NOT_AVAILABLE)
endif()
