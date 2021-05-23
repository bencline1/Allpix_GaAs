# Include set of separate tools shipped with the framework
OPTION(BUILD_TOOLS "Build additional tools and executables" ON)

IF(BUILD_TOOLS)
    # Build the MeshConverter
    ADD_SUBDIRECTORY(mesh_converter)

    # Install the ROOT helper macro's for analysis
    ADD_SUBDIRECTORY(root_analysis_macros)

    # Add APF filed format helper tools
    ADD_SUBDIRECTORY(apf_tools)

    # Add APF filed format helper tools
    ADD_SUBDIRECTORY(weightingpotential_generator)

    #########################
    # Framework tools tests #
    #########################

    OPTION(TEST_TOOLS "Perform unit tests to ensure tool functionality?" ON)

    IF(TEST_TOOLS)
        INCLUDE(ExternalData)
        # cmake-lint: disable=C0103
        # Define resources for external data used in these tests, copy from EOS directly if available
        SET(ExternalData_URL_TEMPLATES "file:///eos/project-a/allpix-squared/www/data/%(algo)/%(hash)"
                                       "https://project-allpix-squared.web.cern.ch/data/%(algo)/%(hash)")

        # Convert field from DF-ISE to APF
        ADD_TOOL_TEST(
            NAME
            mesh_converter_dfise
            EXECUTABLE
            mesh_converter
            OUTPUT
            "sample_field_ElectricField.apf"
            PASS_REGULAR_EXPRESSION
            "8.90408e-06 6.66146e-06 0.000370734 2.88255e-07 2.08785e-07 0.00056371 5.35032e-07 4.05597e-07"
            ARGUMENTS
            -c
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/convert.conf
            -f
            tests/sample_field
            DATA
            tests/sample_field.dat
            tests/sample_field.grd)

        # Generate weighting potential:
        ADD_TOOL_TEST(
            NAME
            generate_weighting_potential
            EXECUTABLE
            generate_potential
            OUTPUT
            model_weightingpotential.apf
            PASS_REGULAR_EXPRESSION
            "0.000500717"
            ARGUMENTS
            --model
            ${CMAKE_SOURCE_DIR}/models/timepix.conf)

        EXTERNALDATA_ADD_TARGET(fetch_test_data_tools)
        SET_TARGET_PROPERTIES(fetch_test_data_tools PROPERTIES EXCLUDE_FROM_ALL TRUE)

        MESSAGE(STATUS "Unit tests: 4 tools functionality tests")
    ELSE()
        MESSAGE(STATUS "Unit tests: tools functionality tests deactivated.")
    ENDIF()
ENDIF()
