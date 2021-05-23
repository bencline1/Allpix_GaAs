###############################
# Framework performance tests #
###############################

IF(TEST_PERFORMANCE)
    FILE(
        GLOB TEST_LIST_PERFORMANCE
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        test_performance/test_*)
    LIST(LENGTH TEST_LIST_PERFORMANCE NUM_TEST_PERFORMANCE)
    SET_PROPERTY(GLOBAL PROPERTY COUNT_TESTS_PERFORMANCE "${NUM_TEST_PERFORMANCE}")
    FOREACH(test ${TEST_LIST_PERFORMANCE})
        GET_FILENAME_COMPONENT(title ${test} NAME_WE)
        ADD_ALLPIX_TEST(${test} "performance/${title}")
    ENDFOREACH()

    # Set up separate target for performance tests
    ADD_CUSTOM_TARGET(
        test_performance
        COMMAND ${CMAKE_CTEST_COMMAND} "-R" "performance" "--output-on-failure" "--no-compress-output" "--test-action" "Test"
    COMMENT "Target for performance tests")
ENDIF()

######################################
# Core framework functionality tests #
######################################

IF(TEST_CORE)
    FILE(
        GLOB TEST_LIST_CORE
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        test_core/test_*)
    LIST(LENGTH TEST_LIST_CORE NUM_TEST_CORE)
    SET_PROPERTY(GLOBAL PROPERTY COUNT_TESTS_CORE "${NUM_TEST_CORE}")
    FOREACH(test ${TEST_LIST_CORE})
        GET_FILENAME_COMPONENT(title ${test} NAME_WE)
        ADD_ALLPIX_TEST(${test} "core/${title}")
    ENDFOREACH()

    # Set up separate target for core tests
    ADD_CUSTOM_TARGET(
        test_core
        COMMAND ${CMAKE_CTEST_COMMAND} "-R" "core/" "--output-on-failure" "--no-compress-output" "--test-action" "Test" "-j4"
    COMMENT "Target for framework core tests")
ENDIF()
