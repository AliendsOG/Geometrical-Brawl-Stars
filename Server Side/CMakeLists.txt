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
add_executable (CMakeProject1 "CMakeProject1.cpp" "CMakeProject1.h")

if(WIN32)
    target_link_libraries(CMakeProject1 PRIVATE ws2_32 winmm)
    target_compile_options(CMakeProject1 PRIVATE /wd4996) 
endif()


# Tell the linker where to find "raylib.lib"
target_link_libraries(CMakeProject1 PRIVATE enet_lib )
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET CMakeProject1 PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
