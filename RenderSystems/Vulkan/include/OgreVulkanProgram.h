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
#ifndef _OgreVulkanProgram_H_
#define _OgreVulkanProgram_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreHardwareVertexBuffer.h"
#include "OgreHighLevelGpuProgram.h"

struct VkVertexInputBindingDescription;
struct VkVertexInputAttributeDescription;

namespace Ogre
{
    struct _OgreVulkanExport VulkanConstantDefinitionBindingParam
    {
        size_t offset;
        size_t size;
    };

    /** Specialisation of GpuProgram to provide support for Vulkan
        Shader Language.
    @remarks
        Vulkan has no target assembler or entry point specification like DirectX 9 HLSL.
        Vertex and Fragment shaders only have one entry point called "main".
        When a shader is compiled, microcode is generated but can not be accessed by
        the application.
        Vulkan also does not provide assembler low level output after compiling.  The Vulkan Render
        system assumes that the Gpu program is a Vulkan Gpu program so VulkanProgram will create a
        VulkanGpuProgram that is subclassed from VulkanGpuProgram for the low level implementation.
        The VulkanProgram class will create a shader object and compile the source but will
        not create a program object.  It's up to VulkanGpuProgram class to request a program object
        to link the shader object to.
    */
    class _OgreVulkanExport VulkanProgram : public HighLevelGpuProgram
    {
    public:
        VulkanProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                       const String &group, bool isManual, ManualResourceLoader *loader,
                       VulkanDevice *device, String &languageName );
        virtual ~VulkanProgram();

        virtual void setRootLayout( GpuProgramType type, const RootLayout &rootLayout );
        virtual void unsetRootLayout( void );

        /// Overridden from GpuProgram
        const String &getLanguage( void ) const;
        /// Overridden from GpuProgram
        GpuProgramParametersSharedPtr createParameters( void );

        void debugDump( String &outString );

        void fillPipelineShaderStageCi( VkPipelineShaderStageCreateInfo &pssCi );

        /// In bytes.
        uint32 getBufferRequiredSize( void ) const;
        /// dstData must be able to hold at least getBufferRequiredSize
        void updateBuffers( const GpuProgramParametersSharedPtr &params, uint8 *RESTRICT_ALIAS dstData );

        const std::map<uint32, VulkanConstantDefinitionBindingParam> &getConstantDefsBindingParams()
            const
        {
            return mConstantDefsBindingParams;
        }

        VulkanRootLayout *getRootLayout( void ) { return mRootLayout; }

        void getLayoutForPso( const VertexDeclaration &vertexElements,
                              FastArray<VkVertexInputBindingDescription> &outBufferBindingDescs,
                              FastArray<VkVertexInputAttributeDescription> &outVertexInputs );

        uint32 getDrawIdLocation() const { return mDrawIdLocation; }

    protected:
        void extractRootLayoutFromSource( void );

        /** Internal load implementation, must be implemented by subclasses.
         */
        void loadFromSource( void );

        /// Internal unload implementation, must be implemented by subclasses
        void unloadHighLevelImpl( void );
        /// Overridden from HighLevelGpuProgram
        void unloadImpl( void );

    private:
        VulkanDevice *mDevice;

        VulkanRootLayout *mRootLayout;

        VkShaderModule mShaderModule;

        typedef std::map<uint32, VkVertexInputAttributeDescription> VertexInputByLocationIdxMap;
        VertexInputByLocationIdxMap mVertexInputs;
        uint8 mNumSystemGenVertexInputs;  // System-generated inputs like gl_VertexIndex

        bool mCustomRootLayout;

        std::vector<GpuConstantDefinition> mConstantDefsSorted;
        std::map<uint32, VulkanConstantDefinitionBindingParam> mConstantDefsBindingParams;
        uint32 mConstantsBytesToWrite;

        uint32 mDrawIdLocation;
    };
}  // namespace Ogre

#endif  // __VulkanProgram_H__
