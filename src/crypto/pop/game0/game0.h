// Copyright (c) 2019 The Ring Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Pop: The first game

/*
Tile connections are encoded bitwise as follows:

                Side 0: 1
            -----------------
            |               |
            |   Internal    |
  Side 3: 8 |    room:      |  Side 1: 2
            |     16        |
            |               |
            -----------------
                Side 2: 4

Moves are packed into the solution vector 1 byte per move as follows:

 Bits 0,1,2: y
      3,4,5: x
        6,7: rotation
*/

#ifndef RING_CRYPTO_POP_GAME0_H
#define RING_CRYPTO_POP_GAME0_H

#include <uint256.h>
#include <hash.h>
#include <util/strencodings.h>
#include <crypto/pop/popgame.h>

#define GAME0_BOARD_SIZE      7                                     // Board is GAME0_BOARD_SIZE x GAME0_BOARD_SIZE tiles
#define GAME0_NUM_TILES       15                                    // Number of tile variations

struct Game0Tile {                                                  // A tile on the board
    int tileTypeIndex;
    int rotation;
};

struct Game0TileType {                                              // A tile type, detailing its connections
    int connections[4];
    int water;
};

class Game0 : public PopGame
{
public:
    Game0();

    void InitGame(uint256 sourceHash) override;                     // Set up a fresh game given source hash. RESETS ALL INTERNAL STATE.
    bool VerifyGameSolution(int targetScore, uint256 gameSourceHash, std::vector<unsigned char> solution, std::string& strError) override;    // Verify solution given a game source hash and a solution. OVERWRITES ALL INTERNAL STATE.

    bool HasNeighbour(int x, int y);                                // Check if {x,y} has a valid neighbour tile
    bool PlaceTile(int x, int y, int rotation, std::string& strError);   // Try and place the current tile. If successful, add the move to the solution and mutate the game state.
    int CalculateScore(std::string& strDesc);                       // Calculate current score

    int GetTilesPlaced() { return tilesPlaced; }                    // Get number of tiles placed so far
    int GetCurrentTileType() { return currentTile; }                // Get current (under-cursor) tile type
    int GetNextTileType() { return nextTile; }                      // Get next tile type
    Game0Tile GetTileAt(int x, int y) { return tileMap[y][x]; }     // Get Tile at given coords
    std::vector<std::pair<int,int>> GetCandles() { return candles; }    // Get coords of rooms contributing to larges group
    std::string DumpTileMap();                                      // Dump current tile map as a string

private:
    int GetNextTile();                                              // Get the next tile, and in doing so mutate the state
    int GetTileTypeIndex(int x, int y);                             // Get index of tile at this location, or -1 if empty
    void FindRooms(int x, int y, int connection);                   // Recursively search for rooms from {x,y}'s given incoming connection

    Game0Tile tileMap[GAME0_BOARD_SIZE][GAME0_BOARD_SIZE];          // The board
    int currentTile, nextTile;                                      // Tile types of current and next tiles
    std::map<std::tuple<int,int,int>, bool> followedBranches;       // Map of followed branches to prevent retreading steps (possibly infinitely!)
    std::vector<std::pair<int,int>> candles, candidateCandles;      // List of rooms contributing to longest connected group
    int tilesPlaced;                                                // Tally of tiles placed
    int connectedRooms, startX, startY;                             // Tally of connected rooms when searching, and marker of start pos
    uint256 gameMutatedHash;                                        // Hash used to generate tiles and mutated every move
};

#endif // RING_CRYPTO_POP_GAME0_H
