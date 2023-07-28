/*
 * agent.h
 *
 *  Created on: Sep 2, 2021
 *      Author: dzhou
 */

#ifndef __AGENT_H_
#define __AGENT_H_

#include "pmfs_config.h"

struct pmfs_agent_tasks {
	unsigned long kuaddr;
	unsigned long size;
};

int pmfs_init_agents(int cpus, int sockets);
void pmfs_agents_fini(void);

extern int pmfs_dele_thrds;
extern int cpus_per_socket;

#endif /* __AGENT_H_ */
