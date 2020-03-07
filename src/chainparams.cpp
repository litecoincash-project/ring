// Copyright (c) 2018-2019 The Ring Developers
// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <consensus/consensus.h>    // Ring-fork: For COINBASE_MATURITY
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

// Note: The Ring genesis block reward is unspendable. Its scriptpubkey is an embedded 32x32 8-bit raw image; decoding it is left as an exercise to the reader :)
#define GENESIS_HASH        "0xb61b8461ebb55edad92a0f733df0734fb0b6593a6abe6d97b4fc1d24be46e416"
#define GENESIS_NONCE       2363402
#define GENESIS_TIMESTAMP   1579273692
#define GENESIS_MERKLE      "0x68a1b35cbd59f83b6e462281a703b58fdecb5e436cf82653720b55ea6852dc97"
#define GENESIS_STRING      "In memory of Pawel 'demon2k13' Wyszynski (21/7/1994 - 3/7/2019) a ring is forged 17/1/2020"
#define GENESIS_IMAGE       "89504E470D0A1A0A0000000D4948445200000020000000200800000000561125280000000467414D410000B18F0BFC6105000000206348524D00007A26000080840000FA00000080E8000075300000EA6000003A98000017709CBA513C00000002624B474400FF878FCCBF0000000970485973000015870000158701B219EEBA0000000774494D4507E307140D2810796DF2C90000031C4944415438CB2D914D6F1B5518469FE7BD77C6F638B19DA48DEDD4260A5584DA20151481C4062A10422AFC0F7E013F8B6D575D14169508422A9568F9688112689D90389DF86B66EC7B1F16657D3647E7D00300291AD8AE6B55141191A2648A043C09401689CD83B5BAA59AFE7632CDA6411000C939803046D77EDF5C3D6974FADBA7FB5731012118E029118816DF9E14E92C495B4D1F7C76414284402F1114D8DC382FE3725EBF5CCF976567FB575100440F8A20D0AB75D332247A352A18929DCE054C20A2F32448F0A093B5D79B584E574A3AED66F65C262A9A01A0624CBB9678A4994FE9935AEABBD7101005798AA078655A54CB32946539C766BEB5C9E13121307A6EBB9188938B642D4918502C382B47D9B575421641AFF276F1F26C56CDF54A9422D39AB7348ED25AC5481953EE1C3A96E5225F94AB30AFCCA78D8469923DBD845E3BBC98DC6CBF2A8687B9C6A362A17A5633552B3A50009D23AA7F5EB87450FA5B9F7CB42EA05E4BDBDB1B6BF58535EA8A1E8058F41B4F0EFA1FF83CACD01F8D5BCE7C7A7D373F9BF89F3D5E8F7EEF68F35ABA3ACF4F8E7FCC764E5FD85E6CD4EB038DA30741B1F7FDA4B765F9E338383FCFBE7AF470EFDF6F73518C3002919C8E5EDE08B45AE3E472ED4E37BFB5FB8E1724010601C4FDCF0F8FDAD3AD5E3EE7E03315C3FE864823250348E1FCDEF57B45583034F656D5B8087EB920A0087A1102F0C8EE4CB74B375CFBE5F7BFD22DE238300A82CC012410935EC77CFBCD7E363CFAA3887A1A2260049D013228D60E06A98B165D19626E57EE8E45518CFE35979D3DF9B030D5EAE367ABF6F079E35CFA3F7502026A74E6FE8BA0C4FD59DCF0573FEE7C6D390051302942ADD66235AA62B54AAEFA876170F1CDB3594B2464F400B58359E5B666CB663E7D2373AEFBE4BE4DFB39445BC1036E671E239A8727ADE5CB07318DA7DFFD10FCCE26CE260C9473697F12196C7777F956F9E0E9EDE747A3CB56326CAC4DD6BBCB2A9AB5BB174B92567CFA53A331F8321CE7E34C1D26556F52EEBFBBD7B1F46C993833C5FD9B479DEB93C77F5B6B352D424ADF9D4EE1FA7611124581C9B3BB3C993F3C8ACD5ED0620957C5D9A20AE3FF0021C5AE38E74A222800000020744558744372656174696F6E54696D6500323031373A30323A31372032333A34303A3031F1B710F60000002574455874646174653A63726561746500323031392D30372D32305431333A34303A31362B30333A303067C362F30000002574455874646174653A6D6F6469667900323031392D30372D32305431333A34303A31362B30333A3030169EDA4F0000001874455874536F667477617265007061696E742E6E657420342E312E36FD4E09E80000001174455874536F75726365004E494B4F4E204433303068FC3E620000000049454E44AE426082"

