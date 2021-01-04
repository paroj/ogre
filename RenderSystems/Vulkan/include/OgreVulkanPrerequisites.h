/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#ifndef _OgreVulkanPrerequisites_H_
#define _OgreVulkanPrerequisites_H_

#include "OgrePrerequisites.h"

#include "OgreDeprecated.h"

#include "OgreLogManager.h"
#include "OgrePixelFormat.h"

#ifdef __MINGW32__
#    ifndef UINT64_MAX
#        define UINT64_MAX 0xffffffffffffffffULL /* 18446744073709551615ULL */
#    endif
#endif

#if defined( __LP64__ ) || defined( _WIN64 ) || ( defined( __x86_64__ ) && !defined( __ILP32__ ) ) || \
    defined( _M_X64 ) || defined( __ia64 ) || defined( _M_IA64 ) || defined( __aarch64__ ) || \
    defined( __powerpc64__ )
#    define OGRE_VK_NON_DISPATCHABLE_HANDLE( object ) typedef struct object##_T *object;
#else
#    define OGRE_VK_NON_DISPATCHABLE_HANDLE( object ) typedef uint64_t object;
#endif

typedef struct VkInstance_T *VkInstance;
typedef struct VkPhysicalDevice_T *VkPhysicalDevice;
typedef struct VkDevice_T *VkDevice;
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkDeviceMemory )
typedef struct VkCommandBuffer_T *VkCommandBuffer;

OGRE_VK_NON_DISPATCHABLE_HANDLE( VkBuffer )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkBufferView )

OGRE_VK_NON_DISPATCHABLE_HANDLE( VkSurfaceKHR )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkSwapchainKHR )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkImage )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkSemaphore )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkFence )

OGRE_VK_NON_DISPATCHABLE_HANDLE( VkRenderPass )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkFramebuffer )

OGRE_VK_NON_DISPATCHABLE_HANDLE( VkShaderModule )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkDescriptorSetLayout )

OGRE_VK_NON_DISPATCHABLE_HANDLE( VkDescriptorPool )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkDescriptorSet )

OGRE_VK_NON_DISPATCHABLE_HANDLE( VkPipelineLayout )
OGRE_VK_NON_DISPATCHABLE_HANDLE( VkPipeline )

#undef OGRE_VK_NON_DISPATCHABLE_HANDLE

struct VkPipelineShaderStageCreateInfo;

#define OGRE_VULKAN_CONST_SLOT_START 16u
#define OGRE_VULKAN_TEX_SLOT_START 24u
#define OGRE_VULKAN_PARAMETER_SLOT 23u
#define OGRE_VULKAN_UAV_SLOT_START 28u

#define OGRE_VULKAN_CS_PARAMETER_SLOT 7u
#define OGRE_VULKAN_CS_CONST_SLOT_START 0u
#define OGRE_VULKAN_CS_UAV_SLOT_START 8u
#define OGRE_VULKAN_CS_TEX_SLOT_START 16u

#define RESTRICT_ALIAS __restrict__
#define RESTRICT_ALIAS_RETURN

namespace Ogre
{
    // Forward declarations
    class VulkanBufferInterface;
    class VulkanCache;
    class VulkanDescriptorPool;
    struct VulkanDevice;
    class VulkanDynamicBuffer;
    struct VulkanGlobalBindingTable;
    class VulkanGpuProgramManager;
    class VulkanProgram;
    class VulkanProgramFactory;
    class VulkanQueue;
    class VulkanRenderSystem;
    class VulkanRootLayout;
    class VulkanStagingBuffer;
    class VulkanTextureGpu;
    class VulkanTextureGpuManager;
    class VulkanVaoManager;
    class VulkanWindow;
    class VulkanDiscardBuffer;
    class VulkanDiscardBufferManager;
    class VulkanRenderPassDescriptor;

    // forward compatibility defines
    struct BufferPacked;
    struct VaoManager;
    struct RootLayout;
    struct VulkanFrameBufferDescKey;

    typedef VulkanRenderPassDescriptor RenderPassDescriptor;
    typedef VulkanFrameBufferDescKey FrameBufferDescKey;

    /// Provides a simple interface similar to that of MTLBuffer
    struct VulkanRawBuffer
    {
        VkBuffer mVboName;
        VulkanDynamicBuffer *mDynamicBuffer;
        uint32 mVboFlag;  /// See VulkanVaoManager::VboFlag
        size_t mUnmapTicket;

        size_t mSize;
        size_t mInternalBufferStart;
        size_t mVboPoolIdx;

        void *map( void );
        void unmap( void );
    };

    namespace ResourceLayout
    {
    enum Layout
    {
        Undefined,
        Texture,
        RenderTarget,
        RenderTargetReadOnly,
        ResolveDest,
        Clear,
        Uav,
        CopySrc,
        CopyDst,
        /// Special layout only used in flushTextureCopyOperations to prevent
        /// conflicts with automatically managed copy encoders
        CopyEnd,
        MipmapGen,
        PresentReady,

        NumResourceLayouts
    };
    }

