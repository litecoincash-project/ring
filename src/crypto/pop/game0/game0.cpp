// Copyright (c) 2019 The Ring Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Pop: The first game

#include <crypto/pop/game0/game0.h>

// Tile atlas
constexpr Game0TileType game0TileAtlas[GAME0_NUM_TILES]  = {
//    Connections from sides              Water
//    0         1       2       3           
    {{2,        1,      0,      0     },  0},
    {{4,        0,      1,      0     },  0},
    {{8,        4,      2,      1     },  0},
    {{8+2,      8+1,    0,      1+2   },  0},
    {{2+4+8,    1+4+8,  1+2+8,  1+2+4 },  0},
    {{2,        1,      0,      0     },  0},
    {{0,        0,      0,      0     },  0},
    {{0,        0,      16,     0     },  0},
    {{16,       0,      8,      4     },  0},
    {{16,       8,      0,      2     },  0},
    {{0,        0,      0,      0     },  0},
    {{4,        8,      1,      2     },  0},
    {{0,        0,      0,      16    },  0},
    {{16,       8,      0,      2     },  0},
    {{0,        0,      0,      0     },  0}
};

Game0::Game0() {
	for (int y = 0; y < GAME0_BOARD_SIZE; y++)
		for (int x = 0; x < GAME0_BOARD_SIZE; x++) {
            Game0Tile t;
            t.tileTypeIndex = -1;
            tileMap[y][x] = t;
        }
}

void Game0::InitGame(uint256 sourceHash) {
    gameSourceHash = sourceHash;
    gameMutatedHash = sourceHash;                           // Mutated hash starts the same as game source hash
    tilesPlaced = 0;
    solution.clear();                                       // Clear our solution
    candles.clear();

    for (int y = 0; y < GAME0_BOARD_SIZE; y++)
        for (int x = 0; x < GAME0_BOARD_SIZE; x++) {
            tileMap[y][x].tileTypeIndex = -1;
            tileMap[y][x].rotation = 0;
        }

    currentTile = GetNextTile();                            // Get the first two tiles
	nextTile = GetNextTile();
}

bool Game0::VerifyGameSolution(int targetScore, uint256 sourceHash, std::vector<unsigned char> solutionToVerify, std::string& strError) {
    // Check solution size
    int tilesToPlace = solutionToVerify.size();
    if (tilesToPlace > GAME0_BOARD_SIZE * GAME0_BOARD_SIZE) {
        strError = "Impossible solution size";
        return false;
    }

    // Set up initial state
    InitGame(sourceHash);
    
    // Rebuild the board, checking for valid placement as we go
    for (int i = 0; i < tilesToPlace; i++) {
        unsigned char thisMove = solutionToVerify[i];                   // Extract the move
        int x = (thisMove >> 3) & 7;
        int y = thisMove & 7;
        int rotation = thisMove >> 6;
        if (!PlaceTile(x, y, rotation, strError))                       // Try to place the tile (mutates state, and updates strError directly in case of error)
            return false;
    }

    // Calculate score and check against threshold
    std::string strDesc;
    int score = CalculateScore(strDesc);
    if (score < targetScore) {
        strError = "Solution does not meet score target; score=" + std::to_string(score) + ", target=" + std::to_string(targetScore);
        return false;
    }

    return true;
}

bool Game0::PlaceTile(int x, int y, int rotation, std::string& strError) {
    // Check for legal placement: Must be in board range
    if (x < 0 || x >= GAME0_BOARD_SIZE || y < 0 || y >= GAME0_BOARD_SIZE) {
        strError = "Attempted out-of-range placement";
        return false;
    }
    
    // Check for legal placement: Must be on unoccupied space
    if (tileMap[y][x].tileTypeIndex != -1) {
        strError = "Attempted to place on an occupied tile";
        return false;
    }
    
    // Check for legal placement: Except on first turn, must have a neighbour
    if (tilesPlaced > 0 && !HasNeighbour(x, y)) {
        strError = "Attempted to place without occupied neighbour";
        return false;
    }

    // Set the tile
    tileMap[y][x].tileTypeIndex = currentTile;
    tileMap[y][x].rotation = rotation;

    // Store the move in solution log
    unsigned char thisMove = (rotation << 6) + (x << 3) + y;
    solution.push_back(thisMove);
    tilesPlaced++;

    // Set current to next, and get next
    currentTile = nextTile;
    nextTile = GetNextTile();

    return true;
}

