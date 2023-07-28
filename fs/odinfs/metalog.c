#include "metalog.h"


void inode_size_phase_info_init(struct phase_meta_info *pinfo)
{

	raw_spin_lock_init(&pinfo->lock);
	pinfo->val = 0;
	pinfo->vals = alloc_percpu_gfp(ssize_t, GFP_KERNEL);
	BUG_ON(!pinfo->vals);
	
	atomic_set(&pinfo->phase, 0);
	pinfo->bias = 0;
	pinfo->inhibit_until = 0;
	pcpu_rwsem_init_rwsem(&pinfo->phase_lock);	
}

void inode_size_phase_info_destroy(struct phase_meta_info *pinfo)
{
	pcpu_rwsem_free_rwsem(&pinfo->phase_lock);
	free_percpu(pinfo->vals);
}

ssize_t inode_size_read(struct phase_meta_info *pinfo)
{
	return pinfo->val;
}

void inode_size_inc(struct phase_meta_info *pinfo, ssize_t value)
{
	phase_read_lock(&pinfo->phase_lock);
	preempt_disable();
	this_cpu_add(*pinfo->vals, value);
	preempt_enable();
	phase_read_unlock(&pinfo->phase_lock);
}

void inode_size_dec(struct phase_meta_info *pinfo, ssize_t value)
{
	phase_read_lock(&pinfo->phase_lock);
	preempt_disable();
	this_cpu_add(*pinfo->vals, -1 * value);
	preempt_enable();
	phase_read_unlock(&pinfo->phase_lock);
}

void inode_size_sync(struct phase_meta_info *pinfo)
{
	int cpu;
	unsigned long flags;

	raw_spin_lock_irqsave(&pinfo->lock, flags);
	for_each_possible_cpu(cpu) {
		ssize_t *val = per_cpu_ptr(pinfo->vals, cpu);
		pinfo->val += *val;
		*val = 0;
	}
	raw_spin_unlock_irqrestore(&pinfo->lock, flags);
}

ssize_t inode_size_sync_read(struct phase_meta_info *pinfo)
{
	int cpu;
	unsigned long flags;
	ssize_t *val, total = 0;

	phase_write_lock(&pinfo->phase_lock);
	raw_spin_lock_irqsave(&pinfo->lock, flags);
	for_each_possible_cpu(cpu) {
		val = per_cpu_ptr(pinfo->vals, cpu);
		pinfo->val += *val;
		*val = 0;
	}
	total = pinfo->val;
	raw_spin_unlock_irqrestore(&pinfo->lock, flags);
	phase_write_unlock(&pinfo->phase_lock);

	return total;
}

void inode_size_reset(struct phase_meta_info *pinfo)
{
	int cpu;
	unsigned long flags;

	raw_spin_lock_irqsave(&pinfo->lock, flags);
	for_each_possible_cpu(cpu)
		*per_cpu_ptr(pinfo->vals, cpu) = 0;
	raw_spin_unlock_irqrestore(&pinfo->lock, flags);
}

void inode_size_change_phase(struct phase_meta_info *pinfo, int phase)
{
	phase_change_lock(&pinfo->phase_lock);
	inode_size_sync(pinfo);
	atomic_set(&pinfo->phase, phase);
	phase_change_unlock(&pinfo->phase_lock);
}
