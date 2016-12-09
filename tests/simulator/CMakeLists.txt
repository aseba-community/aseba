if (ENKI_FOUND)
    include_directories(${ENKI_INCLUDE_DIR})

    add_executable(aseba-test-simulator aseba-test-simulator.cpp)
    target_link_libraries(aseba-test-simulator asebasim asebacompiler asebavmbuffer asebavm ${ASEBA_CORE_LIBRARIES} ${ENKI_LIBRARY})

    # the following tests should succeed
    add_test(robot-simulator-thymio ${EXECUTABLE_OUTPUT_PATH}/aseba-test-simulator)

endif()
