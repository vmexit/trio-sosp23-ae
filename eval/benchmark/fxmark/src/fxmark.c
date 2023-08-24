#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <numa.h>
#include "fxmark.h"

volatile int received = 0;

struct bench_desc {
	const char *name;
	const char *desc;
	struct bench_operations *ops;
};

static struct bench_desc bench_table[] = {
	{"MWCL",
	 "inode allocation: each process creates files at its private directory",
	 &n_inode_alloc_ops},
	{"DWAL",
	 "block allocation: each process appends pages to a private file",
	 &n_blk_alloc_ops},
	{"DWOL",
	 "block write: each process overwrite a pages to a private file",
	 &n_blk_wrt_ops},
	{"MWRM",
	 "directory insert: each process moves files from its private directory to a common direcotry",
	 &n_dir_ins_ops},
	{"DWSL",
	 "journal commit: each process fsync a private file",
	 &n_jnl_cmt_ops},
	{"DWOM",
	 "mtime update: each process updates a private page of the shared file",
	 &n_mtime_upt_ops},
	{"MWRL",
	 "rename a file: each process rename a file in its private directory",
	 &n_file_rename_ops},
	{"DRBL",
	 "file read: each process read a block of its private file",
	 &n_file_rd_ops},
	{"DRBL_bg",
	 "file read with a background writer",
	 &n_file_rd_ops},
	{"DRBM",
	 "shared file read: each process reads its private region of the shared file",
	 &n_shfile_rd_ops},
	{"DRBM_bg",
	 "shared file read with a background writer",
	 &n_shfile_rd_bg_ops},
	{"DRBH",
	 "shared blk read: each process reads the same page of the shared file",
	 &n_shblk_rd_ops},
	{"DRBH_bg",
	 "shared blk read with a background writer",
	 &n_shblk_rd_bg_ops},
	{"MRDL",
	 "directory read: each process reads entries of its private directory",
	 &n_dir_rd_ops},
	{"MRDL_bg",
	 "directory read with a background writer",
	 &n_dir_rd_bg_ops},
	{"MRDM",
	 "shared directory read: each process reads entries of the shared directory",
	 &n_shdir_rd_ops},
	{"MRDM_bg",
	 "shared directory read with a background writer",
	 &n_shdir_rd_bg_ops},
	{"MRPL",
	 "path resolution for a private file",
	 &n_priv_path_rsl_ops},
	{"MRPM",
	 "path resolution: each process does stat() at random files in 8-level directories with 8-branching-out factor",
	 &n_path_rsl_ops},
	{"MRPM_bg",
	 "path resolution  with a background writer",
	 &n_path_rsl_bg_ops},
	{"MRPH",
	 "path resolution at the same level directory",
	 &n_spath_rsl_ops},
	{"MWCM",
	 "each process creates files in their private directory",
	 &u_file_cr_ops},
	{"MWUL",
	 "each process deletes files in their private directory",
	 &u_file_rm_ops},
	{"MWUM",
	 "each process deletes files at the test root directory",
	 &u_sh_file_rm_ops},
	{"DWTL",
	 "each process truncates its private file at the test root directory",
	 &u_file_tr_ops},
	{NULL, NULL, NULL},
};

static struct bench_operations *find_ops(char *type)
{
	struct bench_desc *bd = bench_table; 

	for (; bd->name != NULL; ++bd) {
		if (!strcmp(type, bd->name))
			return bd->ops;
	}
	return NULL;
}


struct fs
{
    char * name;
    int fs_type;
};


static struct fs special_fs[] =  {
    {"splitfs", splitfs},
    {NULL, regfs}
};

static int find_fs(char * fs_name)
{
    struct fs * ptr = special_fs;

    /* If fs_name didn't match any name in the special_fs[], then it is a 
     * reg(ular) fs
     */

    while (1)
    {
        if ( (ptr->name == NULL) || (strcmp(ptr->name, fs_name) == 0) )
            return ptr->fs_type;

        ptr++;
    }

    abort();
}


static int parse_option(int argc, char *argv[], struct cmd_opt *opt)
{
	static struct option options[] = {
		{"type",                      required_argument, 0, 't'},
		{"ncore",                     required_argument, 0, 'n'},
		{"nbg",                       required_argument, 0, 'g'},
		{"duration",                  required_argument, 0, 'd'},
		{"directio",                  required_argument, 0, 'D'},
		{"root",                      required_argument, 0, 'r'},
		{"profbegin",                 required_argument, 0, 'b'},
		{"profend",                   required_argument, 0, 'e'},
		{"proflog",                   required_argument, 0, 'l'},
		{"filesys",                   required_argument, 0, 'f'},
		{"pid",                       required_argument, 0, 'p'},
		{"pidfile",                   required_argument, 0, 'i'},
		{"numa_cpu_node",             required_argument, 0, 'c'},
		{"delegation_threads",        required_argument, 0, 'o'},
		{"delegation_sockets",        required_argument, 0, 's'},
		{"delegate",                  required_argument, 0, 'a'},
		{"rcore",                     required_argument, 0, 'h'},
		{0,                           0,                 0, 0},
	};
	int arg_cnt;

	opt->profile_start_cmd = "";
	opt->profile_stop_cmd  = "";
	opt->profile_stat_file = "";
	for(arg_cnt = 0; 1; ++arg_cnt) {
		int c, idx = 0;
		c = getopt_long(argc, argv, 
				"t:n:g:d:D:r:b:e:l:f:", options, &idx);
		if (c == -1)
			break; 
		switch(c) {
		case 't':
			opt->ops = find_ops(optarg);
			if (!opt->ops)
				return -EINVAL;
			break;
		case 'n':
			opt->ncore = atoi(optarg);
			break;
		case 'g':
			opt->nbg = atoi(optarg);
			break;
		case 'd':
			opt->duration = atoi(optarg);
			break;
		case 'D':
			opt->directio = atoi(optarg);
#if 0	/*optional debug*/
			if(opt->directio)
				fprintf(stderr, "DirectIO Enabled\n");
#endif
			break;
		case 'r':
			opt->root = optarg;
			break;
		case 'b':
			opt->profile_start_cmd = optarg;
			break;
		case 'e':
			opt->profile_stop_cmd = optarg;
			break;
		case 'l':
			opt->profile_stat_file = optarg;
			break;
		case 'f':
			opt->fs = find_fs(optarg);
			break;
		case 'p':
			opt->pid = atoi(optarg);
			break;
		case 'i':
			opt->profile_pid_file = optarg;
			break;
		case 'c':
			opt->numa_cpu_node = atoi(optarg);
			break;
		case 'o':
			opt->delegation_threads = atoi(optarg);
			break;
		case 's':
			opt->delegation_sockets = atoi(optarg);
			break;
		case 'a':
			opt->delegate = atoi(optarg);
			break;
		case 'h':
		    opt->rcore = atoi(optarg);
		    break;
		default:
			return -EINVAL;
		}
	}
	return arg_cnt;
}

