#ifndef __CubeMapping_H__
#define __CubeMapping_H__

#include "SdkSample.h"
// #include "windows.h"

#define USE_LIGHT_ARRAYS

using namespace Ogre;
using namespace OgreBites;

class _OgreSampleClassExport Sample_CubeMapping : public SdkSample, public RenderTargetListener
{
public:
    Sample_CubeMapping()
    {
        mInfo["Title"] = "Cube Mapping";
        mInfo["Description"] = "Demonstrates the cube mapping feature where a wrap-around environment is reflected "
                               "off of an object. Uses render-to-texture to create dynamic cubemaps.";
        mInfo["Thumbnail"] = "thumb_cubemap.png";
        mInfo["Category"] = "Unsorted";
    }

    std::vector<Entity*> tmpEntities;
    std::vector<SceneNode*> tmpSceneNodes;

    bool IsNumlockToggled()
    {
        return 0; //(GetKeyState(VK_NUMLOCK) & 0x0001) != 0;
    }

    bool frameRenderingQueued(const FrameEvent& evt) override
    {
        bool r = SdkSample::frameRenderingQueued(evt); // don't forget the parent updates!

#ifndef USE_LIGHT_ARRAYS
        ColourValue c = ColourValue(0.0f, 0.0f, 0.0f, 1.0f);
        if (IsNumlockToggled())
            c = ColourValue(0.5f, 0.5f, 0.5f, 1.0f);
        if (tmpEntities.size() != 0)
        {
            Entity* e = tmpEntities[0];
            MaterialPtr m = e->getSubEntity(0)->getMaterial();

            GpuProgramParametersSharedPtr gpup = m->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
            for (int i = 0; i <= 19; i++)
            {
                std::string is = std::to_string(i);

                gpup->setNamedConstant("lightDiffuse" + is, c);
                gpup->setNamedConstant("lightSpecular" + is, c);
                gpup->setNamedConstant("lightAttenuation" + is, c);
                gpup->setNamedConstant("lightPosition" + is, c);
                gpup->setNamedConstant("lightDirection" + is, c);
                gpup->setNamedConstant("lightSpotParams" + is, c);
            }
        }
#endif

        return r;
    }

