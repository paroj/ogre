/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#include "OgreVulkanTextureGpuManager.h"

#include "OgreVulkanAsyncTextureTicket.h"
#include "OgreVulkanMappings.h"
#include "OgreVulkanStagingTexture.h"
#include "OgreVulkanTextureGpu.h"
#include "OgreVulkanTextureGpuWindow.h"
#include "OgreVulkanUtils.h"

#include "OgrePixelFormat.h"
#include "OgreVector.h"

#include "OgreException.h"

namespace Ogre
{
    static const bool c_bSkipAliasable = true;

    VulkanTextureGpuManager::VulkanTextureGpuManager( VulkanVaoManager *vaoManager,
                                                      RenderSystem *renderSystem, VulkanDevice *device,
                                                      bool bCanRestrictImageViewUsage ) :
        mDevice( device ),
        mCanRestrictImageViewUsage( bCanRestrictImageViewUsage )
    {
        VkImageCreateInfo imageInfo;
        makeVkStruct( imageInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO );
        imageInfo.extent.width = 4u;
        imageInfo.mipLevels = 1u;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        // clang-format off
        const char *dummyNames[] =
        {
            "Dummy Unknown (2D)",
            "Dummy 1D 4x1",
            "Dummy 1DArray 4x1",
            "Dummy 2D 4x4",
            "Dummy 2DArray 4x4x1",
            "Dummy Cube 4x4",
            "Dummy CubeArray 4x4x1",
            "Dummy 3D 4x4x4"
        };
        // clang-format on

        // Must be large enough to hold the biggest transfer we'll do.
        uint8 c_whiteData[4 * 4 * 6 * 4];
        uint8 c_blackData[4 * 4 * 6 * 4];
        memset( c_whiteData, 0xff, sizeof( c_whiteData ) );
        memset( c_blackData, 0x00, sizeof( c_blackData ) );

        const uint32 bytesPerPixel =
            static_cast<uint32>( PixelUtil::getNumElemBytes( PF_A8B8G8R8 ) );
        PixelBox whiteBox;/*( 4u, 4u, 1u, 6u, bytesPerPixel, bytesPerPixel * 4u,
                             bytesPerPixel * 4u * 4u );*/
        whiteBox.data = c_whiteData;
        PixelBox blackBox( whiteBox );
        blackBox.data = c_blackData;

        // Use a VulkanStagingTexture but we will manually handle the upload.
        // The barriers are easy because there is no work using any of this data
        VulkanStagingTexture *stagingTex =
            NULL;//vaoManager->createStagingTexture( PFG_RGBA8_UNORM, sizeof( c_whiteData ) * 2u );

        return;
        stagingTex->startMapRegion();
        PixelBox box;// = stagingTex->mapRegion( 4u, 4u, 1u, 6u * 2u, PF_A8B8G8R8 );
        // Don't manipulate box.sliceStart in normal code. We're doing this because we have
        // control on how VulkanStagingTexture works (but we don't control how the other APIs work)
        /*box.copyFrom( whiteBox );
        box.sliceStart = 6u;
        box.copyFrom( blackBox );
        box.sliceStart = 0u;*/
        stagingTex->stopMapRegion();

        VkBuffer stagingBuffVboName = stagingTex->_getVboName();

        for( size_t i = 1u; i < TEX_TYPE_2D_ARRAY + 1u; ++i )
        {
            if (c_bSkipAliasable && (i == TEX_TYPE_2D_ARRAY))
            {
                continue;
            }

            if( i == TEX_TYPE_3D)
            {
                imageInfo.extent.depth = 4u;
                imageInfo.imageType = VK_IMAGE_TYPE_3D;
            }
            else
            {
                imageInfo.extent.depth = 1u;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
            }
            if( i == TEX_TYPE_1D)
            {
                imageInfo.extent.height = 1u;
                imageInfo.imageType = VK_IMAGE_TYPE_1D;
            }
            else
            {
                imageInfo.extent.height = 4u;
            }

            if( i == TEX_TYPE_CUBE_MAP )
            {
                imageInfo.arrayLayers = 6u;
                imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }
            else
            {
                imageInfo.arrayLayers = 1u;
                imageInfo.flags = 0;
            }

            VkResult imageResult =
                vkCreateImage( device->mDevice, &imageInfo, 0, &mBlankTexture[i].vkImage );
            checkVkResult( imageResult, "vkCreateImage" );

            setObjectName( device->mDevice, (uint64_t)mBlankTexture[i].vkImage,
                           VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, dummyNames[i] );

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements( device->mDevice, mBlankTexture[i].vkImage, &memRequirements );

            VkDeviceMemory deviceMemory = NULL;/*vaoManager->allocateTexture(
                memRequirements, mBlankTexture[i].vboPoolIdx, mBlankTexture[i].internalBufferStart );*/

            VkResult result = vkBindImageMemory( device->mDevice, mBlankTexture[i].vkImage, deviceMemory,
                                                 mBlankTexture[i].internalBufferStart );
            checkVkResult( result, "vkBindImageMemory" );
        }

        size_t barrierCount = 0u;
        VkImageMemoryBarrier imageMemBarrier[TEX_TYPE_2D_ARRAY + 1u];
        for( size_t i = 1u; i < TEX_TYPE_2D_ARRAY + 1u; ++i )
        {
            if( c_bSkipAliasable && ( i == TEX_TYPE_2D_ARRAY ) )
            {
                continue;
            }

            makeVkStruct( imageMemBarrier[barrierCount], VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER );
            imageMemBarrier[barrierCount].image = mBlankTexture[i].vkImage;
            imageMemBarrier[barrierCount].srcAccessMask = 0;
            imageMemBarrier[barrierCount].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemBarrier[barrierCount].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemBarrier[barrierCount].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemBarrier[barrierCount].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemBarrier[barrierCount].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemBarrier[barrierCount].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageMemBarrier[barrierCount].subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            imageMemBarrier[barrierCount].subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ++barrierCount;
        }

        vkCmdPipelineBarrier( device->mGraphicsQueue.mCurrentCmdBuffer,
                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0u,
                              0, 0u, 0, static_cast<uint32>( barrierCount ), imageMemBarrier );

        for( size_t i = 1u; i < TEX_TYPE_2D_ARRAY + 1u; ++i )
        {
            if( c_bSkipAliasable && ( i == TEX_TYPE_2D_ARRAY ) )
            {
                continue;
            }

            size_t bufStart = 0u;
            if( i == TEX_TYPE_CUBE_MAP )
                bufStart = bytesPerPixel * whiteBox.slicePitch * 6u;  // Point at start of black

            VkBufferImageCopy region;
            memset( &region, 0, sizeof( region ) );

            region.bufferOffset = /*stagingTex->_getInternalBufferStart()*/ + bufStart;
            region.bufferRowLength = 0u;
            region.bufferImageHeight = 0u;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            if( i == TEX_TYPE_CUBE_MAP)
                region.imageSubresource.layerCount = 6u;
            else
                region.imageSubresource.layerCount = 1u;

            region.imageExtent.width = 4u;
            if( i == TEX_TYPE_3D )
                region.imageExtent.depth = 4u;
            else
                region.imageExtent.depth = 1u;
            if( i == TEX_TYPE_1D )
                region.imageExtent.height = 1u;
            else
                region.imageExtent.height = 4u;

            vkCmdCopyBufferToImage( device->mGraphicsQueue.mCurrentCmdBuffer, stagingBuffVboName,
                                    mBlankTexture[i].vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u,
                                    &region );
        }

        barrierCount = 0u;
        for( size_t i = 1u; i < TEX_TYPE_2D_ARRAY + 1u; ++i )
        {
            if( c_bSkipAliasable && ( TEX_TYPE_2D_ARRAY ) )
            {
                continue;
            }

            imageMemBarrier[barrierCount].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemBarrier[barrierCount].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageMemBarrier[barrierCount].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemBarrier[barrierCount].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            ++barrierCount;
        }

        vkCmdPipelineBarrier( device->mGraphicsQueue.mCurrentCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0u, 0, 0u, 0,
                              static_cast<uint32>( barrierCount ), imageMemBarrier );

        //mBlankTexture[TextureTypes::Unknown] = mBlankTexture[TextureTypes::Type2D];
        if( c_bSkipAliasable )
        {
            mBlankTexture[TEX_TYPE_2D_ARRAY] = mBlankTexture[TEX_TYPE_2D];
        }

        mBlankTexture[0].defaultView = 0;

        for( size_t i = 1u; i < TEX_TYPE_2D_ARRAY + 1u; ++i )
        {
            VkImageViewCreateInfo imageViewCi;
            makeVkStruct( imageViewCi, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO );

            imageViewCi.image = mBlankTexture[i].vkImage;
            imageViewCi.viewType = VulkanMappings::get( static_cast<TextureType>( i ) );
            imageViewCi.format = VK_FORMAT_R8G8B8A8_UNORM;

            imageViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCi.subresourceRange.baseMipLevel = 0u;
            imageViewCi.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            imageViewCi.subresourceRange.baseArrayLayer = 0U;
            imageViewCi.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

            VkResult result =
                vkCreateImageView( device->mDevice, &imageViewCi, 0, &mBlankTexture[i].defaultView );
            checkVkResult( result, "vkCreateImageView" );
        }

        // We will be releasing the staging texture memory immediately. We must flush out manually
        mDevice->commitAndNextCommandBuffer( SubmissionType::FlushOnly );
        vkDeviceWaitIdle( mDevice->mDevice );

        //vaoManager->destroyStagingTexture( stagingTex );
        delete stagingTex;
    }
    //-----------------------------------------------------------------------------------
    VulkanTextureGpuManager::~VulkanTextureGpuManager()
    {
        //destroyAll();

        for( size_t i = 1u; i < TEX_TYPE_2D_ARRAY+ 1u; ++i )
        {
            vkDestroyImageView( mDevice->mDevice, mBlankTexture[i].defaultView, 0 );
            mBlankTexture[i].defaultView = 0;

            if( c_bSkipAliasable && ( i == TEX_TYPE_2D_ARRAY ) )
            {
                mBlankTexture[i].vkImage = 0;
            }
            else
            {
                vkDestroyImage( mDevice->mDevice, mBlankTexture[i].vkImage, 0 );
                mBlankTexture[i].vkImage = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    Resource* VulkanTextureGpuManager::createImpl(const String& name, ResourceHandle handle,
                                                  const String& group, bool isManual,
                                                  ManualResourceLoader* loader,
                                                  const NameValuePairList* createParams)
    {
        return OGRE_NEW VulkanTextureGpu(this, name, handle, group);
    }
    //-----------------------------------------------------------------------------------
    VkImage VulkanTextureGpuManager::getBlankTextureVulkanName(
        TextureType textureType ) const
    {
        return mBlankTexture[textureType].vkImage;
    }
    //-----------------------------------------------------------------------------------
    VkImageView VulkanTextureGpuManager::getBlankTextureView(
        TextureType textureType ) const
    {
        return mBlankTexture[textureType].defaultView;
    }
#if 0
    //-----------------------------------------------------------------------------------
    TextureGpu *VulkanTextureGpuManager::createWindowDepthBuffer( void )
    {
        return OGRE_NEW VulkanTextureGpuRenderTarget( GpuPageOutStrategy::Discard, mVaoManager,
                                                      "RenderWindow DepthBuffer",          //
                                                      TextureFlags::NotTexture |           //
                                                          TextureFlags::RenderToTexture |  //
                                                          TextureFlags::RenderWindowSpecific |
                                                          TextureFlags::DiscardableContent,
                                                      TextureTypes::Type2D, this );
    }
    //-----------------------------------------------------------------------------------
    StagingTexture *VulkanTextureGpuManager::createStagingTextureImpl( uint32 width, uint32 height,
                                                                       uint32 depth, uint32 slices,
                                                                       PixelFormatGpu pixelFormat )
    {
        const uint32 rowAlignment = 4u;
        const size_t sizeBytes =
            PixelUtil::getSizeBytes( width, height, depth, slices, pixelFormat, rowAlignment );

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        VulkanStagingTexture *retVal =
            vaoManager->createStagingTexture( PixelUtil::getFamily( pixelFormat ), sizeBytes );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpuManager::destroyStagingTextureImpl( StagingTexture *stagingTexture )
    {
        OGRE_ASSERT_HIGH( dynamic_cast<VulkanStagingTexture *>( stagingTexture ) );

        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        vaoManager->destroyStagingTexture( static_cast<VulkanStagingTexture *>( stagingTexture ) );
    }
    //-----------------------------------------------------------------------------------
    AsyncTextureTicket *VulkanTextureGpuManager::createAsyncTextureTicketImpl(
        uint32 width, uint32 height, uint32 depthOrSlices, TextureType textureType,
        PixelFormatGpu pixelFormatFamily )
    {
        VulkanVaoManager *vaoManager = static_cast<VulkanVaoManager *>( mVaoManager );
        return OGRE_NEW VulkanAsyncTextureTicket( width, height, depthOrSlices, textureType,
                                                  pixelFormatFamily, vaoManager,
                                                  &mDevice->mGraphicsQueue );
    }
    //-----------------------------------------------------------------------------------
    VkImageView VulkanTextureGpuManager::createView( const DescriptorSetTexture2::TextureSlot &texSlot )
    {
        VkImageView retVal = 0;

        CachedTex2ImageViewMap::iterator itor = mCachedTex.find( texSlot );
        if( itor == mCachedTex.end() )
        {
            VulkanTextureGpu *vulkanTexture = static_cast<VulkanTextureGpu *>( texSlot.texture );
            retVal = vulkanTexture->createView( texSlot, false );
            CachedView cachedView;
            cachedView.refCount = 1u;
            cachedView.imageView = retVal;
            mCachedTex.insert( CachedTex2ImageViewMap::value_type( texSlot, cachedView ) );
        }
        else
        {
            ++itor->second.refCount;
            retVal = itor->second.imageView;
        }
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpuManager::destroyView( DescriptorSetTexture2::TextureSlot texSlot,
                                               VkImageView imageView )
    {
        CachedTex2ImageViewMap::iterator itor = mCachedTex.find( texSlot );

        OGRE_ASSERT_MEDIUM(
            itor != mCachedTex.end() &&
            "Did you const_cast DescriptorSetUav, modify it, and destroy it? Double free perhaps?" );
        OGRE_ASSERT_MEDIUM(
            itor->second.imageView == imageView &&
            "Did you const_cast DescriptorSetUav, modify it, and destroy it? Double free perhaps?" );

        if( itor != mCachedTex.end() )
        {
            OGRE_ASSERT_MEDIUM( itor->second.refCount > 0u );
            --itor->second.refCount;
            if( itor->second.refCount == 0u )
            {
                mCachedTex.erase( itor );
                delayed_vkDestroyImageView( mVaoManager, mDevice->mDevice, imageView, 0 );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    VkImageView VulkanTextureGpuManager::createView( const DescriptorSetUav::TextureSlot &texSlot )
    {
        VkImageView retVal = 0;

        CachedUavImageViewMap::iterator itor = mCachedUavs.find( texSlot );
        if( itor == mCachedUavs.end() )
        {
            VulkanTextureGpu *vulkanTexture = static_cast<VulkanTextureGpu *>( texSlot.texture );
            retVal = vulkanTexture->createView( texSlot, false );
            CachedView cachedView;
            cachedView.refCount = 1u;
            cachedView.imageView = retVal;
            mCachedUavs.insert( CachedUavImageViewMap::value_type( texSlot, cachedView ) );
        }
        else
        {
            ++itor->second.refCount;
            retVal = itor->second.imageView;
        }
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void VulkanTextureGpuManager::destroyView( DescriptorSetUav::TextureSlot texSlot,
                                               VkImageView imageView )
    {
        CachedUavImageViewMap::iterator itor = mCachedUavs.find( texSlot );

        OGRE_ASSERT_MEDIUM(
            itor != mCachedUavs.end() &&
            "Did you const_cast DescriptorSetUav, modify it, and destroy it? Double free perhaps?" );
        OGRE_ASSERT_MEDIUM(
            itor->second.imageView == imageView &&
            "Did you const_cast DescriptorSetUav, modify it, and destroy it? Double free perhaps?" );

        if( itor != mCachedUavs.end() )
        {
            OGRE_ASSERT_MEDIUM( itor->second.refCount > 0u );
            --itor->second.refCount;
            if( itor->second.refCount == 0u )
            {
                mCachedUavs.erase( itor );
                delayed_vkDestroyImageView( mVaoManager, mDevice->mDevice, imageView, 0 );
            }
        }
    }
#endif
    //-----------------------------------------------------------------------------------
    bool VulkanTextureGpuManager::checkSupport( PixelFormatGpu format, uint32 textureFlags ) const
    {
        OGRE_ASSERT_LOW(
            textureFlags != TU_NOT_SRV &&
            "Invalid textureFlags combination. Asking to check if format is supported to do nothing" );

        const VkFormat vkFormat = VulkanMappings::get( format );

        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties( mDevice->mPhysicalDevice, vkFormat, &props );

        uint32 features = 0;

        if( !( textureFlags & TU_NOT_SRV ) )
            features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

        if( textureFlags & TU_UAV )
            features |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

        if( textureFlags & TU_RENDERTARGET )
        {
            if( PixelUtil::isDepth( format ) )
            {
                features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            else
            {
                features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
                            VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
            }
        }

        if( textureFlags & TU_AUTOMIPMAP )
        {
            features |= VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
            //if( !PixelUtil::supportsHwMipmaps( format ) )
                return false;
        }

        if( ( props.optimalTilingFeatures & features ) == features )
            return true;

#ifdef OGRE_VK_WORKAROUND_ADRENO_5XX_6XX_MINCAPS
        if( Workarounds::mAdreno5xx6xxMinCaps &&
            ( textureFlags & ( TextureFlags::Uav | TextureFlags::AllowAutomipmaps ) ) == 0u )
        {
            switch( format )
            {
            case PFG_R16_UNORM:
            case PFG_R16_SNORM:
            case PFG_RG16_UNORM:
            case PFG_RG16_SNORM:
            case PFG_RGBA16_UNORM:
            case PFG_RGBA16_SNORM:
                return true;
            default:
                break;
            }
        }
#endif

        return false;
    }
}  // namespace Ogre
