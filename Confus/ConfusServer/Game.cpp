#include <Irrlicht/irrlicht.h>
#include <time.h>
#include <iostream>
#include <RakNet\GetTime.h>
#include <RakNet\BitStream.h>

#include "Game.h"
#include "Player.h"
#include "Flag.h"
#define DEBUG_CONSOLE
#include "../ConfusShared/Debug.h"
#include "../ConfusShared/TeamIdentifier.h"

namespace ConfusServer
{
    const double Game::FixedUpdateInterval = 0.02;
    const double Game::MaxFixedUpdateInterval = 0.1;
	const double Game::ProcessPacketsInterval = 0.01;
    const double Game::MazeDelay = 2.0;
    const double Game::MazeChangeInterval = 60.0 - MazeDelay;

    Game::Game()
        : m_Device(irr::createDevice(irr::video::E_DRIVER_TYPE::EDT_NULL)),
		m_MazeGenerator(m_Device, irr::core::vector3df(0.0f, 0.0f, 0.0f),(19+20+21+22+23+24)), // magic number is just so everytime the first maze is generated it looks the same, not a specific number is chosen
        m_BlueFlag(m_Device, ETeamIdentifier::TeamBlue, &m_TeamScoreManager),
        m_RedFlag(m_Device, ETeamIdentifier::TeamRed, &m_TeamScoreManager)
    {
    }

    Game::~Game()
    {
        for(size_t i = 0u; i < m_PlayerArray.size(); i++)
        {
            //m_PlayerArray[i]->remove();
            //delete(m_PlayerArray[i]);
        }
    }

    void Game::run()
    {
        initializeConnection();

        auto sceneManager = m_Device->getSceneManager();
        m_LevelRootNode = m_Device->getSceneManager()->addEmptySceneNode();

        m_LevelRootNode->setPosition(irr::core::vector3df(1.0f, 1.0f, 1.0f));
        sceneManager->loadScene("Media/IrrlichtScenes/Bases2.irr", nullptr, m_LevelRootNode);
        m_LevelRootNode->setScale(irr::core::vector3df(1.0f, 1.0f, 1.0f));
        m_LevelRootNode->setVisible(true);
        
        processTriangleSelectors();

        m_BlueFlag.setCollisionTriangleSelector(m_Device->getSceneManager(), m_LevelRootNode->getTriangleSelector());
        m_RedFlag.setCollisionTriangleSelector(m_Device->getSceneManager(), m_LevelRootNode->getTriangleSelector());

        m_Connection->addFunctionToMap(static_cast<unsigned char>(ID_NEW_INCOMING_CONNECTION), [this](RakNet::BitStream* a_Data)
        {
            addPlayer(a_Data);
        }); 
		m_Connection->addFunctionToMap(static_cast<unsigned char>(Networking::EPacketType::Player), [this](RakNet::BitStream* a_Data)
		{
			for (size_t i = 0; i < m_PlayerArray.size(); i++)
			{
				m_PlayerArray[static_cast<int>(i)]->updateFromClient(a_Data);
			}
		});

        m_Connection->addFunctionToMap(static_cast<unsigned char>(Networking::EPacketType::PlayerLeft), [this](RakNet::BitStream* a_Data)
        {
            removePlayer(a_Data);
        });

        m_Device->getCursorControl()->setVisible(false);
      
        while(m_Device->run())
        {
			processConnection();
            update();
            processFixedUpdates();
            render();
        }
    }

    void Game::initializeConnection()
    {
        m_Connection = std::make_unique<Networking::Connection>();
        m_TeamScoreManager.setConnection(m_Connection.get());
    }

	void Game::processConnection()
	{
		m_ConnectionUpdateTimer += m_DeltaTime;
		if (m_ConnectionUpdateTimer >= ProcessPacketsInterval)
		{
			m_ConnectionUpdateTimer = 0;
			m_Connection->processPackets();
		}
	}

