/*
 * ring.h
 *
 *  Created on: Sep 6, 2021
 *      Author: dzhou
 */

#ifndef __RING_H_
#define __RING_H_

#include "delegation.h"
#include "pmfs_config.h"

extern int pmfs_num_of_rings_per_socket;

#if PMFS_SOLROS_RING_BUFFER
#include <solros_ring_buffer_api.h>
typedef struct solros_ring_buffer_t pmfs_ring_buffer_t;
#else
#include "simple_ring.h"
typedef struct spmfs_ring_buffer pmfs_ring_buffer_t;
#endif

extern pmfs_ring_buffer_t
	*pmfs_ring_buffer[PMFS_MAX_SOCKET][PMFS_MAX_AGENT_PER_SOCKET];

int pmfs_init_ring_buffers(int sockets);

static inline int pmfs_send_request(pmfs_ring_buffer_t *ring,
				    struct pmfs_delegation_request *request)
{
#if PMFS_SOLROS_RING_BUFFER
	return solros_ring_enqueue(ring, request,
				   sizeof(struct pmfs_delegation_request), 1);
#else
	return pmfs_sr_send_request(ring, request);
#endif
}

static inline int pmfs_recv_request(pmfs_ring_buffer_t *ring,
				    struct pmfs_delegation_request *request)
{
#if PMFS_SOLROS_RING_BUFFER
	return solros_ring_dequeue(ring, request, NULL, 1);
#else
	return pmfs_sr_receive_request(ring, request);
#endif
}

void pmfs_fini_ring_buffers(void);

#endif /* __RING_H_ */
