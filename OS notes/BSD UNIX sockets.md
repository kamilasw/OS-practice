
## basic features of BSD UNIX sockets
### what is a socket?
Socket is an **endpoint for communication**.
It has a **type** (stream or datagram) and a **protocol** (TCP or UDP).
We *use a descriptor to access a socket* - created when socket is initialized.

### integration with I/O subsystem
- Socket descriptors behave like file descriptors (or FIFO)
- to read/write you can use standard functions like read()/write()
- kernel provides socket functionality - system calls can be used to create/manage sockets
- error handling is managed by the OS using:
	- error codes from functions
	- errno variable
	- error messages

### UNIX domain (local) sockets
Sockets appear as files of type "socket" in the filesystem.
Sockets enable **IPC (inter-process communication)** on the same system.

## simple communication (request-reply) with **TCP**

```scss
SERVER                                   CLIENT
-------                                  -------
socket()                                 socket()
   ↓                                        ↓
bind()                                   connect()
   ↓                                        ↓
listen()  <-----------------------------> [TCP Handshake]
   ↓
accept()
   ↓
read()   <-----------------------------  write()   ← request
[process the request]
write()  ----------------------------->  read()    ← reply
   ↓                                        ↓
close()                                  close()

```
### connection setup (opening connection)
1. Server:
	- socket() -> creates a socket
	- bind() -> assigns an address (IP + port) to the socket
	- listen() -> prepares to accept incoming connections
	- accept() -> blocks and waits for a connection from a client
2. Client:
	- socket() -> creates a socket
	- connect() -> initiates connection to the server's address
At this points, TCP establishes *a full-duplex connection between the client and server.*

### communication session
1. Client:
	- write() ->sends data to the server
2. Server:
	- read() -> reads the incoming data
	- write() -> sends a reply to the client
3. Client:
	- read() -> reads the servers response

### connection closure
both sides call **close()** to shut down the connection

## socket creation

### syntax
```C
int socket(int family, int type, int protocol);
```
- family -> protocol/address family (defines a domain)
	- PF_INET -> for connecting to different network (IPv4)
	- PF_INET6 -> (IPv6)
	- PF_UNIX -> for connecting to local ICP
- type -> communication style
	- (SOCK_STREAM - **TCP**)
	- (SOCK_DGRAM - **UDP**)
	- (SOCK_RAW)
- protocol -> specific protocol (optional) 
	- 0 - default
	- IPPROTO_TCP
	- IPPROTO_UDP
	
### return value
- success: non-negative file descriptor
- error: -1 and sets errno

### socket state
after creation: socket is in **closed** state.
to activate it - go through connection set up either as client or server

## socket options
configurable parameters of sockets

### functions for options
functions to the the options

#### syntax:

sets a configuration option on a socket
```C
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
```
- sockfd -> file descriptor returned by socket()
- level -> where the option lives (usually SOL_SOCKET or IPPROTO_TCP)
- optname -> which option you're setting
- optval -> pointer to the value you want to set
- optlen -> size of optval in bytes

##### return value
success: 0
failure: -1 set errno

#### syntax:

retrieves the current value of a socket option
```C
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
```
- sockfd -> file descriptor for the socket
- level -> where the option lives (usually SOL_SOCKET or IPPROTO_TCP)
- optname -> which option you want to query
- optval -> pointer to memory to receive the value
- optlen -> input

##### return value
success: 0
error: -1

### general socket options

