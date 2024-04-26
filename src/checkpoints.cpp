// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "db.h"
#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // Add variables for automatic checkpointing
    int nAutoCheckpointInterval = 1000; // Interval in blocks between automatic checkpoints
    int nAutoCheckpointSpan = 1440; // Maximum span in blocks for automatic checkpoints

    // Add function declaration for automatic checkpoint selection
    uint256 AutoSelectCheckpoint();

    // What makes a good checkpoint block?
    // ...

    // Implement AutoSelectCheckpoint function
    uint256 AutoSelectCheckpoint()
    {
        const CBlockIndex *pindex = GetLastBlockIndex(pindexBest, false);
        while (pindex->pnext && (pindex->GetBlockTime() + CHECKPOINT_MAX_SPAN <= pindexBest->GetBlockTime() || pindex->nHeight + nAutoCheckpointInterval <= pindexBest->nHeight))
            pindex = pindex->pnext;
        return pindex->GetBlockHash();
    }

    // Modify GetLastSyncCheckpoint to use automatic checkpoints
    CBlockIndex* GetLastSyncCheckpoint()
    {
        LOCK(cs_hashSyncCheckpoint);
        if (hashSyncCheckpoint != 0 && mapBlockIndex.count(hashSyncCheckpoint))
            return mapBlockIndex[hashSyncCheckpoint];
        else
            return GetLastCheckpoint(mapBlockIndex);
    }

    // ProcessSyncCheckpoint function with automatic checkpoints
    bool ProcessSyncCheckpoint(CNode* pfrom)
    {
        if (!CheckSignature())
            return false;

        LOCK(Checkpoints::cs_hashSyncCheckpoint);
        if (!mapBlockIndex.count(hashCheckpoint))
        {
            // We haven't received the checkpoint chain, keep the checkpoint as pending
            Checkpoints::hashPendingCheckpoint = hashCheckpoint;
            Checkpoints::checkpointMessagePending = *this;
            printf("ProcessSyncCheckpoint: pending for sync-checkpoint %s\n", hashCheckpoint.ToString().c_str());
            // Ask this guy to fill in what we're missing
            if (pfrom)
            {
                pfrom->PushGetBlocks(pindexBest, hashCheckpoint);
                // ask directly as well in case rejected earlier by duplicate
                // proof-of-stake because getblocks may not get it this time
                pfrom->AskFor(CInv(MSG_BLOCK, mapOrphanBlocks.count(hashCheckpoint)? WantedByOrphan(mapOrphanBlocks[hashCheckpoint]) : hashCheckpoint));
            }
            return false;
        }

        if (!Checkpoints::ValidateSyncCheckpoint(hashCheckpoint))
            return false;

        CTxDB txdb;
        CBlockIndex* pindexCheckpoint = mapBlockIndex[hashCheckpoint];
        if (!pindexCheckpoint->IsInMainChain())
        {
            // checkpoint chain received but not yet main chain
            CBlock block;
            if (!block.ReadFromDisk(pindexCheckpoint))
                return error("ProcessSyncCheckpoint: ReadFromDisk failed for sync checkpoint %s", hashCheckpoint.ToString().c_str());
            if (!block.SetBestChain(txdb, pindexCheckpoint))
            {
                Checkpoints::hashInvalidCheckpoint = hashCheckpoint;
                return error("ProcessSyncCheckpoint: SetBestChain failed for sync checkpoint %s", hashCheckpoint.ToString().c_str());
            }
        }
        txdb.Close();

        if (!Checkpoints::WriteSyncCheckpoint(hashCheckpoint))
            return error("ProcessSyncCheckpoint(): failed to write sync checkpoint %s", hashCheckpoint.ToString().c_str());
        Checkpoints::checkpointMessage = *this;
        Checkpoints::hashPendingCheckpoint = 0;
        Checkpoints::checkpointMessagePending.SetNull();
        printf("ProcessSyncCheckpoint: sync-checkpoint at %s\n", hashCheckpoint.ToString().c_str());
        return true;
    }
}
