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

        # Convert field from DF-ISE to APF
        ADD_TOOL_TEST(
            NAME mesh_converter_dfise
            EXECUTABLE mesh_converter
            OUTPUT "test_structure_0V_ElectricField.apf"
            PASS_REGULAR_EXPRESSION "-7.52928e-06"
            ARGUMENTS -c ${CMAKE_SOURCE_DIR}/conf/tcad_teststructure/convert.conf -f ${CMAKE_SOURCE_DIR}/conf/tcad_teststructure/test_structure_0V
        )

        # Generate weighting potential:
        ADD_TOOL_TEST(
            NAME generate_weighting_potential
            EXECUTABLE generate_potential
            OUTPUT model_weightingpotential.apf
            PASS_REGULAR_EXPRESSION "0.000500717"
            ARGUMENTS --model ${CMAKE_SOURCE_DIR}/models/timepix.conf
        )

        MESSAGE(STATUS "Unit tests: 4 tools functionality tests")
    ELSE()
        MESSAGE(STATUS "Unit tests: tools functionality tests deactivated.")
    ENDIF()
ENDIF()
