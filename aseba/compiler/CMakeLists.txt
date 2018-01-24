set (ASEBACOMPILER_SRC
	compiler.cpp
	errors.cpp
	identifier-lookup.cpp
	lexer.cpp
	parser.cpp
	analysis.cpp
	tree-build.cpp
	tree-expand.cpp
	tree-dump.cpp
	tree-typecheck.cpp
	tree-optimize.cpp
	tree-emit.cpp
)
add_library(asebacompiler ${ASEBACOMPILER_SRC})
set_target_properties(asebacompiler PROPERTIES VERSION ${LIB_VERSION_STRING} 
                                        SOVERSION ${LIB_VERSION_MAJOR})

install(TARGETS asebacompiler
                LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
)

set (ASEBACORE_HDR_COMPILER
	compiler.h
	errors_code.h
)
install(FILES ${ASEBACORE_HDR_COMPILER}
	DESTINATION include/aseba/compiler
)