    bool mouseReleased(const MouseButtonEvent& evt) override
    {
        if (mTrayMgr->mouseReleased(evt))
            return true;
        if (evt.button == BUTTON_MIDDLE)
        {
            std::vector<GpuProgramPtr> tmpGPUPrograms;
            ResourceManager::ResourceMapIterator tmpItr = GpuProgramManager::getSingleton().getResourceIterator();
            if (tmpItr.begin() == tmpItr.end())
                tmpItr = HighLevelGpuProgramManager::getSingleton().getResourceIterator();
            for (ResourceManager::ResourceMapIterator::const_iterator i = tmpItr.begin(); i != tmpItr.end(); ++i)
            {
                const ResourcePtr tmpResourcePtr = i->second;

                const Ogre::String& tmpName = tmpResourcePtr->getName();
                if (/*tmpName == "Test_AlphaRejection_VS" ||*/
                    tmpName == "TestBug_PS")
                {
                    GpuProgramPtr tmpGpuProgram = GpuProgramManager::getSingleton().getByName(tmpName);
                    if (tmpGpuProgram)
                        tmpGPUPrograms.push_back(tmpGpuProgram);
                }
            }

            for (size_t i = 0; i < tmpGPUPrograms.size(); i++)
                tmpGPUPrograms[i]->reload();
        }

#ifdef USE_LIGHT_ARRAYS
        if (evt.button == BUTTON_RIGHT)
        {
            static std::vector<Light*> tmpLights;
            if (tmpLights.size() == 0)
            {
                SceneNode* tmpParent = mSceneMgr->getRootSceneNode()->createChildSceneNode();
                tmpParent->setPosition(Vector3(0.0f, 2.0f, 0.0f));
                // Create the lights
                for (int i = 0; i < 50; i++)
                {
                    Light* tmpLight = mSceneMgr->createLight();
                    tmpParent->attachObject(tmpLight);

                    tmpLight->setCastShadows(false);
                    tmpLight->setType(Light::LT_POINT);
                    tmpLight->setDiffuseColour(ColourValue(0.5f, 0.5f, 0.5f, 1.0f));
                    tmpLight->setSpecularColour(ColourValue(0.01f, 0.01f, 0.01f));
                    tmpLight->setAttenuation(1000000.0f, 0.0f, 0.01f, 0.0f);

                    tmpLights.push_back(tmpLight);
                }
            }
            else
            {
                SceneNode* tmpParent = tmpLights[0]->getParentSceneNode();
                // Destroy the lights
                for (int i = 0; i < (int)tmpLights.size(); i++)
                {
                    Light* tmpLight = tmpLights[i];

                    tmpLight->detachFromParent();
                    mSceneMgr->destroyLight(tmpLight);
                }
                mSceneMgr->destroySceneNode(tmpParent);

                // Clear the list of lights
                tmpLights.clear();
            }
        }
#endif

        return true;
    }

protected:
    // Helper functions taken from my main project
    Vector3 __GetPosition(Camera* obj) { return obj->getDerivedPosition(); }
    void __SetPosition(Camera* obj, const Vector3& vec)
    {
        Node* tmpNode = obj->getParentNode();
        tmpNode->setPosition(vec);
    }
    void __LookAt(Camera* obj, const Vector3& pos)
    {
        Vector3 tmpPosition = __GetPosition(obj);
        __SetDirection(obj, (pos - tmpPosition).normalisedCopy(), true);
    }
    Quaternion __GetOrientation(Camera* obj) { return obj->getDerivedOrientation(); }
    void __SetDirection(Camera* obj, const Vector3& vec, bool yawFixed)
    {
        Quaternion mOrientation = Quaternion::IDENTITY;

        if (vec == Vector3::ZERO)
            return;

        Vector3 zAdjustVec = -vec;
        zAdjustVec.normalise();

        Quaternion targetWorldOrientation;

        if (yawFixed)
        {
            Vector3 mYawFixedAxis = Vector3::UNIT_Y;

            Vector3 xVec = mYawFixedAxis.crossProduct(zAdjustVec);
            xVec.normalise();

            Vector3 yVec = zAdjustVec.crossProduct(xVec);
            yVec.normalise();

            targetWorldOrientation.FromAxes(xVec, yVec, zAdjustVec);
        }
        else
        {
            Quaternion mRealOrientation = __GetOrientation(obj);

            Vector3 axes[3];
            mRealOrientation.ToAxes(axes);
            Quaternion rotQuat;
            if ((axes[2] + zAdjustVec).squaredLength() < 0.00005f)
            {
                rotQuat.FromAngleAxis(Radian(Math::PI), axes[1]);
            }
            else
            {
                rotQuat = axes[2].getRotationTo(zAdjustVec);
            }
            targetWorldOrientation = rotQuat * mRealOrientation;
        }

        mOrientation = targetWorldOrientation;

        SceneNode* tmpNode = obj->getParentSceneNode();
        tmpNode->_setDerivedOrientation(mOrientation);
        tmpNode->_update(true, false);
    }

    void setupContent() override
    {
        const float tmpSize = 70.0f;
        const float tmpStepSize = 2.0f;
        for (float x = -tmpSize; x < tmpSize; x += tmpStepSize)
        {
            for (float y = -tmpSize; y < tmpSize; y += tmpStepSize)
            {
                Entity* tmpEntity = mSceneMgr->createEntity("ogrehead.mesh");
                tmpEntity->setMaterialName("TestBug");
                SceneNode* tmpNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
                tmpNode->attachObject(tmpEntity);
                tmpNode->setPosition(Vector3(x, 0.0f, y));
                tmpNode->setScale(Vector3(0.1f, 0.02f, 0.1f));

                tmpEntities.push_back(tmpEntity);
                tmpSceneNodes.push_back(tmpNode);
            }
        }

        float tmpDistance = 2.0f;
        __SetPosition(mCamera, Vector3(tmpDistance, tmpDistance, tmpDistance));
        __LookAt(mCamera, Vector3::ZERO);
        mCamera->setNearClipDistance(0.01f);
        mCamera->setFarClipDistance(1000.0f);

        mCameraMan->setStyle(CS_MANUAL);

        MaterialManager::getSingleton().setDefaultTextureFiltering(Ogre::TFO_ANISOTROPIC);
        MaterialManager::getSingleton().setDefaultAnisotropy(16);

        mTrayMgr->showCursor();
    }

    void cleanupContent() override
    {
        for (size_t i = 0; i < tmpEntities.size(); i++)
        {
            tmpEntities[i]->detachFromParent();
            mSceneMgr->destroyEntity(tmpEntities[i]);
        }
        tmpEntities.clear();

        for (size_t i = 0; i < tmpSceneNodes.size(); i++)
        {
            mSceneMgr->destroySceneNode(tmpSceneNodes[i]);
        }
        tmpSceneNodes.clear();
    }
};

#endif
