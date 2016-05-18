#pragma once
#include <Irrlicht\irrlicht.h>
#include <vector>

#include "../ConfusShared/Flag.h"
#include "IUIElement.h"

namespace Confus
{
	class ClientTeamScore;

	class ScoreGUI : public IUIElement
	{
	private:
		irr::video::ITexture* m_ScoreTexture;
		irr::core::dimension2du m_ImageMaxSize;
		std::vector<irr::gui::IGUIImage*> m_ScoreImages;
		ConfusShared::Flag* m_Flag;
		ClientTeamScore& m_TeamScore;
	public:
		ScoreGUI(irr::IrrlichtDevice* a_Device, ConfusShared::Flag* a_Flag, irr::core::dimension2du a_ImageMaxSize, 
			irr::video::ITexture* a_ScoreTexture, irr::core::vector2df a_RelativePosition, ClientTeamScore& a_TeamScore);
		virtual void update() override;
		~ScoreGUI();
	};
}

