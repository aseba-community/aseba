set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /MANIFEST:NO")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")

set(MSVC_VERSION 1300)
set(MSVC_INCREMENTAL_DEFAULT ON)

add_compile_options(-fuse-ld=lld-link)
add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_XKEYCHECK_H -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
add_compile_options(-fms-extensions -fms-compatibility -Wno-ignored-attributes
					-Wno-unused-local-typedef
				    -Wno-expansion-to-defined -Wno-pragma-pack -Wno-ignored-pragma-intrinsic
					-Wno-unknown-pragmas -Wno-invalid-token-paste -Wno-deprecated-declarations -Wno-macro-redefined
					-Wno-dllimport-static-field-def
					-Wno-unused-command-line-argument
					-Wno-unknown-argument
					-Wno-int-to-void-pointer-cast)
