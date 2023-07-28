/*
 * utils.h
 *
 *  Created on: Sep 14, 2021
 *      Author: dzhou
 */

#ifndef __PMFS_UTILS_H_
#define __PMFS_UTILS_H_

#include <asm/page.h>

#define PMFS_ROUNDUP_PAGE(addr) (((addr)&PAGE_MASK) + PAGE_SIZE)

#endif /* __PMFS_UTILS_H_ */
