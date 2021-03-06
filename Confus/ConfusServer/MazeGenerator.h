#pragma once
#include <stack>

#include "Maze.h"
namespace ConfusServer
{
	/// <summary>
	/// Generates the maze
	/// </summary>
	class MazeGenerator
	{
	private:
		/// <summary>
		/// The maze that the players walk in.
		/// </summary>
		Maze m_MainMaze;

		/// <summary>
		/// The maze that is used to generate a new maze. The main maze is steadely replaced by this one.
		/// Does not render walls.
		/// </summary>
		Maze m_ReplacementMaze;

		/// <summary>
		/// This stack is used by the maze generating algorithm to keep track of mazeTiles that might still have neighbours that can be accesed
		/// </summary>
		std::stack<MazeTile, std::vector<std::shared_ptr<MazeTile>>> m_TileStack;

		/// <summary>
		/// The seed used to randomly chose an available neighbour and thus the seed that determines the layout of the maze.
		/// </summary>
		int m_Seed;
	public:
		/// <summary>
		/// Loads the necessary textures
		/// </summary>
		/// <param name="a_Device"> The instance of the IrrlichtDevice </param>
		/// <param name="a_StartPosition">The startposition for walls.</param>
		/// <param name="a_InitialSeed">The initial seed used to generate the first maze.</param>
		MazeGenerator(irr::IrrlichtDevice * a_Device, irr::core::vector3df a_StartPosition, int a_InitialSeed);

		/// <summary>
		/// The fixed update used to update the state of the main maze
		/// </summary>
		void fixedUpdate();

		/// <summary>
		///  replaces the main maze with a newly generated replacement maze
		/// </summary>
		/// <param name="a_Seed">Seed used to make a new maze</param>
		void refillMainMaze(int a_Seed);

		/// <summary>
		/// Default destructor, could be omitted
		/// </summary>
		~MazeGenerator();
	private:
		/// <summary>
		/// Loads the necessary textures
		/// </summary>
		/// <param name="a_Maze">The 2d vector to be used to check for neighbours.</param>
		/// <param name="a_Tile">The tile in the 2d vector that is checked for neighbours.</param>
		std::vector<std::shared_ptr<MazeTile>> getNeighbours(std::vector<std::vector<std::shared_ptr<MazeTile>>> & a_Maze, MazeTile& a_Tile);

		/// <summary>
		/// Using a depth first search algorithm generates a maze for the given 2d vector maze with a seed
		/// </summary>
		/// <param name="a_Maze">The 2d maze that is going to be used in the generation.</param>
		/// <param name="a_Seed">The seed that is going to be used in the generation.</param>
		void generateMaze(std::vector<std::vector<std::shared_ptr<MazeTile>>> &  a_Maze, int a_Seed);

		/// <summary>
		/// Replaces the main maze with the replacement maze, making sure that the walls in the main maze are lowered and raised
		/// </summary>
		void replaceMainMaze();
	};
}

