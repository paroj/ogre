/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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

#ifndef _OgreVulkanRenderSystem_H_
#define _OgreVulkanRenderSystem_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreRenderSystem.h"
#include "OgreVulkanGlobalBindingTable.h"
#include "OgreVulkanProgram.h"

#include "OgreVulkanRenderPassDescriptor.h"

namespace Ogre
{
    class HardwareBufferManager;
    struct VulkanHlmsPso;
    class VulkanSupport;

    /**
       Implementation of Vulkan as a rendering system.
    */
    class _OgreVulkanExport VulkanRenderSystem : public RenderSystem
    {
        bool mInitialized;
        HardwareBufferManager *mHardwareBufferManager;

        VkBuffer mIndirectBuffer;
        unsigned char *mSwIndirectBufferPtr;

        VulkanProgramFactory *mVulkanProgramFactory0;
        VulkanProgramFactory *mVulkanProgramFactory1;
        VulkanProgramFactory *mVulkanProgramFactory2;
        VulkanProgramFactory *mVulkanProgramFactory3;

        VkInstance mVkInstance;
        VulkanSupport *mVulkanSupport;

        // TODO: AutoParamsBuffer probably belongs to MetalDevice (because it's per device?)
        std::unique_ptr<v1::VulkanHardwareBufferCommon> mAutoParamsBuffer;
        size_t mAutoParamsBufferIdx;
        uint8 *mCurrentAutoParamsBufferPtr;
        size_t mCurrentAutoParamsBufferSpaceLeft;
        size_t mHistoricalAutoParamsSize[60];

        // For v1 rendering.
        IndexData *mCurrentIndexBuffer;
        VertexData *mCurrentVertexBuffer;
        VkPrimitiveTopology mCurrentPrimType;

        VulkanDevice *mActiveDevice;

        VulkanDevice *mDevice;

        VulkanCache *mCache;

        VulkanRenderPassDescriptor    *mCurrentRenderPassDescriptor;

        uint32_t mStencilRefValue;
        bool mStencilEnabled;

        bool mTableDirty;
        bool mComputeTableDirty;
        VulkanGlobalBindingTable mGlobalTable;
        VulkanGlobalBindingTable mComputeTable;
        // Vulkan requires a valid handle when updating descriptors unless nullDescriptor is present
        // So we just use a dummy. The dummy texture we get it from TextureGpuManager which needs
        // to create some anyway for different reasons
        v1::VulkanHardwareBufferCommon *mDummyBuffer;
        v1::VulkanHardwareBufferCommon *mDummyTexBuffer;
        VkImageView mDummyTextureView;
        VkSampler mDummySampler;

        // clang-format off
        VulkanFrameBufferDescMap    mFrameBufferDescMap;
        VulkanFlushOnlyDescMap      mFlushOnlyDescMap;
        uint32                      mEntriesToFlush;
        bool                        mVpChanged;
        bool                        mInterruptedRenderCommandEncoder;
        // clang-format on

        bool mValidationError;
        bool mHasValidationLayers;

        typedef std::set<VulkanRenderPassDescriptor*> RenderPassDescriptorSet;
        RenderPassDescriptorSet mRenderPassDescs;

        PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
        PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
        VkDebugReportCallbackEXT mDebugReportCallback;

        /// Declared here to avoid constant reallocations
        FastArray<VkImageMemoryBarrier> mImageBarriers;

        void addInstanceDebugCallback( void );

        void bindDescriptorSet() const;

        void flushRootLayout( void );
        void flushRootLayoutCS( void );

    public:
        VulkanRenderSystem();
        ~VulkanRenderSystem();

        virtual void shutdown( void );

        virtual const String &getName( void ) const;
        virtual const String &getFriendlyName( void ) const;
        void refreshConfig();
        void initConfigOptions();
        virtual void setConfigOption( const String &name, const String &value );

        virtual HardwareOcclusionQuery *createHardwareOcclusionQuery( void );

        virtual String validateConfigOptions( void );

        virtual RenderSystemCapabilities *createRenderSystemCapabilities( void ) const;

        void resetAllBindings( void );

        void initializeVkInstance( void );

        VkInstance getVkInstance( void ) const { return mVkInstance; }

        virtual RenderWindow *_createRenderWindow( const String &name, uint32 width, uint32 height,
                                             bool fullScreen, const NameValuePairList *miscParams = 0 );

        virtual String getErrorDescription( long errorNumber ) const;

        virtual void _useLights( const LightList &lights, unsigned short limit );
        virtual void _setWorldMatrix( const Matrix4 &m );
        virtual void _setViewMatrix( const Matrix4 &m );
        virtual void _setProjectionMatrix( const Matrix4 &m );

        virtual void _setSurfaceParams( const ColourValue &ambient, const ColourValue &diffuse,
                                        const ColourValue &specular, const ColourValue &emissive,
                                        Real shininess, TrackVertexColourType tracking = TVC_NONE );
        virtual void _setPointSpritesEnabled( bool enabled );
        virtual void _setPointParameters( Real size, bool attenuationEnabled, Real constant, Real linear,
                                          Real quadratic, Real minSize, Real maxSize );

        void flushUAVs( void );

        void _setParamBuffer( GpuProgramType shaderStage, const VkDescriptorBufferInfo &bufferInfo );
        void _setConstBuffer( size_t slot, const VkDescriptorBufferInfo &bufferInfo );
        void _setConstBufferCS( size_t slot, const VkDescriptorBufferInfo &bufferInfo );
        void _setTexBuffer( size_t slot, VkBufferView bufferView );
        void _setTexBufferCS( size_t slot, VkBufferView bufferView );
        void _setReadOnlyBuffer( size_t slot, const VkDescriptorBufferInfo &bufferInfo );