    void Game::processTriangleSelectors()
    {
        auto sceneManager = m_Device->getSceneManager();
        auto metatriangleSelector = sceneManager->createMetaTriangleSelector();
        
        irr::core::array<irr::scene::ISceneNode*> nodes;
        sceneManager->getSceneNodesFromType(irr::scene::ESNT_ANY, nodes);
        for(irr::u32 i = 0; i < nodes.size(); ++i)
        {
            irr::scene::ISceneNode* node = nodes[i];
            irr::scene::ITriangleSelector* selector = nullptr;
            node->setDebugDataVisible(irr::scene::EDS_BBOX_ALL);

            switch(node->getType())
            {
            case irr::scene::ESNT_CUBE:
            case irr::scene::ESNT_ANIMATED_MESH:
                selector = m_Device->getSceneManager()->createTriangleSelectorFromBoundingBox(node);
                break;
            case irr::scene::ESNT_MESH:
            case irr::scene::ESNT_SPHERE:
                selector = sceneManager->createTriangleSelector(((irr::scene::IMeshSceneNode*)node)->getMesh(), node);
                break;
            case irr::scene::ESNT_TERRAIN:
                selector = sceneManager->createTerrainTriangleSelector((irr::scene::ITerrainSceneNode*)node);
                break;
            case irr::scene::ESNT_OCTREE:
                selector = sceneManager->createOctreeTriangleSelector(((irr::scene::IMeshSceneNode*)node)->getMesh(), node);
                break;
            default:
                break;
            }

            if(selector)
            {
                metatriangleSelector->addTriangleSelector(selector);
                selector->drop();
            }
        }
        m_LevelRootNode->setTriangleSelector(metatriangleSelector);
    }

    void Game::update()
    {
        m_PreviousTicks = m_CurrentTicks;
        m_CurrentTicks = m_Device->getTimer()->getTime();
        m_DeltaTime = (m_CurrentTicks - m_PreviousTicks) / 1000.0;

        static float currentDelay = 0.0f;
        static int currentSeed;
        m_MazeTimer += m_DeltaTime;
        if(m_MazeTimer >= MazeChangeInterval)
        {
            if(currentDelay >= MazeDelay)
            {
                m_MazeGenerator.refillMainMaze(currentSeed);
                m_MazeTimer = 0.0f;
                currentDelay = 0.0f;
            }
            if(currentDelay == 0.0f)
            {
                currentSeed = static_cast<int>(time(0)) % 1000;
                broadcastMazeChange(currentSeed);
            }
            currentDelay += static_cast<float>(m_DeltaTime);
        }

        updatePlayers();
    }

    void Game::processFixedUpdates()
    {
        m_FixedUpdateTimer += m_DeltaTime;
        m_FixedUpdateTimer = irr::core::min_(m_FixedUpdateTimer, MaxFixedUpdateInterval);
        while(m_FixedUpdateTimer >= FixedUpdateInterval)
        {
            m_FixedUpdateTimer -= FixedUpdateInterval;
            fixedUpdate();
        }
    }

    void Game::fixedUpdate()
    {
		m_MazeGenerator.fixedUpdate();
    }

    void Game::broadcastMazeChange(int a_Seed)
    {
        int newTime = static_cast<int>(RakNet::GetTimeMS()) + (static_cast<int>(MazeDelay * 1000));

        RakNet::BitStream bitStream;
        bitStream.Write(static_cast<RakNet::MessageID>(Networking::EPacketType::MazeChange));
        bitStream.Write(newTime);
        bitStream.Write(a_Seed);
        m_Connection->broadcastBitStream(bitStream);
    }

