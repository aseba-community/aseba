add_executable(asebaswitch
	switch.cpp
)

target_link_libraries(asebaswitch ${ASEBA_ZEROCONF_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

install(TARGETS asebaswitch RUNTIME
	DESTINATION bin
)
