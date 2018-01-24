# Core libraries
add_subdirectory(common)
add_subdirectory(transport)

# Target and client specific libraries
add_subdirectory(vm)
add_subdirectory(compiler)

# User programs
add_subdirectory(clients)
add_subdirectory(switches)
add_subdirectory(targets)
