add_library(aseba_conf INTERFACE)
target_link_libraries(aseba_conf INTERFACE cpp_features)

target_compile_definitions(aseba_conf INTERFACE -DASEBA_ASSERT)

# reduce the amount of recursive include trash on Windows
if (WIN32)
	target_compile_definitions(aseba_conf INTERFACE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()
