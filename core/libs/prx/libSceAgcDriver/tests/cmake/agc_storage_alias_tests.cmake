add_test_executable(agc_storage_alias_tests EXCLUDE_FROM_ALL tests/StorageAlias.cpp tests/StorageAliasMain.cpp)
target_include_directories(agc_storage_alias_tests PRIVATE ${LIBS_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/core/shader/recompiler ${ANYPS5_VULKAN_HEADERS_INCLUDE_DIR})
target_link_libraries(agc_storage_alias_tests PRIVATE libSceAgcDriver libc)
set_target_properties(agc_storage_alias_tests PROPERTIES CXX_EXTENSIONS OFF)
configure_windows_unwind(agc_storage_alias_tests)

if(BUILD_TESTING)
    set_target_properties(agc_storage_alias_tests PROPERTIES EXCLUDE_FROM_ALL OFF)
    add_test(NAME agc_storage_alias COMMAND agc_storage_alias_tests)
    if(WIN32)
        set_tests_properties(agc_storage_alias PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libc>")
    endif()
endif()
