#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>

// Flag set by "--verbose" argument
static int verbose_flag;

// File descriptors/pipes data arrays
static int d_index;
static int *file_pointers;
static int POINTERS_SIZE;
static int *pipe_or_no;

// pipe descriptors
static int pipe_fd[2];

// file descriptor
static int fd;

// Exit return value
static int return_value;

// in/out/err
static int cmd_fd[3];

// number of arguments fed into command
static int com_args;
static int c_index;

// to parse commands
static int ind;

// for --wait option
static int *wait_pids;
static int *start_commands;
static int *end_commands;
static int w_index;
static int WAIT_SIZE;

//// Flag set by "--profile" argument
static int profile_flag;

// rusage variables
struct rusage usage;
struct timeval old_user_time;
struct timeval old_system_time;
struct timeval old_child_user_time;
struct timeval old_child_system_time;



/*
signal handler
*/
void sighandler (int sign){
	// error message upon catching message
	fprintf(stderr, "Signal %u caught \n", sign);
	perror("");
	exit(sign);
}


/*
set return value
*/
void return_val(int value) {
	if (return_value < value) {
		// update if value is larger than previous return value
		return_value = value;
	}
}


/*
Checks if argument is an option
*/
int isOption(char *arg) {
	if (arg[0] == '-' && arg[1] == '-') {
		return 1;
	} else {
		return 0;
	}
}


/*
Checks if string is numeric
*/
int isNumeric(char *st) {
	for (int i = 0; i < strlen(st) ; i++) {
		if (!isdigit(st[i])) {
			// character in string is not a digit
			return 0;
		}
	}
	return 1;
}


/*
Checks if there is enough space allocated in the file_pointer array
to add another file. If not, double size of file array.
*/
void addToFileArray() {

	if (d_index == POINTERS_SIZE) {
		// not enough space allocated; double size of array
		POINTERS_SIZE = POINTERS_SIZE * 2;
		file_pointers = realloc(file_pointers, POINTERS_SIZE * sizeof(int));
		pipe_or_no = realloc(pipe_or_no, POINTERS_SIZE * sizeof(int));
	}

	if (w_index == WAIT_SIZE) {
		// not enough space allocated; double size of array
		WAIT_SIZE = WAIT_SIZE * 2;
		wait_pids = realloc(wait_pids, 	WAIT_SIZE * sizeof(int));
		start_commands = realloc(start_commands, WAIT_SIZE * sizeof(int));
		end_commands = realloc(end_commands, 	WAIT_SIZE * sizeof(int));
	}

	if (errno == ENOMEM) {
		// 	could not open file
		fprintf(stderr, "Error: Not enough space in memory to increase array \
size\n.");
		perror("");

		// update return value
		return_val(1);
	}

}


/*
input: file path & opening flag
Attempts to open file and returns error if cannot do so
*/
void open_file(const char *file, int open_flags) {
	// get file descriptor
	fd = open(file, open_flags, 0644);

	if (fd == -1) {
		// 	could not open file
		fprintf(stderr, "Error: Could not open files %s.\n", optarg);
		perror("");
		// update return value if higher than previous value
		return_val(1);
	}

	// add file to file descriptor array
	addToFileArray();
	file_pointers[d_index] = fd;
	pipe_or_no[d_index] = 0;

	// increase d_index
	d_index++;
}


/*
Executes command
*/
void exec_cmd(char *const args[]) {
	// attempt to fork
	pid_t c_pid = fork();

	if (c_pid < 0) {
		// 	could not create subprocess
		fprintf(stderr, "Error: Could not create child process.\n");

		// update return value if higher than previous value
		return_val(1);
	} else if (c_pid == 0) {
		// change standard input, output and error
		dup2(file_pointers[cmd_fd[0]], 0);
		dup2(file_pointers[cmd_fd[1]], 1);
		dup2(file_pointers[cmd_fd[2]], 2);

		for (int i = 0; i < d_index; i++) {
			close(file_pointers[i]);
		}

		if (execvp(args[0], args) == -1) {
			// 	could not execute command
			fprintf(stderr, "Error: Could not execute command %s.\n", args[0]);
			perror("");
			// update return value if higher than previous value
			return_val(1);
		}
	} else {
		// add child process to array
		wait_pids[w_index] = c_pid;
		w_index++;

		// close parent fds
		if (pipe_or_no[cmd_fd[0]]) {
			close(file_pointers[cmd_fd[0]]);
			file_pointers[cmd_fd[0]] = -1;
		}

		if (pipe_or_no[cmd_fd[1]]) {
			close(file_pointers[cmd_fd[1]]);
			file_pointers[cmd_fd[1]] = -1;
		}

		if (pipe_or_no[cmd_fd[2]]) {
				close(file_pointers[cmd_fd[2]]);
				file_pointers[cmd_fd[2]] = -1;
		}
	}
}


