﻿#include "PlayerHandler.h"
#include "Networking/ClientConnection.h"
#include "Audio/PlayerAudioEmitter.h"
#include "../ConfusShared/PacketType.h"
#include "../ConfusShared/EHitIdentifier.h"
#include "../ConfusShared/Physics/PhysicsWorld.h"
#include "../ConfusShared/Networking/PlayerStructs.h"
#include "Game.h"

namespace Confus
{
	PlayerHandler::PlayerHandler(irr::IrrlichtDevice* a_Device,
		ConfusShared::Physics::PhysicsWorld& a_PhysicsWorld, Audio::AudioManager& a_AudioManager, Game& a_MainGame)
		: m_PlayerNode(a_Device, a_PhysicsWorld, 1),
		m_AudioManager(a_AudioManager),
		m_PhysicsWorld(a_PhysicsWorld),
		m_Device(a_Device),
		m_MainGame(a_MainGame)
	{
	}

	PlayerHandler::PlayerConstruct::PlayerConstruct(ConfusShared::Player* a_Player, std::unique_ptr<Audio::PlayerAudioEmitter> a_AudioEmitter)
		: Player(a_Player),
		AudioEmitter(std::move(a_AudioEmitter))
	{
	}

	void PlayerHandler::setConnection(Confus::Networking::ClientConnection* a_Connection)
	{
		m_Connection = a_Connection;
		m_PlayerController = std::make_unique<LocalPlayerController>(m_PlayerNode, *m_Connection);
		m_PlayerNode.setGUID(m_Connection->getID());

		m_Connection->addFunctionToMap(static_cast<RakNet::MessageID>(ConfusShared::Networking::EPacketType::MainPlayerJoined), [this](RakNet::Packet* a_Data)
		{
			addOwnPlayer(a_Data);
		});

		m_Connection->addFunctionToMap(static_cast<RakNet::MessageID>(ConfusShared::Networking::EPacketType::PlayerLeft), [this](RakNet::Packet* a_Data)
		{
			removePlayer(a_Data);
		});

		m_Connection->addFunctionToMap(static_cast<RakNet::MessageID>(ConfusShared::Networking::EPacketType::OtherPlayerJoined), [this](RakNet::Packet* a_Data)
		{
			addOtherPlayer(a_Data);
		});

		m_Connection->addFunctionToMap(static_cast<RakNet::MessageID>(ConfusShared::Networking::EPacketType::UpdatePosition), [this](RakNet::Packet* a_Data)
		{
			updateOtherPlayer(a_Data);
		});

		m_Connection->addFunctionToMap(static_cast<RakNet::MessageID>(ConfusShared::Networking::EPacketType::UpdateHealth), [this](RakNet::Packet* a_Data)
		{
			updateHealth(a_Data);
		});
	}

	void PlayerHandler::update()
	{
		for (auto& playerPair : m_Players)
		{
			playerPair.second.Player->update();
			playerPair.second.AudioEmitter->updatePosition();
		}
	}

    void PlayerHandler::fixedUpdate() const
	{
        m_PlayerController->fixedUpdate();
	}

	void PlayerHandler::updateOtherPlayer(RakNet::Packet* a_Data)
	{
		RakNet::BitStream bitstreamIn(a_Data->data, a_Data->length, false);
		bitstreamIn.IgnoreBytes(sizeof(RakNet::MessageID));

		for (size_t i = 0; i < m_Players.size(); ++i)
		{
			ConfusShared::Networking::Server::PlayerUpdate updateFromServer;
			bitstreamIn.Read(updateFromServer);

            auto iterator = m_Players.find(updateFromServer.ID);
            if (iterator != m_Players.end())
            {
                auto player = iterator->second.Player;
                player->setPosition(updateFromServer.Position);
                player->changeState(updateFromServer.State);
                if(updateFromServer.ID != m_PlayerNode.getGUID())
                {
                    player->setRotation(updateFromServer.Rotation);
                }
            }
		}
	}

