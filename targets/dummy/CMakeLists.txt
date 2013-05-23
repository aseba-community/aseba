add_executable(asebadummynode dummynode.cpp dummynode_description.c)
target_link_libraries(asebadummynode asebavmbuffer asebavm ${ASEBA_CORE_LIBRARIES})
install(TARGETS asebadummynode RUNTIME DESTINATION bin LIBRARY DESTINATION bin)