/* 
    NXC Lab Torrent-esque P2P File Sharing System Assignment (2022 Fall) 
    Author: JS Park 

    torrent_functions.h is a header file for functions related to torrent data handling.
    Although the source code for the functions declared in this file is not provided,
    you still can (and should) use the functions in this header in your implementations,
    as long as you compile your main.c with torrent_functions.o (or torrent_functions_MAC.o).

    You are free to use any of the functions in this file for the final hand-in of your assignment.
*/

#ifndef TORRENT_FUNCTIONS_H
#define TORRENT_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STRING_LEN 128
#define MAX_FILE_SIZE (128*1024*1024ULL) // 128MiB
#define BLOCK_SIZE_DEFAULT (32*1024ULL) // 32KiB  
#define MAX_BLOCK_NUM (MAX_FILE_SIZE/BLOCK_SIZE_DEFAULT)
#define MAX_PEER_NUM 128
#define PEER_UPDATE_INTERVAL_MSEC_BASE 3000
#define PEER_EVICTION_REQ_NUM 5
#define MAX_TORRENT_NUM 512

typedef struct torrent_file torrent_file;
typedef struct torrent_info torrent_info;
extern torrent_file *global_torrent_list [MAX_TORRENT_NUM];
extern int num_torrents, peer_update_interval_msec;

// Struct for managing torrent data and associated information
struct torrent_file 
{
    char name[STRING_LEN];              // Name of the torrent
    unsigned int hash;                  // Hash value of the torrent

    unsigned int size;                  // Size of the torrent file (!= block_size*block_num)
    unsigned int downloaded_block_num;  // Number of downloaded blocks
    unsigned int block_num;             // Total number of blocks
    unsigned int block_size;            // Size of each block
    char block_info [MAX_BLOCK_NUM];    // Block info of the torrent (0: not downloaded, 1: downloaded)
    char *block_ptrs[MAX_BLOCK_NUM];    // Pointers to the start of each block
    char *data;                         // Pointer to the start of torrent data

    // A peer is defined by a unique <ip, port> pair, which is separately stored in peer_ip[]/port_port[] array.
    // The same peer index is used across all peer_[XXX] arrays for each unique peer.
    
    unsigned int num_peers;                             // Number of peers on this torrent
    char peer_ip[MAX_PEER_NUM][STRING_LEN];             // IP address of each peer
    int peer_port[MAX_PEER_NUM];                        // Port number of each peer
    char peer_req_num[MAX_PEER_NUM];                    // Number of pending requests to peers 
    char peer_block_info[MAX_PEER_NUM][MAX_BLOCK_NUM];  // Block info of each peer
};

// Struct for storing torrent information only.
struct torrent_info 
{
    char name[STRING_LEN];
    unsigned int hash;
    unsigned int size, block_num, block_size;
    char block_info [MAX_BLOCK_NUM];
};

/*
    BUGFIX - 2021-11-07
    The original code assumed that the same peer would be in the same idx location on different torrents, which is stupid...
    This struct is added to implement a linked list of peers over different torrents.

    NOTE:
    The original intention of tracking whether a peer has been requested was to avoid sending multiple block request to the same peer and stressing the said peer.
    Peer evicition is a sperate feature handled by peer_req_num[] array in EACH torrent sturct, and is for checking if a peer is alive(responsive),
    and is NOT intended for fairness / evicting peers that does not upload its blocks etc...
    I unfortunately used the same "peer_req_num" name for both features, which caused some confusion according to the QnA board.

    This assignment was created in a hurry, and I didn't had the time to thoroughly test the code. I appologize for the confusion.

    - JS Park
*/

// Linked list of peers
typedef struct peer_linked_list
{
    char ip[STRING_LEN];
    int port;
    struct peer_linked_list *next;
} peer_linked_list;
// Returns 1 if the peer is already in the linked list(requested), 0 otherwise. Also adds the peer to the linked list.
int is_peer_requested_add_if_not(peer_linked_list *peer_list, char *ip, int port);
// Function to free the linked list.
void free_peer_linked_list(peer_linked_list *peer_list);



// RETURN VALUES: Functions with integer return values return 0 on success, and -1 on failure, unless otherwise specified.

// Sleeps for milliseconds
void sleep_ms (int milliseconds);

// Returns the current time in milliseconds
unsigned int get_time_msec();

// Loads files and parses it into torrent_file struct. Torrent is added to global_torrent_list.
int make_file_into_torrent (char* torrent_name, char* input_file_path);

// Saves torrent data to a file
int save_torrent_into_file (torrent_file *torrent, char *output_file_path);

// Inits dynamic data structures in torrent_file struct 
// (All data initialized EXCEPT name, hash, size, block_num, block_size, block_info)
int init_torrent_dynamic_data (torrent_file *torrent);

// Adds torrent to global_torrent_list
int add_torrent (torrent_file* torrent);

// Removes torrent from global_torrent_list
int remove_torrent (unsigned int torrent_hash);

// Get pointer to the torrent from global_torrent_list from its hash value.
// Returns NULL if not found. Use this to check if a torrent is already in the list.
torrent_file *get_torrent (unsigned int torrent_hash);

// Copies torrent information from torrent_info struct into torrent struct
int copy_info_to_torrent (torrent_file *torrent, torrent_info *info);

// Copies torrent information from torrent struct into torrent_info struct
int copy_torrent_to_info (torrent_file *torrent, torrent_info *info);

// Request torrent info from a peer, using the torrent's hash value
int request_from_hash (unsigned int torrent_hash, char *peer_ip, int peer_port);


// Adds peer to a torrent.
// Also updates peer's block info, if peer_block_info != NULL.
// If peer_block_info == NULL, it skips the block info update.
// Returns an error message and -1 if the peer is already in the torrent.
// Use get_peer_idx() to check if the peer is already in the torrent, before calling this function!
int add_peer_to_torrent (torrent_file *torrent, char* peer_ip, int peer_port, char* peer_block_info);

// Removes peer from a torrent
int remove_peer_from_torrent (torrent_file *torrent, char *peer_ip, int peer_port);

// Returns the peer index from a torrent from its IP address and port number (-1 if not found)
int get_peer_idx (torrent_file *torrent, char *peer_ip, int peer_port);

// Updates peer's block information in a torrent
void update_peer_block_info (torrent_file *torrent, char* peer_ip, int peer_port, char* block_info);


// Prints torrent information
void print_torrent_info (torrent_file *torrent);

// Prints all torrent information in a global_torrent_list
void print_all_torrents ();

// Prints all torrent information in a global_torrent_list, in a compact form
void print_torrent_status ();

#endif // TORRENT_FUNCTIONS_H