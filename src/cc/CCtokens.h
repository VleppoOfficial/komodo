/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


/*
 CCassetstx has the functions that create the EVAL_ASSETS transactions. It is expected that rpc calls would call these functions. For EVAL_ASSETS, the rpc functions are in rpcwallet.cpp
 
 CCassetsCore has functions that are used in two contexts, both during rpc transaction create time and also during the blockchain validation. Using the identical functions is a good way to prevent them from being mismatched. The must match or the transaction will get rejected.
 */

#ifndef CC_TOKENS_H
#define CC_TOKENS_H

#include "CCinclude.h"

enum ETokenLicenseFlags // currently unused, may come in handy later - dan
{
	TLF_NOCOPYRIGHT = 1,
	TLF_PERFORM = 2,
	TLF_DISPLAY = 4,
	TLF_COPY = 8,
	TLF_MODIFY = 16,
	TLF_DISTRIBUTE = 32,
	TLF_SUBLICENSE = 64,
};

// CCcustom
bool TokensValidate(struct CCcontract_info *cp,Eval* eval,const CTransaction &tx, uint32_t nIn);
bool TokensExactAmounts(bool goDeeper, struct CCcontract_info *cpTokens, int64_t &inputs, int64_t &outputs, Eval* eval, const CTransaction &tx, uint256 tokenid);
std::string CreateToken(int64_t txfee, int64_t tokensupply, std::string name, std::string description, double ownerPerc, int32_t licensetype, uint256 datahash, int64_t value, std::string ccode, vscript_t nonfungibleData);
std::string TokenTransfer(int64_t txfee, uint256 assetid, std::vector<uint8_t> destpubkey, int64_t total);
std::string UpdateToken(int64_t txfee, uint256 tokenid, uint256 datahash, int64_t value, std::string ccode, int32_t licensetype);

int64_t HasBurnedTokensvouts(struct CCcontract_info *cp, Eval* eval, const CTransaction& tx, uint256 reftokenid);
CPubKey GetTokenOriginatorPubKey(CScript scriptPubKey);
bool IsTokenMarkerVout(CTxOut vout);
bool IsTokenBatonVout(CTxOut vout);
bool GetLatestTokenUpdate(uint256 tokenid, uint256 &latesttxid);
int32_t GetOwnerPubkeys(uint256 txid, uint256 reftokenid, struct CCcontract_info* cp, std::vector<uint256> &foundtxids, std::vector<std::vector<uint8_t>> &owners, std::vector<uint8_t> searchpubkey);
double GetTokenOwnershipPercent(CPubKey pk, uint256 tokenid);

int64_t GetTokenBalance(CPubKey pk, uint256 tokenid);
UniValue TokenInfo(uint256 tokenid);
UniValue TokenViewUpdates(uint256 tokenid, int32_t samplenum, int recursive);
UniValue TokenOwners(uint256 tokenid, int currentonly);
UniValue TokenInventory(CPubKey pk, int currentonly);
UniValue TokenList();


#endif
