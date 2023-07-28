#include <linux/percpu.h>
#include <linux/uaccess.h>

#include "agent.h"
#include "delegation.h"
#include "pmfs_config.h"
#include "pmfs_stats.h"
#include "pmfs_utils.h"
#include "ring.h"

struct pmfs_notifyer_array {
};

static inline int pmfs_choose_rings(void)
{
	return xor_random() % pmfs_num_of_rings_per_socket;
}

/* Per-client CPU variable */
/* Here we assume those values are initialized to 0 */
DEFINE_PER_CPU(struct pmfs_notifyer_array, pmfs_completed_cnt);

unsigned int pmfs_do_read_delegation(struct pmfs_sb_info *sbi,
				     struct mm_struct *mm, unsigned long uaddr,
				     unsigned long kaddr, unsigned long bytes,
				     int zero, long *pmfs_issued_cnt,
				     struct pmfs_notifyer *pmfs_completed_cnt,
				     int wait_hint)
{
	int socket;
	unsigned int ret = 0;
	struct pmfs_delegation_request request;
	unsigned long i = 0, uaddr_end = 0;
	int block;
	int thread;
	PMFS_DEFINE_TIMING_VAR(prefault_time);
	PMFS_DEFINE_TIMING_VAR(send_request_time);

	/* TODO: Check the validity of the user-level address */

	/*
   * access the user address while still at the process's address space
   * to let the kernel handles various situations: e.g., page not mapped,
   * page swapped out.
   *
   * Surprisingly, the overhead of this part is significant. However,
   * currently it looks to me no point to optimize this part; The bottleneck
   * is in the delegation thread, not the main thread.
   */

	PMFS_START_TIMING(pre_fault_r_t, prefault_time);
	uaddr_end = PMFS_ROUNDUP_PAGE(uaddr + bytes - 1);
	for (i = uaddr; i < uaddr_end; i += PAGE_SIZE) {
		unsigned long target_addr = i;

		/*
     * Do not access an address that is out of the buffer provided by the user
     */

		if (i > uaddr + bytes - 1)
			target_addr = uaddr + bytes - 1;

		pmfs_dbg_delegation(
			"uaddr: %lx, bytes: %ld, target_addr: %lx\n", uaddr,
			bytes, target_addr);

		ret = __clear_user((void *)target_addr, 1);

		if (ret != 0) {
			PMFS_END_TIMING(pre_fault_r_t, prefault_time);
			goto out;
		}
	}
	PMFS_END_TIMING(pre_fault_r_t, prefault_time);
	/*
   * We have ensured that [kaddr, kaddr + bytes - 1) falls in the same
   * kernel page.
   */

	/* which socket to delegate */

	block = pmfs_get_block_from_addr(sbi, (void *)kaddr);
	socket = pmfs_block_to_socket(sbi, block);

	/* inc issued cnt */
	pmfs_issued_cnt[socket]++;

	request.type = PMFS_REQUEST_READ;
	request.mm = mm;
	request.uaddr = uaddr;
	request.kaddr = kaddr;
	request.bytes = bytes;
	request.zero = zero;

	/* Let the access threads increase the complete cnt */
	request.notify_cnt = &(pmfs_completed_cnt[socket].cnt);
	request.wait_hint = wait_hint;

	PMFS_START_TIMING(send_request_r_t, send_request_time);
	do {
		thread = pmfs_choose_rings();
		ret = pmfs_send_request(pmfs_ring_buffer[socket][thread],
					&request);
	} while (ret == -EAGAIN);

	wake_up_interruptible(&delegation_queue[socket][thread]);

	PMFS_END_TIMING(send_request_r_t, send_request_time);

out:
	return ret;
}

/* make this a global variable so that the compiler will not optimize it */

DEFINE_PER_CPU(int, pmfs_no_optimize);

