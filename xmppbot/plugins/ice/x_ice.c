/*
 * x_ice.c
 *
 *  Created on: Aug 16, 2011
 *      Author: artemka
 */

#include <sys/types.h>
#include <linux/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[x_ice] "
#include <x_types.h>
#include <x_stun.h>

#include <ice/x_ice.h>

/**
 *
 */
EXPORT_SYMBOL void
x_ice_destroy(ice_session_t *ice)
{
}

EXPORT_SYMBOL ice_session_t *x_ice_new
(int (*valid_pair_callback)(ice_candidate_t *local, ice_candidate_t *remote, void *callback_data),
		void *callback_data)
{
	ice_session_t *session;
	struct sockaddr_in addr;

	ENTER;

	if((local == NULL) || (remote == NULL) || (callback_data == NULL))
		return NULL;

	session = calloc(1, sizeof(ice_session_t));
	if(session == NULL)
		return NULL;

	/**
	 * Gather LOCAL candidates
	 */


    /**
     * Gather REFLEXIVE candidates
     */

    /**
     * Gather RELAYED candidates
     */

    EXIT;

    return sess;
}

EXPORT_SYMBOL int
x_ice_add_local_candidate(ice_session_t *ice, ice_candidate_t *cand)
{
	return 0;
}

EXPORT_SYMBOL int
x_ice_add_remote_candidate(ice_session_t *ice, ice_candidate_t *cand)
{
	return 0;
}


/**
 * Get all local interfaces & IP addresses for them
 */
/* TODO change to list type */
char* ice_get_local_ips()
{
	struct ifreq *ifr;
	struct ifconf ifc;
	int sock;
	int numif;
	char* list = NULL;

	// Get number of interfaces
	ifc.ifc_len = 0;
	ifc.ifc_ifcu.ifcu_req = NULL;

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
		perror("socket");
		return NULL;
	}

	if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
		perror("ioctl");
		return NULL;
	}

	if ((ifr = malloc(ifc.ifc_len)) == NULL) {
		perror("malloc");
		return NULL;
	}
	ifc.ifc_ifcu.ifcu_req = ifr;

	if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
		perror("ioctl2");
		return NULL;
	}

	close(s);

	numif = ifc.ifc_len / sizeof(struct ifreq);
	for (int i = 0; i < numif; i++) {
		struct ifreq *r = &ifr[i];
	 	struct sockaddr_in *sin = (struct sockaddr_in *)&r->ifr_addr;
	 	printf("%-8s : %s\n", r->ifr_name, inet_ntoa(sin->sin_addr));
	}

	return list;
}


/**
 * Candidate
 */

ice_candidate_t* ice_candidate_new(ice_candidate_type_t type)
{
	ice_candidate_t* cand;

	cand = (ice_candidate_t*)malloc(sizeof(ice_candidate_t));
	if(cand == NULL) {
		return NULL;
	}

	memset(cand, 0, sizeof(ice_candidate_t));

	cand->type = type;

	return cand;
}

void ice_candidate_free(ice_candidate_t* cand)
{
	if(cand == NULL) {
		return;
	}

	if(cand->turn->username) {
		free(cand->turn->username);
	}

	if(cand->turn->password) {
		free(cand->turn->password);
	}

	free(cand);
}


#if 0
static void *
ice_session_input_worker(void *arg)
  {
    ice_session_t *_this;
    int i;
    int err;
    struct sockaddr cliaddr;
    struct sockaddr_in myaddr;
    struct in_addr *inaddr;
    char buf[128];
    stun_hdr_t *hdr;
    socklen_t len;
    stun_packet_t *pkt;
    fd_set sockset;
    struct timeval tv;
    int sock;
    long long int lasttime;
    long long int curtime;
#define DEFAULT_STUN_TIMEOUT_SEC 5
    long long int waittime;

    ENTER;

    _this = (ice_session_t *) arg;
    if (!_this)
    return NULL;

    /*
     if (!_this->addrs || !_this->candidates || _this->status < 0)
     return -1;
     */

    if (_this->sock < 0)
    return NULL;

    sock = dup(_this->sock);

    if (sock < 0)
    return NULL;

    /* setup address */
    memset(&cliaddr, 0, sizeof(struct sockaddr));
    cliaddr.sa_family = AF_INET;
    inaddr = (struct in_addr *) (cliaddr.sa_data + sizeof(in_port_t));
    inaddr->s_addr = htonl(INADDR_ANY);

    /* set socket to nonblcking mode */
    fcntl(sock, F_SETFL, O_NONBLOCK);

    /* determine expiration time */
    waittime = DEFAULT_STUN_TIMEOUT_SEC * 1000000;
    gettimeofday(&tv, NULL);
    lasttime = ((long long int) tv.tv_sec) * 1000000 + (long long int) tv.tv_usec;
    lasttime += waittime;

    pthread_mutex_lock(&_this->data_mx);

    for (;;)
      {
        FD_ZERO(&sockset);
        FD_SET(sock,&sockset);

        /* recalculate select timeout */
        gettimeofday(&tv, NULL);
        curtime = ((long long int) tv.tv_sec) * 1000000
        + (long long int) tv.tv_usec;
        waittime = lasttime - curtime;
        if (waittime <= 0)
        break;

        tv.tv_sec = waittime / 1000000;
        tv.tv_usec = waittime % 1000000;

        err = select(sock + 1, &sockset, NULL, NULL, &tv);

        if (err <= 0)
        continue;

        if (FD_ISSET(sock, &sockset))
          {
            while ((len = recvfrom(sock, (void *) buf, sizeof(buf), 0, &cliaddr,
                        &len)) > 0)
              {
                hdr = (stun_hdr_t *) buf;
                if (hdr->ver_type.raw == htons(STUN_RESP))
                  {
                    pkt = stun_packet_from_hdr(hdr, "qwe");
                    if (pkt)
                      {
                        i = stun_attr_id(pkt, STUN_X_MAPPED_ADDRESS);
                        if (i >= 0)
                          {
                            stun_from_x_mapped(pkt->attrs[i].data,
                                (struct sockaddr_in *) &myaddr);
                            printf("+++++++++++++ ### MY PUBLIC IP '%s:%d'!\n",
                                inet_ntoa(myaddr.sin_addr),
                                ntohs(myaddr.sin_port));
                          }
                      }
                    break;
                  }
              }
          }
      }

    /* finished */
    /* NOTIFY REMOTE */
    _this->status = 1;
    pthread_mutex_unlock(&_this->data_mx);

    return NULL;
  }
#endif
