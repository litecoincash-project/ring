// Copyright (c) 2018-2019 The Ring Developers
// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>

#include <base58.h>
#include <bech32.h>
#include <script/script.h>
#include <util/strencodings.h>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include <assert.h>
#include <string.h>
#include <algorithm>

namespace
{
class DestinationEncoder : public boost::static_visitor<std::string>
{
private:
    const CChainParams& m_params;

public:
    explicit DestinationEncoder(const CChainParams& params) : m_params(params) {}

    std::string operator()(const CKeyID& id) const
    {
        std::vector<unsigned char> data = m_params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
        data.insert(data.end(), id.begin(), id.end());
        return EncodeBase58Check(data);
    }

    std::string operator()(const CScriptID& id) const
    {
        std::vector<unsigned char> data = m_params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
        data.insert(data.end(), id.begin(), id.end());
        return EncodeBase58Check(data);
    }

    std::string operator()(const WitnessV0KeyHash& id) const
    {
        std::vector<unsigned char> data = {0};
        data.reserve(33);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.begin(), id.end());
        return bech32::Encode(m_params.Bech32HRP(), data);
    }

    std::string operator()(const WitnessV0ScriptHash& id) const
    {
        std::vector<unsigned char> data = {0};
        data.reserve(53);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.begin(), id.end());
        return bech32::Encode(m_params.Bech32HRP(), data);
    }

    std::string operator()(const WitnessUnknown& id) const
    {
        if (id.version < 1 || id.version > 16 || id.length < 2 || id.length > 40) {
            return {};
        }
        std::vector<unsigned char> data = {(unsigned char)id.version};
        data.reserve(1 + (id.length * 8 + 4) / 5);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.program, id.program + id.length);
        return bech32::Encode(m_params.Bech32HRP(), data);
    }

    std::string operator()(const CNoDestination& no) const { return {}; }
};

// Ring-fork: Decode any foreign address and transform it into a Ring CTxDestination
CTxDestination DecodeAnyDestination(const std::string& str, const CChainParams& params, std::string& detectedType)
{
    const Consensus::Params& consensusParams = params.GetConsensus();

    std::vector<unsigned char> data;
    uint160 hash;
    // Try and parse as non-bech32
    if (DecodeBase58Check(str, data)) {
        // For each coin
        for (int i = 0; i < Consensus::FOREIGN_COIN_COUNT; i++) {
            // Try p2pkh: Data is RIPEMD160(SHA256(pubkey))
            const std::vector<unsigned char>& pubkey_prefix = std::vector<unsigned char>(1,consensusParams.foreignCoinP2PKHPrefixes[i]);
            if (data.size() == hash.size() + pubkey_prefix.size() && std::equal(pubkey_prefix.begin(), pubkey_prefix.end(), data.begin())) {
                std::copy(data.begin() + pubkey_prefix.size(), data.end(), hash.begin());
                detectedType = consensusParams.foreignCoinNames[i] + " (P2PKH)";
                return CKeyID(hash);
            }

            // Try p2sh: Data is RIPEMD160(SHA256(cscript))
            const std::vector<unsigned char>& script_prefix = std::vector<unsigned char>(1,consensusParams.foreignCoinP2SHPrefixes[i]);
            if (data.size() == hash.size() + script_prefix.size() && std::equal(script_prefix.begin(), script_prefix.end(), data.begin())) {
                std::copy(data.begin() + script_prefix.size(), data.end(), hash.begin());
                detectedType = consensusParams.foreignCoinNames[i] + " (P2SH)";
                return CScriptID(hash);
            }

            // Try p2sh alt prefix (because of LTC, see Thrasher's comment in https://github.com/litecoin-project/litecoin/issues/433)
            if (consensusParams.foreignCoinP2SH2Prefixes[i] == 0)  // Skip foreign coins without a SCRIPT_KEY2 prefix (most of them)
                continue;
            const std::vector<unsigned char>& script_prefix2 = std::vector<unsigned char>(1,consensusParams.foreignCoinP2SH2Prefixes[i]);
            if (data.size() == hash.size() + script_prefix2.size() && std::equal(script_prefix2.begin(), script_prefix2.end(), data.begin())) {
                std::copy(data.begin() + script_prefix2.size(), data.end(), hash.begin());
                detectedType = consensusParams.foreignCoinNames[i] + " (P2SH with alt prefix)";
                return CScriptID(hash);
            }
        }
    }
    data.clear();

    // Try and parse as bech32 for each coin
    for (int i = 0; i < Consensus::FOREIGN_COIN_COUNT; i++) {
        if (consensusParams.foreignCoinBech32HRPs[i] == "") // Skip foreign coins which don't support bech32
            continue;
        auto bech = bech32::Decode(str);
        if (bech.second.size() > 0 && bech.first == consensusParams.foreignCoinBech32HRPs[i]) {
            // Bech32 decoding
            int version = bech.second[0]; // The first 5 bit symbol is the witness version (0-16)
            // The rest of the symbols are converted witness program bytes.
            data.reserve(((bech.second.size() - 1) * 5) / 8);
            if (ConvertBits<5, 8, false>([&](unsigned char c) { data.push_back(c); }, bech.second.begin() + 1, bech.second.end())) {
                if (version == 0) {
                    {
                        WitnessV0KeyHash keyid;
                        if (data.size() == keyid.size()) {
                            std::copy(data.begin(), data.end(), keyid.begin());
                            detectedType = consensusParams.foreignCoinNames[i] + " witness_v0_keyhash (P2WPKH)";
                            return keyid;
                        }
                    }
                    {
                        WitnessV0ScriptHash scriptid;
                        if (data.size() == scriptid.size()) {
                            std::copy(data.begin(), data.end(), scriptid.begin());
                            detectedType = consensusParams.foreignCoinNames[i] + " witness_v0_scripthash (P2WSH)";
                            return scriptid;
                        }
                    }
                }
                /*
                if (version > 16 || data.size() < 2 || data.size() > 40) {
                    return CNoDestination();
                }*/
                WitnessUnknown unk;
                unk.version = version;
                std::copy(data.begin(), data.end(), unk.program);
                unk.length = data.size();
                detectedType = consensusParams.foreignCoinNames[i] + " witness_unknown (P2WUN)";
                return unk;
            }
        }
    }

    return CNoDestination();
}

