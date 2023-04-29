#include "../Process.hpp"
#include "filesystems/VFS.hpp"
#include "fcntl.h"
#include "errno.h"

class ProcessFDSocket : public IProcessFD
{
private:
    struct Peer
    {
        bool isValid;
        ProcessFD *fd;

        bool Closed();

        Peer() : isValid(false), fd(NULL) {}
    };

    struct Message
    {
        vector<uint8_t> buffer;
        bool isValid;
        Peer *peer;

        Message() : isValid(false), peer(NULL) {}
    };

    vector<Peer *> peers;
    vector<Message *> messages;
    vector<Peer *> pendingPeers;

    bool closed;
    bool connectionRefused;
    bool nonBlocking;

    AtomicLock socketLock;
public:
    uint32_t domain;
    uint32_t type;
    uint32_t protocol;
    uint16_t port;

    ProcessFDSocket();

    ProcessFDSocket(uint32_t domain, uint32_t type, uint32_t protocol, uint16_t port = 0);

    bool IsNonBlocking();

    void AddPendingPeer(ProcessFD *fd);

    Peer *PendingPeer();

    Peer *AddPeer(ProcessFD *fd);

    Peer *FindPeer(ProcessFDSocket *socket);

    bool Connected();

    bool ConnectionRefused();

    void RefuseConnection();

    void EnqueueMessage(const void *buffer, uint64_t length, Peer *peer);

    bool HasMessage(Peer *peer);

    vector<uint8_t> GetMessage(Peer *peer);

    vector<uint8_t> PeekMessage(Peer *peer);

    virtual void Close() override;

    virtual uint64_t Read(void *buffer, uint64_t length, int *error) override;

    virtual uint64_t Write(const void *buffer, uint64_t length, int *error) override;

    virtual int64_t Seek(uint64_t offset, int whence, int *error) override;

    virtual dirent *ReadEntries() override;

    virtual struct stat Stat(int *error) override;
};
