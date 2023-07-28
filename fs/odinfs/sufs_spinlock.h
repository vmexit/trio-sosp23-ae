#ifndef __PMFS_SUFS_SPINLOCK_H__
#define __PMFS_SUFS_SPINLOCK_H__

void sufs_spin_lock(volatile int * lock);
void sufs_spin_unlock(volatile int * lock);

void sufs_spin_init(volatile int * lock);

int sufs_spin_trylock(volatile int * lock);

#endif
