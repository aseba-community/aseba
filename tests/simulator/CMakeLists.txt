if (ENKI_FOUND)
    add_executable(aseba-test-simulator aseba-test-simulator.cpp)
    target_link_libraries(aseba-test-simulator asebasim asebacompiler asebavmbuffer asebavm asebacommon enki)

    # the following tests should succeed
    add_test(robot-simulator-thymio aseba-test-simulator)

    qt5_use_modules(aseba-test-simulator  Core)


endif()
