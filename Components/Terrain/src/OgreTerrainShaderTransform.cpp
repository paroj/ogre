// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
#include "OgreTerrainShaderTransform.h"

#include "OgrePass.h"
#include "OgreShaderProgramSet.h"
#include "OgreShaderParameter.h"
#include "OgreMaterialSerializer.h"
#include "OgreShaderGenerator.h"
#include "OgreShaderFunction.h"
#include "OgreShaderProgram.h"
#include "OgreTerrainQuadTreeNode.h"

namespace Ogre {
using namespace RTShader;

/************************************************************************/
/*                                                                      */
/************************************************************************/
String TerrainTransform::Type = "TerrainTransform";

bool TerrainTransform::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass)
{
    auto terrainAny = srcPass->getUserObjectBindings().getUserAny("Terrain");
    if (terrainAny.has_value())
    {
        auto terrain = any_cast<const Terrain*>(terrainAny);
        mCompressed = terrain->_getUseVertexCompression();
        mAlign = terrain->getAlignment();
    }

    return true;
}

void TerrainTransform::updateGpuProgramsParams(Renderable* rend, const Pass* pass,
                                               const AutoParamDataSource* source,
                                               const LightList* pLightList)
{
    if(!mPointTrans)
        return;

    auto qtn = static_cast<TerrainQuadTreeNode*>(rend); // cant dynamic_cast due to private inheritance
    auto terrain = qtn->getTerrain();
    mPointTrans->setGpuParameter(terrain->getPointTransform());

    float baseUVScale = 1.0f / (terrain->getSize() - 1);
    mBaseUVScale->setGpuParameter(baseUVScale);
}

//-----------------------------------------------------------------------
bool TerrainTransform::createCpuSubPrograms(ProgramSet* programSet)
{
    static Operand::OpMask heightAxis[3] = {Operand::OPM_Y, Operand::OPM_Z, Operand::OPM_X};

    Program* vsProgram = programSet->getCpuProgram(GPT_VERTEX_PROGRAM);
    Function* vsEntry = vsProgram->getEntryPointFunction();

    auto posType = mCompressed ? GCT_INT2 : GCT_FLOAT4;

    auto wvpMatrix = vsProgram->resolveParameter(GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);

    auto lodMorph = vsProgram->resolveAutoParameterInt(GpuProgramParameters::ACT_CUSTOM, GCT_FLOAT2, Terrain::LOD_MORPH_CUSTOM_PARAM);

    auto positionIn = vsEntry->resolveInputParameter(Parameter::SPC_POSITION_OBJECT_SPACE, posType);
    auto positionOut = vsEntry->resolveOutputParameter(Parameter::SPC_POSITION_PROJECTIVE_SPACE);

    auto delta = vsEntry->resolveInputParameter(Parameter::SPC_TEXTURE_COORDINATE1, GCT_FLOAT2);

    // Add dependency.
    vsProgram->addDependency("FFPLib_Transform");
    vsProgram->addDependency("TerrainTransforms");

    auto stage = vsEntry->getStage(FFP_VS_TRANSFORM);

    if(mCompressed)
    {
        mPointTrans = vsProgram->resolveParameter(GCT_MATRIX_4X4, "pointTrans");
        mBaseUVScale = vsProgram->resolveParameter(GCT_FLOAT1, "baseUVScale");
        auto height = vsEntry->resolveInputParameter(Parameter::SPC_TEXTURE_COORDINATE0, GCT_FLOAT1);
        auto uv = vsEntry->resolveOutputParameter(Parameter::SPC_TEXTURE_COORDINATE0, GCT_FLOAT2);
        auto position = vsEntry->resolveLocalParameter(Parameter::SPC_POSITION_OBJECT_SPACE);

        stage.callFunction("expandVertex",
                       {In(mPointTrans), In(mBaseUVScale), In(positionIn), In(height), Out(position), Out(uv)});
        positionIn = position;
    }

    stage.callFunction("FFP_Transform", wvpMatrix, positionIn, positionOut);
    stage.callFunction("applyLODMorph",
                       {In(delta), In(lodMorph), InOut(positionOut).mask(heightAxis[mAlign])});

    return true;
}

String TerrainSurface::Type = "TerrainSurface";

bool TerrainSurface::preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass)
{
    mParent = any_cast<const Terrain*>(srcPass->getUserObjectBindings().getUserAny("Terrain"));

    srcPass->createTextureUnitState(mParent->getLayerTextureName(0, 0));

    mUseSpecularMapping = true;

    if(mUseSpecularMapping)
    {
        // we use that to inject our specular map
        srcPass->setVertexColourTracking(TVC_SPECULAR);
        srcPass->setSpecular(ColourValue::White);
    }

    auto tu = dstPass->createTextureUnitState();
    tu->setTexture(mParent->getTerrainNormalMap());
    tu->setTextureAddressingMode(TAM_CLAMP);

    if(auto lm = mParent->getLightmap())
    {
        tu = dstPass->createTextureUnitState();
        tu->setTexture(lm);
        tu->setTextureAddressingMode(TAM_CLAMP);
    }

    return true;
}

