/*
 * Copyright (C) 1996-2017 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_FDE_H
#define SQUID_FDE_H

#include "comm.h"
#include "defines.h"
#include "ip/Address.h"
#include "ip/forward.h"
#include "security/forward.h"
#include "typedefs.h" //DRCB, DWCB

#if USE_DELAY_POOLS
class ClientInfo;
#endif
class dwrite_q;

/**
 * READ_HANDLER functions return < 0 if, and only if, they fail with an error.
 * On error, they must pass back an error code in 'errno'.
 */
typedef int READ_HANDLER(int, char *, int);

/**
 * WRITE_HANDLER functions return < 0 if, and only if, they fail with an error.
 * On error, they must pass back an error code in 'errno'.
 */
typedef int WRITE_HANDLER(int, const char *, int);

class _fde_disk
{
public:
    _fde_disk() { wrt_handle = nullptr; }

    DWCB *wrt_handle;
    void *wrt_handle_data = nullptr;
    dwrite_q *write_q = nullptr;
    dwrite_q *write_q_tail = nullptr;
    off_t offset = 0;
};

class fde
{

public:
    fde() {
        *ipaddr = 0;
        *desc = 0;
        read_handler = nullptr;
        write_handler = nullptr;
        read_method = nullptr;
        write_method = nullptr;
    }

    /// Clear the fde class back to NULL equivalent.
    void clear() { *this = fde(); }

    /// True if comm_close for this fd has been called
    bool closing() const { return flags.close_request; }

    /* NOTE: memset is used on fdes today. 20030715 RBC */
    static void DumpStats(StoreEntry *);

    char const *remoteAddr() const;
    void dumpStats(StoreEntry &, int) const;
    bool readPending(int) const;

    /// record a transaction on this FD
    void noteUse() { ++pconn.uses; }

public:

    /// global table of FD and their state.
    static fde* Table;

    unsigned int type = 0;
    unsigned short remote_port = 0;

    Ip::Address local_addr;
    tos_t tosToServer = '\0';      /**< The TOS value for packets going towards the server.
                                        See also tosFromServer. */
    nfmark_t nfmarkToServer = 0;   /**< The netfilter mark for packets going towards the server.
                                        See also nfmarkFromServer. */
    int sock_family = 0;
    char ipaddr[MAX_IPSTRLEN];            /* dotted decimal address of peer */
    char desc[FD_DESC_SZ];

    struct _fde_flags {
        bool open = false;
        bool close_request = false; ///< true if file_ or comm_close has been called
        bool write_daemon = false;
        bool socket_eof = false;
        bool nolinger = false;
        bool nonblocking = false;
        bool ipc = false;
        bool called_connect = false;
        bool nodelay = false;
        bool close_on_exec = false;
        bool read_pending = false;
        //bool write_pending; //XXX seems not to be used
        bool transparent = false;
    } flags;

    int64_t bytes_read = 0;
    int64_t bytes_written = 0;

    struct {
        int uses = 0;                   /* ie # req's over persistent conn */
    } pconn;

#if USE_DELAY_POOLS
    /// pointer to client info used in client write limiter or nullptr if not present
    ClientInfo * clientInfo = nullptr;
#endif
    unsigned epoll_state = 0;

    _fde_disk disk;
    PF *read_handler;
    void *read_data = nullptr;
    PF *write_handler;
    void *write_data = nullptr;
    AsyncCall::Pointer timeoutHandler;
    time_t timeout = 0;
    time_t writeStart = 0;
    void *lifetime_data = nullptr;
    AsyncCall::Pointer closeHandler;
    AsyncCall::Pointer halfClosedReader; /// read handler for half-closed fds
    READ_HANDLER *read_method;
    WRITE_HANDLER *write_method;
    Security::SessionPointer ssl;
    Security::ContextPointer dynamicTlsContext; ///< cached and then freed when fd is closed
#if _SQUID_WINDOWS_
    struct {
        long handle = (long)nullptr;
    } win32;
#endif
    tos_t tosFromServer = '\0';        /**< Stores the TOS flags of the packets from the remote server.
                                            See FwdState::dispatch(). Note that this differs to
                                            tosToServer in that this is the value we *receive* from the,
                                            connection, whereas tosToServer is the value to set on packets
                                            *leaving* Squid.  */
    unsigned int nfmarkFromServer = 0; /**< Stores the Netfilter mark value of the connection from the remote
                                            server. See FwdState::dispatch(). Note that this differs to
                                            nfmarkToServer in that this is the value we *receive* from the,
                                            connection, whereas nfmarkToServer is the value to set on packets
                                            *leaving* Squid.   */
};

#define fd_table fde::Table

int fdNFree(void);

#define FD_READ_METHOD(fd, buf, len) (*fde::Table[fd].read_method)(fd, buf, len)
#define FD_WRITE_METHOD(fd, buf, len) (*fde::Table[fd].write_method)(fd, buf, len)

#endif /* SQUID_FDE_H */