    enum BufferPackedTypes
    {
        BP_TYPE_VERTEX,
        BP_TYPE_INDEX,
        BP_TYPE_CONST,
        BP_TYPE_TEX,
        BP_TYPE_READONLY,
        BP_TYPE_UAV,
        BP_TYPE_INDIRECT,
        NUM_BUFFER_PACKED_TYPES
    };

    namespace ResourceAccess
    {
    /// Enum identifying the texture access privilege
    enum ResourceAccess
    {
        Undefined = 0x00,
        Read = 0x01,
        Write = 0x02,
        ReadWrite = Read | Write
    };
    }



    struct ResourceTransition
    {
        //GpuTrackedResource *resource;

        ResourceLayout::Layout      oldLayout;
        ResourceLayout::Layout      newLayout;

        /// If oldAccess == Undefined, it means there are no previous stage dependencies
        /// AND there is no guarantee previous contents will be preserved.
        ResourceAccess::ResourceAccess oldAccess;
        /// newAccess == Undefined is invalid
        ResourceAccess::ResourceAccess newAccess;

        /// If oldStageMask == Undefined, it means there are no previous
        /// stage dependencies (e.g. beginning of the frame)
        uint8 oldStageMask;
        /// If newStageMask == Undefined is invalid
        uint8 newStageMask;
    };

    namespace LoadAction
    {
        enum LoadAction
        {
            /// Do not care about the initial contents. Useful for colour buffers
            /// if you intend to cover the whole screen with opaque draws.
            /// Not recommended for depth & stencil.
            DontCare,
            /// Clear the whole resource or a fragment of it depending on
            /// viewport settings.
            /// Actual behavior depends on the API used.
            Clear,
            /** On tilers, will clear the subregion.
                On non-tilers, it's the same as LoadAction::Load and assumes
                the resource is already cleared.
            @remarks
                This is useful for shadow map atlases. On desktop you probably
                want to render to a 8192x8192 shadow map in the following way:
                    clear( 0, 0, 8192, 8192 );
                    foreach( shadowmap in shadowmaps )
                        render( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );

                However this is heavily innefficient for tilers. In tilers you probably
                want to do the following:
                    foreach( shadowmap in shadowmaps )
                        clear( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );
                        render( shadowmap.x, shadowmap.y, shadowmap.width, shadowmap.height );

                ClearOnTilers allows you to perform the most efficient shadow map rendering
                on both tilers & non-tilers using the same compositor script, provided the
                full clear pass was set to only execute on non-tilers.
            */
            ClearOnTilers,
            /// Load the contents that were stored in the texture.
            Load
        };
    }
    namespace StoreAction
    {
        enum StoreAction
        {
            /// Discard the contents after we're done with the current pass.
            /// Useful if you only want colour and don't care what happens
            /// with the depth & stencil buffers.
            /// Discarding contents either improves framerate or battery duration
            /// (particularly important in mobile), or makes rendering
            /// friendlier for SLI/Crossfire.
            DontCare,
            /// Keep the contents of what we've just rendered
            Store,
            /// Resolve MSAA rendering into resolve texture.
            /// Contents of MSAA texture are discarded.
            /// It is invalid to use this flag without an MSAA texture.
            MultisampleResolve,
            /// Resolve MSAA rendering into resolve texture.
            /// Contents of MSAA texture are kept.
            /// It is valid to use this flag without an MSAA texture.
            StoreAndMultisampleResolve,
            /// If texture is MSAA, has same effects as MultisampleResolve.
            /// If texture is not MSAA, has same effects as Store.
            /// There is one exception: If texture is MSAA w/ explicit resolves,
            /// but no resolve texture was set, it has same effects as Store
            ///
            /// To be used only by the Compositor.
            /// Should not be used in the actual RenderPassDescriptor directly.
            StoreOrResolve
        };
    }

    struct DescBindingRange
    {
        uint16 start;  // Inclusive
        uint16 end;    // Exclusive
        DescBindingRange();

        size_t getNumUsedSlots( void ) const { return end - start; }
        bool isInUse( void ) const { return start < end; }
        bool isValid( void ) const { return start <= end; }

        bool isDirty( uint8 minDirtySlot ) const
        {
            return ( minDirtySlot >= start ) & ( minDirtySlot < end );
        }
    };

    namespace DescBindingTypes
    {
        /// The order is important as it affects compatibility between root layouts
        ///
        /// If the relative order is changed, then the following needs to be modified:
        ///     - VulkanRootLayout::bind
        ///     - RootLayout::findParamsBuffer (if ParamBuffer stops being the 1st)
        ///     - c_rootLayoutVarNames
        ///     - c_bufferTypes (Vulkan)
        enum DescBindingTypes
        {
            ParamBuffer,
            ConstBuffer,
            ReadOnlyBuffer,
            TexBuffer,
            Texture,
            Sampler,
            UavBuffer,
            UavTexture,
            NumDescBindingTypes
        };
    }  // namespace DescBindingTypes

    template <typename T>
    using FastArray = std::vector<T>;

    typedef FastArray<VkSemaphore> VkSemaphoreArray;
    typedef FastArray<VkFence> VkFenceArray;
    typedef FastArray<ResourceTransition> ResourceTransitionArray;

