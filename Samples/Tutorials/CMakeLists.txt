cmake_minimum_required (VERSION 2.8)
project(OgreTutorialsSample)

# required for Ogre 1.11+
set(CMAKE_CXX_STANDARD 11)

## [discover_ogre]
# specify which components you need.
# The CONFIG flag makes sure you get OGRE instead of OGRE-next
find_package(OGRE REQUIRED COMPONENTS Bites RTShaderSystem CONFIG)

# add the source files as usual
add_executable(0_Bootstrap Bootstrap.cpp)

find_package(Qt5 REQUIRED COMPONENTS Core Gui)

if(OGRE_STATIC)
    find_package(ZLIB REQUIRED)
    # merely silence cmake, we wont use it
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
endif()

# this also sets the includes and pulls third party dependencies
target_link_libraries(0_Bootstrap OgreBitesQt OgreRTShaderSystem)
## [discover_ogre]

add_executable(1_FirstScene BasicTutorial1.cpp)

target_link_libraries(1_FirstScene ${OGRE_LIBRARIES})

add_executable(2_CamerasAndShadows BasicTutorial2.cpp)

target_link_libraries(2_CamerasAndShadows ${OGRE_LIBRARIES})
