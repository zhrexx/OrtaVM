cmake_minimum_required(VERSION 3.31.6)
project(orta VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)
if(NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()

set(SRCDIR ${CMAKE_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -ggdb")
    add_compile_definitions(DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Ofast -Os -s -g0 -flto")
endif()

add_compile_definitions(GITHASH="${GIT_HASH}")
add_compile_definitions(_VERSION=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

find_library(MATH_LIBRARY m)

set(TARGETS orta fcfx xd repl xtoa nyva)

add_executable(orta ${SRCDIR}/orta.c)
target_include_directories(orta PRIVATE ${SRCDIR})
if(MATH_LIBRARY)
    target_link_libraries(orta ${MATH_LIBRARY})
endif()

add_executable(fcfx ${SRCDIR}/fcfx.c)
target_include_directories(fcfx PRIVATE ${SRCDIR})
if(MATH_LIBRARY)
    target_link_libraries(fcfx ${MATH_LIBRARY})
endif()

add_executable(xd ${SRCDIR}/xd.c)
target_include_directories(xd PRIVATE ${SRCDIR})
if(MATH_LIBRARY)
    target_link_libraries(xd ${MATH_LIBRARY})
endif()

add_executable(repl ${SRCDIR}/repl.c)
target_include_directories(repl PRIVATE ${SRCDIR})
if(MATH_LIBRARY)
    target_link_libraries(repl ${MATH_LIBRARY})
endif()

add_executable(xtoa ${SRCDIR}/xtoa.c)
target_include_directories(xtoa PRIVATE ${SRCDIR})
if(MATH_LIBRARY)
    target_link_libraries(xtoa ${MATH_LIBRARY})
endif()

add_executable(nyva ${SRCDIR}/nyva.c)
if(MATH_LIBRARY)
    target_link_libraries(nyva ${MATH_LIBRARY})
endif()

option(BUILD_STATIC "Build with static linking" OFF)
if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_library(orta_lib ${SRCDIR}/orta.h)
set_target_properties(orta_lib PROPERTIES
        LINKER_LANGUAGE C
        PUBLIC_HEADER ${SRCDIR}/orta.h
        OUTPUT_NAME orta
)

find_program(ZIG_EXECUTABLE zig)
if(ZIG_EXECUTABLE)
    add_custom_target(rom
            COMMAND ${ZIG_EXECUTABLE} build-exe ${SRCDIR}/rom.zig -femit-bin=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rom
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            SOURCES ${SRCDIR}/rom.zig
    )
endif()

find_program(XXD_EXECUTABLE xxd)
if(XXD_EXECUTABLE)
    add_custom_target(release
            COMMAND ${XXD_EXECUTABLE} -i std.x > ${SRCDIR}/std.h
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

install(TARGETS ${TARGETS}
        RUNTIME DESTINATION bin
)

install(TARGETS orta_lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include
)