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
#include "OgreVulkanProgram.h"

#include "OgreLogManager.h"
#include "OgreProfiler.h"
#include "OgreVulkanDevice.h"
#include "OgreVulkanMappings.h"
#include "OgreVulkanRootLayout.h"

#include "OgreStringConverter.h"
#include "OgreVulkanUtils.h"

#include "OgreRenderSystemCapabilities.h"
#include "OgreRoot.h"

#define OgreProfileExhaustive OgreProfile

namespace Ogre
{
    //-----------------------------------------------------------------------
    VulkanProgram::VulkanProgram( ResourceManager *creator, const String &name, ResourceHandle handle,
                                  const String &group, bool isManual, ManualResourceLoader *loader,
                                  VulkanDevice *device, String &languageName ) :
        HighLevelGpuProgram( creator, name, handle, group, isManual, loader ),
        mDevice( device ),
        mRootLayout( 0 ),
        mShaderModule( 0 ),
        mNumSystemGenVertexInputs( 0u ),
        mCustomRootLayout( false ),
        mConstantsBytesToWrite( 0 )
    {
        if( createParamDictionary( "VulkanProgram" ) )
        {
            setupBaseParamDictionary();
        }

        // Manually assign language now since we use it immediately
        mSyntaxCode = languageName;
        mDrawIdLocation = /*( mShaderSyntax == GLSL ) */ true ? 15 : 0;
    }
    //---------------------------------------------------------------------------
    VulkanProgram::~VulkanProgram()
    {
        // Have to call this here reather than in Resource destructor
        // since calling virtual methods in base destructors causes crash
        if( isLoaded() )
        {
            unload();
        }
        else
        {
            unloadHighLevel();
        }
    }
    //-----------------------------------------------------------------------
    void VulkanProgram::extractRootLayoutFromSource( void )
    {
        if( mRootLayout )
            return;  // Programmatically specified

        const String rootLayoutHeader = "## ROOT LAYOUT BEGIN";
        const String rootLayoutFooter = "## ROOT LAYOUT END";

        size_t posStart = mSource.find( rootLayoutHeader );
        if( posStart == String::npos )
        {
            LogManager::getSingleton().logMessage( "Error " + mName + ": failed to find required '" +
                                                   rootLayoutHeader + "'" );
            mCompileError = true;
            return;
        }
        posStart += rootLayoutHeader.size();
        const size_t posEnd = mSource.find( "## ROOT LAYOUT END", posStart );
        if( posEnd == String::npos )
        {
            LogManager::getSingleton().logMessage( "Error " + mName + ": failed to find required '" +
                                                   rootLayoutFooter + "'" );
            mCompileError = true;
            return;
        }

        /*VulkanGpuProgramManager *vulkanProgramManager =
            static_cast<VulkanGpuProgramManager *>( VulkanGpuProgramManager::getSingletonPtr() );
        mRootLayout = vulkanProgramManager->getRootLayout(
            mSource.substr( posStart, posEnd - posStart ).c_str(), mType == GPT_COMPUTE_PROGRAM, mName );*/
    }
    //-----------------------------------------------------------------------
    void VulkanProgram::setRootLayout( GpuProgramType type, const RootLayout &rootLayout )
    {
        mCustomRootLayout = true;
        //HighLevelGpuProgram::setRootLayout( type, rootLayout );

        /*VulkanGpuProgramManager *vulkanProgramManager =
            static_cast<VulkanGpuProgramManager *>( VulkanGpuProgramManager::getSingletonPtr() );
        mRootLayout = vulkanProgramManager->getRootLayout( rootLayout );*/
    }
    //-----------------------------------------------------------------------
    void VulkanProgram::unsetRootLayout( void )
    {
        mRootLayout = 0;
        mCustomRootLayout = false;
    }
    //-----------------------------------------------------------------------
    void VulkanProgram::debugDump( String &outString )
    {
        outString += mName;
        outString += "\n";
        //mRootLayout->dump( outString );
        outString += "\n";
        outString += "\n";
        outString += mSource;
    }
    //-----------------------------------------------------------------------
    void VulkanProgram::loadFromSource()
    {
        mCompileError = false;

        extractRootLayoutFromSource();

        VkShaderModuleCreateInfo moduleCi;
        makeVkStruct( moduleCi, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO );
        moduleCi.codeSize = mSource.size();
        moduleCi.pCode = (uint32*)mSource.data();
        VkResult result = vkCreateShaderModule( mDevice->mDevice, &moduleCi, 0, &mShaderModule );
        checkVkResult( result, "vkCreateShaderModule" );

        setObjectName( mDevice->mDevice, (uint64_t)mShaderModule,
                        VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, mName.c_str() );
    }
    //---------------------------------------------------------------------------
    void VulkanProgram::unloadImpl()
    {
        unloadHighLevelImpl();

        if( !mCustomRootLayout )
            mRootLayout = 0;
    }
    //-----------------------------------------------------------------------
    void VulkanProgram::unloadHighLevelImpl( void )
    {
        if( mShaderModule )
        {
            vkDestroyShaderModule( mDevice->mDevice, mShaderModule, 0 );
            mShaderModule = 0;
        }
    }
    //-----------------------------------------------------------------------
    static VkShaderStageFlagBits get( GpuProgramType programType )
    {
        switch( programType )
        {
        // clang-format off
        case GPT_VERTEX_PROGRAM:    return VK_SHADER_STAGE_VERTEX_BIT;
        case GPT_FRAGMENT_PROGRAM:  return VK_SHADER_STAGE_FRAGMENT_BIT;
        case GPT_GEOMETRY_PROGRAM:  return VK_SHADER_STAGE_GEOMETRY_BIT;
        case GPT_HULL_PROGRAM:      return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case GPT_DOMAIN_PROGRAM:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case GPT_COMPUTE_PROGRAM:   return VK_SHADER_STAGE_COMPUTE_BIT;
            // clang-format on
        }
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    void VulkanProgram::fillPipelineShaderStageCi( VkPipelineShaderStageCreateInfo &pssCi )
    {
        makeVkStruct( pssCi, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
        pssCi.stage = get( mType );
        pssCi.module = mShaderModule;
        pssCi.pName = "main";
    }
    //-----------------------------------------------------------------------
    uint32 VulkanProgram::getBufferRequiredSize( void ) const { return mConstantsBytesToWrite; }
    //-----------------------------------------------------------------------
    void VulkanProgram::updateBuffers( const GpuProgramParametersSharedPtr &params,
                                       uint8 *RESTRICT_ALIAS dstData )
    {
        vector<GpuConstantDefinition>::type::const_iterator itor = mConstantDefsSorted.begin();
        vector<GpuConstantDefinition>::type::const_iterator endt = mConstantDefsSorted.end();

        while( itor != endt )
        {
            const GpuConstantDefinition &def = *itor;

            void *RESTRICT_ALIAS src;
            src = (void *)&( *( params->getConstantList().begin() + def.physicalIndex ) );

            memcpy( &dstData[def.logicalIndex], src, def.elementSize * def.arraySize * sizeof( float ) );

            ++itor;
        }
    }
#if 0
    //---------------------------------------------------------------------
    void VulkanProgram::getLayoutForPso(
        const VertexElement2VecVec &vertexElements,
        FastArray<VkVertexInputBindingDescription> &outBufferBindingDescs,
        FastArray<VkVertexInputAttributeDescription> &outVertexInputs )
    {
        OgreProfileExhaustive( "VulkanProgram::getLayoutForPso" );

        outBufferBindingDescs.reserve( vertexElements.size() + 1u );  // +1 due to DRAWID
        outVertexInputs.reserve( mVertexInputs.size() );

        const size_t numShaderInputs = mVertexInputs.size();
        size_t numShaderInputsFound = mNumSystemGenVertexInputs;

        size_t uvCount = 0;

        // Iterate through the vertexElements and see what is actually used by the shader
        const size_t vertexElementsSize = vertexElements.size();
        for( size_t bufferIdx = 0; bufferIdx < vertexElementsSize; ++bufferIdx )
        {
            VertexElement2Vec::const_iterator it = vertexElements[bufferIdx].begin();
            VertexElement2Vec::const_iterator en = vertexElements[bufferIdx].end();

            VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_MAX_ENUM;

            uint32 bindAccumOffset = 0u;

            while( it != en )
            {
                size_t locationIdx = VulkanVaoManager::getAttributeIndexFor( it->mSemantic );

                if( it->mSemantic == VES_TEXTURE_COORDINATES )
                    locationIdx += uvCount++;

                // In GLSL locationIdx == itor->second.location as they're hardcoded
                // However in HLSL these values are generated by the shader and may not match
                VertexInputByLocationIdxMap::const_iterator itor =
                    mVertexInputs.find( static_cast<uint32>( locationIdx ) );

                if( itor != mVertexInputs.end() )
                {
                    if( it->mInstancingStepRate > 1u )
                    {
                        OGRE_EXCEPT(
                            Exception::ERR_RENDERINGAPI_ERROR,
                            "Shader: '" + mName + "' Vulkan only supports mInstancingStepRate = 0 or 1 ",
                            "VulkanProgram::getLayoutForPso" );
                    }
                    else if( inputRate == VK_VERTEX_INPUT_RATE_MAX_ENUM )
                    {
                        if( it->mInstancingStepRate == 0u )
                            inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                        else
                            inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                    }
                    else if( ( it->mInstancingStepRate == 0u &&
                               inputRate != VK_VERTEX_INPUT_RATE_VERTEX ) ||
                             ( it->mInstancingStepRate == 1u &&
                               inputRate != VK_VERTEX_INPUT_RATE_INSTANCE ) )
                    {
                        OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                                     "Shader: '" + mName +
                                         "' can only have all-instancing or all-vertex rate semantics "
                                         "for the same vertex buffer, but it is mixing vertex and "
                                         "instancing semantics for the same buffer idx",
                                     "VulkanProgram::getLayoutForPso" );
                    }

                    outVertexInputs.push_back( itor->second );
                    VkVertexInputAttributeDescription &inputDesc = outVertexInputs.back();
                    inputDesc.format = VulkanMappings::get( it->mType );
                    inputDesc.binding = static_cast<uint32_t>( bufferIdx );
                    inputDesc.offset = bindAccumOffset;

                    ++numShaderInputsFound;
                }

                bindAccumOffset += VertexElement::getTypeSize( it->mType );
                ++it;
            }

            if( inputRate != VK_VERTEX_INPUT_RATE_MAX_ENUM )
            {
                // Only bind this buffer's entry if it's actually used by the shader
                VkVertexInputBindingDescription bindingDesc;
                bindingDesc.binding = static_cast<uint32_t>( bufferIdx );
                bindingDesc.stride = bindAccumOffset;
                bindingDesc.inputRate = inputRate;
                outBufferBindingDescs.push_back( bindingDesc );
            }
        }

        // Check if DRAWID is being used
        {
            const size_t locationIdx = 15u;
            VertexInputByLocationIdxMap::const_iterator itor = mVertexInputs.find( locationIdx );

            if( itor != mVertexInputs.end() )
            {
                outVertexInputs.push_back( itor->second );
                VkVertexInputAttributeDescription &inputDesc = outVertexInputs.back();
                inputDesc.format = VK_FORMAT_R32_UINT;
                inputDesc.binding = static_cast<uint32>( locationIdx );
                inputDesc.offset = 0u;

                ++numShaderInputsFound;

                VkVertexInputBindingDescription bindingDesc;
                bindingDesc.binding = static_cast<uint32>( locationIdx );
                bindingDesc.stride = 4u;
                bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                outBufferBindingDescs.push_back( bindingDesc );
            }
        }

        if( numShaderInputsFound < numShaderInputs )
        {
            OGRE_EXCEPT( Exception::ERR_RENDERINGAPI_ERROR,
                         "The shader requires more input attributes/semantics than what the "
                         "VertexArrayObject / v1::VertexDeclaration has to offer. You're "
                         "missing a component",
                         "VulkanProgram::getLayoutForPso" );
        }
    }
#endif
    //-----------------------------------------------------------------------
    const String &VulkanProgram::getLanguage( void ) const
    {
        static const String language = "spirv";
        return language;
    }
    //-----------------------------------------------------------------------
    GpuProgramParametersSharedPtr VulkanProgram::createParameters( void )
    {
        GpuProgramParametersSharedPtr params = HighLevelGpuProgram::createParameters();
        params->setTransposeMatrices( true );
        return params;
    }
    //-----------------------------------------------------------------------
}  // namespace Ogre
