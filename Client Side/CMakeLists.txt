# CMakeList.txt : CMake project for Ceva game client, include source and define
# project specific logic here.
#

# Add source to this project's executable.

# FORCE SYSTEM-WIDE WINDOWS DEFINITIONS FOR ALL ENET FILES
if(WIN32)
    add_compile_definitions(WIN32 _WINDOWS HAS_WINSOCK2 _AMD64_)
endif()
# 2. Automatically gather ALL .c files inside the enet folder
file(GLOB ENET_SOURCES "enetOG/*.c")

add_library(enet_lib STATIC ${ENET_SOURCES})
target_include_directories(enet_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/enetOG/include")
# 3. Define the Server Executable (compiles your code + all ENet files)
add_executable (CMakeTarget "Ceva game client.cpp" "Ceva game client.h" "raylib/include/raylib.h" "raylib/include/raymath.h" "raylib/include/rlgl.h" "raylib/raylib-6.0_win64_msvc16/include/raylib.h" "raylib/raylib-6.0_win64_msvc16/include/raymath.h" "raylib/raylib-6.0_win64_msvc16/include/rlgl.h")

if(WIN32)
    target_link_libraries(CMakeTarget PRIVATE ws2_32 winmm)
    target_compile_options(CMakeTarget PRIVATE /wd4996) 
endif()

# Link your project with raylib
# Tell the compiler where to find "raylib.h"
target_include_directories(CMakeTarget PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/raylib/include")

# Tell the linker where to find "raylib.lib"
target_link_libraries(CMakeTarget PRIVATE enet_lib "${CMAKE_CURRENT_SOURCE_DIR}/raylib/lib/raylib.lib" winmm)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CMakeTarget PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
