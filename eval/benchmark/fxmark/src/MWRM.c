#pragma GCC diagnostic ignored "-Wcomment"
/**
 * Nanobenchmark: ADD
 *   DI. PROCESS = {move files from /test/$PROCESS/* to /test}
 *       - TEST: dir. insert
 */	      
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "fxmark.h"
#include "util.h"

static int stop_pre_work;

#define FILES_CREATE_PER_WORKER 50000

static void set_test_root(struct worker *worker, char *test_root)
{
	struct fx_opt *fx_opt = fx_opt_worker(worker);
	sprintf(test_root, "%s/%d", fx_opt->root, worker->id);
}

static void set_test_file(struct worker *worker, 
			  uint64_t file_id, char *test_file)
{
	struct fx_opt *fx_opt = fx_opt_worker(worker);
	sprintf(test_file, "%s/%d/n_dir_ins-%d-%" PRIu64 ".dat",
		fx_opt->root, worker->id, worker->id, file_id);
}

static void set_renamed_root(struct worker *worker, char * renamed_root)
{
    struct fx_opt *fx_opt = fx_opt_worker(worker);
    sprintf(renamed_root, "%s/shared", fx_opt->root);
}

static void set_renamed_test_file(struct worker *worker, 
				  uint64_t file_id, char *test_file)
{
	struct fx_opt *fx_opt = fx_opt_worker(worker);
	sprintf(test_file, "%s/shared/n_dir_ins-%d-%" PRIu64 ".dat",
		fx_opt->root, worker->id, file_id);
}

static void sighandler(int x)
{
	stop_pre_work = 1;
}

static int pre_work(struct worker *worker)
{
    struct bench *bench =  worker->bench;
    char path[PATH_MAX];
    int fd, i, create_files, rc = 0, max_id = -1;

    /* find the largest worker id */
    for (i = 0; i < bench->ncpu; ++i)
    {
        struct worker *w = &bench->workers[i];
        if (w->id > max_id)
            max_id = w->id;
    }

    create_files = FILES_CREATE_PER_WORKER;

    /* creating private directory */
    set_test_root(worker, path);
    rc = mkdir_p(path);
    if (rc)
        goto err_out;

    /* perform pre_work for 10 * bench->duration */
    if (signal(SIGALRM, sighandler) == SIG_ERR) {
        rc = errno;
        goto err_out;
    }
    alarm(bench->duration * 10);

    /* time to create files */
    for (i = 0; i < create_files && !stop_pre_work; ++i, ++worker->private[0])
    {
        set_test_file(worker, worker->private[0], path);

        if ((fd = open(path, O_CREAT | O_RDWR, S_IRWXU)) == -1)
        {
            if (errno == ENOSPC)
            {
                --worker->private[0];
                rc = 0;
                goto out;
            }
            rc = errno;
            goto err_out;
        }
        close(fd);
    }

    goto out;

err_out:
    bench->stop = 1;
out:
    return rc;
}

static int main_work(struct worker *worker)
{
	struct bench *bench = worker->bench;
	char old_path[PATH_MAX], new_path[PATH_MAX];
	uint64_t iter;
	int rc = 0;

	set_renamed_root(worker, new_path);
    rc = mkdir_p(new_path);

	for (iter = 0; iter < worker->private[0] && !bench->stop; ++iter) {

		set_test_file(worker, iter, old_path);
		set_renamed_test_file(worker, iter, new_path);

		rc = rename(old_path, new_path);
		if (rc)
		{
		    goto err_out;
		}
	}
out:
	bench->stop = 1;
	worker->works = (double)iter;
	return rc;
err_out:
	rc = errno;
	goto out;
}

struct bench_operations n_dir_ins_ops = {
	.pre_work  = pre_work, 
	.main_work = main_work,
};
