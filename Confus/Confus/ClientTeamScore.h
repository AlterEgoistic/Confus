#pragma once

namespace Confus
{
	/// <summary> Class to set the team score with </summary>
	class ClientTeamScore 
    {
	public:
		/// <summary> The red team score, static so it can be accessed from anywhere. </summary>
		static int RedTeamScore;
		/// <summary> The blue team score, static so it can accessed from anywhere. </summary>
		static int BlueTeamScore;
	};
}
