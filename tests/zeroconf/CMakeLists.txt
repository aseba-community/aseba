add_executable(test-zeroconf test-zeroconf.cpp)
target_link_libraries(test-zeroconf ${ASEBA_CORE_LIBRARIES} ${ASEBA_ZEROCONF_LIBRARIES} ${DNSSD_LIBRARIES})

add_test(NAME test-zeroconf COMMAND ${EXECUTABLE_OUTPUT_PATH}/test-zeroconf)
