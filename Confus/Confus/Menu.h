#pragma once
#include "../ConfusShared/BaseGame.h"

namespace Confus
{
	/// <summary>
	/// Represents the initial menu state the game once it is loaded
	/// </summary>
	/// <seealso cref="ConfusShared::BaseGame" />
	class Menu : public ConfusShared::BaseGame
    {
    private:
		/// <summary>The text displayed in the menu</summary>
		irr::gui::IGUIStaticText* m_Text;
    private:
		/// <summary>Starts this instance</summary>
		virtual void start() override;

		/// <summary>Updates this instance.</summary>
		virtual void update() override;

		/// <summary>Fixeds the update.</summary>
		virtual void fixedUpdate() override;

		/// <summary>Ends this instance.</summary>
		virtual void end() override;

		/// <summary>Runs the game.</summary>
		void runGame();
    public:
		/// <summary>Initializes a new instance of the <see cref="Menu"/> class.</summary>
		/// <param name="a_Device">The active Irrlicht device</param>
		/// <param name="a_EventManager">The active event manager to handle input with</param>
		Menu(irr::IrrlichtDevice* a_Device, ConfusShared::EventManager* a_EventManager);
    };
}