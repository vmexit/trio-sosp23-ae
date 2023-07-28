#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "agent.h"
#include "delegation.h"
#include "pmfs_config.h"
#include "pmfs_stats.h"
#include "ring.h"

#include "pmfs.h"

#if PMFS_SOLROS_RING_BUFFER
#include <solros_ring_buffer_api.h>
#else
#include "simple_ring.h"
#endif

pmfs_ring_buffer_t *pmfs_ring_buffer[PMFS_MAX_SOCKET][PMFS_MAX_AGENT_PER_SOCKET];

static int pmfs_number_of_sockets;
int pmfs_num_of_rings_per_socket;

#if PMFS_SOLROS_RING_BUFFER
/* 64B alignment */
#define PMFS_RING_ALIGN (64)
#endif

int pmfs_init_ring_buffers(int sockets)
{
	int i, j;

	pmfs_number_of_sockets = sockets;
	pmfs_num_of_rings_per_socket = pmfs_dele_thrds;

	for (i = 0; i < sockets; i++)
		for (j = 0; j < pmfs_dele_thrds; j++) {
			pmfs_ring_buffer_t *ret;
			/* nonblocking ring buffer for each socket */
#if PMFS_SOLROS_RING_BUFFER
			ret = solros_ring_create(PMFS_RING_SIZE,
						 PMFS_RING_ALIGN, i, 0);
#else
			ret = pmfs_sr_create(
				i, sizeof(struct pmfs_delegation_request),
				PMFS_RING_SIZE);
#endif

			if (ret == NULL)
				goto err;

			pmfs_ring_buffer[i][j] = ret;
		}

	return 0;

err:
	pmfs_fini_ring_buffers();

	return -ENOMEM;
}

void pmfs_fini_ring_buffers(void)
{
	int i, j;

	for (i = 0; i < pmfs_number_of_sockets; i++)
		for (j = 0; j < pmfs_num_of_rings_per_socket; j++) {
			if (!pmfs_ring_buffer[i][j])
				continue;

#if PMFS_SOLROS_RING_BUFFER
			solros_ring_destroy(pmfs_ring_buffer[i][j]);
#else
			pmfs_sr_destroy(pmfs_ring_buffer[i][j]);
#endif
		}
}
