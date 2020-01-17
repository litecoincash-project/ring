// Copyright (c) 2019 The Ring Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Pop: Base class for a pop game

#ifndef RING_CRYPTO_POP_POPGAME_H
#define RING_CRYPTO_POP_POPGAME_H

#include <uint256.h>
#include <hash.h>
#include <util/strencodings.h>

struct CAvailableGame
{
    uint256 gameSourceHash;
    int blocksRemaining;
    bool isPrivate;
};

class PopGame
{
public:
    virtual void InitGame(uint256 sourceHash){};

    std::vector<unsigned char> GetSolution() { return solution; }   // Get solution vector
    std::string GetSolutionHexStr() { return HexStr(&solution[0], &solution[solution.size()]); }    // Get solution as a hex string

    uint256 GetSourceHash() { return gameSourceHash; }              // Get source hash
    virtual bool VerifyGameSolution(uint256 gameSourceHash, std::vector<unsigned char> solution, std::string& strError){};    // Verify solution given a game source hash and a solution. OVERWRITES ALL INTERNAL STATE.

protected:
    uint256 gameSourceHash;                                         // Source hash of the game
    std::vector<unsigned char> solution;                            // The solution vector
};

#endif