/*
prints --wait commands and exit codes
*/
void wait_print(int start, int end, char **argv, int exit_stat) {
	printf("[Exit Status: %u] ", exit_stat);
	for (int i = start; i <= end; i++){
		printf("%s ", argv[i]);
	}
	printf("\n");
}


/*
print rusage information
*/
void option_profile(int who) {
	if (getrusage(who, &usage)== 0) {
		// success
		struct timeval user_time = usage.ru_utime;
		struct timeval os_time = usage.ru_stime;

		if (who == RUSAGE_SELF) {
			printf("Ressources used by option: \n");
			printf("Time spent executing user instructions: %u seconds, %u microseconds; \n",
				(int)((double)user_time.tv_sec - (double)old_user_time.tv_sec),
				(int)((double)user_time.tv_usec - (double)old_user_time.tv_usec));
			printf("Time spent on operating system code on behalf of process: %u seconds, %u microseconds; \n",
				(int)((double)os_time.tv_sec - (double)old_system_time.tv_sec),
				(int)((double)os_time.tv_usec - (double)old_system_time.tv_usec));
			old_user_time = user_time;
			old_system_time = os_time;
		} else {
			printf("Ressources used by children processes: \n");
			printf("Time spent executing user instructions: %u seconds, %u microseconds; \n",
				(int)((double)user_time.tv_sec - (double)old_child_user_time.tv_sec),
				(int)((double)user_time.tv_usec - (double)old_child_user_time.tv_usec));
			printf("Time spent on operating system code on behalf of process: %u seconds, %u microseconds; \n",
				(int)((double)os_time.tv_sec - (double)old_child_system_time.tv_sec),
				(int)((double)os_time.tv_usec - (double)old_child_system_time.tv_usec));
		}

	} else {
		fprintf(stderr, "Error: Could not retrieve usage data.\n");
		return_val(1);
	}
}