| option                  | type           | description                                                                                      |
| ----------------------- | -------------- | ------------------------------------------------------------------------------------------------ |
| SO_ACCEPTCONN           | int            | gets the result of 'is the socket in listen mode?'                                               |
| SO_BREADCAST            | int            | allow sending datagrams to broadcast address                                                     |
| SO_DEBUG                | int            | enable kernel debugging                                                                          |
| SO_DONTROUTE            | int            | bypass routing tables (send directly to interface)                                               |
| SO_ERROR                | int            | retrieve pending error                                                                           |
| SO_KEEPALIVE            | int            | send keepalive probes (TCP-specific)                                                             |
| SO_LINGER               | struct linger  | delay close() to send remaining data -> control whether close() waits for remaining data to send |
| SO_OOBINLINE            | int            | deliver out-of-band data inline                                                                  |
| SO_RCVBUF/SO_SNDBUF     | int            | set receive/send buffer sizes                                                                    |
| SO_RCVLOWAT/SO_SNDLOWAT | int            | minimum # of bytes before read/write returns                                                     |
| SO_RCVTIMEO/SO_SNDTIMEO | struct timeval | timeout for read/write ops -> avoid blocking forever on read/write                               |
| SO_REUSEADDR            | int            | allow reusing local address in bind() -> used by servers to quickly restart after crashing       |
| SO_TYPE                 | int            | get socket type (eg. SOCK_STREAM, SOCK_DGRAM)                                                    |

### TCP specific options
to access them use: **level=IPPROTO_TCP**
only applies to TCP sockets

| options       | type | description                                                                                                                   |
| ------------- | ---- | ----------------------------------------------------------------------------------------------------------------------------- |
| TCP_KEEPALIVE | int  | time between keepalive probes (if SO_KEEPAVILE is on) -> used to detect dead peers in long-lived connections                  |
| TCP_MAXRT     | int  | max retransmission time                                                                                                       |
| TCP_MAXSEG    | int  | max segment size (MMS)                                                                                                        |
| TCP_NODELAY   | int  | disable Nagle's algorithm -> send small packets immediately -> used in latency-sensitive apps like games or real-time systems |
| TCP_QUICKACK  | int  | send ACKs immediately instead of delaying (linux-only) -> useful for protocols that rely on low RTT                           |

## address structures (POSIX) 
each socket needs: address structure which specifies *where to connect or bind*

### generic placeholder
used for function parameters like bind() connect() accept()
```C
struct sockaddr {
    sa_family_t sa_family; // Address family (AF_INET, AF_UNIX, etc.)
    char        sa_data[]; // Address data (family-specific)
};
```

you cast specific address structs to sockaddr* when passing to system calls

### IPv4 address structure
used when binding or connecting to IPv4 addresses (AF_INET)
```C
struct sockaddr_in {
    sa_family_t     sin_family; // AF_INET
    in_port_t       sin_port;   // Port number (16-bit)
    struct in_addr  sin_addr;   // IPv4 address (32-bit)
};
```

sin_port and sin_addr must be in network byte order (use htons() htonl())

### IPv6 address structure
used for IPv6 communication (AF_INET6)

```C
struct sockaddr_in6 {
    sa_family_t     sin6_family;    // AF_INET6
    in_port_t       sin6_port;      // Port (16-bit)
    uint32_t        sin6_flowinfo;  // Flow label (QoS)
    struct in6_addr sin6_addr;      // IPv6 address (128-bit)
    uint32_t        sin6_scope_id;  // Interface index (e.g., for link-local addresses)
};
```

also requiers network byte order for port and addr


### UNIX domain socket address
used for local IPC using UNIX domain sockets (AF_UNIX/AF_LOCAL)

```C
struct sockaddr_un {
    sa_family_t sun_family; // AF_UNIX
    char        sun_path[]; // Filesystem path (e.g., /tmp/socket)
};
```

no IP or port - just a file path

## conversion functions
### byte order conversion functions
- network protocols use big-endian
- many machines use little-endian
so we must convert values
used when dealing with binary integer values like port numbers or IP addresses in raw binary
#### conversion functions
these are required when working with sockaddr_in and similar structs

| host type | function | convert                        |
| --------- | -------- | ------------------------------ |
| uint16_t  | htons()  | host to network short (16-bit) |
| uint16_t  | ntohs()  | network to host short          |
| uint32_t  | htonl()  | host to network long (32-bit)  |
| uint32_t  | ntohl()  | network to host long           |

### address conversion functions
convert between human-readable IP strings and binary representations 
- from "192.168.0.1" to struct in_addr

#### IPv4 address conversions

##### conversion without error handling (discouraged)

