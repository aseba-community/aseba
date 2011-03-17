add_executable(asebatestcompile
	asebatestcompile.cpp
)

target_link_libraries(asebatestcompile asebacompiler asebacore ${EXTRA_LIBS})

install(TARGETS asebatestcompile RUNTIME
	DESTINATION bin
	LIBRARY DESTINATION lib
)

add_executable(asebatestcompilerun
	asebatestcompilerun.cpp
)

target_link_libraries(asebatestcompilerun asebacompiler asebavm asebacore ${EXTRA_LIBS})

install(TARGETS asebatestcompilerun RUNTIME
	DESTINATION bin
	LIBRARY DESTINATION lib
)

# the following tests should succeed
add_test(basic-arithmetic ${EXECUTABLE_OUTPUT_PATH}/asebatestcompile ${CMAKE_CURRENT_SOURCE_DIR}/data/basic-arithmetic.txt)


# the following tests should fail
add_test(chained-conditional ${EXECUTABLE_OUTPUT_PATH}/asebatestcompile --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/chained-conditional.txt)
add_test(implicit-conditional ${EXECUTABLE_OUTPUT_PATH}/asebatestcompile --fail ${CMAKE_CURRENT_SOURCE_DIR}/data/implicit-conditional.txt)