unsigned int pmfs_do_write_delegation(struct pmfs_sb_info *sbi,
				      struct mm_struct *mm, unsigned long uaddr,
				      unsigned long kaddr, unsigned long bytes,
				      int zero, int flush_cache, int sfence,
				      long *pmfs_issued_cnt,
				      struct pmfs_notifyer *pmfs_completed_cnt,
				      int wait_hint)
{
	int socket;
	struct pmfs_delegation_request request;
	int ret = 0;
	unsigned long i = 0, uaddr_end = 0;
	int block;
	int thread;

	PMFS_DEFINE_TIMING_VAR(prefault_time);
	PMFS_DEFINE_TIMING_VAR(send_request_time);

	/* TODO: Check the validity of the user-level address */

	/*
   * access the user address while still at the process's address space
   * to let the kernel handles various situations: e.g., page not mapped,
   * page swapped out.
   *
   * Surprisingly, the overhead of this part is significant. However,
   * currently it looks to me no point to optimize this part; The bottleneck
   * is in the delegation thread, not the main thread.
   */
	if (!zero) {
		PMFS_START_TIMING(pre_fault_w_t, prefault_time);
		uaddr_end = PMFS_ROUNDUP_PAGE(uaddr + bytes - 1);
		for (i = uaddr; i < uaddr_end; i += PAGE_SIZE) {
			unsigned long target_addr = i;
			int tmp;

			/*
       * Do not access an address that is out of the buffer provided by the user
       */

			if (i > uaddr + bytes - 1)
				target_addr = uaddr + bytes - 1;

			pmfs_dbg_delegation(
				"uaddr: %lx, bytes: %ld, target_addr: %lx\n",
				uaddr, bytes, target_addr);

			ret = copy_from_user(&tmp,
					     (void *)target_addr, 1);

			this_cpu_write(pmfs_no_optimize, tmp);

			if (ret != 0) {
				PMFS_END_TIMING(pre_fault_w_t, prefault_time);
				goto out;
			}
		}
		PMFS_END_TIMING(pre_fault_w_t, prefault_time);
	}

	/*
   * We have ensured that [kaddr, kaddr + bytes - 1) falls in the same
   * kernel page.
   */

	/* which socket to delegate */
	block = pmfs_get_block_from_addr(sbi, (void *)kaddr);
	socket = pmfs_block_to_socket(sbi, block);

	/* inc issued cnt */
	pmfs_issued_cnt[socket]++;

	request.type = PMFS_REQUEST_WRITE;
	request.mm = mm;
	request.uaddr = uaddr;
	request.kaddr = kaddr;
	request.bytes = bytes;
	request.zero = zero;
	request.flush_cache = flush_cache;

	/* Let the access threads increase the complete cnt */
	request.notify_cnt = &(pmfs_completed_cnt[socket].cnt);
	request.wait_hint = wait_hint;

	PMFS_START_TIMING(send_request_w_t, send_request_time);
	do {
		thread = pmfs_choose_rings();
		ret = pmfs_send_request(pmfs_ring_buffer[socket][thread],
					&request);
	} while (ret == -EAGAIN);

	wake_up_interruptible(&delegation_queue[socket][thread]);

	PMFS_END_TIMING(send_request_w_t, send_request_time);

out:
	return ret;
}

void pmfs_complete_delegation(long *pmfs_issued_cnt,
			      struct pmfs_notifyer *pmfs_completed_cnt)
{
	int i = 0;

	for (i = 0; i < PMFS_MAX_SOCKET; i++) {
		long issued = pmfs_issued_cnt[i];
		unsigned long cond_cnt = 0;

		/* TODO: Some kind of backoff is needed here? */
		while (issued != atomic_read(&(pmfs_completed_cnt[i].cnt))) {
			cond_cnt++;

			if (cond_cnt >= PMFS_APP_CHECK_COUNT) {
				cond_cnt = 0;
				if (need_resched())
					cond_resched();
			}
		}
	}

}