void TerrainSurface::updateGpuProgramsParams(Renderable* rend, const Pass* pass,
                                               const AutoParamDataSource* source,
                                               const LightList* pLightList)
{
    auto terrain = any_cast<const Terrain*>(pass->getUserObjectBindings().getUserAny("Terrain"));
    mUVMul->setGpuParameter(terrain->getLayerUVMultiplier(0));
}

//-----------------------------------------------------------------------
bool TerrainSurface::createCpuSubPrograms(ProgramSet* programSet)
{
    Program* vsProgram = programSet->getCpuProgram(GPT_VERTEX_PROGRAM);
    Program* psProgram = programSet->getCpuProgram(GPT_FRAGMENT_PROGRAM);
    Function* psMain = psProgram->getMain();

    psProgram->addDependency("FFPLib_Transform");
    psProgram->addDependency("SGXLib_NormalMap");

    auto uvVS = vsProgram->getMain()->resolveOutputParameter(Parameter::SPC_TEXTURE_COORDINATE0, GCT_FLOAT2);
    auto uvPS = psMain->resolveInputParameter(uvVS);

    mUVMul = psProgram->resolveParameter(GCT_FLOAT1, "uvMul");

    auto sampler = psProgram->resolveParameter(GCT_SAMPLER2D, "sampler", 0);
    auto globalNormal = psProgram->resolveParameter(GCT_SAMPLER2D, "globalNormal", 1);
    auto lightMap = psProgram->resolveParameter(GCT_SAMPLER2D, "lightMap", 2);

    auto normal = psMain->resolveLocalParameter(Parameter::SPC_NORMAL_VIEW_SPACE);
    auto ITMat = psProgram->resolveParameter(GpuProgramParameters::ACT_NORMAL_MATRIX);

    auto diffuse = psMain->resolveLocalParameter(Parameter::SPC_COLOR_DIFFUSE);
    auto texel = psMain->resolveLocalParameter(GCT_FLOAT4, "texel");
    auto lmTexel = psMain->resolveLocalParameter(GCT_FLOAT4, "lmTexel");
    auto uvscaled = psMain->resolveLocalParameter(GCT_FLOAT2, "uvscaled");

    auto outDiffuse = psMain->resolveOutputParameter(Parameter::SPC_COLOR_DIFFUSE);

    auto stage = psMain->getStage(FFP_PS_COLOUR_BEGIN);
    stage.callFunction("SGX_FetchNormal", globalNormal, uvPS, normal);
    stage.callFunction("FFP_Transform", ITMat, normal, normal);

    auto psSpecular = psMain->resolveLocalParameter(Parameter::SPC_COLOR_SPECULAR);
    stage.assign(Vector4::ZERO, psSpecular);
    stage.mul(uvPS, mUVMul, uvscaled);
    stage.sampleTexture(sampler, uvscaled, texel);

    // fake vertexcolour input
    if(mUseSpecularMapping)
        stage.assign(texel, diffuse);

    // apply lightmap after lighting calculations (FFP_PS_COLOUR_BEGIN + 1)
    if(mParent->getLightmap())
    {
        auto sceneCol = psProgram->resolveParameter(GpuProgramParameters::ACT_DERIVED_SCENE_COLOUR);
        stage = psMain->getStage(FFP_PS_COLOUR_BEGIN + 2);
        stage.sampleTexture(lightMap, uvPS, lmTexel);
        stage.callFunction("FFP_Lerp", {In(sceneCol).xyz(), In(diffuse).xyz(), In(lmTexel).x(), Out(outDiffuse).xyz()});
        stage.mul(In(lmTexel).x(), psSpecular, psSpecular);
        // also assign to local
        stage.assign(outDiffuse, diffuse);
    }

    stage = psMain->getStage(FFP_PS_TEXTURING);
    stage.mul(texel, outDiffuse, outDiffuse);

    // Add specular to out colour.
    psMain->getStage(FFP_PS_COLOUR_END)
        .add(In(outDiffuse).xyz(), In(psSpecular).xyz(), Out(outDiffuse).xyz());
    return true;
}

//-----------------------------------------------------------------------
const String& TerrainTransformFactory::getType() const
{
    return TerrainTransform::Type;
}
SubRenderState* TerrainTransformFactory::createInstanceImpl()
{
    return OGRE_NEW TerrainTransform();
}

const String& TerrainSurfaceFactory::getType() const
{
    return TerrainSurface::Type;
}
SubRenderState* TerrainSurfaceFactory::createInstanceImpl()
{
    return OGRE_NEW TerrainSurface();
}

}