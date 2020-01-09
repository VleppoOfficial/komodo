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

#include "CCagreements.h"

/*

Note: version numbers should be reset after contract acceptance

Agreements transaction types:
	
	'p' - agreement proposal (possibly doesn't have CC inputs):
	vins.* normal input
	vin.n-1 previous proposal baton (optional)
		CC input - unlocks validation
	vout.0 marker
		can't be spent
		sent to global address
	vout.1 response hook
		can be spent by 'p', 'c' or 'u' transactions
		sent to seller/buyer 1of2 address
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'p'
		proposaltype (proposal create, proposal update, contract update, contract cancel)
		datahash
		initiatorpubkey
		receiverpubkey (can't be changed in subsequent updates, enforced by validation)
		[description]
		[mediatorpubkey]
		[deposit] (can't be changed if proposal was accepted)
		[mediatorfee](can't be changed if proposal was accepted)
		[agreementtxid]
		[prevproposaltxid]
		[depositsplit](only if tx is a contract cancel request)
	
	'c' - proposal acceptance and contract creation:
	vins.* normal input
	vin.n-1 latest proposal by seller
	vout.0 marker
		can't be spent
		sent to global address
	vout.1 update baton
		can be spent by 'u' transactions, provided they also spend the appropriate 'p' transaction
		sent to seller/buyer 1of2 address
	vout.2 seller dispute baton
		can be spent by 'd' transactions
		sent to seller CC address
	vout.3 buyer dispute baton
		can be spent by 'd' transactions
		sent to buyer CC address
	vout.4 invoice hook (can also be deposit)
		sent to agreements global CC address
		if no mediator:
			can be spent by a Settlements Payment transaction or 'u' transaction
		if mediator exists:
			can be spent by a Settlements Payment transaction (if buyer), 'u' transaction, or a 'r' transaction (if mediator)
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'c'
		sellerpubkey
		buyerpubkey
		mediator (true/false)
		[refagreementtxid]
	
	'u' - contract update:
	vins.* normal input
	vin.n-1 latest proposal by other party
	vout.0 next update baton
		can be spent by 'u' transactions, provided they also spend the appropriate 'p' transaction
		sent to seller/buyer 1of2 address
	vout.1 deposit split to party 1
		sent to party 1 normal address
	vout.2 deposit split to party 2
		sent to party 2 normal address
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'u'
		initiatorpubkey
		confirmerpubkey
		lastupdatetxid
		updateproposaltxid
		type (contract update, contract cancel)
	
	'd' - contract dispute:
	vins.* normal input
	vin.n-1 previous dispute by disputer
	vout.0 next dispute baton
		can be spent by 'd' transactions
		sent to disputer CC address
	vout.1 response hook (can also be mediator fee)
		can be spent by 'r' transactions
		if no mediator:
			sent to disputer CC address
		if mediator exists:
			sent to mediator CC address
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'd'
		lastdisputetxid(is this needed?)
		disputetype(light, heavy)
		initiatorpubkey(is this needed?)
		receiverpubkey(is this needed?)
		[description]
		[disputehash]
		
	'r' - dispute resolve:
	vins.* normal input
	vin.n-1 dispute that is being resolved
	vout.0 mediator fee OR change
		if no mediator:
			sent to disputer CC address
		if mediator exists:
			sent to mediator CC address
	vout.1 deposit redeem
		sent to either party 1 or 2, dependent on mediator
	vout.n-2 change
	vout.n-1 OP_RETURN
		EVAL_AGREEMENTS 'r'
		disputetxid
		verdict(closed by disputer, closed by mediator, deposit redeemed)
		[rewardedpubkey]
		[message]
	
Agreements RPCs:
	
	agreementaddress
		Gets relevant agreement CC, normal, global addresses, etc.
	agreementlist
		Gets every marker in the Agreements global address.
	agreementpropose(name [receiverpubkey][datahash][description][mediatorpubkey][deposit][mediatorfee][prevproposaltxid])
		Creates a proposal(update) type transaction, which will need to be confirmed by the buyer.
	agreementupdate(agreementtxid datahash [description][mediatorpubkey][prevproposaltxid])
		Creates a contract update type transaction, which will need to be confirmed by the other party.
	agreementcancel(agreementtxid datahash [description][depositsplit][prevproposaltxid])
		Creates a contract cancel type transaction, which will need to be confirmed by the other party.
	agreementaccept(proposaltxid)
		Accepts a proposal type transaction. 
		This RPC is context aware:
			if the 'p' tx is a non-contract proposal, it will create a 'c' tx
			if it is update request, then it will create 'u' tx that is of update type
			if it is cancel request, then it will create 'u' tx that is of cancel type
	agreementdispute(agreementtxid disputetype [description][disputehash])
		Creates a new agreement dispute.
	agreementresolve(agreementtxid disputetxid verdict [rewardedpubkey][message])
		Resolves the specified agreement dispute.
	agreementinfo(txid)
		Retrieves info about the specified Agreements transaction.
			- Check funcid of transaction.
			- If 'p':
				- Check proposaltype and data.
				result: success;
				name: name;
				type: funcid desc;
				initiator: pubkey1;
				- If "p":
					receiver: pubkey2 (or none);
					mediator: pubkey3 (or none);
					deposit: number (or none);
					mediatorfee: number (or none);
				- If "u":
					receiver: pubkey2;
					mediator: pubkey3 (or none);
				- If "t":
					receiver: pubkey2;
					depositsplit: percentage; (how much the receiver will get)
				iteration: iteration;
				datahash: datahash;
				description: description (or none);
				prevproposaltxid: txid (or none);
			- If 'c':
				- Check accepted proposal and verify if its 'p' type.
				result: success;
				txid: txid;
				name: name;
				type: funcid desc;
				seller: pubkey1;
				buyer: pubkey2;
				- Check for any 'u' transactions
				- If canceled:
					status: canceled;
				- Else if deposit taken by mediator:
					status: failed;
				- Else if deposit spent by Settlements Payment:
					- If payment has been withdrawn by buyer:
						status: failed;
					- Else if payment has been withdrawn by seller:
						status: completed;
				- Else:
					status: active;
				iteration: iteration;
				datahash: datahash;
				description: description (or none);
				- Check for any 'd' transactions in both vouts
					sellerdisputes: number;
					buyerdisputes: number;
				- If mediator = true:
					mediator: pubkey3 (or none);
					deposit: number;
					mediatorfee: number;
				proposaltxid: proposaltxid; <- which proposal was accepted
				refagreementtxid: txid (or none);
			- If 'u':
				result: success;
				txid: txid;
				- Get update type
				- If "u":
					type: contract update;
				- If "c":
					type: contract cancel;
				- Check accepted proposal and verify if its 'p' type.
		TODO: Finish this later
	agreementviewupdates(agreementtxid [samplenum][recursive])
		Retrieves a list of updates for the specified agreement.
	agreementviewdisputes(agreementtxid [samplenum][recursive])
		Retrieves a list of disputes for the specified agreement.
	agreementinventory([pubkey])
		Retrieves every agreement wherein the specified pubkey is the buyer, seller or mediator.
		Can look up both current and past agreements.

Agreements validation code sketch
	
	- Fetch transaction
	- Generic checks (measure boundaries, etc. Check other modules for inspiration)
	- Get transaction funcid
	- Switch case:
		if funcid == 'c':
			- Fetch the opret and make sure all data is included. If opret is malformed or data is missing, invalidate
			- Get sellerpubkey, buyerpubkey, mediator and refagreementtxid
				- Buyerpubkey: confirm that it isn't null and that current tx came from this pubkey
			- Check if vout0 marker exists and is sent to global addr. If not, invalidate
			- Check if vout1 update baton exists and is sent to seller/buyer 1of2 address. Confirm that 1of2 address can only be spent by the specified seller/buyer pubkeys.
			- Check if vout2 seller dispute baton exists and is sent to seller CC address. Confirm that the address can only be spent by the specified seller pubkey.
			- Check if vout3 buyer dispute baton exists and is sent to buyer CC address. Confirm that the address can only be spent by the specified buyer pubkey.
			- Check if vout4 exists and its nValue is >= 10000 sats.
				TODO: what to do with this? Which eval code to use for the deposit?
			- Check if vin.n-1 exists and is a 'p' type transaction. If it is not correct or non-existant, invalidate, otherwise:
				- Check if vout0 marker exists and is sent to global addr. If not, invalidate
				- Check if vin.n-1 prevout is vout1 in 'p' tx
				- Fetch the opret from 'p' tx and make sure all data is included. If opret is malformed or data is missing, invalidate
				- Get proposaltype, initiatorpubkey, receiverpubkey, mediatorpubkey, agreementtxid, deposit, mediatorfee, depositsplit, datahash, description
					- Proposaltype: must be "p" (proposal). If it is "u" (contract update) or "t" (contract terminate), invalidate
					- Initiatorpubkey: confirm that 'p' tx came from this pubkey. Make sure that initiatorpubkey == sellerpubkey
					- Receiverpubkey: confirm that the current tx came from this pubkey. Make sure that receiverpubkey == buyerpubkey
					- Mediatorpubkey: if mediator == false, must be null (or failing that, just ignored). If mediator == true, must be a valid pubkey
					- Agreementtxid: can be either null or valid agreement txid, however it must be the same as the refagreementtxid in current tx
					- Deposit: if mediator == false, must be 0. Otherwise, check if the vout4 nValue is the same as this value
					- Mediatorfee: if mediator == false, must be 0. Otherwise, must be more than 10000 satoshis
					- Depositsplit: must be 0 (or failing that, just ignored)
					- Datahash and description doesn't need to be validated, can be w/e
				- Check if vin.n-1 exists in the 'p' type transaction. If it does, get the prevout txid, then run the Løøp. If it returns false, invalidate
			- Misc Business rules
				- Make sure that sellerpubkey != buyerpubkey != mediatorpubkey
			return true?
		if funcid == 'p': 
				- Check if marker exists and is sent to global addr. If not, invalidate
				- Check if response hook exists and is sent to 1of2 CC addr (we'll confirm if this address is correct later)
				- Fetch the opret from currenttx and make sure all data is included. If opret is malformed or data is missing, invalidate
				- Get initiatorpubkey, receiverpubkey, mediatorpubkey, proposaltype, agreementtxid, deposit, mediatorfee, depositsplit, datahash, description
					- Initiatorpubkey: confirm that this tx came from this pubkey
					- Receiverpubkey: check if pubkey is valid
					- Mediatorpubkey: can be either null or valid pubkey
					- Proposaltype: can be 'a'(proposal amend),'u'(contract update) or 't' (contract terminate). If it is 'c'(proposal create), invalidate as 'c' types shouldn't be able to trigger validation
					- Agreementtxid: if proposaltype is 'a', agreementtxid must be zeroid.
						Otherwise, check if agreementtxid is valid (optionally, check if its 'c' type and that initiator and receiver match)
					- Deposit: only relevant if proposaltype is 'a' and mediatorpubkey is valid. Is otherwise ignored for now
					- Mediatorfee: only relevant if proposaltype is 'a' and mediatorpubkey is valid, in which case it must be at least 10000 satoshis. Is otherwise ignored for now
					- Depositsplit: only relevant if proposaltype is 't', and the deposit is non-zero. The deposit must be evenly divisible so that there are no remaining coins left
					- Datahash and description doesn't need to be validated, can be w/e
				- Save all this data somewhere
				- If a 'p' type is being validated, that means it probably spent a previous 'p' type, therefore check vin.n-1 prevout and get the prevouttxid. if it doesn't exist or is incorrect, invalidate
				Run the Løøp(currenttxid, currentfuncid, prevouttxid):
					- Get transaction(prevouttxid)
					- Check if marker exists and is sent to global addr. If not, invalidate
					- Check if response hook exists and is spent by currenttxid. If not, invalidate
					- Fetch the opret from prevouttx and make sure all data is included. If opret is malformed or data is missing, invalidate
					- Get initiatorpubkey, receiverpubkey, mediatorpubkey, proposaltype, agreementtxid, deposit, mediatorfee, depositsplit, datahash, description
						- Initiatorpubkey: confirm that this tx came from this pubkey
						- Receiverpubkey: check if pubkey is valid. 
						- Mediatorpubkey: can be either null or valid pubkey
		
		
	RecursiveProposalLøøp(proposaltxid,)
	- Get proposal tx
	- Check if marker exists and is sent to global addr. If not, invalidate
	- Check if response hook exists and is sent to 1of2 CC addr (we'll confirm if this address is correct later)
	
*/