CTxDestination DecodeDestination(const std::string& str, const CChainParams& params)
{
    std::vector<unsigned char> data;
    uint160 hash;
    if (DecodeBase58Check(str, data)) {
        // base58-encoded Ring addresses.
        // Public-key-hash-addresses have version 0 (or 111 testnet).
        // The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
        const std::vector<unsigned char>& pubkey_prefix = params.Base58Prefix(CChainParams::PUBKEY_ADDRESS);
        if (data.size() == hash.size() + pubkey_prefix.size() && std::equal(pubkey_prefix.begin(), pubkey_prefix.end(), data.begin())) {
            std::copy(data.begin() + pubkey_prefix.size(), data.end(), hash.begin());
            return CKeyID(hash);
        }
        // Script-hash-addresses have version 5 (or 196 testnet).
        // The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
        const std::vector<unsigned char>& script_prefix = params.Base58Prefix(CChainParams::SCRIPT_ADDRESS);
        if (data.size() == hash.size() + script_prefix.size() && std::equal(script_prefix.begin(), script_prefix.end(), data.begin())) {
            std::copy(data.begin() + script_prefix.size(), data.end(), hash.begin());
            return CScriptID(hash);
        }
    }
    data.clear();
    auto bech = bech32::Decode(str);
    if (bech.second.size() > 0 && bech.first == params.Bech32HRP()) {
        // Bech32 decoding
        int version = bech.second[0]; // The first 5 bit symbol is the witness version (0-16)
        // The rest of the symbols are converted witness program bytes.
        data.reserve(((bech.second.size() - 1) * 5) / 8);
        if (ConvertBits<5, 8, false>([&](unsigned char c) { data.push_back(c); }, bech.second.begin() + 1, bech.second.end())) {
            if (version == 0) {
                {
                    WitnessV0KeyHash keyid;
                    if (data.size() == keyid.size()) {
                        std::copy(data.begin(), data.end(), keyid.begin());
                        return keyid;
                    }
                }
                {
                    WitnessV0ScriptHash scriptid;
                    if (data.size() == scriptid.size()) {
                        std::copy(data.begin(), data.end(), scriptid.begin());
                        return scriptid;
                    }
                }
                return CNoDestination();
            }
            if (version > 16 || data.size() < 2 || data.size() > 40) {
                return CNoDestination();
            }
            WitnessUnknown unk;
            unk.version = version;
            std::copy(data.begin(), data.end(), unk.program);
            unk.length = data.size();
            return unk;
        }
    }
    return CNoDestination();
}
} // namespace

