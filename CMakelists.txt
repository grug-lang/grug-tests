cmake_minimum_required(VERSION 3.16)
project(grug C)

# -----------------------------
# Compiler selection
# -----------------------------
# Respect CC if provided, default to clang

if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER clang CACHE STRING "" FORCE)
endif()

message(STATUS "Compilation will use ${CMAKE_C_COMPILER_ID}")

# -----------------------------
# Base compiler flags
# -----------------------------
set(BASE_WARNINGS
    -g
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wshadow
    -Wuninitialized
    -Wunused-macros
    # -Wfatal-errors
)

add_compile_options(${BASE_WARNINGS})

# Clang-specific flags
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-gdwarf-4)
endif()

# -----------------------------
# Feature flags (env-controlled)
# -----------------------------

# ASAN
if(DEFINED ENV{ASAN})
    message(STATUS "ASAN was turned on")
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# COVERAGE
if(DEFINED ENV{COVERAGE})
    message(STATUS "COVERAGE was turned on")
    add_compile_options(--coverage)
    add_link_options(--coverage)

    file(GLOB GCDA_FILES "*.gcda")
    if(GCDA_FILES)
        file(REMOVE ${GCDA_FILES})
    endif()
endif()

# SHUFFLES
if(DEFINED ENV{SHUFFLES})
    message(STATUS "SHUFFLES=$ENV{SHUFFLES} was passed")
    add_compile_definitions(SHUFFLES=$ENV{SHUFFLES})
endif()

# VALGRIND (runtime only, informational)
if(DEFINED ENV{VALGRIND})
    message(STATUS "VALGRIND was turned on")
endif()

# ANALYZE
if(DEFINED ENV{ANALYZE})
    message(STATUS "ANALYZE was turned on")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_compile_options(--analyzer)
    else()
        add_compile_options(--analyze)
    endif()
endif()

# -----------------------------
# Build tests.so (shared lib)
# -----------------------------
add_library(tests SHARED tests.c)

set_target_properties(tests PROPERTIES
    OUTPUT_NAME tests
    POSITION_INDEPENDENT_CODE ON
)
if (DEFINED WIN32) 
	set_target_properties(tests PROPERTIES PREFIX "")
endif()

target_link_libraries(tests
    PRIVATE
        m
)

# Needed to match -rdynamic behavior
if(UNIX AND NOT APPLE)
    target_link_options(tests PRIVATE -rdynamic)
endif()

# -----------------------------
# Build smoketest (executable)
# -----------------------------
add_executable(smoketest smoketest.c)

list(APPEND CMAKE_BUILD_RPATH ${CMAKE_CURRENT_BINARY_DIR})

# -----------------------------
# Run smoketest target
# -----------------------------
add_custom_target(run
    COMMAND
        $<$<BOOL:$ENV{VALGRIND}>:valgrind --quiet>
        $<TARGET_FILE:smoketest>
    DEPENDS smoketest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# -----------------------------
# Coverage report
# -----------------------------
if(DEFINED ENV{COVERAGE})
    add_custom_target(coverage
        COMMAND gcovr
            --gcov-executable "llvm-cov gcov"
            --html-details coverage.html
            --html-theme github.green
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS smoketest tests
    )
endif()
