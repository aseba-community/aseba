set (ASEBAVMBUFFER_SRC
	vm-buffer.c
)
add_library(asebavmbuffer STATIC ${ASEBAVMBUFFER_SRC})
set_target_properties(asebavmbuffer PROPERTIES VERSION ${LIB_VERSION_STRING} 
                                        SOVERSION ${LIB_VERSION_MAJOR})

install(TARGETS asebavmbuffer
                LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
)

target_link_libraries(asebavmbuffer asebavm)