converts an IPv4 address in string format 'p' to a binary address 'in_addr_t'
```c
#include <arpa/inet.h>
in_addr_t inet_addr(const char *p);
```

the binary structure:
```c
#include <netinet/in.h>  // or <arpa/inet.h>
struct in_addr {
    in_addr_t s_addr;  // 32-bit IPv4 address (usually unsigned long or uint32_t)
};
```
###### return value
success: returns valid structure
failure: (in_addr_t)-1 however 255.255.255.255 also maps to -1

converts an IPv4 address from binary form to human-readable dot format
```c
char *inet_ntoa(struct in_addr inaddr);
```
**not thread safe**: returns a pointer to static memory - subsequent calls override previous results

##### conversion with error handling (non-POSIX but preferred)

converts from string to binary form, storing the result in `*adrptr`
```c
int inet_aton(const char *p, struct in_addr *addrptr);
```

###### return value
success: 1
error: 0 (invalid IP string)

#### IPv4 & IPv6 address conversions with error handling (POSIX)

##### binary to string (presentation format)

converts a numeric (binary) IP address to textual string form
```c
#include <arpa/inet.h>
const char *inet_ntop(
    int af,              // AF_INET for IPv4, AF_INET6 for IPv6
    const void *src,     // pointer to binary address (e.g. struct in_addr or in6_addr)
    char *dst,           // buffer to write the string
    socklen_t size       // size of buffer `dst`
);
```
handles both IPv4 and IPv6

###### return value
success: points to dst
error: NULL and sets errno

##### string to binary (network form)

converts string IP to binary IP
```c
#include <arpa/inet.h>
int inet_pton(
    int af,             // address family (AF_INET or AF_INET6)
    const char *src,    // string like "192.168.0.1" or "::1"
    void *dst           // pointer to buffer to store the binary result
);
```
reliable for:
- IPv4: dot format
- IPv6: colon-separated hex groups

###### return value
success: 1
failure:
	0 if not valid address
	-1 if **af** is invalid (not AF_INET or AF_INET6)


### converting host name-addresses
how to use hostnames in network programming:
- converting domain names "domain.com" -> IP addresses
- vice versa
#### struct hostent
holds info about a host including its addresses

```c
struct hostent {
    char    *h_name;       // official name of the host
    char    **h_aliases;   // list of alias names
    int     h_addrtype;    // address type (AF_INET or AF_INET6)
    int     h_length;      // length of address in bytes (4 for IPv4, 16 for IPv6)
    char    **h_addr_list; // list of pointers to IP addresses
};
```


#### getting host info from a database
functions rarely used today:
- gethostnet() – sequentially retrieves entries from the DNS-like host database
- endhostnet() - closes the database opened by gethostnet()

#### hostname-IP conversion functions

##### name->IP
```c
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

int getaddrinfo(
    const char *node,             // Hostname (e.g. "example.com") or IP address string (e.g. "8.8.8.8");
                                  // NULL means local address (useful for server sockets)
    const char *service,          // Service name (e.g. "http") or port number as string (e.g. "80")
    const struct addrinfo *hints, // Optional input struct to specify preferences (e.g. AF_INET, SOCK_STREAM);
                                  // can be NULL if no preferences
    struct addrinfo **res         // Output: linked list of results (you must free with freeaddrinfo())
);
```

###### return value:
success: 0
error: non-zero error code (use gai_strerror() to convert to string)

```c
void freeaddrinfo(struct addrinfo *res);/* memory dealloc. of struct addrinfo*/
```

##### IP->name
```c
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

int getnameinfo(
    const struct sockaddr *sa,   // Pointer to socket address (usually from getaddrinfo() or accept())
    socklen_t salen,             // Size of the socket address structure (e.g. sizeof(struct sockaddr_in))
    char *host,                  // Output buffer for hostname or IP string
    size_t hostlen,              // Size of the host buffer
    char *serv,                  // Output buffer for service name (e.g. "http") or port number
    size_t servlen,              // Size of the service buffer
    int flags                    // Optional flags:
                                 // NI_NUMERICHOST – return numeric address instead of hostname
                                 // NI_NUMERICSERV – return port number instead of service name
);

```