    void Game::addPlayer(RakNet::BitStream* a_Data)
    {
        char id = 0;
        char blueMembers = 0;
        char redMembers = 0;
        std::vector<bool> availableID(11, true);

        for(unsigned char i = 0u; i < m_PlayerArray.size(); i++)
        {
            availableID[m_PlayerArray[static_cast<int>(i)]->ID] = false;
        }

        for(unsigned char i = 0u; i < availableID.size(); i++)
        {
            if(availableID[static_cast<int>(i+1)])
            {
                id = static_cast<char>(i+1);
                break;
            }
        }

        ETeamIdentifier teamID = m_PlayerArray.size() % 2 == 0 ? ETeamIdentifier::TeamRed : ETeamIdentifier::TeamBlue;

        Player* newPlayer = new Player(m_Device, id, teamID, false);
        newPlayer->setLevelCollider(m_Device->getSceneManager(), m_LevelRootNode->getTriangleSelector());
        m_PlayerArray.push_back(newPlayer);

        RakNet::BitStream newClientStream;
        newClientStream.Write(static_cast<RakNet::MessageID>(Networking::EPacketType::MainPlayerJoined));
        newClientStream.Write(static_cast<char>(id));
        newClientStream.Write(static_cast<ETeamIdentifier>(teamID));
        newClientStream.Write(static_cast<size_t>(m_PlayerArray.size()));

        int playerIndex = 0;

        for(size_t i = 0u; i < m_PlayerArray.size(); i++)
        {
            if(m_PlayerArray[i]->ID == id)
            {
                playerIndex = i;
            }
            newClientStream.Write(static_cast<char>(m_PlayerArray[i]->ID));
            newClientStream.Write(static_cast<ETeamIdentifier>(m_PlayerArray[i]->TeamIdentifier));
        }

        m_Connection->broadcastBitStreamToClient(newClientStream, playerIndex);
        std::cout << "[Game Class] Player id: " << static_cast<int>(id) << " joined." << std::endl;

        RakNet::BitStream otherClientsStream;
        otherClientsStream.Write(static_cast<RakNet::MessageID>(Networking::EPacketType::OtherPlayerJoined));
        otherClientsStream.Write(static_cast<char>(id));
        otherClientsStream.Write(static_cast<ETeamIdentifier>(teamID));
        m_Connection->broadcastBitStream(otherClientsStream);
    }

    void Game::removePlayer(RakNet::BitStream* a_Data)
    {
        a_Data->IgnoreBytes(sizeof(RakNet::MessageID));
        char id;
        static_cast<char>(a_Data->Read(id));
        std::cout << "[Game Class] Player id: " << static_cast<int>(id) << " left." << std::endl;
        deletePlayer(id);
    }


    void Game::deletePlayer(char a_PlayerID)
    {
        for(size_t i = 0u; i < m_PlayerArray.size(); i++)
        {
            if(m_PlayerArray[i]->ID == a_PlayerID)
            {
                m_PlayerArray[i]->remove();
                delete(m_PlayerArray[i]);
                m_PlayerArray.erase(m_PlayerArray.begin() + i);
            }
        }

        RakNet::BitStream stream;
        stream.Write(static_cast<RakNet::MessageID>(Networking::EPacketType::PlayerLeft));
        stream.Write(a_PlayerID);
        m_Connection->broadcastBitStream(stream);
    }

    void Game::updatePlayers()
    {
        for(size_t i = 0u; i < m_PlayerArray.size(); i++)
        {
            RakNet::BitStream stream;
            stream.Write(static_cast<RakNet::MessageID>(Networking::EPacketType::UpdatePosition));
            Player* player = m_PlayerArray[i];

			if (player != nullptr)
			{
				ConfusShared::Networking::PlayerInfo playerInfo;
				playerInfo.playerID = static_cast<char>(player->ID);
				playerInfo.position = player->CameraNode->getAbsolutePosition();
				playerInfo.rotation = player->CameraNode->getRotation();
				playerInfo.newState = player->PlayerState;
				playerInfo.playerHealth = static_cast<unsigned int>(player->PlayerHealth.getHealth());
				stream.Write(playerInfo);

				m_Connection->broadcastPacket(&stream, nullptr);

				if (player->userTimedOut())
				{
					deletePlayer(playerInfo.playerID);
					std::cout << "[Game Class] Player id: " << static_cast<int>(playerInfo.playerID) << " timed out." << std::endl;
				}
			}
        }
    }

    void Game::render()
    {
        m_Device->getVideoDriver()->beginScene(true, true, irr::video::SColor(255, 100, 101, 140));
        m_Device->getSceneManager()->drawAll();
        //m_Device->getGUIEnvironment()->drawAll();
        m_Device->getVideoDriver()->endScene();
    }
}