int
main (int argc, char **argv) {
	// passed option identifier
	int c;

	verbose_flag = 0;
	profile_flag = 0;
	return_value = 0;

	// open file flags
	int oflags = 0;

	// initialize file descriptor array
	d_index = 0;
	POINTERS_SIZE = 5;
	file_pointers = malloc(POINTERS_SIZE * sizeof(int));
	pipe_or_no = malloc(POINTERS_SIZE * sizeof(int));

	// initialize command waiting array
	WAIT_SIZE = 5;
	wait_pids = malloc(WAIT_SIZE * sizeof(int));
	start_commands = malloc(WAIT_SIZE * sizeof(int));
	end_commands = malloc(WAIT_SIZE * sizeof(int));
	w_index = 0;

	// argument list for --wait printing
	char **arguments = malloc(argc *sizeof(char *));
	for (int i = 0; i < argc; i++){
		arguments[i] = argv[i];
	}

	if (argc == 1) {
		// no options/arguments provided
		fprintf(stderr, "No options have been provided.\n");
		exit(0);
	}

	if (!isOption(argv[optind])){
		// first argument is not an option
		fprintf(stderr, "Warning: The argument %s is not an option and will be \
ignored.\n", argv[optind]);
	}

	// rusage variables initialize

	while (1) {

		static struct option long_options[] = {

			// file flags
			{"append",		no_argument,				0,							'a'},
			{"cloexec",		no_argument,				0,							'e'},
			{"creat",			no_argument,				0,							'c'},
			{"directory",	no_argument,				0,							'd'},
			{"dsync",			no_argument,				0,							'D'},
			{"excl",			no_argument,				0,							'E'},
			{"nofollow",	no_argument,				0,							'n'},
			{"nonblock",	no_argument,				0,							'N'},
			{"rsync",			no_argument,				0,							'r'},
			{"sync",			no_argument,				0,							's'},
			{"trunc",			no_argument,				0,							't'},

			// file-opening flags
			{"rdonly",		required_argument,	0,							'R'},
			{"rdwr",			required_argument,	0,							'b'},
			{"wronly",		required_argument,	0,							'w'},
			{"pipe",			no_argument,				0,							'p'},

			// subcommand options
			{"command",		required_argument,	0,							'C'},
			{"wait",			no_argument,				0,							'W'},

			// miscellaneous options
			{"close",			required_argument,	0,							'o'},
			{"verbose",		no_argument,				&verbose_flag,		1},
			{"profile",		no_argument,				&profile_flag,		1},
			{"abort",			no_argument,				0,							'A'},
			{"catch",			required_argument,	0,							'h'},
			{"ignore",		required_argument,	0,							'i'},
			{"default",		required_argument,	0,							'f'},
			{"pause",			no_argument,				0,							'P'},
			{0, 					0, 									0, 								0}
		};

		// stores the option index
		int option_index = 0;

		// get argument type
		c = getopt_long (argc, argv, "",
			long_options, &option_index);

		// stop loop if no more arguments
		if (c == -1)
			break;


		switch (c) {

			// flag set
			case 0:
			break;


			//--append
			case 'a':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--append] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--append ");
			}

			oflags |= O_APPEND;
			break;


			// --cloexec
			case 'e':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--cloexec] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--cloexec ");
			}

			oflags |= O_CLOEXEC;
			break;


			// --creat
			case 'c':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--creat] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--creat ");
			}

			oflags |= O_CREAT;
			break;


			// --directory
			case 'd':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--directory] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--directory ");
			}

			oflags |= O_DIRECTORY;
			break;


			// --dsync
			case 'D':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--dsync] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--dsync ");
			}

			oflags |= O_DSYNC;
			break;


			// --excl
			case 'E':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--excl] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--excl ");
			}

			oflags |= O_EXCL;
			break;


			// --nofollow
			case 'n':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--nofollow] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--nofollow ");
			}

			oflags |= O_NOFOLLOW;
			break;


			// --nonblock
			case 'N':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--nonblock] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--nonblock ");
			}

			oflags |= O_NONBLOCK;
			break;


			// --rsync
			case 'r':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--rsync] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--rsync ");
			}

			oflags |= O_RSYNC;
			break;


			// --sync
			case 's':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--sync] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--sync ");
			}

			oflags |= O_SYNC;
			break;


			// --trunc
			case 't':
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--trunc] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				printf("--trunc ");
			}

			oflags |= O_TRUNC;
			break;


			// --rdonly f
			case 'R':

			if (isOption(optarg) || optarg == NULL) {
				if (verbose_flag) {
					printf("--rdonly\n");
				}
				// missing file after option
				fprintf(stderr, "Error: [--rdonly f] requires an argument; no file was \
provided.\n");
				optind--;
				return_val(1);
				break;
			}

			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--rdonly f] only accepts a single argument. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			//if verbose_flag is set
			if (verbose_flag) {
				printf("--rdonly %s\n", optarg);
			}

			oflags |= O_RDONLY;

			// open read only file
			open_file(optarg, oflags);

			// clear oflags
			oflags = 0;

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --rdwr f
			case 'b':

			if (isOption(optarg) || optarg == NULL) {
				if (verbose_flag) {
					printf("--rdwr\n");
				}
				// missing file after option
				fprintf(stderr, "Error: [--rdwr f] requires an argument; no file was \
provided.\n");
				optind--;
				return_val(1);
				break;
			}

			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--rdwr f] only accepts a single argument. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			//if verbose_flag is set
			if (verbose_flag) {
				printf("--rdwr %s\n", optarg);
			}

			oflags |= O_RDWR;

			// open read/write file
			open_file(optarg, oflags);

			// clear oflags
			oflags = 0;

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --wronly f
			case 'w':

			if (isOption(optarg) || optarg == NULL) {
				// missing file after option
				if (verbose_flag) {
					printf("--wronly\n");
				}
				fprintf(stderr, "Error: [--wronly f] requires an argument; no file was \
provided.\n");
				optind--;
				return_val(1);
				break;
			}

			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--wronly f] only accepts a single argument. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			//if verbose_flag is set
			if (verbose_flag) {
				printf("--wronly %s\n", optarg);
			}

			oflags |= O_WRONLY;

			// open output file
			open_file(optarg, oflags);

			// clear oflags
			oflags = 0;

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --pipe
			case 'p':

			//if verbose_flag is set
			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--pipe\n");
			}
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--pipe] does not accept any arguments. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (pipe(pipe_fd) == -1) {
				// creating pipe failed
				fprintf(stderr, "Error: Could not create pipe.\n");
				return_val(1);

				// update return value if higher than previous value
				if (errno == EINVAL) {
					perror("");
				}

				break;
			}

			// add pipe descriptors to file descriptors array

			// read end
			file_pointers[d_index] = pipe_fd[0];
			pipe_or_no[d_index] = 1;
			d_index++;

			//write end
			file_pointers[d_index] = pipe_fd[1];
			pipe_or_no[d_index] = 1;
			d_index++;

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --command i o e cmd args
			case 'C':
			addToFileArray();
			com_args = 0;
			c_index = 0;
			int curr_index = optind-1;
			start_commands[w_index] = curr_index - 1;

			while (curr_index != argc && !isOption(argv[curr_index])) {
				com_args++;
				curr_index++;
			}
			end_commands[w_index] = curr_index - 1;

			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--command ");
				if (!isOption(optarg)) {
					// some arguments provided
					for (int i = -1; i < com_args - 1; i++) {
						printf("%s ", argv[optind + i]);
					}
				}
				printf("\n");
			}

			if (isOption(optarg) || optarg == NULL || com_args < 4) {
				// missing arguments
				fprintf(stderr, "Error: [--command i o e cmd args] requires at least 4 \
arguments: standard input, standard output, standard error and a cmd to execute \
with 0 or more arguments. The required number of arguments was not provided.\n");
				return_val(1);
				break;
			}

			if (!isNumeric(optarg) || !isNumeric(argv[optind]) ||
				!isNumeric(argv[optind + 1])) {
				// 1st 3 arguments not numbers
				fprintf(stderr, "Error: The first 3 arguments of \
[--command i o e cmd args] must be integers.\n");
				return_val(1);
				break;
			}

				// setting standard input
			cmd_fd[0] = atoi(optarg);
				// setting standard output
			cmd_fd[1] = atoi(argv[optind]);
				// setting standard error
			cmd_fd[2] = atoi(argv[optind+1]);

			if (cmd_fd[0] >= d_index || cmd_fd[1] >= d_index || cmd_fd[2] >= d_index) {
					// fd out of range
				fprintf(stderr, "Error: One or more of the provided file descriptors is \
out of range. These files have not yet been opened or created.\n");
				return_val(1);
				break;
			}

			if (file_pointers[cmd_fd[0]] == -1 || file_pointers[cmd_fd[1]] == -1
				|| file_pointers[cmd_fd[2]] == -1) {
				// 1st 3 arguments not numbers
				fprintf(stderr, "Error: One or more of the provided file descriptors is \
invalid. These files could not be opened or have been closed.\n");
				return_val(1);
				break;
			}


			// creating argument data array
			char **args = malloc((com_args - 2)*sizeof(char *));

			ind = optind + 2;

			while (ind != argc && !isOption(argv[ind])) {
					// adding arguments to array
				args[c_index] = argv[ind];
				c_index++;
				ind++;
			}
			args[c_index] = NULL;

				// execute command
			exec_cmd(args);

				// free memory allocation
			free(args);

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --wait
			// TODO: fix profile for children
			case 'W':
			if (optind != argc && !isOption(argv[optind])){
					// more than one argument is provided
				fprintf(stderr, "Warning: [--wait] does not accept any arguments. \
	%s is not an option and will be ignored.\n", argv[optind]);
			}
			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--wait\n");
			}

			for (int i = 0; i < d_index; i++) {
				close(file_pointers[i]);
			}


			int status;
			int exit_stat;

			if (profile_flag) {
				if (getrusage(RUSAGE_CHILDREN, &usage)== 0) {
					old_child_user_time = usage.ru_utime;
					old_child_system_time = usage.ru_stime;
				} else {
					fprintf(stderr, "Error: Could not retrieve usage data.\n");
					return_val(1);
				}
			}



			for (int i = 0; i < w_index; i++) {
				// wait for command to complete
				waitpid(wait_pids[i], &status, 0);
				if (WIFEXITED(status)) {
					// get exit status
					exit_stat = WEXITSTATUS(status);
					wait_print(start_commands[i],end_commands[i], arguments, exit_stat);

					// update return value
					return_val(exit_stat);
				} else {
					fprintf(stderr, "Error: Child process did not exit as expected.\n");
					return_val(1);
				}
			}

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
				option_profile(RUSAGE_CHILDREN);
			}

			break;


			// --close N
			case 'o':
			if (isOption(optarg) || optarg == NULL) {
				// missing file number after option
				if (verbose_flag) {
					if (oflags != 0) {
						printf("\n");
					}
					printf("--close\n");
				}
				fprintf(stderr, "Error: [--close N] requires an argument; no file number \
	was provided.\n");
				optind--;
				return_val(1);
				break;
			}

			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--close N] only accepts a single argument. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--close %s\n", optarg);
			}

			if (!isNumeric(optarg)) {
				// argument is not file descriptor index.
				fprintf(stderr, "Error: Argument %s is not a file number.  \
	[--close N] requires an integer argument.\n", optarg);
				return_val(1);
				break;
			}

			int close_file = atoi(optarg);

			if (close_file >= d_index) {
					// argument out of range
				fprintf(stderr, "Error: The file descriptor %u is \
	out of range. This file has not yet been opened or created.\n", close_file);
				return_val(1);
				break;
			}

			if (file_pointers[close_file] != -1) {
				close(file_pointers[close_file]);
			}

			file_pointers[close_file] = -1;

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --abort
			case 'A':
			if (optind != argc && !isOption(argv[optind])){
					// more than one argument is provided
				fprintf(stderr, "Warning: [--nofollow] does not accept any arguments. \
	%s is not an option and will be ignored.\n", argv[optind]);
			}
			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--abort\n");
			}
			raise(SIGSEGV);
			break;


			// --catch N
			case 'h':
			if (isOption(optarg) || optarg == NULL) {

				// missing file number after option
				if (verbose_flag) {
					if (oflags != 0) {
						printf("\n");
					}
					printf("--catch\n");
				}
				fprintf(stderr, "Error: [--catch N] requires an argument; no signal number \
	was provided.\n");
				optind--;
				return_val(1);
				break;
			}

			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--catch N] only accepts a single argument. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--catch %s\n", optarg);
			}

			if (!isNumeric(optarg)) {
				// argument is not signal number.
				fprintf(stderr, "Error: Argument %s is not a signal number.  \
	[--catch N] requires an integer argument.\n", optarg);
				return_val(1);
				break;
			}

			int sign = atoi(optarg);

			if (sign < 1 || sign > 31) {
				// argument is not valid signal descriptor index.
				fprintf(stderr, "Error: Argument %u is not a valid signal number.\n",
					sign);
				return_val(1);
				break;
			}

			if (signal(sign, sighandler) == SIG_ERR) {
				fprintf(stderr, "Error: There was an error in handling the signal.\n");
				return_val(1);
			}

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --ignore N
			case 'i':
			if (isOption(optarg) || optarg == NULL) {

				// missing file number after option
				if (verbose_flag) {
					if (oflags != 0) {
						printf("\n");
					}
					printf("--ignore\n");
				}
				fprintf(stderr, "Error: [--ignore N] requires an argument; no signal number \
	was provided.\n");
				optind--;
				return_val(1);
				break;
			}

			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--ignore N] only accepts a single argument. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--ignore %s\n", optarg);
			}

			if (!isNumeric(optarg)) {
				// argument is not signal number.
				fprintf(stderr, "Error: Argument %s is not a signal number.  \
	[--ignore N] requires an integer argument.\n", optarg);
				return_val(1);
				break;
			}

			int sign2 = atoi(optarg);

			if (sign2 < 1 || sign2 > 31) {
				// argument is not valid signal descriptor index.
				fprintf(stderr, "Error: Argument %u is not a valid signal number.\n",
					sign2);
				return_val(1);
				break;
			}

			if (signal(sign2, SIG_IGN) == SIG_ERR) {
				fprintf(stderr, "Error: There was an error in handling the signal.\n");
				return_val(1);
			}

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --default N
			case 'f':
			if (isOption(optarg) || optarg == NULL) {

				// missing file number after option
				if (verbose_flag) {
					if (oflags != 0) {
						printf("\n");
					}
					printf("--default\n");
				}
				fprintf(stderr, "Error: [--default N] requires an argument; no signal number \
	was provided.\n");
				optind--;
				return_val(1);
				break;
			}

			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--default N] only accepts a single argument. \
