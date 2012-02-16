add_executable(asebadummynode dummynode.cpp dummynode_description.c)
target_link_libraries(asebadummynode asebavmbuffer asebavm asebacore ${EXTRA_LIBS})
install(TARGETS asebadummynode RUNTIME DESTINATION bin LIBRARY DESTINATION bin)