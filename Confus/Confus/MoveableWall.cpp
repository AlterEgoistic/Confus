#include <IrrAssimp/IrrAssimp.h>

#include "MoveableWall.h"
#include "Game.h"

namespace Confus
{
    MoveableWall::MoveableWall(irr::IrrlichtDevice* a_Device, irr::core::vector3df a_RegularPosition,
        irr::core::vector3df a_HiddenPosition, float a_TransitionSpeed)
        : m_RegularPosition(a_RegularPosition),
        m_HiddenPosition(a_HiddenPosition),
        m_TransitionSpeed(a_TransitionSpeed)
    {
        loadMesh(a_Device->getSceneManager());
        loadTextures(a_Device->getVideoDriver());
        m_MeshNode->setPosition(m_RegularPosition);
        m_MeshNode->setMaterialTexture(0, m_RegularTexture);
    }

    void MoveableWall::loadTextures(irr::video::IVideoDriver* a_VideoDriver)
    {
        m_RegularTexture = a_VideoDriver->getTexture("Media/Moveable wall/Concrete.png");
        m_TransparentTexture = a_VideoDriver->getTexture("Media/Moveable wall/Transparent.png");
    }

    void MoveableWall::loadMesh(irr::scene::ISceneManager* a_SceneManager)
    {
        IrrAssimp importer(a_SceneManager);
        m_MeshNode = a_SceneManager->addAnimatedMeshSceneNode(importer.getMesh("Media/Moveable wall/Moveable wall.3DS"));
        m_MeshNode->setMaterialType(irr::video::E_MATERIAL_TYPE::EMT_TRANSPARENT_ALPHA_CHANNEL);
    }

    void MoveableWall::hide()
    {
        m_TargetPosition = m_HiddenPosition;
        m_Transitioning = true;
    }

    void MoveableWall::rise()
    {
        m_TargetPosition = m_RegularPosition;
        m_Transitioning = true;
    }

    void MoveableWall::fixedUpdate()
    {
        if(m_Transitioning)
        {
            updatePosition();
            updateTransparency();
        }
    }

    void MoveableWall::updateTransparency()
    {
        auto distance = (m_HiddenPosition - m_MeshNode->getPosition()).getLength();
        if(distance > 0.0f)
        {
            auto deltaDistance = distance / (m_HiddenPosition - m_RegularPosition).getLength();
            m_MeshNode->setMaterialTexture(0, deltaDistance >= m_SolifyPoint ? m_RegularTexture : m_TransparentTexture);
        }
    }

    void MoveableWall::updatePosition()
    {
        auto distance = (m_TargetPosition - m_MeshNode->getPosition()).getLength();
        if(distance > 0.0f)
        {
            auto clampedSpeed = irr::core::clamp(m_TransitionSpeed, 0.0f, distance);
            auto velocity = ((m_TargetPosition - m_MeshNode->getPosition()) / distance) * clampedSpeed;
            m_MeshNode->setPosition(m_MeshNode->getPosition() + velocity);
        }
        else
        {
            m_Transitioning = false;
        }
    }
}
