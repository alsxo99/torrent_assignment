/* 
    NXC Lab Torrent-esque P2P File Sharing System Assignment (2022 Fall) 
    Author: JS Park 

    network_functions.h is a header file for functions related to network handling.
    Although the source code for the functions declared in this file is not provided,
    you still can (and should) use the functions in this header in your implementations, 
    as long as you compile your main.c with network_functions.o (or network_functions_MAC.o).

    You are free to use any of the functions in this file for the final hand-in of your assignment,
    EXCEPT for the functions with an _ans suffix.
*/

#ifndef NETWORK_FUNCTIONS_H
#define NETWORK_FUNCTIONS_H

#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "torrent_functions.h"

#define DEFAULT_PORT 12781
#define PORT_RANGE 10
#define MAX_CONNECTIONS 16
#define MAX_LISTEN_TIME_MSEC_BASE 180
#define IS_TRACKER 0

extern int listen_port, max_listen_time_msec, silent_mode;
extern unsigned int id_hash;

// RETURN VALUES: Functions with integer return values return 0 on success, and -1 on failure, unless otherwise specified.

// Open a socket and listen for incoming connections. Returns the socket file descriptor.
int listen_socket (int port);

// Accept an incoming connection with a timeout. Returns the new socket file descriptor.
int accept_socket(int sockfd, struct sockaddr_in *cli_addr, socklen_t *clilen, int timeout_msec);

// Connect to a remote host. Returns the socket file descriptor.
int connect_socket(char *host, int port);

// Send data to a socket. Returns the number of bytes sent. (size: Data size to send)
int send_socket(int sockfd, char *buf, int size);

// Receive data from a socket. Returns the number of bytes received. (size: Buffer size)
int recv_socket(int sockfd, char *buf, int size);

// Close a socket. Returns 0 on success.
int close_socket(int sockfd);



/*  
    All protocol messages are in string, except the ones specified as BINARY.

    String messages are formatted as "[COMMAND] [ARG1] [ARG2] [ARG3]", WITH A SPACE between each argument,
    using sprintf into a "STRING_LEN" sized buffer before sending the it using send_socket(sockfd, buf, STRING_LEN).

    BINARY messages are sent directly using send_socket(sockfd, [BINARY_DATA_POINTER], [SIZE_OF_THE_BINARY_DATA]).

    For example, for protocol "[COMMAND] [STR_ARG1] [STR_ARG2] [STR_ARG3]"[BINARY_ARG1][BINARY_ARG2]
    The code to send the message would be:
        char buf[STRING_LEN];
        sprintf(buf, "%s %s %s %s", [COMMAND], [ARG1], [ARG2], [ARG3]); // (%s if [ARG] is a string, %d if [ARG] is an integer, %x if [ARG] is a hash value...)
        send_socket(sockfd, buf, STRING_LEN);
        send_socket(sockfd, [POINTER_TO_BINARY_ARG1], [SIZE_OF_BINARY_ARG1]);
        send_socket(sockfd, [POINTER_TO_BINARY_ARG2], [SIZE_OF_BINARY_ARG2]);

    Refer to the example implementation of request_torrent_from_peer() and push_torrent_to_peer() in main.c for more details.

    Note that hash values are sent as string characters of hexadecimal values, and integer values are sent as string characters of decimal values.
    Ex. Requesting torrent info from peer - Protocol: "REQUEST_TORRENT [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH]"
    If my listening port is 12781, my id_hash is 0xabcd1234 (2882343476), and the torrent hash is 0x1234abcd (305441741):
    A packet with the string "REQUEST_TORRENT 12781 abcd1234 1234abcd" is sent to the peer.
*/

// Request torrent INFO from a remote host. Returns 0 on success.
// Message protocol: "REQUEST_TORRENT [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH]"
int request_torrent_from_peer_ans(char *peer, int port, unsigned int torrent_hash);

