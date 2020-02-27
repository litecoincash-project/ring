// Copyright (c) 2018-2019 The Ring Developers
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <crypto/common.h>
#include <crypto/pow/minotaur.h>    // Ring-fork
#include <chainparams.h>            // Ring-fork: Hive

// Ring-fork
#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

// Ring-fork: Seperate block hash and pow hash
uint256 CBlockHeader::GetPowHash() const
{
    return Minotaur(BEGIN(nVersion), END(nNonce));
}

// Ring-fork: Hash arbitrary date with Minotaur
uint256 CBlockHeader::MinotaurHashArbitrary(const char *data)
{
    return Minotaur(BEGIN(data), END(data));
}

// Ring-fork: Seperate block hash and pow hash: Include powHash in ToString()
// Ring-fork: Hive: Include block type in ToString()
// Ring-fork: Pop: Include block type in ToString()
std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(type=%s, hash=%s, powHash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        IsHiveMined(Params().GetConsensus()) ? "hive" : IsPopMined(Params().GetConsensus()) ? "pop" : "pow",
        GetHash().ToString(),
        GetPowHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
