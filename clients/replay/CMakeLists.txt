add_executable(asebarec
	rec.cpp
)
target_link_libraries(asebarec ${ASEBA_CORE_LIBRARIES})
install(TARGETS asebarec RUNTIME
	DESTINATION bin
)

add_executable(asebaplay
	play.cpp
)
target_link_libraries(asebaplay ${ASEBA_CORE_LIBRARIES})
install(TARGETS asebaplay RUNTIME
	DESTINATION bin
)
