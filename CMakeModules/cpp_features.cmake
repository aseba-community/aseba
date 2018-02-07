#The cpp_features virtual target defines the set of c++11/14/17 features requiered to build aseba
add_library(cpp_features INTERFACE)
# target_compile_features(cpp_features INTERFACE
# 	cxx_auto_type
# 	cxx_aggregate_default_initializers
# 	cxx_relaxed_constexpr
# 	cxx_decltype
# 	cxx_deleted_functions
# 	cxx_inline_namespaces
# 	cxx_noexcept
# 	cxx_nullptr
# 	cxx_override
# 	cxx_range_for
# 	cxx_raw_string_literals
# 	cxx_right_angle_brackets
# 	cxx_return_type_deduction
# 	cxx_strong_enums
# 	cxx_trailing_return_types
# 	cxx_variadic_templates
# 	cxx_uniform_initialization
# )
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