###### return value:
success: 0
error: non zero value (use gai_strerror() )

## binding a name (address) to a socket

### syntax
```c
#include <sys/socket.h>
int bind(
    int sockfd,                  //Socket file descriptor
    const struct sockaddr *addr,//Pointer to address to bind
    socklen_t addrlen            //Size of the structure `addr` points to
);
```

### what bind() does
assigns a local address to a socket
- gives an IP+port so other can reach the socket
- The `sockfd` must already be created (via `socket()`)
- `addr` specifies which IP and port to use

### return value
success: 0
error: -1 errno set

### when is bind() needed?
required for servers
clients skip this

### what happens if `port==0`
then the system assigns an ephemeral port (a random available one)
- used when you want a temp client socket without caring about exact port
### what happens if `IPaddress==INADDR_ANY (0.0.0.0)`
kernel binds the socket to all available interfaces on the machine
#### when does the OS assign the address?
if the INADDR_ANY is used ->address assignment is deferred until:
- socket is connected (TCP) 
- datagram is sent (UDP)
ensures proper routing/interface selection

### file system permissions (for UNIX domain sockets)
the socket address may be a file (for AF_UNIX)
- the path must be absolute
- permissions are influenced by the unmask

### multihoming models
multihoming = one machine, multiple network interfaces

| Model      | Description                                                                                   |
| ---------- | --------------------------------------------------------------------------------------------- |
| **Strong** | Only accept datagrams that arrive on the **same IP/interface** the socket was bound to        |
| **Weak**   | Accept datagrams that match the destination IP, even if they arrived on a different interface |

## opening TCP connection

- the listening and accepter sockets:
	- share the same port number on the server side
	- but each client gets a unique `<client_IP>:<client_port>` → `<server_IP>:<server_port>` connection
- new accepted sockets inherit options BUT non-blocking mode has to be set using fcntl() with F_SETFL

### non blocking mode
```c
#include <fcntl.h>
int sockfd, val;
val = fcntl(sockfd, F_GETFL, 0);
fcntl(sockfd, F_SETFL, val | O_NONBLOCK);
```
sets the socket into non-blocking mode
- connect() and accept() return immediately

#### example: non blocking connect()+select()
```c
int sockfd = socket(...);
fcntl(sockfd, F_SETFL, O_NONBLOCK);

int ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
if (ret < 0 && errno == EINPROGRESS) {
    // Connection is in progress — wait for it to complete
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);

    struct timeval timeout = {5, 0}; // wait max 5 seconds
    int sel = select(sockfd + 1, NULL, &wfds, NULL, &timeout);

    if (sel > 0 && FD_ISSET(sockfd, &wfds)) {
        // Socket is ready for writing: check for errors
        int err;
        socklen_t len = sizeof(err);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);
        if (err == 0) {
            // ✅ Connected successfully
        } else {
            // ❌ Connection failed
        }
    } else {
        // ❌ Timeout or select() error
    }
}

```

### TCP connection lifecycle
| Client (Active)                      | Server (Passive) |
| ------------------------------------ | ---------------- |
| `socket()`                           | `socket()`       |
| _(optional)_ `bind()`                | `bind()`         |
| `connect()`                          | `listen()`       |
|                                      | `accept()`       |
| communication via `read()`/`write()` |                  |

### client-side TCP setup (active socket)

- **ephemeral binding** - if the socket isn't already bound with bind() the OS automatically binds it to an random local port
	- **UNIX domain exception** - when using UNIX domain sockets, automatic binding doesnt happen
- **connection refused (ECONNREFUSED)** - if the server's queue of pending connections is full or server isnt listening
- **interrupted connection (INTR)** - if connect() is interrupted by signal
#### syntax
initiates a 3-way TCP handshake with the server
```c
int connect(
    int sockfd,                        //client socket descriptor (from socket())
    const struct sockaddr *servaddr,  //pointer to server's address (IPv4 or IPv6)
    socklen_t servaddrlen             //size of the address struct
);
```

