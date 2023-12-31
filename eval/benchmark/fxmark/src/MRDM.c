/**
 * Nanobenchmark: Read operation
 *   RD. PROCESS = {read entries of the shared directory}
 */	      
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "fxmark.h"
#include "util.h"

static int stop_pre_work;

static void set_test_root(struct worker *worker, char *test_root)
{
	struct fx_opt *fx_opt = fx_opt_worker(worker);
	sprintf(test_root, "%s", fx_opt->root);
}

static void set_test_file(struct worker *worker, 
			  uint64_t file_id, char *test_file)
{
	struct fx_opt *fx_opt = fx_opt_worker(worker);
	sprintf(test_file, "%s/n_shdir_rd-%d-%" PRIu64 ".dat",
		fx_opt->root, worker->id, file_id);
}

static void sighandler(int x)
{
	stop_pre_work = 1;
}

static int pre_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
	char path[PATH_MAX];
	int fd, rc = 0;

	/* perform pre_work for bench->duration */
	if (signal(SIGALRM, sighandler) == SIG_ERR) {
		rc = errno;
		goto err_out;
	}
	alarm(bench->duration);

	/* create private directory */
	set_test_root(worker, path);
	rc = mkdir_p(path);
	if (rc) goto err_out;

	/* create files at the private directory */
	for (; !stop_pre_work; ++worker->private[0]) {
		set_test_file(worker, worker->private[0], path);
		if ((fd = open(path, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
			if (errno == ENOSPC)
				goto out;
			goto err_out;
		}
		close(fd);
	}
out:
	return rc; 
err_out:
	rc = errno;
	goto out;
}

static int main_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
	char dir_path[PATH_MAX];
	DIR *dir;
	struct dirent entry;
	struct dirent *result;
	uint64_t iter = 0;
	int rc = 0;

	set_test_root(worker, dir_path);
	while (!bench->stop) {
#ifdef SUFS
	    dir = sufs_opendir(dir_path);
#else
		dir = opendir(dir_path);
#endif
		if (!dir) goto err_out;
		for (; !bench->stop; ++iter) {
#ifdef SUFS
		    rc = sufs_glibc_readdir_r(dir, &entry, &result);
#else
			rc = readdir_r(dir, &entry, &result);
#endif
			if (rc) goto err_out;
		}
#ifdef SUFS
        sufs_closedir(dir);
#else
		closedir(dir);
#endif
	}
out:
	bench->stop = 1;
	worker->works = (double)iter;
	return rc;
err_out:
	rc = errno;
	goto out;
}

struct bench_operations n_shdir_rd_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
};
