
set(TEST_LIST
  test_opendialog.c
  test_opendialog_cpp.cpp
  test_opendialog_native.c
  test_opendialogmultiple.c
  test_opendialogmultiple_cpp.cpp
  test_opendialogmultiple_native.c
  test_opendialogmultiple_enum.c
  test_opendialogmultiple_enum_native.c
  test_pickfolder.c
  test_pickfolder_cpp.cpp
  test_pickfolder_native.c
  test_savedialog.c
  test_savedialog_native.c)
  
foreach (TEST ${TEST_LIST})
  string(REPLACE "." "_" CLEAN_TEST_NAME ${TEST})
  add_executable(${CLEAN_TEST_NAME}
    ${TEST})
  target_link_libraries(${CLEAN_TEST_NAME}
    PUBLIC nfd)
endforeach()
