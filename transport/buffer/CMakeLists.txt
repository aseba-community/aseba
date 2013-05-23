set (ASEBAVMBUFFER_SRC
	vm-buffer.c
)
add_library(asebavmbuffer ${ASEBAVMBUFFER_SRC})
install(TARGETS asebavmbuffer ARCHIVE
	DESTINATION lib
)