//===========================================================================
//
// Opret encoding/decoding functions
//
//===========================================================================

CScript EncodeAgreementCreateOpRet(std::string name, uint256 datahash, std::vector<uint8_t> creatorpubkey, std::vector<uint8_t> clientpubkey, int64_t deposit, int64_t timelock)
{
    CScript opret; uint8_t evalcode = EVAL_AGREEMENTS, funcid = 'n';
    opret << OP_RETURN << E_MARSHAL(ss << evalcode << funcid << name << datahash << creatorpubkey << clientpubkey << deposit << timelock);
    return(opret);
}

uint8_t DecodeAgreementCreateOpRet(CScript scriptPubKey, std::string &name, uint256 &datahash, std::vector<uint8_t> &creatorpubkey, std::vector<uint8_t> &clientpubkey, int64_t &deposit, int64_t &timelock)
{
    std::vector<uint8_t> vopret; uint8_t evalcode, funcid;
    GetOpReturnData(scriptPubKey, vopret);
    if (vopret.size() > 2 && E_UNMARSHAL(vopret, ss >> evalcode; ss >> funcid; ss >> name; ss >> datahash; ss >> creatorpubkey; ss >> clientpubkey; ss >> deposit; ss >> timelock) != 0 && evalcode == EVAL_AGREEMENTS)
    {
        return(funcid);
    }
    return(0);
}

