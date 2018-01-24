add_library(aseba_conf INTERFACE)
target_link_libraries(aseba_conf INTERFACE cpp_features)

target_compile_definitions(aseba_conf INTERFACE -DASEBA_ASSERT)
target_include_directories(aseba_conf INTERFACE ${PROJECT_SOURCE_DIR}/aseba)

# reduce the amount of recursive include trash on Windows
if (WIN32)
	target_compile_definitions(aseba_conf INTERFACE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()
