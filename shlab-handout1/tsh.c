/*
 * tsh - A tiny shell program with job control
 *
 * Matthew Christensen - mrc621
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE 1024 /* max line size */
#define MAXARGS 128  /* max args on a command line */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
char sbuf[MAXLINE];      /* for composing sprintf messages */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
int parseargs(char **argv, int *cmds, int *stdin_redir, int *stdout_redir);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Execute the shell's read/eval loop */
    while (1)
    {

        /* Read command line */
        if (emit_prompt)
        {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin))
        { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
*/
void eval(char *cmdline)
{
    char *args[MAXARGS];
    int bg = parseline(cmdline, args);
    int pgid = 0;

    if (args[0] == NULL)
    {
        return;
    }

    builtin_cmd(args); //If it's a built in command, just run it

    int cmds[20]; //Todo: make this not a magical number (max num of commands?)
    int stdin_redir[20];
    int stdout_redir[20];

    int numCommands = parseargs(args, cmds, stdin_redir, stdout_redir);

    // int old_p[2];

    for (int i = 0; i < numCommands; i++) //Create children
    {

        // int p[2];

        // if (i < numCommands - 1) //Create pip if we not at the last child (numPipes = numChildren - 1)
        // {
        //     if (pipe(p) == -1)
        //     {
        //         fprintf(stderr, "Pipe Failed");
        //         return;
        //     }
        // }

        // if (stdin_redir[i] > -1) //If we have input redirection
        // {
        //     FILE *pipeFile = fdopen(p[STDIN_FILENO], "r"); //Open file
        //     dup2(fileno(pipeFile), STDIN_FILENO);          //Redirect
        // }
        // else if (i > 0) //If this isn't the first command
        // {
        //     dup2(old_p[STDIN_FILENO], STDIN_FILENO); //STDIN to read end of pipe
        //     close(p[STDOUT_FILENO]);                 //WRITE end closed
        // }

        // if (stdout_redir[i] > -1) //If we have out redirection
        // {
        //     FILE *pipeFile = fdopen(p[STDOUT_FILENO], "w"); //Open file
        //     dup2(fileno(pipeFile), STDOUT_FILENO);          //Redirect
        // }
        // else if (i < numCommands - 1) //If this isn't the last command
        // {
        //     dup2(p[STDOUT_FILENO], STDOUT_FILENO); //STDOUT to write end of pipe
        //     close(p[STDIN_FILENO]);                //READ in closed
        // }

        int pid = fork();

        if (pid == 0) //If we are the CHILD
        {
            execve(args[cmds[i]], &args[cmds[i]], environ);
        }
        else //If we are the PARENT
        {
            // old_p[0] = p[0];
            // old_p[1] = p[1];

            if (i == 0) //If this is the first child created
            {
                pgid = pid; //Then the group id is set the the first child process group
            }
            setpgid(pid, pgid); //Set all children to have our process group id (which is the pid of the first child created)

            waitpid(pid, NULL, 0); //Wait for the childen to be completed in order

            // close(old_p[STDIN_FILENO]);
            // close(old_p[STDOUT_FILENO]);
        }
    }

    // int cmds[]

    // parseargs(args, args, );
    return;
}

/*
 * parseargs - Parse the arguments to identify pipelined commands
 *
 * Walk through each of the arguments to find each pipelined command.  If the
 * argument was | (pipe), then the next argument starts the new command on the
 * pipeline.  If the argument was < or >, then the next argument is the file
 * from/to which stdin or stdout should be redirected, respectively.  After it
 * runs, the arrays for cmds, stdin_redir, and stdout_redir all have the same
 * number of items---which is the number of commands in the pipeline.  The cmds
 * array is populated with the indexes of argv corresponding to the start of
 * each command sequence in the pipeline.  For each slot in cmds, there is a
 * corresponding slot in stdin_redir and stdout_redir.  If the slot has a -1,
 * then there is no redirection; if it is >= 0, then the value corresponds to
 * the index in argv that holds the filename associated with the redirection.
 *
 */
int parseargs(char **argv, int *cmds, int *stdin_redir, int *stdout_redir)
{
    int argindex = 0; /* the index of the current argument in the current cmd */
    int cmdindex = 0; /* the index of the current cmd */

    if (!argv[argindex])
    {
        return 0;
    }

    cmds[cmdindex] = argindex;
    stdin_redir[cmdindex] = -1;
    stdout_redir[cmdindex] = -1;
    argindex++;
    while (argv[argindex])
    {
        if (strcmp(argv[argindex], "<") == 0)
        {
            argv[argindex] = NULL;
            argindex++;
            if (!argv[argindex])
            { /* if we have reached the end, then break */
                break;
            }
            stdin_redir[cmdindex] = argindex;
        }
        else if (strcmp(argv[argindex], ">") == 0)
        {
            argv[argindex] = NULL;
            argindex++;
            if (!argv[argindex])
            { /* if we have reached the end, then break */
                break;
            }
            stdout_redir[cmdindex] = argindex;
        }
        else if (strcmp(argv[argindex], "|") == 0)
        {
            argv[argindex] = NULL;
            argindex++;
            if (!argv[argindex])
            { /* if we have reached the end, then break */
                break;
            }
            cmdindex++;
            cmds[cmdindex] = argindex;
            stdin_redir[cmdindex] = -1;
            stdout_redir[cmdindex] = -1;
        }
        argindex++;
    }

    return cmdindex + 1;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'')
    {
        buf++;
        delim = strchr(buf, '\'');
    }
    else
    {
        delim = strchr(buf, ' ');
    }

    while (delim)
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'')
        {
            buf++;
            delim = strchr(buf, '\'');
        }
        else
        {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;

    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
    {
        argv[--argc] = NULL;
    }
    return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv)
{
    if (strcmp(argv[0], "quit") == 0) //If the command is quit
    {
        exit(EXIT_SUCCESS);
    }
    return 0; /* not a builtin command */
}

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}
