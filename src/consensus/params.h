// Copyright (c) 2018-2019 The Ring Developers
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RING_CONSENSUS_PARAMS_H
#define RING_CONSENSUS_PARAMS_H

#include <uint256.h>
#include <limits>
#include <map>
#include <string>

#include <amount.h> // Ring-fork: Hive params need CAmount

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;
};

/**
 * Parameters that influence chain consensus.
 */

const int FOREIGN_COIN_COUNT = 4;   // Ring-fork: Foreign key count

struct Params {
    uint256 hashGenesisBlock;
    //int nSubsidyHalvingInterval;  // Ring-fork: No halving on chain
    /* Block hash that is excepted from BIP16 enforcement */
    uint256 BIP16Exception;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    int BIP65Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nExpectedBlockSpacing;          // Ring-fork: Anticipated actual spacing, used in time estimations
    //int64_t nPowTargetTimespan;           // Ring-fork: Removed
    int64_t DifficultyAdjustmentInterval() const { return 1; }  // Ring-fork: Difficulty adjustment happens every block
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;

    // Ring-fork: Consensus params for foreign chains
    std::string foreignCoinNames[FOREIGN_COIN_COUNT];
    unsigned char foreignCoinP2PKHPrefixes[FOREIGN_COIN_COUNT];
    unsigned char foreignCoinP2SHPrefixes[FOREIGN_COIN_COUNT];
    unsigned char foreignCoinP2SH2Prefixes[FOREIGN_COIN_COUNT];
    unsigned char foreignCoinWIFPrefixes[FOREIGN_COIN_COUNT];
    std::string foreignCoinBech32HRPs[FOREIGN_COIN_COUNT];

    // Ring-fork: General consensus params
    int lastInitialDistributionHeight;      // Height of last block containing initial distribution payouts to foreign coins
    uint256 powLimitInitialDistribution;    // Lower-than-powLimit difficulty for initial distribution blocks only
    int slowStartBlocks;                    // Scale initial block reward up over this many blocks    
    CAmount blockSubsidyPow;                // Miner rewards for each block type
    CAmount blockSubsidyHive;
    CAmount blockSubsidyPopPrivate;
    CAmount blockSubsidyPopPublic;

    // Ring-fork: Hive-related consensus params
    CAmount dwarfCost;                  // Cost of a dwarf
    std::string dwarfCreationAddress;   // Unspendable address for dwarf creation
    std::string hiveCommunityAddress;   // Community fund address
    int communityContribFactor;         // Optionally, donate dct_value/maxCommunityContribFactor to community fund
    int dwarfGestationBlocks;           // The number of blocks for a new dwarf to mature
    int dwarfLifespanBlocks;            // The number of blocks a dwarf lives for after maturation
    uint256 powLimitHive;               // Highest (easiest) dwarf hash target
    uint32_t hiveNonceMarker;           // Nonce marker for hivemined blocks
    int minHiveCheckBlock;              // Don't bother checking below this height for Hive blocks (not used for consensus/validation checks, just efficiency when looking for potential DCTs)
    int hiveBlockSpacingTarget;         // Target Hive block frequency (1 out of this many blocks should be Hive)
    int hiveBlockSpacingTargetTypical;  // Observed Hive block frequency (1 out of this many blocks are observed to be Hive)
    int minK;                           // Minimum chainwork scale for Hive blocks (see Hive whitepaper section 5)
    int maxK;                           // Maximum chainwork scale for Hive blocks (see Hive whitepaper section 5)
    double maxHiveDiff;                 // Hive difficulty at which max chainwork bonus is awarded
    int maxKPow;                        // Maximum chainwork scale for PoW blocks
    double powSplit1;                   // Below this Hive difficulty threshold, PoW block chainwork bonus is halved
    double powSplit2;                   // Below this Hive difficulty threshold, PoW block chainwork bonus is halved again
    int maxConsecutiveHiveBlocks;       // Maximum hive blocks that can occur consecutively before a PoW block is required
    int hiveDifficultyWindow;           // How many blocks the SMA averages over in hive difficulty adjust

    // Ring-fork: Pop-related consensus fields
    int popBlocksPerHive;               // Expected number of pop blocks per Hive block. Note that increasing this here is not enough to spawn additional games, etc; this is used for time estimations.
    uint32_t popNonceMarker;            // Nonce marker for popmined blocks
    int popMinPrivateGameDepth;         // Private game source transactions must be at least this many blocks deep
    int popMaxPrivateGameDepth;         // Private game source transactions must be at most this many blocks deep
    int popMaxPublicGameDepth;          // Public game source transactions must be at most this many blocks deep
    int popScoreAdjustWindowSize;       // Windows size for adjusting pop score target
    int popMinScoreTarget;              // Min score target
    int popMaxScoreTarget;              // Max score target
};
} // namespace Consensus

#endif // RING_CONSENSUS_PARAMS_H
