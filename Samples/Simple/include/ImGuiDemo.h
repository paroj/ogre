#pragma once

#include "SdkSample.h"
#include "OgreImGuiOverlay.h"
#include <OgreImGuiInputListener.h>

using namespace Ogre;
using namespace OgreBites;

class _OgreSampleClassExport Sample_ImGui : public SdkSample, public RenderTargetListener
{
    std::unique_ptr<ImGuiInputListener> mImguiListener;
    InputListenerChain mListenerChain;
public:
    // Basic constructor
    Sample_ImGui()
    {
        mInfo["Title"] = "Dear ImGui integration";
        mInfo["Description"] = "Overlay ImGui interactions";
        mInfo["Category"] = "Unsorted";
        mInfo["Thumbnail"] = "thumb_imgui.png";
    }

    void setupContent(void)
    {

        mTrayMgr->showCursor();
        mCameraMan->setStyle(OgreBites::CS_ORBIT);
        mCameraMan->setYawPitchDist(Degree(0), Degree(0), 600);

        SceneNode* lightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        lightNode->setPosition(0, 10, 15);
        lightNode->attachObject(mSceneMgr->createLight("MainLight"));

        auto ps = mSceneMgr->createParticleSystem("ps", "Examples/JetEngine1");
        auto e = ps->getEmitter(0);
        e->setMaxTimeToLive(1);
        e->setMinTimeToLive(1);
        e->setEmissionRate(500000);
        ps->setParticleQuota(500000);

        SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        node->attachObject(ps);
    }
};