std::string Game0::DumpTileMap() {
    std::string output;
	for (int y = 0; y < GAME0_BOARD_SIZE; y++) {
		for (int x = 0; x < GAME0_BOARD_SIZE; x++) {
            int i = tileMap[y][x].tileTypeIndex;
            std::string s = std::to_string(i);
            s = std::string(2 - s.length(), '0') + s;
            output += i == -1 ? "### " : (s + ":" + std::to_string(tileMap[y][x].rotation) + " ");
        }
        output += "\n";
    }
    return output;
}

int Game0::GetTileTypeIndex(int x, int y) {
    if (x >= 0 && x < GAME0_BOARD_SIZE && y >= 0 && y < GAME0_BOARD_SIZE)
        return tileMap[y][x].tileTypeIndex;
    else
        return -1;    
}

bool Game0::HasNeighbour(int x, int y) {
    if (GetTileTypeIndex(x + 1, y) != -1) return true;
    if (GetTileTypeIndex(x - 1, y) != -1) return true;
    if (GetTileTypeIndex(x, y + 1) != -1) return true;
    if (GetTileTypeIndex(x, y - 1) != -1) return true;

    return false;
}

int Game0::GetNextTile() {
    CHashWriter ss(SER_GETHASH, 0);
    ss << gameMutatedHash;
    ss << solution;
    gameMutatedHash = ss.GetHash();
    return gameMutatedHash.ByteAt(13) % GAME0_NUM_TILES;
}

int Game0::CalculateScore(std::string& strDesc) {
    int score = 0;
    strDesc = "";
    
    // Find largest group of connected rooms
    int maxConnectedRooms = 0;
    candles.clear();
    for (int y = 0; y < GAME0_BOARD_SIZE; y++)
		for (int x = 0; x < GAME0_BOARD_SIZE; x++) {                    // For each cell
            Game0Tile tile = tileMap[y][x];
            if (tile.tileTypeIndex < 0) continue;
            Game0TileType tileType = game0TileAtlas[tile.tileTypeIndex];            
            for (int side = 0; side < 4; side++)                        // For each side connection
                if (tileType.connections[side] & (1 << 4)) {            // If connected to an internal room..
                    connectedRooms = 0;                                 // ..set off looking for other rooms
                    startX = x;
                    startY = y;
                    followedBranches.clear();
                    candidateCandles.clear();
                    candidateCandles.push_back(std::make_pair(x,y));
                    FindRooms(x, y, (side + tile.rotation) % 4);
                    if (connectedRooms > maxConnectedRooms) {
                        maxConnectedRooms = connectedRooms;
                        candles = candidateCandles;
                    }
                }
        }   
    if (maxConnectedRooms > 0) {
        int roomScore = (maxConnectedRooms + 1) * 10;
        score += roomScore;
        strDesc += std::to_string(roomScore) + " from connected rooms (10 points each)\n";
    }

    // TODO: Add other scoring things here (contiguous water, etc)

    return score;
}

void Game0::FindRooms(int x, int y, int connection) {
    // Don't follow the same path twice
    std::tuple<int,int,int> tup = std::make_tuple(x, y, connection);
    auto it = followedBranches.find(tup);
    if (it != followedBranches.end())
        return;
    followedBranches[tup] = true;

    // Get next tile coords and figure out which of its connections we're coming in on
    switch(connection) {
        case 0: y--; connection = 2; break;
        case 1: x++; connection = 3; break;
        case 2: y++; connection = 0; break;
        case 3: x--; connection = 1; break;
    }
    
    // Check next tile coords aren't out of bounds
    if (y < 0 || y >= GAME0_BOARD_SIZE || x < 0 || x >= GAME0_BOARD_SIZE)
        return;

    // Check we haven't come back to the start
    if (x == startX && y == startY)
        return;

    // Check it's not empty
    Game0Tile tile = tileMap[y][x];
    if (tile.tileTypeIndex == -1)
        return;

    // Rotate incoming connection
    connection -= tile.rotation;
    if (connection < 0)
        connection += 4;

    // Check to see what this connection's connected to
    int connections = game0TileAtlas[tile.tileTypeIndex].connections[connection];

    // Check for connection to an internal room
    if (connections & (1 << 4)) {
        candidateCandles.push_back(std::make_pair(x,y));
        connectedRooms++;
        return;
    }

    // Check for outgoing connections and follow them
    for (int i = 0; i < 4; i++)
        if (connections & (1 << i))
            FindRooms(x, y, (i + tile.rotation) % 4);
}
