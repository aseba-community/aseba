if (APPLE)
	add_definitions(-DDISABLE_WEAK_CALLBACKS)
endif (APPLE)
set (ASEBAVM_SRC
	vm.c
	natives.c
)
add_library(asebavm STATIC ${ASEBAVM_SRC})
target_link_libraries(asebavm aseba_conf)
set_target_properties(asebavm PROPERTIES VERSION ${LIB_VERSION_STRING} 
                                        SOVERSION ${LIB_VERSION_MAJOR})

install(TARGETS asebavm 
                LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
)