// Push torrent INFO to peer. Returns 0 on success.
// Message protocol: "PUSH_TORRENT [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH]"[TORRENT_INFO]
// [TORRENT_INFO] is the BINARY torrent_info struct copied from the torrent_file from the peer who is sending the message. (See torrent_functions.h)
int push_torrent_to_peer_ans(char *peer, int port, torrent_file *torrent);

// Request peer's peer list from peer. Returns 0 on success.
// Message protocol: "REQUEST_PEERS [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH]"
int request_peers_from_peer_ans(char *peer, int port, unsigned int torrent_hash);

// Push my list of peers to peer. Returns 0 on success.
// Be sure to remove the receiving peer from the peer list, if there is one. (HINT: Use temporary copy of the peer IP/PORT list)
// Message protocol: "PUSH_PEERS [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH] [TORRENT_NUM_PEERS]"[PEER_IPS][PEER_PORTS]
// [TORRENT_NUM_PEERS] is the integer number of peers in the torrent.             (See torrent_functions.h)
// [PEER_IPS] is (MAX_PEER_NUM * STRING_LENG) byte BINARY data of peer IPs.       (See torrent_functions.h)
// [PEER_PORTS] is (MAX_PEER_NUM * sizeof(int)) byte BINARY data of peer ports.   (See torrent_functions.h)
int push_peers_to_peer_ans(char *peer, int port, torrent_file *torrent);

// Request peer's block info from peer. Returns 0 on success.
// Message protocol: "REQUEST_BLOCK_INFO [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH]"
int request_block_info_from_peer_ans(char *peer, int port, unsigned int torrent_hash);

// Push torrent block info to peer. Returns 0 on success.
// Message protocol: "PUSH_BLOCK_INFO [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH]"[MY_BLOCK_INFO]
// [MY_BLOCK_INFO] refers to the BINARY block info of the peer who is sending the message. (See torrent_functions.h)
int push_block_info_to_peer_ans(char *peer, int port, torrent_file *torrent);

// Request block from peer. Returns 0 on success.
// Message protocol: "REQUEST_BLOCK [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH] [BLOCK_INDEX]"
// [BLOCK_INDEX] is the integer index of the block to request.
int request_block_from_peer_ans(char *peer, int port, torrent_file *torrent, int block_idx);

// Push block to peer. Returns 0 on success.
// Message protocol: "PUSH_BLOCK [MY_LISTEN_PORT] [MY_ID_HASH] [TORRENT_HASH] [BLOCK_INDEX]"[BLOCK_DATA]
// [BLOCK_INDEX] is the integer index of the block to request.
// [BLOCK_DATA] refers "block_size" bytes of BINARY data from the block data of the peer who is sending the message. (See torrent_functions.h)
int push_block_to_peer_ans(char *peer, int port, torrent_file *torrent, int block_idx);


/*
    Parses messages from peers. Reads the command and arguments from the message,
    and calls the appropriate functions to handle the message.
    If a message is received, but there are no torrent entries corresponding to that torrent_hash,
    the message is simply ignored.
    If a torrent exists for the given hash, but the message is from an unknown peer (no matching ip/port combination),
    make sure to add the peer to the peer list of that given torrent!
*/
int server_routine_ans ();

/* 
    Iterate through the global torrent list and take appropriate actions.

    1. Iterate through the torrent list and request missing blocks from peers with 
    the said block, IF there weren't any previous requests to that peer, for this 
    client_routine_ans call.

    2. Every "peer_update_interval_msec" msec, select a new torrent from the list,
    and request block info updates from all peers in the torrent's peer list.
    Also, select a random peer from the torrent's peer list, and request a peer list
    from the selected peer.

    3. If a request to a peer fails, increment the pending request count for that peer,
    from the current torrent's peer_req_num.

    4. If the peer's number of pending requests on the current torrent's peer_req_num[] is 
    greater than PEER_EVICTION_REQ_NUM, remove the peer from the current torrent's peer list.
*/
int client_routine_ans ();

#endif // NETWORK_FUNCTIONS_H