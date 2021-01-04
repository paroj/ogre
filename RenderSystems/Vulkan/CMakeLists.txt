#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Configure Vulkan RenderSystem build

file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

set( HEADER_FILES ${HEADER_FILES})
#set( SOURCE_FILES ${SOURCE_FILES})

set( SOURCE_FILES
src/OgreVulkanEngineDll.cpp
src/OgreVulkanPlugin.cpp
src/OgreVulkanSupport.cpp
src/OgreVulkanDevice.cpp
src/OgreVulkanWindow.cpp
src/OgreVulkanQueue.cpp
src/OgreVulkanTextureGpu.cpp
src/OgreVulkanTextureGpuWindow.cpp
src/OgreVulkanMappings.cpp
src/OgreVulkanUtils.cpp
src/OgreVulkanHardwareBufferCommon.cpp
src/OgreVulkanHardwareBufferManager.cpp
src/OgreVulkanDiscardBufferManager.cpp
src/OgreVulkanProgram.cpp
src/OgreVulkanTextureGpuManager.cpp
src/OgreVulkanRenderPassDescriptor.cpp
src/OgreVulkanRenderSystem.cpp)


include_directories(
  BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include_directories( ${Vulkan_INCLUDE_DIRS} )

add_library(RenderSystem_Vulkan ${OGRE_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(RenderSystem_Vulkan OgreMain ${Vulkan_LIBRARIES})

if( UNIX AND NOT ANDROID )
	target_link_libraries(RenderSystem_Vulkan vulkan xcb-randr X11-xcb)
endif()

if (OGRE_CONFIG_THREADS)
  target_link_libraries(RenderSystem_Vulkan ${OGRE_THREAD_LIBRARIES})
endif ()

ogre_config_framework(RenderSystem_Vulkan)

ogre_config_plugin(RenderSystem_Vulkan)
install(FILES ${HEADER_FILES} DESTINATION include/OGRE/RenderSystems/Vulkan)
install(FILES ${PLATFORM_HEADERS} DESTINATION include/OGRE/RenderSystems/Vulkan/${PLATFORM_HEADER_INSTALL})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION include/OGRE/RenderSystems/Vulkan)