	void PlayerHandler::updateHealth(RakNet::Packet* a_Data)
	{
		RakNet::BitStream inputStream(a_Data->data, a_Data->length, false);
		inputStream.IgnoreBytes(sizeof(RakNet::MessageID));

        long long id;
		inputStream.Read(id);
        auto iterator = m_Players.find(id);
        if (iterator != m_Players.end())
        {
            auto player = iterator->second.Player;
            int health;
            EHitIdentifier hitIdentifier;

            inputStream.Read(health);
            inputStream.Read(hitIdentifier);

            if(health > player->getHealthInstance()->getHealth())
            {
                player->getHealthInstance()->heal(health - player->getHealthInstance()->getHealth());
            }
            else if(health < player->getHealthInstance()->getHealth())
            {
                player->getHealthInstance()->damage(player->getHealthInstance()->getHealth() - health, hitIdentifier);
            }
        }
	}

	void PlayerHandler::addOwnPlayer(RakNet::Packet* a_Data)
	{
		RakNet::BitStream bitstreamIn(a_Data->data, a_Data->length, false);

		bitstreamIn.IgnoreBytes(sizeof(RakNet::MessageID));
        long long newPlayerID;
        bitstreamIn.Read(newPlayerID);
        m_PlayerNode.setGUID(newPlayerID);
		ConfusShared::ETeamIdentifier teamID;
		bitstreamIn.Read(teamID);
		m_PlayerNode.setTeamIdentifier(teamID, m_Device);

		size_t size;
		bitstreamIn.Read(size);
    	for (size_t i = 0u; i < size; i++)
		{
			ConfusShared::Networking::Server::NewPlayer playerInfo;
			bitstreamIn.Read(playerInfo);
 			ConfusShared::Player* newPlayer = new ConfusShared::Player(m_Device, m_PhysicsWorld, playerInfo.ID);
			newPlayer->setTeamIdentifier(playerInfo.Team, m_Device);
			m_Players.emplace(playerInfo.ID, PlayerConstruct(newPlayer, 
				std::make_unique<Audio::PlayerAudioEmitter>(newPlayer, &m_AudioManager)));
		}
		// Add self
		int score = 0;
		bitstreamIn.Read(score);
		m_MainGame.getClientTeamScore().setTeamScore(ConfusShared::ETeamIdentifier::TeamRed, score);
		bitstreamIn.Read(score);
		m_MainGame.getClientTeamScore().setTeamScore(ConfusShared::ETeamIdentifier::TeamBlue, score);
		int seed = 0;
		bitstreamIn.Read(seed);
		m_MainGame.getMazeGenerator().refillMainMazeRequest(seed, RakNet::GetTimeMS());
		m_Players.emplace(m_PlayerNode.getGUID(), PlayerConstruct(&m_PlayerNode, std::make_unique<Audio::PlayerAudioEmitter>(&m_PlayerNode, &m_AudioManager)));
	}

	void PlayerHandler::addOtherPlayer(RakNet::Packet* a_Data)
	{
		RakNet::BitStream bitstreamIn(a_Data->data, a_Data->length, false);

		bitstreamIn.IgnoreBytes(sizeof(RakNet::MessageID));

		ConfusShared::Networking::Server::NewPlayer player;
		bitstreamIn.Read(player);   

		ConfusShared::Player* newPlayer = new ConfusShared::Player(m_Device, m_PhysicsWorld, player.ID);
		newPlayer->setTeamIdentifier(player.Team, m_Device);
		m_Players.emplace(player.ID, PlayerConstruct(newPlayer, std::make_unique<Audio::PlayerAudioEmitter>(newPlayer, &m_AudioManager)));
	}

	void PlayerHandler::removePlayer(RakNet::Packet* a_Data)
	{
		RakNet::BitStream bitstreamIn(a_Data->data, a_Data->length, false);
		bitstreamIn.IgnoreBytes(sizeof(RakNet::MessageID));

		long long id;
		bitstreamIn.Read(id);
        auto playerPair = m_Players.find(id);
        if (playerPair != m_Players.end())
        {
            playerPair->second.Player->remove();
            delete(playerPair->second.Player);
            m_Players.erase(playerPair);
        }
	}

	void PlayerHandler::handleInput(ConfusShared::EventManager* a_EventManager) const
	{
		m_PlayerController->handleInput(*a_EventManager);
	}

	ConfusShared::Player* PlayerHandler::getMainPlayer()
	{
		return &m_PlayerNode;
	}
}