#define LAST_ID_BLOCK_HASH  "0x0"
#define LAST_ID_CHAINWORK   "0x0"
#define LAST_ID_HEIGHT      1000000

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{    
    const char* pszTimestamp = GENESIS_STRING;
    const CScript genesisOutputScript = CScript() << OP_RETURN << ParseHex(GENESIS_IMAGE);
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        //consensus.nSubsidyHalvingInterval = 210000;   // No halving on chain
        //consensus.BIP16Exception = uint256S("0x0");   // No BIP16 exception on chain
        consensus.BIP34Height = 100;
        consensus.BIP34Hash = uint256();                // Not needed; activated on unforkable block below initial distro end
        consensus.BIP65Height = 1;                      // BIP65 & 66 active since start
        consensus.BIP66Height = 1;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 90;               // Targeting 1 pow block (3 blocks across all types) in this many seconds
        consensus.nExpectedBlockSpacing = consensus.nPowTargetSpacing / 3;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 800; // 80% of 1000 blocks required for UASF activation
        consensus.nMinerConfirmationWindow = 1000;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601;    // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;      // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = GENESIS_TIMESTAMP;               // Begin activation period with genesis
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = GENESIS_TIMESTAMP + 31536000;      // Genesis + 1 year

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = GENESIS_TIMESTAMP;            // Begin activation period with genesis
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = GENESIS_TIMESTAMP + 31536000;   // Genesis + 1 year

        // Ring-fork: Foreign coin fields
        const int fcc = Consensus::FOREIGN_COIN_COUNT;
        std::string foreignCoinNames[fcc] =           {"BTC", "LCC",  "DOGE", "DASH"};
        unsigned char foreignCoinP2PKHPrefixes[fcc] = {0,     28,     30,     76};
        unsigned char foreignCoinP2SHPrefixes[fcc] =  {5,     5,      22,     16};      // Any LCC addresses starting 3 (very old pre-fork P2SH) will read as BTC addresses due to same version byte. Display issue only; keys should still import fine.
        unsigned char foreignCoinP2SH2Prefixes[fcc] = {0,     50,     0,      0};       // Only used by LCC and Litecoin; skipped if 0.
        unsigned char foreignCoinWIFPrefixes[fcc] =   {128,   176,    158,    204};    
        std::string foreignCoinBech32HRPs[fcc] =      {"bc",  "lcc",  "",     ""};	    // Blank means no bech32 support in foreign chain

        std::copy(std::begin(foreignCoinNames), std::end(foreignCoinNames), std::begin(consensus.foreignCoinNames));        
        std::copy(std::begin(foreignCoinP2PKHPrefixes), std::end(foreignCoinP2PKHPrefixes), std::begin(consensus.foreignCoinP2PKHPrefixes));
        std::copy(std::begin(foreignCoinP2SHPrefixes), std::end(foreignCoinP2SHPrefixes), std::begin(consensus.foreignCoinP2SHPrefixes));
        std::copy(std::begin(foreignCoinP2SH2Prefixes), std::end(foreignCoinP2SH2Prefixes), std::begin(consensus.foreignCoinP2SH2Prefixes));
        std::copy(std::begin(foreignCoinWIFPrefixes), std::end(foreignCoinWIFPrefixes), std::begin(consensus.foreignCoinWIFPrefixes));
        std::copy(std::begin(foreignCoinBech32HRPs), std::end(foreignCoinBech32HRPs), std::begin(consensus.foreignCoinBech32HRPs));

        // Ring-fork: General consensus fields
        consensus.lastInitialDistributionHeight = LAST_ID_HEIGHT;                                                                   // Height of last block containing initial distribution payouts to foreign coins
        consensus.powLimitInitialDistribution = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");       // Lower-than-powLimit difficulty for initial distribution blocks only
        consensus.slowStartBlocks = 2000;                                                                                           // Scale initial block reward up over this many blocks
        consensus.blockSubsidyPow = 0.2 * COIN;             // Miner rewards for each block type
        consensus.blockSubsidyHive = 1 * COIN;
        consensus.blockSubsidyPopPrivate = 0.2 * COIN;
        consensus.blockSubsidyPopPublic = 0.05 * COIN;

        // Ring-fork: Hive: Consensus Fields
        consensus.dwarfCost = 0.01 * COIN;                  // Cost of a dwarf
        consensus.dwarfCreationAddress = "RNGSummonADwarvenMiningArmyXYzDNsz";  // Unspendable address for dwarf creation
        consensus.hiveCommunityAddress = "RNGBEERnxAdgdRfSa45NFy5bZfQPYbztkn";  // Community fund address
        consensus.communityContribFactor = 5;               // Optionally, donate dct_value/maxCommunityContribFactor to community fund
        consensus.dwarfGestationBlocks = 2880;              // The number of blocks for a new dwarf to mature (approx 24 hours)
        consensus.dwarfLifespanBlocks = 30000;              // The number of blocks a dwarf lives for after maturation (approx 10 days)
        consensus.powLimitHive = uint256S("0fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");  // Highest (easiest) dwarf hash target
        consensus.minHiveCheckBlock = consensus.lastInitialDistributionHeight + 1;   // Don't bother checking below this height for Hive blocks (not used for consensus/validation checks, just efficiency when looking for potential DCTs)
        consensus.hiveBlockSpacingTarget = 2;               // Target Hive block frequency (1 out of this many Hive and pow blocks combined should be Hivemined)
        consensus.hiveBlockSpacingTargetTypical = 2;        // Observed Hive block frequency (1 out of this many Hive and pow blocks combined are observed to be Hive)
        consensus.hiveNonceMarker = 1;                      // Nonce marker for hivemined blocks
        consensus.minK = 2;                                 // Minimum chainwork scale for Hive blocks (see Hive whitepaper section 5)
        consensus.maxK = 16;                                // Maximum chainwork scale for Hive blocks (see Hive whitepaper section 5)
        consensus.maxHiveDiff = 0.006;                      // Hive difficulty at which max chainwork bonus is awarded
        consensus.maxKPow = 5;                              // Maximum chainwork scale for PoW blocks
        consensus.powSplit1 = 0.005;                        // Below this Hive difficulty threshold, PoW block chainwork bonus is halved
        consensus.powSplit2 = 0.0025;                       // Below this Hive difficulty threshold, PoW block chainwork bonus is halved again
        consensus.maxConsecutiveHiveBlocks = 2;             // Maximum hive blocks that can occur consecutively before a PoW block is required
        consensus.hiveDifficultyWindow = 36;                // How many blocks the SMA averages over in hive difficulty adjust

        // Ring-fork: Pop: Consensus fields
        consensus.popBlocksPerHive = 1;                     // Expected number of pop blocks per Hive block. Note that increasing this here is not enough to spawn additional games, etc; this is used for time estimations.
        consensus.popNonceMarker = 2;                       // Nonce marker for popmined blocks
        consensus.popMinPrivateGameDepth = COINBASE_MATURITY;                           // Private game source transactions must be at least this many blocks deep
        consensus.popMaxPrivateGameDepth = COINBASE_MATURITY + 50;                      // Private game source transactions must be at most this many blocks deep
        consensus.popMaxPublicGameDepth = consensus.popMaxPrivateGameDepth + 200;       // Public game source transactions must be at most this many blocks deep
        consensus.popScoreAdjustWindowSize = 24;            // Windows size for adjusting pop score target
        consensus.popMinScoreTarget = 70;                   // Min score target
        consensus.popMaxScoreTarget = 200;                  // Max score target

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S(LAST_ID_CHAINWORK);                  // At lastInitialDistributionHeight

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S(LAST_ID_BLOCK_HASH);                // At lastInitialDistributionHeight

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xce;
        pchMessageStart[1] = 0xfe;
        pchMessageStart[2] = 0x83;
        pchMessageStart[3] = 0x9a;
        nDefaultPort = 28971;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(GENESIS_TIMESTAMP, GENESIS_NONCE, 0x1e0ffff0, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == uint256S(GENESIS_HASH));
        assert(genesis.hashMerkleRoot == uint256S(GENESIS_MERKLE));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        //vSeeds.emplace_back("xxxxx.xxxxx");
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,60);  // for R
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,81);  // for Z
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,175);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "rng";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        vFixedSeeds.clear();
        vSeeds.clear();
        checkpointData = {
            {
                {0, uint256S(GENESIS_HASH)},
                {consensus.lastInitialDistributionHeight, uint256S(LAST_ID_BLOCK_HASH)},  // Last initial distribution block
            }
        };

        chainTxData = ChainTxData{
            /* nTime    */ GENESIS_TIMESTAMP,
            /* nTxCount */ 0,
            /* dTxRate  */ 0.001
        };

        /* Allow fallback fee on mainnet */
        m_fallback_fee_enabled = true;
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        //consensus.nSubsidyHalvingInterval = 210000;   // No halving on chain
        //consensus.BIP16Exception = uint256S("0x0");   // No BIP16 exception on chain
        consensus.BIP34Height = 100;
        consensus.BIP34Hash = uint256();    // Not needed
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 90; // 90 secs
        consensus.nExpectedBlockSpacing = consensus.nPowTargetSpacing / 3;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 75; // Require 75% of last 100 blocks to activate rulechanges
        consensus.nMinerConfirmationWindow = 100;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = GENESIS_TIMESTAMP;               // Begin activation period with genesis
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = GENESIS_TIMESTAMP + 31536000;      // Genesis + 1 year

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = GENESIS_TIMESTAMP;            // Begin activation period with genesis
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = GENESIS_TIMESTAMP + 31536000;   // Genesis + 1 year

        // Ring-fork: Foreign coin fields
        const int fcc = Consensus::FOREIGN_COIN_COUNT;
        std::string foreignCoinNames[fcc] =           {"tBTC",  "tLCC", "tDOGE",    "tDASH"};
        unsigned char foreignCoinP2PKHPrefixes[fcc] = {111,     127,    113,        140};
        unsigned char foreignCoinP2SHPrefixes[fcc] =  {196,     196,    196,        19};
        unsigned char foreignCoinP2SH2Prefixes[fcc] = {0,       58,     0,          0};     // Only used by LCC and Litecoin; skipped if 0.
        unsigned char foreignCoinWIFPrefixes[fcc] =   {239,     239,    241,        239};
        std::string foreignCoinBech32HRPs[fcc] =      {"tb",    "tlcc", "",         ""};   	// Blank means no bech32 support in foreign chain

        std::copy(std::begin(foreignCoinNames), std::end(foreignCoinNames), std::begin(consensus.foreignCoinNames));        
        std::copy(std::begin(foreignCoinP2PKHPrefixes), std::end(foreignCoinP2PKHPrefixes), std::begin(consensus.foreignCoinP2PKHPrefixes));
        std::copy(std::begin(foreignCoinP2SHPrefixes), std::end(foreignCoinP2SHPrefixes), std::begin(consensus.foreignCoinP2SHPrefixes));
        std::copy(std::begin(foreignCoinP2SH2Prefixes), std::end(foreignCoinP2SH2Prefixes), std::begin(consensus.foreignCoinP2SH2Prefixes));
        std::copy(std::begin(foreignCoinWIFPrefixes), std::end(foreignCoinWIFPrefixes), std::begin(consensus.foreignCoinWIFPrefixes));
        std::copy(std::begin(foreignCoinBech32HRPs), std::end(foreignCoinBech32HRPs), std::begin(consensus.foreignCoinBech32HRPs));

        // Ring-fork: General consensus fields
        consensus.lastInitialDistributionHeight = 2000;                                                                             // Height of last block containing initial distribution payouts to foreign coins
        consensus.powLimitInitialDistribution = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");       // Lower-than-powLimit difficulty for initial distribution blocks only
        consensus.slowStartBlocks = 400;                                                                                            // Scale initial block reward up over this many blocks      
        consensus.blockSubsidyPow = 0.2 * COIN;             // Miner rewards for each block type
        consensus.blockSubsidyHive = 1 * COIN;
        consensus.blockSubsidyPopPrivate = 0.2 * COIN;
        consensus.blockSubsidyPopPublic = 0.05 * COIN;

        // Ring-fork: Hive: Consensus Fields
        consensus.dwarfCost = 0.01 * COIN;                  // Cost of a dwarf
        consensus.dwarfCreationAddress = "SUmmonTheTestnetDwarvenArmyXaNdvvm";  // Unspendable address for dwarf creation
        consensus.hiveCommunityAddress = "SW4fmbrApQcUhDd2RtVsRVts7ptizk39C9";  // Community fund address
        consensus.communityContribFactor = 5;               // Optionally, donate dct_value/maxCommunityContribFactor to community fund
        consensus.dwarfGestationBlocks = 720;               // The number of blocks for a new dwarf to mature (approx 24 hours)
        consensus.dwarfLifespanBlocks = 30000;              // The number of blocks a dwarf lives for after maturation (approx 10 days)
        consensus.powLimitHive = uint256S("0fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");  // Highest (easiest) dwarf hash target
        consensus.minHiveCheckBlock = consensus.lastInitialDistributionHeight + 1;   // Don't bother checking below this height for Hive blocks (not used for consensus/validation checks, just efficiency when looking for potential DCTs)
        consensus.hiveBlockSpacingTarget = 2;               // Target Hive block frequency (1 out of this many Hive and pow blocks combined should be Hivemined)
        consensus.hiveBlockSpacingTargetTypical = 2;        // Observed Hive block frequency (1 out of this many Hive and pow blocks combined are observed to be Hive)
        consensus.hiveNonceMarker = 1;                      // Nonce marker for hivemined blocks
        consensus.minK = 2;                                 // Minimum chainwork scale for Hive blocks (see Hive whitepaper section 5)
        consensus.maxK = 16;                                // Maximum chainwork scale for Hive blocks (see Hive whitepaper section 5)
        consensus.maxHiveDiff = 0.001;                      // Hive difficulty at which max chainwork bonus is awarded
        consensus.maxKPow = 5;                              // Maximum chainwork scale for PoW blocks
        consensus.powSplit1 = 0.0004;                       // Below this Hive difficulty threshold, PoW block chainwork bonus is halved
        consensus.powSplit2 = 0.0002;                       // Below this Hive difficulty threshold, PoW block chainwork bonus is halved again
        consensus.maxConsecutiveHiveBlocks = 2;             // Maximum hive blocks that can occur consecutively before a PoW block is required
        consensus.hiveDifficultyWindow = 36;                // How many blocks the SMA averages over in hive difficulty adjust

        // Ring-fork: Pop: Consensus fields
        consensus.popBlocksPerHive = 1;                     // Expected number of pop blocks per Hive block. Note that increasing this here is not enough to spawn additional games, etc; this is used for time estimations.
        consensus.popNonceMarker = 2;                       // Nonce marker for popmined blocks
        consensus.popMinPrivateGameDepth = COINBASE_MATURITY;                           // Private game source transactions must be at least this many blocks deep
        consensus.popMaxPrivateGameDepth = consensus.popMinPrivateGameDepth + 50;       // Private game source transactions must be at most this many blocks deep
        consensus.popMaxPublicGameDepth = consensus.popMaxPrivateGameDepth + 200;       // Public game source transactions must be at most this many blocks deep
        consensus.popScoreAdjustWindowSize = 24;            // Windows size for adjusting pop score target
        consensus.popMinScoreTarget = 70;                   // Min score target
        consensus.popMaxScoreTarget = 210;                  // Max score target

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x1304303f1de");    // 5400

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x468f8ab3e6e257f7e21a0c330986e3fd70413a381e77ce16afbadabfd26b733a"); // 5400

        pchMessageStart[0] = 0xb2;
        pchMessageStart[1] = 0xc4;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0x93;
        nDefaultPort = 28981;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(GENESIS_TIMESTAMP, GENESIS_NONCE, 0x1e0ffff0, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();       
        assert(consensus.hashGenesisBlock == uint256S(GENESIS_HASH));
        assert(genesis.hashMerkleRoot == uint256S(GENESIS_MERKLE));

        vFixedSeeds.clear();
        vSeeds.emplace_back("testnet.ringcoin.tech");
        
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,63); // For S
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,78); // For Y
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,238);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "trng";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;

        checkpointData = {
            {
                {0, uint256S(GENESIS_HASH)},
                {consensus.lastInitialDistributionHeight, uint256S("0xf953639bd2fa1645d38806dd41d2c92a51b4b0957d12382992c17f76e93adda8")},
                {5400, uint256S("0x468f8ab3e6e257f7e21a0c330986e3fd70413a381e77ce16afbadabfd26b733a")}
            }
        };

        chainTxData = ChainTxData{
            /* nTime    */ 1582987997,
            /* nTxCount */ 5603,
            /* dTxRate  */ 0.029
        };

        /* enable fallback fee on testnet */
        m_fallback_fee_enabled = true;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID = "regtest";
        //consensus.nSubsidyHalvingInterval = 150;      // No halving on chain
        //consensus.BIP16Exception = uint256S("0x0");   // No BIP16 exception on chain
        consensus.BIP34Height = 100; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();    // Not needed
        consensus.BIP65Height = 1; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1; // BIP66 activated on regtest (Used in functional tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetSpacing = 2.5 * 60; // 2.5 minutes
        consensus.nExpectedBlockSpacing = consensus.nPowTargetSpacing / 3;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144;       // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateVersionBitsParametersFromArgs(args);

        genesis = CreateGenesisBlock(GENESIS_TIMESTAMP, GENESIS_NONCE, 0x1e0ffff0, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();       
        assert(consensus.hashGenesisBlock == uint256S(GENESIS_HASH));
        assert(genesis.hashMerkleRoot == uint256S(GENESIS_MERKLE));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
                //{0, uint256S(GENESIS_HASH)},
            }
        };

        chainTxData = ChainTxData{
            /* nTime    */ GENESIS_TIMESTAMP,
            /* nTxCount */ 0,
            /* dTxRate  */ 0.001
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";

        /* enable fallback fee on regtest */
        m_fallback_fee_enabled = true;
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateVersionBitsParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateVersionBitsParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() != 3) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end");
        }
        int64_t nStartTime, nTimeout;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld\n", vDeploymentParams[0], nStartTime, nTimeout);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams(gArgs));
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}
