#pragma once
#include <irrlicht/irrlicht.h>

#include "Networking\ClientConnection.h"
#include "Audio\PlayerAudioEmitter.h"
#include "Health.h"
#include "Weapon.h"

namespace Confus 
{
    enum class EFlagEnum;
	enum class ETeamIdentifier;
    class EventManager;
    class Flag;
    enum class EPlayerState : unsigned char
    {
        ALIVE, CARRYINGFLAG, ATTACKING, DEAD
    };

    class Player : irr::scene::IAnimationEndCallBack, public irr::scene::ISceneNode
    {   
    public:
		/// <summary> The IAnimatedMeshSceneNode for the player </summary>
        irr::scene::IAnimatedMeshSceneNode* PlayerNode;
        irr::scene::ICameraSceneNode* CameraNode = nullptr;
		EFlagEnum* CarryingFlag;
		ETeamIdentifier* TeamIdentifier;    
        Flag* FlagPointer = nullptr;
		Health PlayerHealth;
       
    private:
        Audio::PlayerAudioEmitter* m_SoundEmitter;

        void createAudioEmitter();
        /// <summary> The weapon bone index of the animation for the weapon </summary>
        static const irr::u32 WeaponJointIndex;
        static const unsigned LightAttackDamage;
        static const unsigned HeavyAttackDamage;
        /// <summary> The player's weapon </summary>
        Weapon m_Weapon;
        /// <summary> Whether the player is currently attacking or not </summary>
        bool m_Attacking = false;
        /// <summary> Is this the main player? </summary>
        bool m_IsMainPlayer = false;
        /// <summary> The player's mesh </summary>
        irr::scene::IAnimatedMesh* m_Mesh;
        /// <summary> The player's unique ID. </summary>
        unsigned int m_PlayerID = 0;
        /// <summary> The player's active state. </summary>
        EPlayerState m_PlayerState = EPlayerState::ALIVE;
        /// <summary> The player's health, ranging from 127 to -127. </summary>
        int8_t m_PlayerHealth = 100;

    public:
        Player(irr::IrrlichtDevice* a_Device, irr::s32 a_id, ETeamIdentifier a_TeamIdentifier, bool a_MainPlayer);
		~Player();
        void fixedUpdate();
        void update();
        ///<summary> Respawns the player to their base, public so round resets etc. can call this </summary>
        void respawn();
        virtual void render();
        /// <summary> Returns the bounding box of the player's mesh </summary>
        virtual const irr::core::aabbox3d<irr::f32> & getBoundingBox() const;
        /// <summary> Handles the input based actions </summary>
        /// <param name="a_EventManager">The current event manager</param>
        void handleInput(EventManager& a_EventManager);
        void setLevelCollider(irr::scene::ISceneManager* a_SceneManager, irr::scene::ITriangleSelector* a_Level);
        /// <summary> Sets the connection to the server. </summary>
        void setConnection(Networking::ClientConnection* a_Connection);
        
        /// <summary> Sets the connection to the server. </summary>
        void sendMessageToServer() const;

    private:
        /// <summary> Starts the walking animation, which is the default animation </summary>
        void startWalking() const;
        
        /// <summary> Initializes the shared attack variables </summary>
        void initializeAttack();

        /// <summary> Starts the light attack, dealing normal damage </summary>
        void startLightAttack();

        /// <summary> Starts the heavy attack, which deals more damage </summary>
        void startHeavyAttack();

        /// <summary> Called when the animation finishes </summary>
        /// <remarks> Generally used for the attack animations only </remarks>
        /// <param name="node">The node whoms animation finished</param>
        virtual void OnAnimationEnd(irr::scene::IAnimatedMeshSceneNode* node) override;
        irr::SKeyMap m_KeyMap[6];
    };
}