    /// Aligns the input 'offset' to the next multiple of 'alignment'.
    /// Alignment can be any value except 0. Some examples:
    ///
    /// alignToNextMultiple( 0, 4 ) = 0;
    /// alignToNextMultiple( 1, 4 ) = 4;
    /// alignToNextMultiple( 2, 4 ) = 4;
    /// alignToNextMultiple( 3, 4 ) = 4;
    /// alignToNextMultiple( 4, 4 ) = 4;
    /// alignToNextMultiple( 5, 4 ) = 8;
    ///
    /// alignToNextMultiple( 0, 3 ) = 0;
    /// alignToNextMultiple( 1, 3 ) = 3;
    inline size_t alignToNextMultiple( size_t offset, size_t alignment )
    {
        return ( (offset + alignment - 1u) / alignment ) * alignment;
    }

    /** Used for efficient removal in std::vector and std::deque (like an std::list)
        However it assumes the order of elements in container is not important or
        something external to the container holds the index of an element in it
        (but still should be kept deterministically across machines)
        Basically it swaps the iterator with the last iterator, and pops back
        Returns the next iterator
    */
    template<typename T>
    typename T::iterator efficientVectorRemove( T& container, typename T::iterator& iterator )
    {
        const size_t idx = iterator - container.begin();
        *iterator = container.back();
        container.pop_back();

        return container.begin() + idx;
    }

    namespace v1
    {
        class VulkanHardwareBufferCommon;
    }  // namespace v1


    typedef Texture TextureGpu;
    typedef PixelFormat PixelFormatGpu;
    typedef VulkanStagingBuffer StagingBuffer;

    enum StagingStallType
    {
        /// Next map will not stall.
        STALL_NONE,

        /// Next map call will cause a stall. We can't predict how long, but
        /// on average should be small. You should consider doing something
        /// else then try again.
        STALL_PARTIAL,

        /// The whole pipeline is brought to a stop. We have to wait for the GPU
        /// to finish all operations issued so far. This can be very expensive.
        /// Grab a different StagingBuffer.
        STALL_FULL,

        NUM_STALL_TYPES
    };

    namespace SubmissionType
    {
        enum SubmissionType
        {
            /// Send the work we have so far to the GPU, but there may be more under way
            ///
            /// A fence won't be generated automatically to protect this work, but
            /// there may be one if getCurrentFence or acquireCurrentFence
            /// was called
            FlushOnly,
            /// Same as FlushOnly + fences the data so that
            /// VaoManager::mDynamicBufferCurrentFrame can be incremented and
            /// waitForTailFrameToFinish works without stalling.
            ///
            /// It's like forcing the end of a frame without swapping windows, e.g.
            /// when waiting for textures to finish streaming.
            NewFrameIdx,
            /// Same as NewFrameIdx + swaps windows (presents).
            /// This is the real end of the frame.
            EndFrameAndSwap
        };
    }

}  // namespace Ogre

#define OGRE_ASSERT_HIGH(x) OgreAssert((x), "high")
#define OGRE_ASSERT_MEDIUM(x) OgreAssert((x), "medium")
#define OGRE_ASSERT_LOW(x) OgreAssert((x), "low")

#define OGRE_VK_EXCEPT( code, num, desc, src ) \
    OGRE_EXCEPT( code, desc + ( "\nVkResult = " + vkResultToString( num ) ), src )

#if OGRE_COMPILER == OGRE_COMPILER_MSVC
#    define checkVkResult( result, functionName ) \
        do \
        { \
            if( result != VK_SUCCESS ) \
            { \
                OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, functionName " failed", \
                                __FUNCSIG__ ); \
            } \
        } while( 0 )
#else
#    define checkVkResult( result, functionName ) \
        do \
        { \
            if( result != VK_SUCCESS ) \
            { \
                OGRE_VK_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR, result, functionName " failed", \
                                __PRETTY_FUNCTION__ ); \
            } \
        } while( 0 )
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#    if !defined( __MINGW32__ )
#        define WIN32_LEAN_AND_MEAN
#        ifndef NOMINMAX
#            define NOMINMAX  // required to stop windows.h messing up std::min
#        endif
#    endif
#endif

// Lots of generated code in here which triggers the new VC CRT security warnings
#if !defined( _CRT_SECURE_NO_DEPRECATE )
#    define _CRT_SECURE_NO_DEPRECATE
#endif

#if( OGRE_PLATFORM == OGRE_PLATFORM_WIN32 ) && !defined( __MINGW32__ ) && !defined( OGRE_STATIC_LIB )
#    ifdef RenderSystem_Vulkan_EXPORTS
#        define _OgreVulkanExport __declspec( dllexport )
#    else
#        if defined( __MINGW32__ )
#            define _OgreVulkanExport
#        else
#            define _OgreVulkanExport __declspec( dllimport )
#        endif
#    endif
#elif defined( RenderSystem_Vulkan_EXPORTS )
#    define _OgreVulkanExport __attribute__( ( visibility( "default" ) ) )
#else
#    define _OgreVulkanExport
#endif

#endif  //#ifndef _OgreVulkanPrerequisites_H_