##### return value
success: 0
error: -1 sets errno

### server side setup (passive socket)

#### syntax:
```c
int listen(
    int sockfd,   // server socket descriptor (already bound!)
    int backlog   // max number of pending connections in queue
);
```
- marks socket as **passive** - ready to accept connections

##### return value:
success: 0
error: -1 sets errno

#### syntax:
```c
int accept(
    int sockfd,                          //listening socket descriptor
    struct sockaddr *cliaddr,            //will be filled with client’s address
    socklen_t *addrlen                   //in/out: size of cliaddr buffer
);
```
- accepts first client waiting in queue
- original `sockfd` stays open for more accept calls

##### return value:
success: returns a new socket descriptor for communication with that client
error: -1 errno set

## TCP sockets - data transfer

all functions require the header:
```c
#include <sys/socket.h>
```

### basic and extended data transfer functions

#### read
reads up to `nbytes` from the socket info `buff`
```c
ssize_t read(int sockfd, void *buff, size_t nbytes);
```
works on any file descriptor

##### return value
success:  >0 numberof bytes read
connection closed by peer: 0
error: -1 errno set

#### write
writes up to `nbytes` from `buff` into the socket
```c
ssize_t write(int sockfd, const void *buff, size_t nbytes);
```
its generic

##### return value
success: >0 num of bytes written
error: -1 check errno

#### recv
socket-specific read function - lets u use extra flags for custom behaviour
```c
ssize_t recv(int sockfd, void *buff, size_t nbytes, int flags);
```


##### return value
succes: >0 number of bytes received
peer has performed an orderly shutdown: 0
error: -1

#### send
socket-specific write function - accepts flags for more control
```c
ssize_t send(int sockfd, const void *buff, size_t nbytes, int flags);
```

##### return value
success: >=0 num of bytes sent
error: -1

### flags for `recv()/send()`

| Flag           | Meaning                                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `MSG_PEEK`     | Peek at incoming data **without removing** it from queue                                                                           |
| `MSG_OOB`      | Receive **Out-of-Band** data (urgent TCP data)                                                                                     |
| `MSG_DONTWAIT` | Make call **non-blocking**                                                                                                         |
| `MSG_WAITALL`  | `recv()`: wait until **all `nbytes`** are received, unless an error or signal occurs - useful when expecting a fixed-sized message |

### reading/writing with multiple buffers

#### syntax
```c
struct iovec {
    void  *iov_base;  // pointer to buffer
    size_t iov_len;   // number of bytes to transfer
};
```

reads/writes all the buffers in one sys call
```c
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t writev(int sockfd, const struct iovec *iov, int iovcnt);
```
- `iov` -> array of iovec structs (buffers)
- `iovcnt` -> num of buffers

used for gather-write or scatter-read patterns

### what if peer closed their read-end?
when calling write():
- if the other end already closed reading side then:
	- the call fails
	- errno = EPIPE
	- kernel send SIGPIPE by default

### remote closed
A `read()` returns `0` when:
- Peer performed a **graceful shutdown** (e.g., `close()` on their end)
- You’ve read all remaining bytes in the socket’s buffer
This is how you detect connection termination on your side.

## closing TCP connection

### partial socket shutdown

use when you want to close just one direction of the connection (read or write)
```c
#include <sys/socket.h>
int shutdown(
    int sockfd,   // socket descriptor
    int how       // what to shut down
);
```
- AFTER the direction shuts down:
	- reading returns 0 (EOF)
	- writing may fail with EPIPE

#### `how` options:

|Value|Meaning|
|---|---|
|`SHUT_RD` (0)|Disallow further **reads** (incoming data)|
|`SHUT_WR` (1)|Disallow further **writes** (outgoing data)|
|`SHUT_RDWR`(2)|Disallow **both reads and writes**|

#### return values
success: 0
fail: -1 errno set

### full socket destruction
closes the socket entirely - both read and write
```c
#include <sys/socket.h>
int close(int sockfd);
```
Also decreases the file descriptor reference count (just like `close()` on files). If the count hits zero → the socket is destroyed.

