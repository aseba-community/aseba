add_executable(asebadummynode dummynode.cpp dummynode_description.c)
target_link_libraries(asebadummynode asebavmbuffer asebavm asebazeroconfqt asebacommon)
install(TARGETS asebadummynode RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
