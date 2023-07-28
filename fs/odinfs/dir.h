/*
 * dir.h
 *
 *  Created on: Nov 26, 2021
 *      Author: dzhou
 */

#ifndef __DIR_H_
#define __DIR_H_

#include "balloc.h"
#include "pmfs_def.h"

int pmfs_insert_dir_tree(struct super_block *sb,
			 struct pmfs_inode_info_header *sih, const char *name,
			 int namelen, struct pmfs_direntry *direntry);

struct pmfs_direntry *pmfs_find_dentry(struct super_block *sb,
				       struct pmfs_inode *pi, struct inode *inode,
				       const char *name, unsigned long name_len);

static inline void pmfs_delete_dir_tree(struct super_block *sb,
		  struct pmfs_inode_info_header *sih)
{
	pmfs_destroy_range_node_tree(&sih->rb_tree);
}


#endif /* DIR_H_ */