`close()` disallows further **reception and transmission**, and its effects also depend on:
- Whether other file descriptors still reference the socket (via `dup()`, `fork()`, etc.)
- Whether `SO_LINGER` is set (explained in slide 2)
#### return values
success: 0
fail: -1 errno set

### what is `linger`?
linger - delay window to allow outgoing data to be transmitted before actually destroying the socket

#### how to control it?
```c
struct linger {
    int l_onoff;   // enable(≠0) or disable(0) linger
    int l_linger;  // timeout in seconds
};
```

how to set it up:
```c
struct linger sl;
sl.l_onoff = 1;       // enable linger
sl.l_linger = 5;      // wait up to 5 seconds

setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));

```
#### behavior

##### `l_onoff = 0` → default (no linger)
- `close()` returns **immediately**
- OS **tries** to send remaining data in the background
- But you don't wait to find out if it succeeded
- **Receiving buffer** (on your side) is **dropped**

##### `l_onoff ≠ 0` → linger is enabled
- `close()` (or `shutdown()`) **blocks**
- Waits until:
    - All pending data is **sent**, or
    - `l_linger` timeout (in seconds) is reached
- If time runs out → remaining data is dropped
- **Receiving buffer** is still dropped

## `SOCK_STREAM` sockets

- **Reliable**: no packet loss; if it gets lost, TCP resends it.
- **Sequenced**: data is received in the same order it was sent. 
- **Full-duplex**: you can send and receive at the same time (bidirectional).
- **Octet stream** = raw stream of bytes (not message-based).

### must be connected
A socket of type `SOCK_STREAM` must be in a connected state before any data may be sent or received.
Unlike UDP (`SOCK_DGRAM`), you can’t just fire off packets — **connection must be established first**.

### no record boundaries
- TCP is a **stream**, not a message protocol.
- If you send `"hello"` then `"world"`, the receiver may:
    - receive `"hellow"` then `"orld"`
    - or receive `"h"`, `"e"`, `"ll"`, `"o world"`...
- It’s your job to define **message boundaries** (e.g., using length-prefix, delimiters, etc.)

### buffered output
- When `send()` or `write()` returns, it means:
    - The data was copied to the **kernel send buffer**
    - It has **not necessarily** reached the remote peer
- Actual delivery depends on:
    - **Network conditions**
    - **Receiver's readiness**
    - **TCP flow control**
 TCP guarantees delivery — just not **instantaneous** delivery.
### broken connections
When a stream is broken (e.g., the peer **closed** their socket):
- Your next `send()` or `write()` might:
    - Fail with `errno = EPIPE`
    - Raise a `SIGPIPE` signal  - which by default **terminates your program** unless handled
### out of band data support
TCP includes a **minimal** mechanism for “urgent” data:
- Limited to **1 byte**
- You send it with `MSG_OOB` flag in `send()`
- The idea: interrupt/control messages (e.g., "STOP")

## simple communication (request-reply) with **UDP**

**UDP is connectionless** (datagram-based):
- No connection setup (`connect()` not used)
- Each message is an independent unit
- No guarantees (order, delivery, duplicates)

### communication flow
We have two programs:
- **Server**: waits for messages, replies
- **Client**: sends a request, waits for reply

```scss
           UDP Request-Reply Flow (simplified)
           ===================================

      [Server]                          [Client]
   -----------------              -----------------
   socket()                        socket()
   bind()                          bind() (optional)
   recvfrom()   <--------------    sendto()
   [process request]
   sendto()     -------------->    recvfrom()
   close()                         close()

           <-- communication happens here -->

```

### server side

1. **`socket()`**
    - Create a UDP socket (`SOCK_DGRAM`)
2. **`bind()`**
    - Bind to a local port/IP (so client can send data to you)
3. **`recvfrom()`**
    - Waits (blocks) for an incoming datagram
    - Gets the client's address from the datagram
4. **`sendto()`**
    - Send the reply back to the client using the address from `recvfrom()`   
5. **`close()`**
    - Release the socket

### client side

1. **`socket()`**
    - Create a UDP socket