CKey DecodeSecret(const std::string& str)
{
    CKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data)) {
        const std::vector<unsigned char>& privkey_prefix = Params().Base58Prefix(CChainParams::SECRET_KEY);
        if ((data.size() == 32 + privkey_prefix.size() || (data.size() == 33 + privkey_prefix.size() && data.back() == 1)) &&
            std::equal(privkey_prefix.begin(), privkey_prefix.end(), data.begin())) {
            bool compressed = data.size() == 33 + privkey_prefix.size();
            key.Set(data.begin() + privkey_prefix.size(), data.begin() + privkey_prefix.size() + 32, compressed);
        }

        // Ring-fork: Allow private key import from supported foreign chains
        const Consensus::Params& consensusParams = Params().GetConsensus();
        for (int i = 0; i < Consensus::FOREIGN_COIN_COUNT; i++) {
            const std::vector<unsigned char>& foreign_privkey_prefix = std::vector<unsigned char>(1,consensusParams.foreignCoinWIFPrefixes[i]);
            if ((data.size() == 32 + foreign_privkey_prefix.size() || (data.size() == 33 + foreign_privkey_prefix.size() && data.back() == 1)) &&
                std::equal(foreign_privkey_prefix.begin(), foreign_privkey_prefix.end(), data.begin())) {
                bool compressed = data.size() == 33 + foreign_privkey_prefix.size();
                key.Set(data.begin() + foreign_privkey_prefix.size(), data.begin() + foreign_privkey_prefix.size() + 32, compressed);
            }
        }        
    }
    if (!data.empty()) {
        memory_cleanse(data.data(), data.size());
    }
    return key;
}

std::string EncodeSecret(const CKey& key)
{
    assert(key.IsValid());
    std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::SECRET_KEY);
    data.insert(data.end(), key.begin(), key.end());
    if (key.IsCompressed()) {
        data.push_back(1);
    }
    std::string ret = EncodeBase58Check(data);
    memory_cleanse(data.data(), data.size());
    return ret;
}

CExtPubKey DecodeExtPubKey(const std::string& str)
{
    CExtPubKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data)) {
        const std::vector<unsigned char>& prefix = Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY);
        if (data.size() == BIP32_EXTKEY_SIZE + prefix.size() && std::equal(prefix.begin(), prefix.end(), data.begin())) {
            key.Decode(data.data() + prefix.size());
        }
    }
    return key;
}

std::string EncodeExtPubKey(const CExtPubKey& key)
{
    std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY);
    size_t size = data.size();
    data.resize(size + BIP32_EXTKEY_SIZE);
    key.Encode(data.data() + size);
    std::string ret = EncodeBase58Check(data);
    return ret;
}

CExtKey DecodeExtKey(const std::string& str)
{
    CExtKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data)) {
        const std::vector<unsigned char>& prefix = Params().Base58Prefix(CChainParams::EXT_SECRET_KEY);
        if (data.size() == BIP32_EXTKEY_SIZE + prefix.size() && std::equal(prefix.begin(), prefix.end(), data.begin())) {
            key.Decode(data.data() + prefix.size());
        }
    }
    return key;
}

std::string EncodeExtKey(const CExtKey& key)
{
    std::vector<unsigned char> data = Params().Base58Prefix(CChainParams::EXT_SECRET_KEY);
    size_t size = data.size();
    data.resize(size + BIP32_EXTKEY_SIZE);
    key.Encode(data.data() + size);
    std::string ret = EncodeBase58Check(data);
    memory_cleanse(data.data(), data.size());
    return ret;
}

std::string EncodeDestination(const CTxDestination& dest)
{
    return boost::apply_visitor(DestinationEncoder(Params()), dest);
}

CTxDestination DecodeDestination(const std::string& str)
{
    return DecodeDestination(str, Params());
}

// Ring-fork: Decode any foreign address and transform it into a Ring CTxDestination
CTxDestination DecodeAnyDestination(const std::string& str, std::string& detectedType)
{
    return DecodeAnyDestination(str, Params(), detectedType);
}

bool IsValidDestinationString(const std::string& str, const CChainParams& params)
{
    return IsValidDestination(DecodeDestination(str, params));
}

bool IsValidDestinationString(const std::string& str)
{
    return IsValidDestinationString(str, Params());
}