static void usage(FILE *out)
{
	extern const char *__progname;
	struct bench_desc *bd = bench_table; 

	fprintf(out, "Usage: %s\n", __progname);
	fprintf(out, "  --type     = benchmark type\n");
	for (; bd->name != NULL; ++bd)
		fprintf(out, "    %s: %s\n", bd->name, bd->desc);
	fprintf(out, "  --ncore     = number of core\n");
	fprintf(out, "  --nbg       = number of background worker\n");
	fprintf(out, "  --duration  = duration in seconds\n");
	fprintf(out, "  --directio  = file flag set O_DIRECT : 0-false, 1-true\n"
		"                                         (only valid for DWxx type)\n");
	fprintf(out, "  --root      = test root directory\n");
	fprintf(out, "  --profbegin = profiling start command\n");
	fprintf(out, "  --profend   = profiling stop command\n");
	fprintf(out, "  --proflog   = profiling log file\n");
	fprintf(out, "  --filesys   = name of the file system\n");
	fprintf(out, "  --numa_cpu_node = runs the benchmark on specified node\n");
	fprintf(out, "  --delegation_threads    = number of delegation threads for odinfs\n");
	fprintf(out, "  --delegation_sockets    = number of delegation sockets for odinfs\n");
	fprintf(out, "  --delegate              = enable delegation for odinfs\n");
	fprintf(out, "  --rcore                 = adapt core numbers to match socket boundary\n");
}

static void init_bench(struct bench *bench, struct cmd_opt *opt)
{
	struct fx_opt *fx_opt = fx_opt_bench(bench);

	bench->duration = opt->duration;
	bench->directio = opt->directio;
	strncpy(bench->profile_start_cmd,
		opt->profile_start_cmd, BENCH_PROFILE_CMD_BYTES);
	strncpy(bench->profile_stop_cmd,
		opt->profile_stop_cmd, BENCH_PROFILE_CMD_BYTES);
	strncpy(bench->profile_stat_file,
		opt->profile_stat_file, PATH_MAX);
	strncpy(fx_opt->root, opt->root, PATH_MAX);
	bench->ops = *opt->ops;

    bench->fs = opt->fs;
	bench->pid = opt->pid;

	if (opt->profile_pid_file)
	{
        strncpy(bench->profile_pid_file,
            opt->profile_pid_file, PATH_MAX);
	}

	bench->numa_cpu_node = opt->numa_cpu_node;
}

static void sigusr2_handler(int signum) {
	received = 1;
}

/* remove core in place */
static void remove_core(unsigned int *cores, int len, int core) {
	int idx, i;
	for (idx = 0; idx < len; ++idx) {
		if (cores[idx] == core) {
			break;
		}
	}
	for (i = idx; i < len - 1; ++i) {
		cores[i] = cores[i + 1];
	}
}

int main(int argc, char *argv[])
{
	struct cmd_opt opt = {
		.ops = NULL,
		.ncore = 0, 
		.nbg = 0, 
		.duration = 0, 
		.directio = 0, 
		.root = NULL,
		.profile_start_cmd = NULL,
		.profile_stop_cmd = NULL,
		.profile_stat_file = NULL,
		.fs = 0,
		.pid = -1,
		.profile_pid_file = NULL,
		.numa_cpu_node = -1,
		.delegation_threads = 0,
		.delegation_sockets = 0,
		.delegate = 0,
		.rcore = 0,
	};
	struct bench *bench; 

	int tot_core = PHYSICAL_CHIPS * CORE_PER_CHIP;

	/* register signal handler for log */
	signal(SIGUSR2, sigusr2_handler);

	/* parse command line options */
	if (parse_option(argc, argv, &opt) < 4) {
		usage(stderr);
		exit(1);
	}
	if (opt.delegate) {
		/* change ncore and seq_cores to adapt odinfs */
		if (opt.delegation_threads > 0) {

			for (int i = 0; i < opt.delegation_sockets; ++i) {
				for (int j = 0; j < opt.delegation_threads; ++j) {
					int core = i * CORE_PER_CHIP + j;
					remove_core(seq_cores, tot_core, core);
					tot_core --;
				}
			}
		}
	}
	/* create, initialize, and run a bench */ 
	bench = alloc_bench(opt.ncore, opt.nbg, tot_core, opt.delegation_threads,
	        opt.delegation_sockets, opt.rcore);

	init_bench(bench, &opt);
	run_bench(bench);
	report_bench(bench, stdout);

	return 0;
}
