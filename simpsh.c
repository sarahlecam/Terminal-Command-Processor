#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/resource.h>

int cmpstr(char *a, char* b)
{
  int i = 0;
  while (a[i] != '\0')
    {
      if (b[i] == '\0')
	return 1;
      if (a[i] != b[i])
	return 1;
      i++;
    }
  return b[i] != '\0';
}

int validfd (char *endptr, long fd, int fn)
{
  if (*endptr == NULL)
    {
      if (fd < 0 || (fd >= fn))
	{
	  return 0; /* false */
	}
      return 1;
    }
  return 0; /* false */
}

/* check syntax */
int flag_syntax (int optind, int argc, char**argv) {
  char *ptr = NULL;
  if (optind < argc)
    {
      ptr = argv[optind];
    }
  /* either at the end or --option is followed by another --option */
  if (optind == argc || (ptr != NULL && *(ptr) == '-'))
    {
      /* true */
      return 1;
    }
  else
    {
      return 0;
    }
}

/* signal handler */
void catch_handler(int sig) {
  /* exit shell with status sig */
  fprintf(stderr, "%d caught\n", sig);
  exit(sig);
}

int opt_syntax (int optind, int argc, char**argv) {
  char* fileptr = NULL;
  if (optind < argc)
    {
      fileptr = argv[optind];
    }
  //  fprintf(stderr, "Test:\noptind: %d, argv[optind]: %s, argv[optind-1]: %s\n", optind, argv[optind], argv[optind-1]);

  /* convert to long */
  long num = 0;
  char* endp = NULL;
  num = strtol(argv[optind-1], &endp, 10);
  if(*endp) {
    fprintf(stderr, "Invalid number\n");
    return 0;
  }

  if (optind == argc || (fileptr != NULL && *(fileptr) == '-'))
    return 1;
  return 0;
}

void print_waitStatus(int first_index, int second_index, char**argv, int exit_status) {
  int i = first_index;
  /* print exit status */
  fprintf(stdout, "%d ", exit_status);
  while(i <= second_index) {
    fprintf(stdout, "%s ", argv[i]);
    i++;
    }
  fprintf(stdout, "\n");
}