        virtual void _setCurrentDeviceFromTexture( Texture *texture );
        virtual void _setTexture( size_t unit, bool enabled, const TexturePtr& texPtr );

        virtual void _setTextureCoordCalculation( size_t unit, TexCoordCalcMethod m,
                                                  const Frustum *frustum = 0 );
        virtual void _setTextureBlendMode( size_t unit, const LayerBlendModeEx &bm );
        virtual void _setTextureMatrix( size_t unit, const Matrix4 &xform );

        virtual VulkanFrameBufferDescMap &_getFrameBufferDescMap( void ) { return mFrameBufferDescMap; }
        virtual VulkanFlushOnlyDescMap &_getFlushOnlyDescMap( void ) { return mFlushOnlyDescMap; }
        virtual RenderPassDescriptor *createRenderPassDescriptor( void );

        virtual void setStencilBufferParams( uint32 refValue, const StencilState &stencilParams );

        virtual void _beginFrame( void );
        virtual void _endFrame( void );

        void bindDescriptorSet( VulkanVaoManager *&vaoManager );


        virtual void _render( const RenderOperation &op );

        virtual void bindGpuProgramParameters( GpuProgramType gptype,
                                               const GpuProgramParametersPtr& params,
                                               uint16 variabilityMask );
        virtual void bindGpuProgramPassIterationParameters( GpuProgramType gptype );

        virtual void clearFrameBuffer( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTarget,
                                       uint8 mipLevel );

        virtual Real getHorizontalTexelOffset( void );
        virtual Real getVerticalTexelOffset( void );
        virtual Real getMinimumDepthInputValue( void );
        virtual Real getMaximumDepthInputValue( void );

        virtual void preExtraThreadsStarted();
        virtual void postExtraThreadsStarted();
        virtual void registerThread();
        virtual void unregisterThread();
        virtual unsigned int getDisplayMonitorCount() const { return 1; }

        virtual void flushCommands( void );

        virtual void beginProfileEvent( const String &eventName );
        virtual void endProfileEvent( void );
        virtual void markProfileEvent( const String &event );

        virtual void initGPUProfiling( void );
        virtual void deinitGPUProfiling( void );
        virtual void beginGPUSampleProfile( const String &name, uint32 *hashCache );
        virtual void endGPUSampleProfile( const String &name );

        virtual bool hasAnisotropicMipMapFilter() const { return true; }

        virtual void setClipPlanesImpl( const PlaneList &clipPlanes );
        virtual void initialiseFromRenderSystemCapabilities( RenderSystemCapabilities *caps,
                                                             RenderTarget *primary );

        virtual void beginRenderPassDescriptor( RenderPassDescriptor *desc, TextureGpu *anyTarget,
                                                uint8 mipLevel, const Vector4 *viewportSizes,
                                                const Vector4 *scissors, uint32 numViewports,
                                                bool overlaysEnabled, bool warnIfRtvWasFlushed );
        void executeRenderPassDescriptorDelayedActions( bool officialCall );
        virtual void executeRenderPassDescriptorDelayedActions( void );
        inline void endRenderPassDescriptor( bool isInterruptingRender );
        virtual void endRenderPassDescriptor( void );

        DepthBuffer *_createDepthBufferFor( RenderTarget* renderTarget);

        void notifySwapchainCreated( VulkanWindow *window );
        void notifySwapchainDestroyed( VulkanWindow *window );

        virtual void flushPendingAutoResourceLayouts( void );
        //virtual void executeResourceTransition( const ResourceTransitionArray &rstCollection );

        VulkanDevice *getVulkanDevice() const { return mDevice; }
        void _notifyDeviceStalled();

        void _notifyActiveEncoderEnded( bool callEndRenderPassDesc );
        void _notifyActiveComputeEnded( void );

        void debugCallback( void );

        virtual bool isSameLayout( ResourceLayout::Layout a, ResourceLayout::Layout b,
                                   const TextureGpu *texture, bool bIsDebugCheck ) const;

        void _setViewport(Viewport *vp);
        void _setRenderTarget(RenderTarget *target);
        void clearFrameBuffer(unsigned int buffers, const ColourValue& colour = ColourValue::Black,
                              Real depth = 1.0f, unsigned short stencil = 0);
        // TODO:
        void setScissorTest(bool enabled, const Rect& rect = Rect()) {}
        void setStencilState(const StencilState& state) {}
        void _setPolygonMode(PolygonMode level) {}
        void _convertProjectionMatrix(const Matrix4& matrix,
            Matrix4& dest, bool forGpuProgram = false) {dest = matrix;}
        void _setDepthBias(float constantBias, float slopeScaleBias = 0.0f) {}
        void _setDepthBufferParams(bool depthTest, bool depthWrite, CompareFunction depthFunction) {}
        void _setCullingMode(CullingMode mode) {}
        void _setAlphaRejectSettings(CompareFunction func, unsigned char value, bool alphaToCoverage) {}
        void setColourBlendState(const ColourBlendState& state) {}
        void _setSampler(size_t texUnit, Sampler& s) {}
        MultiRenderTarget * createMultiRenderTarget(const String & name) { return NULL; }
    };
}  // namespace Ogre

#endif