//===========================================================================
//
// Validation
//
//===========================================================================

bool AgreementsValidate(struct CCcontract_info *cp, Eval* eval, const CTransaction &tx, uint32_t nIn)
{
	return(eval->Invalid("no validation yet"));
}

//===========================================================================
//
// Helper functions
//
//===========================================================================

int64_t IsAgreementsvout(struct CCcontract_info *cp,const CTransaction& tx,int32_t v)
{
    char destaddr[64];
    if ( tx.vout[v].scriptPubKey.IsPayToCryptoCondition() != 0 )
    {
        if ( Getscriptaddress(destaddr,tx.vout[v].scriptPubKey) > 0 && strcmp(destaddr,cp->unspendableCCaddr) == 0 )
            return(tx.vout[v].nValue);
    }
    return(0);
}

//===========================================================================
//
// RPCs
//
//===========================================================================

UniValue AgreementCreate(const CPubKey& pk, uint64_t txfee, std::string name, uint256 datahash, std::vector<uint8_t> clientpubkey, int64_t deposit, int64_t timelock)
{
    CMutableTransaction mtx = CreateNewContextualCMutableTransaction(Params().GetConsensus(), komodo_nextheight());
	CPubKey mypk;
    struct CCcontract_info *cp,C; cp = CCinit(&C,EVAL_AGREEMENTS);
	
    if ( txfee == 0 )
        txfee = 10000;
    mypk = pk.IsValid()?pk:pubkey2pk(Mypubkey());
	
	CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "not done yet");
	
    /*for(auto txid : bindtxids)
    {
        if (myGetTransaction(txid,tx,hashBlock)==0 || (numvouts=tx.vout.size())<=0)
            CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "cant find bindtxid " << txid.GetHex());
        if (DecodeGatewaysBindOpRet(depositaddr,tx.vout[numvouts-1].scriptPubKey,tmptokenid,coin,totalsupply,oracletxid,M,N,pubkeys,taddr,prefix,prefix2,wiftype)!='B')
            CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "invalid bindtxid " << txid.GetHex());
    }
	
    if ( AddNormalinputs(mtx,mypk,amount,64,pk.IsValid()) >= txfee + deposit )
    {
        for (int i=0; i<100; i++) mtx.vout.push_back(MakeCC1vout(EVAL_PEGS,(amount-txfee)/100,pegspk));
        return(FinalizeCCTxExt(pk.IsValid(),0,cp,mtx,mypk,txfee,EncodePegsCreateOpRet(bindtxids)));
    }
	
    CCERR_RESULT("agreementscc",CCLOG_INFO, stream << "error adding normal inputs");*/
}

/*
UniValue AgreementCreate();
	creates new proposals, creator becomes seller
UniValue AgreementAccept();
	accepts proposals and turns them into contracts, only done by designated client

//Requests and related
UniValue AgreementRequestUpdate();
	creates an update request, only needed in contract phase
UniValue AgreementUpdate();
	updates an agreement, if in contract phase you also need request from other party
UniValue AgreementRequestCancel();
UniValue AgreementCancel();


UniValue AgreementInfo(agreementtxid);
	lists info about specific agreement (TODO: what info?)
UniValue AgreementList();
	lists all agreements/proposals/contracts (canceled ones may be omitted?)
(?)UniValue AgreementInventory();

*/
