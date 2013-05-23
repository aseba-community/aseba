set (ASEBAVM_SRC
	vm.c
	natives.c
)
add_library(asebavm ${ASEBAVM_SRC})
install(TARGETS asebavm ARCHIVE
	DESTINATION lib
)