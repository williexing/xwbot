/*
 * x_ice.h
 *
 *  Created on: 23.08.2011
 *      Author: mgolubev
 */

#ifndef X_ICE_H_
#define X_ICE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


typedef struct ice_session {
	int common_socket;
} ice_session_t;


typedef struct ice_addr {
	union {
	  struct sockaddr     addr;
	  struct sockaddr_in  ip4;
	  struct sockaddr_in6 ip6;
	} s;
} ice_addr_t;


typedef enum {
  ICE_TURN_RELAY_TYPE_UDP,
  ICE_TURN_RELAY_TYPE_TCP,
  ICE_TURN_RELAY_TYPE_TLS
} ice_relay_type_t;


typedef struct turn_server {
	ice_addr_t       addr;
	char*            username;
	char*            password;
	ice_relay_type_t type;
} turn_server_t;


/**
 * ICE candidate
 */

typedef enum {
	ICE_CANDIDATE_TYPE_HOST,           // LOCAL MACHINE
	ICE_CANDIDATE_TYPE_REFLEXIVE,      // STUN
	ICE_CANDIDATE_TYPE_PEER_REFLEXIVE, // STUN
	ICE_CANDIDATE_TYPE_RELAYED,        // TURN
} ice_candidate_type_t;


typedef struct ice_candidate {
	ice_candidate_type_t type;
	ice_addr_t           addr;
	uint32_t             priority;
	uint32_t             component_id;
	uint32_t             foundation;
	turn_server_t        turn;
} ice_candidate_t;


typedef struct ice_candidate_list {
	ice_candidate_t  candidate;
	ice_candidate_t* next;
} ice_candidate_list_t;


#endif /* X_ICE_H_ */
