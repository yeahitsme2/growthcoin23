// ppcoin: sync-checkpoint master key
const std::string CSyncCheckpoint::strMasterPubKey = "04d6ca556127c38dc67bea3783f96c4e30390e44e6770cc6532bfbdf619920898c9f597c3488c16850d8b05db813dc1af1fde228094380a6efae83bd74bfe30bf9";

std::string CSyncCheckpoint::strMasterPrivKey = "";

// Checkpointing mode enum
enum CPMode
{
    // Strict checkpoints policy, perform conflicts verification and resolve conflicts
    STRICT = 0,
    // Advisory checkpoints policy, perform conflicts verification but don't try to resolve them
    ADVISORY = 1,
    // Permissive checkpoints policy, don't perform any checking
    PERMISSIVE = 2
};

// Returns true if block passes checkpoint checks
bool CheckHardened(int nHeight, const uint256& hash);

// Return conservative estimate of total number of blocks, 0 if unknown
int GetTotalBlocksEstimate();

// Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex);

extern uint256 hashSyncCheckpoint;
extern CSyncCheckpoint checkpointMessage;
extern uint256 hashInvalidCheckpoint;
extern CCriticalSection cs_hashSyncCheckpoint;

CBlockIndex* GetLastSyncCheckpoint();
bool WriteSyncCheckpoint(const uint256& hashCheckpoint);
bool AcceptPendingSyncCheckpoint();
uint256 AutoSelectSyncCheckpoint();
bool CheckSync(const uint256& hashBlock, const CBlockIndex* pindexPrev);
bool WantedByPendingSyncCheckpoint(uint256 hashBlock);
bool ResetSyncCheckpoint();
void AskForPendingSyncCheckpoint(CNode* pfrom);
bool SetCheckpointPrivKey(std::string strPrivKey);
bool SendSyncCheckpoint(uint256 hashCheckpoint);
bool IsMatureSyncCheckpoint();
bool IsSyncCheckpointTooOld(unsigned int nSeconds);

// ppcoin: synchronized checkpoint
class CUnsignedSyncCheckpoint
{
public:
    int nVersion;
    uint256 hashCheckpoint;      // checkpoint block

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashCheckpoint);
    )

    void SetNull()
    {
        nVersion = 1;
        hashCheckpoint = 0;
    }

    std::string ToString() const
    {
        return strprintf(
                "CSyncCheckpoint(\n"
                "    nVersion       = %d\n"
                "    hashCheckpoint = %s\n"
                ")\n",
            nVersion,
            hashCheckpoint.ToString().c_str());
    }

    void print() const
    {
        printf("%s", ToString().c_str());
    }
};

class CSyncCheckpoint : public CUnsignedSyncCheckpoint
{
public:
    std::vector<unsigned char> vchMsg;
    std::vector<unsigned char> vchSig;

    CSyncCheckpoint()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(vchMsg);
        READWRITE(vchSig);
    )

    void SetNull()
    {
        CUnsignedSyncCheckpoint::SetNull();
        vchMsg.clear();
        vchSig.clear();
    }

    bool IsNull() const
    {
        return (hashCheckpoint == 0);
    }

    uint256 GetHash() const
    {
        return SerializeHash(*this);
    }

    bool RelayTo(CNode* pnode) const
    {
        // returns true if wasn't already sent
        if (pnode->hashCheckpointKnown != hashCheckpoint)
        {
            pnode->hashCheckpointKnown = hashCheckpoint;
            pnode->PushMessage("checkpoint", *this);
            return true;
        }
        return false;
    }

    bool CheckSignature();
    bool ProcessSyncCheckpoint(CNode* pfrom);
};

// Add function declaration for automatic checkpoint selection
uint256 AutoSelectCheckpoint();