int main(int argc, char **argv) {
  int c;
  int fn = 0;
  int verbose_flag = 0;
  int exit_status = 0; /* exit status 0 if all successful */
  int max_extstat = 0;
  int prof_flag = 0;
  int random;

  struct timeval oldutime;
  struct timeval oldstime;

  struct timeval newutime;
  struct timeval newstime;
  oldutime.tv_sec = 0;
  oldutime.tv_usec = 0;
  oldstime.tv_sec = 0;
  oldstime.tv_usec = 0;


  /* lookahead to get size and allocate fd array */
  int fd_size = 0;
  int i;
  for (i = 0; i < argc; i++)
  {
    if (cmpstr(argv[i],"--rdonly\0")==0 || cmpstr(argv[i],"--wronly\0")==0 || cmpstr(argv[i],"--rdwr\0")==0)
      {
        fd_size++; /* each file has 1 fd */
      }
    else if (cmpstr(argv[i],"--pipe")==0)
      {
	fd_size += 2; /* each pipe has 2 fd */
      }
  }

  int j = 0;


  /* dynamically allocate size for fd */
  int *fd = (int*) malloc(sizeof(int)*fd_size);
  int *pipe_array = (int*) malloc(sizeof(int)*fd_size);
  int *proc_array = (int*) malloc(sizeof(int)*16); //max at 16 processes
  int proc_index = 0;
  /* used to track the first and last index of command for printing */
  int *first_index = (int*) malloc(sizeof(int)* argc);
  int *last_index = (int*) malloc(sizeof(int)* argc);
  int command_num = 0;
  /* initialize to -2 just for own personal error checking methods */
  while(j < argc) {
    first_index[j] = -2;
    last_index[j] = -2;
    j++;
  }

  char **arguments = (char**) malloc(sizeof(char*) * argc);
  for(j = 0; j < argc; j++) {
    arguments[j] = argv[j];
  }
  j = 0;



  int flag = 0;

  while (1) {
    static struct option long_options[] =
      {
	/* File flag */
     	{ "append", no_argument, 0, 'a' },
	{ "cloexec", no_argument, 0, 'b' },
	{ "creat", no_argument, 0, 'd' },
	{ "directory", no_argument, 0, 'e' },
     	{ "dsync", no_argument, 0, 'f' },
	{ "excl", no_argument, 0, 'g' },
	{ "nofollow", no_argument, 0, 'h' },
	{ "nonblock", no_argument, 0, 'i' },
	{ "rsync", no_argument, 0, 'j' },
	{ "sync", no_argument, 0, 'k' },
	{ "trunc", no_argument, 0, 'l' },

	/* File-opening options */
	{ "rdonly", required_argument, 0, 'r' },
	{ "wronly", required_argument, 0, 'w' },
	{ "rdwr", required_argument, 0, 'x' },
	{ "pipe", no_argument, 0, 'y' },

	/* Miscellaneous options */
        { "verbose", no_argument, 0, 'v' },
	{ "profile", no_argument, 0, 'm'},
	{ "abort", no_argument, 0, 'n' },
	{ "catch", required_argument, 0, 'o' },
	{ "ignore", required_argument, 0, 'p' },
	{ "default", required_argument, 0, 'q' },
	{ "pause", no_argument, 0, 's' },
	{ "close", required_argument, 0, 't'},

	/* Subcommand options */
	{ "command", required_argument, 0, 'c' },
	{ "wait", no_argument, 0, 'z' },
	{ 0, 0, 0, 0 }
      };
    int option_index = 0;
    c = getopt_long(argc, argv, "", long_options, &option_index);

    /* Detect the end of options */
    if (c == -1) {
      break;
    }

    switch (c)
      {
      /* File flag cases */
      case 'a':
      case 'b':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
	if(flag_syntax (optind, argc, argv)) {
	  /* set flags since valid syntax */
	  if(c == 'a')
	    flag |= O_APPEND;
	  else if (c == 'b')
	    flag |= O_CLOEXEC;
	  else if (c == 'd')
	    flag |= O_CREAT;
	  else if (c == 'e')
	    flag |= O_DIRECTORY;
	  else if (c == 'f')
	    flag |= O_DSYNC;
	  else if (c == 'g')
	    flag |= O_EXCL;
	  else if (c == 'h')
	    flag |= O_NOFOLLOW;
	  else if (c == 'i')
	    flag |= O_NONBLOCK;
	  else if (c == 'j')
	    flag |= O_RSYNC;
	  else if (c == 'k')
	    flag |= O_SYNC;
	  else if (c == 'l')
	    flag |= O_TRUNC;
	}
	/* Invalid syntax, print error and don't set flag. */
	else {
	  fprintf(stderr, "Syntax error: %s has no arguments.\n", argv[optind-1]);
	  exit_status = 1;
	}
	/* valid syntax, check for verbose flag */
	if(verbose_flag == 1) {
	  fprintf(stdout, "%s\n", argv[optind-1]);
	}
	break;

	/* file-opening options */
      case 'r':
      case 'w':
      case 'x':
	/* check for flags */
	if(c == 'r') {
	  flag = (O_RDONLY | flag);
	}
	else if (c == 'w') {
	  flag = (O_WRONLY | flag);
	}
	else {
	  flag = (O_RDWR | flag);
	}

	/* check syntax */
	char* fileptr = NULL;
	if (optind < argc)
	  {
	    fileptr = argv[optind];
	  }

	/* last option or next next arg is an option */
	if (optind == argc || (fileptr != NULL && *(fileptr) == '-'))
	  {
	    /* open file */
	    fd[fn] = open(optarg, flag, 0755);
	    pipe_array[fn] = 0;
	    if (fd[fn] == -1)
	      {
		fprintf(stderr, "Error opening file: %s\n", optarg);
		fd[fn] = -1; /* map to a dummy fd */
		fn++;
		exit_status = 1;
		flag = 0; /* reset flags after opening file */
		break;
	      }
	    fn++;
	  }
	else /* print syntax error message */
	  {
	    fprintf(stderr, "Syntax error: %s has one argument only.\n", argv[optind-2]);
	    exit_status = 1;
	  }
	flag = 0;
	 /* check verbose flag */
	 if (verbose_flag)
	   {
	     if(c == 'r')
	       fprintf(stdout, "--rdonly %s\n", optarg);
	     else if(c == 'x')
               fprintf(stdout, "--rdwr %s\n", optarg);
	     else
               fprintf(stdout, "--wronly %s\n", optarg);
	   }
	 break;

	 /* pipe */
      case 'y':
	if(flag_syntax (optind, argc, argv)) {
	  int pipefd[2];
	  int errval; /* check for valid flags */
	  if(pipe2(pipefd, flag) == -1) { /* fail */
	    errval = errno;
	    fprintf(stderr, "Pipe failed.\n");
	    /* failed because invalid flags */
	    if (errno == EINVAL) {
	      fprintf(stderr, "Invalid pipe flag.\n");
	    }
	    exit_status = 1;
	    flag = 0;
	  }


	  else { /* pipe successful, read index 0, write index 1 */
	    fd[fn] = pipefd[0];
	    pipe_array[fn] = 1;
	    fn++;
	    fd[fn] = pipefd[1];
	    pipe_array[fn] = 1;
	    fn++;
	    flag = 0;
	  }
	  /* consumes two file numbers */
	  /* executes commands and places into logical file #'s output..? */
	}
	else {
          fprintf(stderr, "Syntax error: %s has no arguments.\n", argv[optind-1]);
	  exit_status = 1;
        }
	/* valid syntax, check for verbose flag */
	if(verbose_flag == 1) {
	  fprintf(stdout, "%s\n", argv[optind-1]);
	}
        break;


	/* subcommand options, command and wait */
      case 'c':
	/* cannot start block with declaration */
	random = 0;
	/* char array for args of cmd */
	char **c_args = malloc(sizeof(char*) * argc);
	char **tmp = c_args; /* keep track of beginning of c_args */
	int index = optind + 2;

	/* syntax check: must have at least 4 args */
	if (index >= argc)
	  {
	    fprintf(stderr, "Syntax error: invalid arguments for --command.\n");
	    exit_status = 1;
	    break;
	  }

	/* check that 4th arg must be an arg, not an option */
	char* cptr = argv[index];
	if (*(cptr) == '-' || *(cptr+1) == '-')
	  {
	    fprintf(stderr, "Syntax error: invalid arguments for --command.\n");
	    exit_status = 1;
	    break;
	  }

	/* store input/output/error values */
	char* iendptr = NULL;
	char* oendptr = NULL;
	char* eendptr = NULL;
	long in = strtol(argv[index-3], &iendptr, 10);
	long out = strtol(argv[index-2], &oendptr, 10);
	long err = strtol(argv[index-1], &eendptr, 10);

	/* check for valid fd */
	if (!validfd(iendptr,in,fn) || !validfd(oendptr,out,fn) || !validfd(eendptr,err,fn))
	  {
	    fprintf(stderr, "Invalid file descriptor.\n");
	    exit_status = 1;
	    break;
	  }

	/* if file is closed it's an error to access it */
	if((fd[in] == -1) || (fd[out] == -1) || (fd[err] == -1)) {
	  fprintf(stderr, "Error: Accessing invalid file\n");
	  exit_status = 1;
	  break;
	}

	/* check verbose flag */
	if (verbose_flag)
	  {
	    fprintf(stdout, "--command %ld %ld %ld ", in, out, err);
	  }

	first_index[command_num] = index;

	char *ptr = argv[index];
	/* if arg is not an option, store into array */
	while ((index < argc) && (*(ptr) != '-' || *(ptr+1) != '-'))
	  {
	    if (verbose_flag) /* verbose flag */
	      fprintf(stdout, "%s ", argv[index]);

	    *c_args = argv[index];
	    c_args++;
	    index++;
	    ptr = argv[index];
	  }
	if(verbose_flag)
	  fprintf(stdout, "\n");
	*c_args = NULL; /* null terminate the args */

	last_index[command_num] = index-1;

	//       	fprintf(stderr, "First Index: %s, Last Index: %s\n",argv[first_index[command_num]], argv[last_index[command_num]]);

	//	print_waitStatus(first_index[command_num], last_index[command_num], argv, 10);
	command_num++;

	int pid = fork();
	int status;
	if (pid >= 0) /* fork successful */
	  {
	    if (pid == 0) /* child process */
	      {
		/* use dup2 to specify i o e */
		if (dup2(fd[in], 0) < 0)
		  { /* error handling */
		    fprintf(stderr, "Error redirecting input.\n");
		    exit_status = 1;
		  }
		if (dup2(fd[out], 1) < 0)
		  {
		    fprintf(stderr, "Error redirecting output.\n");
		    exit_status = 1;
		  }
		if (dup2(fd[err], 2) < 0)
		  {
		    fprintf(stderr, "Error redirecting stderr.\n");
		    exit_status = 1;
		  }
		/* close child fds */
		int i;
		for(i = 0; i < fn; i++) {
		  close(fd[i]);
		}

		/* execvp returns -1 if error */
		if (execvp(*tmp, tmp) < 0)
		  {
		    fprintf(stderr, "Error executing command: %s\n", *tmp);
		    exit(1); /* exit child process */
		  }
	      }
	    /* parent process */
	    else
	      {
		proc_array[proc_index] = pid;
		proc_index++;

		/* close the end of the pipe that is being used, use fd[in,out,err] to check */
		/* this doesn't close only the end that's being used so TODO this */
		if(pipe_array[in] == 1) {
		  close(fd[in]);
		}
		if(pipe_array[out] == 1) {
		  close(fd[out]);
		}
		if(pipe_array[err] == 1) {
		  close(fd[err]);
		}

		//waitpid(pid, &status, 0);
	      }
	  }
	else
	  {
	    fprintf(stderr, "Fork failed.");
	    exit_status = 1;
	  }
	break;

	/* wait */
      case 'z':
	if(flag_syntax(optind, argc, argv)) {
	  if(verbose_flag == 1)
	    fprintf(stdout, "--wait\n");

	  /* TODO: 16 vs total number of processes */
	  int i;
	  for(i = 0; i < fn; i++) {
	    close(fd[i]);
	  }
	  int status;
	  int extstat; /* child's exit status */

	  /* getrusage on child processes */
	  struct rusage start_ru;
	  struct rusage end_ru;
	  getrusage(RUSAGE_CHILDREN, &start_ru);

	  /* call wait on all child processes */
	  for(i = 0; i < proc_index; i++) {
	    /* getrusage for children */


	    waitpid(proc_array[i], &status, 0);
	    if (WIFSIGNALED(status)) {
	      raise(WTERMSIG(status));
	    }
	    /* output exit status, copy of command + args */
	    if (WIFEXITED(status)) {
	      extstat = WEXITSTATUS(status);
	      print_waitStatus(first_index[i], last_index[i], arguments, extstat);
	      if (extstat > max_extstat)
		max_extstat = extstat;
	    }
	    else {
	      fprintf(stderr, "Child process not terminated normally.\n");
	      exit_status = 1;
	    }
	  }/* end for */
	  getrusage(RUSAGE_CHILDREN, &end_ru);
	  fprintf(stdout, "Child Processes: User CPU Time Used: %lld seconds, %lld microseconds. System CPU Time Used: %lld seconds, %lld microseconds.\n", \
	      (long long)(end_ru.ru_utime.tv_sec - start_ru.ru_utime.tv_sec), (long long)(end_ru.ru_utime.tv_usec - start_ru.ru_utime.tv_usec), \
	      (long long)(end_ru.ru_stime.tv_sec - start_ru.ru_stime.tv_sec), (long long)(end_ru.ru_stime.tv_usec - start_ru.ru_stime.tv_usec));
	}
	else {
	  fprintf(stderr, "Syntax error: --wait has no arguments\n");
	}
	break;

	/* close */
      case 't':
	if(opt_syntax(optind, argc, argv)) {
	  if(verbose_flag == 1)
	    fprintf(stdout, "%s %s\n", argv[optind-2], argv[optind-1]);
	  long num;
	  char*endptr = NULL;
	  num = strtol(argv[optind-1], &endptr, 10);
	  if(*endptr){
	    fprintf(stderr, "Error converting number\n");
	    break;
	  }
	  /* check if the file even exists */
	  if((num > (fn-1)) || (num < 0)) {
	    fprintf(stderr, "File does not exist\n");
	    exit_status = 1;
	    break;
	  }
	  close(fd[num]);
	  fd[num] = -1;
	}
	else {
	  fprintf(stderr, "Syntax error: Invalid arguments for --close\n");
	}

      /* Misc options */
      case 'v':
	/* check syntax */
	if(flag_syntax (optind, argc, argv)){
	  if(verbose_flag == 1)
	    fprintf(stdout, "--verbose\n");
	  else
	      verbose_flag = 1;
	}
	else {
	    fprintf(stderr, "Syntax error: --verbose has no arguments.\n");
	    exit_status = 1;
	  }
 	break;

	/* profile */
      case 'm':
	if(flag_syntax(optind, argc, argv)) {
	    prof_flag = 1;
	    if(verbose_flag == 1)
	      fprintf(stdout, "--profile\n");
	  }
	else {
	  fprintf(stderr, "Syntax error: --profile has no arguments.\n");
	}
	break;

	/* abort */
      case 'n':
	if(verbose_flag == 1)
	  fprintf(stdout, "--abort\n");

	if(flag_syntax(optind, argc, argv))
	  raise(SIGSEGV);
	else {
	  fprintf(stderr, "Syntax error: --abort has no arguments.\n");
	  exit_status = 1;
	}
	break;

	/* catch */
      case 'o':
	if(opt_syntax(optind, argc, argv)) {
	  int sig_num = atoi(argv[optind-1]);
	  if (verbose_flag == 1)
	    fprintf(stdout, "--catch %d\n", sig_num);
	  if (signal(sig_num, &catch_handler) == SIG_ERR)
	    fprintf(stderr, "Error handling signal.\n");
	}
	else {
	  fprintf(stderr, "Syntax error: --catch has one argument.\n");
	  exit_status = 1;
	}
	break;

	/* ignore */
      case 'p':
	if (opt_syntax(optind, argc, argv)) {
	  int sig_num = atoi(argv[optind-1]);
	  if (verbose_flag == 1)
	    fprintf(stdout, "--ignore %d\n", sig_num);
	  if (signal(sig_num, SIG_IGN) == SIG_ERR)
	    fprintf(stderr, "Error handling signal.\n");
	}
	else {
	  fprintf(stderr, "Syntax error: --ignore has one argument.\n");
	  exit_status = 1;
	}
	break;

	/* default */
      case 'q':
	if (opt_syntax(optind, argc, argv)) {
	  int sig_num = atoi(argv[optind-1]);
	  if (verbose_flag == 1)
	    fprintf(stdout, "--default %d\n", sig_num);
	  if (signal(sig_num, SIG_DFL) == SIG_ERR)
	    fprintf(stderr, "Error handling signal.\n");
	}
	else {
	  fprintf(stderr, "Syntax error: --default has one argument.\n");
	  exit_status = 1;
	}
	break;

	/* pause */
      case 's':
	if (verbose_flag == 1)
	  fprintf(stdout, "--pause\n");

	if(flag_syntax(optind, argc, argv)) {
	  if (pause() == -1)
	    fprintf(stderr, "Pause failed.\n");
	}
	else {
	  fprintf(stderr, "Syntax error: --pause has no arguments.\n");
	  exit_status = 1;
	}
	break;

      case ':':
      case '?':
	/* getopt prints message to stderr */
	break;
      default:
	break;
      }
    /* getrusage */
    /* need to get user and system time */
    if(prof_flag == 1) {
      struct rusage ru;
      getrusage(RUSAGE_SELF, &ru);

      /* newtime used for getting the current rusage time */
      newutime = ru.ru_utime;
      newstime = ru.ru_stime;

      fprintf(stdout, "User CPU Time Used: %lld seconds, %lld microseconds. System CPU Time Used: %lld seconds, %lld microseconds.\n", \
	      (long long)(newutime.tv_sec - oldutime.tv_sec), (long long)(newutime.tv_usec - oldutime.tv_usec), \
	      (long long)(newstime.tv_sec - oldstime.tv_sec), (long long)(newstime.tv_usec - oldstime.tv_usec));
      /* update oldtime to the current time (which will become the old time) */
      oldutime = newutime;
      oldstime = newstime;
    }
  }

  if (max_extstat == 0)
    exit(exit_status);
  else
    exit(max_extstat);
}