2. **`bind()`**
    - (Optional) binds to local IP/port (can be omitted; OS picks one)    
3. **`sendto()`**
    - Sends the request datagram to the server’s IP/port    
4. **`recvfrom()`**
    - Waits for a reply from the server
5. **`close()`**
    - Close the socket

### I/O functions in UDP communications

Datagram = one atomic message

- If the datagram is bigger than the buffer in `recvfrom()`, the rest is **discarded**.
- There’s **no streaming** like TCP -  you get each message once and completely, or not at all.

```c
#include <sys/socket.h>
ssize_t recvfrom(int sockfd, void *buff, size_t nbytes, int flags,
                 struct sockaddr *from, socklen_t *paddrlen);

ssize_t sendto(int sockfd, const void *buff, size_t nbytes, int flags,
               const struct sockaddr *to, socklen_t addrlen);
```

#### parameters:
|Parameter|Applies to|Description|
|---|---|---|
|`sockfd`|both|The UDP socket file descriptor (created with `socket()`)|
|`buff`|both|Pointer to a buffer used to receive or send data|
|`nbytes`|both|Number of bytes to read/write|
|`flags`|both|Optional flags (e.g. `MSG_DONTWAIT`, `MSG_PEEK`)|
|`from`|`recvfrom()`|If not `NULL`, points to a structure that gets filled with sender's address|
|`paddrlen`|`recvfrom()`|Initially contains the size of `from`; updated to actual size used|
|`to`|`sendto()`|Address of the target receiver (e.g., a `sockaddr_in` struct)|
|`addrlen`|`sendto()`|Size of the structure pointed to by `to`|

#### examples:

##### receive a datagram and learn who sent it
```c
char buffer[512];
struct sockaddr_in sender_addr;
socklen_t addrlen = sizeof(sender_addr);

ssize_t received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                            (struct sockaddr*)&sender_addr, &addrlen);
```

##### send a datagram to a specific target
```c
struct sockaddr_in target;
target.sin_family = AF_INET;
target.sin_port = htons(12345);
inet_pton(AF_INET, "192.168.0.10", &target.sin_addr);

sendto(sockfd, buffer, length, 0, (struct sockaddr*)&target, sizeof(target));
```

##### check or set the max datagram size
```c
int size;
socklen_t len = sizeof(size);
getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &size, &len);
```

## connected UDP sockets

`write()` does not let you specify the destination address.
- if you want to use `write()` with UDP, you must first **connect the socket** to a specific remote address using `connect()`.

Normally, UDP is **connectionless**: each `sendto()` specifies the destination address.
But if you call `connect()` on a UDP socket:
- It **locks** the socket to one destination.  
- You can then use `write()`/`read()` (or `send()`/`recv()` without `from`/`to`).

### disconnecting a connected UDP socket
This clears the “connected” state.
```c
connect(sockfd, NULL, 0);
```
or use:

`PF_UNSPEC` is a protocol family used to signal “disconnect”.
```c
connect(sockfd, &addr_with_family_PF_UNSPEC, 0);
```


## retrieving socket names (addresses)

### get you own sockets address
```c
#include <sys/socket.h>

int getsockname(int sockfd, struct sockaddr *name, socklen_t *namelen);
```
Retrieves the **local address** that the socket is bound to (your IP and port).
Can be used **after `bind()`** or even **after `connect()`**.

useful when you let the OS pick a port (e.g. bind with port `0`) and want to know what was chosen.

### get the peer's address
```c
#include <sys/socket.h>

int getpeername(int sockfd, struct sockaddr *name, socklen_t *namelen);
```
Only works if the socket is **connected** (e.g., connected TCP or connected UDP).

Returns the **remote peer’s IP and port**.

### before either function!!!
You must **initialize** `*namelen` with the size of the `struct sockaddr` you're passing in.

```c
socklen_t len = sizeof(struct sockaddr_in);
struct sockaddr_in addr;
getsockname(sockfd, (struct sockaddr *)&addr, &len);
```
Both functions **modify** `*namelen` to the actual size used.