%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--default %s\n", optarg);
			}

			if (!isNumeric(optarg)) {
				// argument is not signal number.
				fprintf(stderr, "Error: Argument %s is not a signal number.  \
	[--default N] requires an integer argument.\n", optarg);
				return_val(1);
				break;
			}

			int sign3 = atoi(optarg);

			if (sign3 < 1 || sign3 > 31) {
				// argument is not valid signal descriptor index.
				fprintf(stderr, "Error: Argument %u is not a valid signal number.\n",
					sign3);
				return_val(1);
				break;
			}

			if (signal(sign3, SIG_DFL) == SIG_ERR) {
				fprintf(stderr, "Error: There was an error in handling the signal.\n");
				return_val(1);
			}

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			// --pause
			case 'P':
			//if verbose_flag is set
			if (verbose_flag) {
				if (oflags != 0) {
					printf("\n");
				}
				printf("--pause\n");
			}
			if (optind != argc && !isOption(argv[optind])){
				// more than one argument is provided
				fprintf(stderr, "Warning: [--pause] does not accept any arguments. \
	%s is not an option and will be ignored.\n", argv[optind]);
			}

			if (pause() == -1) {
				fprintf(stderr, "Error: Program was unable to pause.\n");
				return_val(1);
			}

			if (profile_flag) {
				option_profile(RUSAGE_SELF);
			}

			break;


			default:
			fprintf(stderr, "Error: Command not recognized.\n");
			return_val(1);
			break;
		}
	}

	if (getrusage(RUSAGE_SELF, &usage)== 0) {
		// success
		struct timeval final_user_time = usage.ru_utime;
		struct timeval final_system_time = usage.ru_stime;

		printf("\nTotal Ressources used by Parent Processes: \n");
		printf("Time spent executing user instructions: %u seconds, %u microseconds; \n",
			(int)final_user_time.tv_sec, (int)final_user_time.tv_usec);
		printf("Time spent on operating system code on behalf of process: %u seconds, %u microseconds; \n",
			(int)final_system_time.tv_sec, (int)final_system_time.tv_usec);

	} else {
		fprintf(stderr, "Error: Could not retrieve usage data.\n");
		return_val(1);
	}

	if (getrusage(RUSAGE_CHILDREN, &usage) == 0) {
		// success
		struct timeval final_user_time = usage.ru_utime;
		struct timeval final_system_time = usage.ru_stime;

		printf("Total Ressources used by Child Processes: \n");
		printf("Time spent executing user instructions: %u seconds, %u microseconds; \n",
			(int)final_user_time.tv_sec, (int)final_user_time.tv_usec);
		printf("Time spent on operating system code on behalf of process: %u seconds, %u microseconds; \n",
			(int)final_system_time.tv_sec, (int)final_system_time.tv_usec);

	} else {
		fprintf(stderr, "Error: Could not retrieve usage data.\n");
		return_val(1);
	}

	// close all opened files
	for (int i = 0; i < d_index; i++) {
		if (file_pointers[i] != -1) {
			close(file_pointers[i]);
		}
	}

		// free file data array memory
	free(file_pointers);
	free(pipe_or_no);
	free(wait_pids);
	free(start_commands);
	free(end_commands);
	free(arguments);

	exit(return_value);
}