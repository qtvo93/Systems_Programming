#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>

#include "builtin.h"
#include "parse.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
/*******************************************
 * Set to 1 to view the command line parse *
 *******************************************/
#define DEBUG_PARSE 0

void print_banner ()
{
    printf ("                    ________   \n");
    printf ("_________________________  /_  \n");
    printf ("___  __ \\_  ___/_  ___/_  __ \\ \n");
    printf ("__  /_/ /(__  )_(__  )_  / / / \n");
    printf ("_  .___//____/ /____/ /_/ /_/  \n");
    printf ("/_/ Type 'exit' or ctrl+c to quit\n\n");
}


/* returns a string for building the prompt
 *
 * Note:
 *   If you modify this function to return a string on the heap,
 *   be sure to free() it later when appropirate!  */
static char* build_prompt ()
{
	char* path;	
	char* cd = getcwd (NULL, 0 );
	char* pr = "$ ";
	
	path = malloc (strlen(cd) + strlen(pr) + 1);
	strcpy(path, cd);
	strcat(path, pr);
	free (cd);
    return path;
}


/* return true if command is found, either:
 *   - a valid fully qualified path was supplied to an existing file
 *   - the executable file was found in the system's PATH
 * false is returned otherwise */
static int command_found (const char* cmd)
{
    char* dir;
    char* tmp;
    char* PATH;
    char* state;
    char probe[PATH_MAX];

    int ret = 0;

    if (access (cmd, X_OK) == 0)
        return 1;

    PATH = strdup (getenv("PATH"));

    for (tmp=PATH; ; tmp=NULL) {
        dir = strtok_r (tmp, ":", &state);
        if (!dir)
            break;

        strncpy (probe, dir, PATH_MAX-1);
        strncat (probe, "/", PATH_MAX-1);
        strncat (probe, cmd, PATH_MAX-1);

        if (access (probe, X_OK) == 0) {
            ret = 1;
            break;
        }
    }

    free (PATH);
    return ret;
}


/* Called upon receiving a successful parse.
 * This function is responsible for cycling through the
 * tasks, and forking, executing, etc as necessary to get
 * the job done! */
void execute_tasks (Parse* P)
{
    unsigned int t;
	int fd[2];
	int in, out;
	pid_t *pid;	
	pid = malloc (P->ntasks * sizeof(*pid));
	
	if (P->infile) {
		in =  open (P->infile, 0);
	} else {
		in = STDIN_FILENO;
	}	
	for (t = 0; t < P->ntasks - 1; t++) {
		pipe(fd);
		pid[t] = fork();	
		if (!pid[t]) {
			close(fd[0]);
			if (in != STDIN_FILENO){
				if (dup2(in, STDIN_FILENO) == -1) {
					fprintf(stderr, "dup2() failed\n");
					exit(EXIT_FAILURE);
				}
				close (in);
			}
			if (fd[1] != STDOUT_FILENO) {
				if (dup2(fd[1], STDOUT_FILENO) == -1) {
					fprintf(stderr, "dup2() failed\n");
					exit(EXIT_FAILURE);
				}
				close (fd[1]);
			}
			if (is_builtin (P->tasks[t].cmd)) {
				builtin_execute(P->tasks[t]);
			} else if (command_found (P->tasks[t].cmd)) {
				execvp(P->tasks[t].cmd, P->tasks[t].argv);
			} else {
				printf("pssh: command not found: %s\n", P->tasks[t].cmd);
				exit(EXIT_SUCCESS);
			}
		} 		
		close (fd[1]);
		if (in != STDIN_FILENO)
			close(in);		
		in = fd[0];
		if (!strcmp(P->tasks[t].cmd, "exit")) 
			exit(EXIT_SUCCESS);
	}
	
	if (P->outfile) {
		out =  open (P->outfile, O_CREAT | O_WRONLY, 0664);
	} else {
		out =  STDOUT_FILENO;
	}
	pid[t] = fork();	
	if (!pid[t]) {
		if (in != STDIN_FILENO){
			if (dup2(in, STDIN_FILENO) == -1) {
				fprintf(stderr, "dup2() failed\n");
				exit(EXIT_FAILURE);
			}
			close (in);
		}
		if (out != STDOUT_FILENO) {
			if (dup2(out, STDOUT_FILENO) == -1) {
				fprintf(stderr, "dup2() failed\n");
				exit(EXIT_FAILURE);
			}
			close (out);
		}
		if (is_builtin (P->tasks[t].cmd)) {
			builtin_execute(P->tasks[t]);
		} else if (command_found (P->tasks[t].cmd)) {
			execvp(P->tasks[t].cmd, P->tasks[t].argv);
		} else {
			printf("pssh: command not found: %s\n", P->tasks[t].cmd);
			exit(EXIT_SUCCESS);
		}				
	} 
	if (!strcmp(P->tasks[t].cmd, "exit")) 
		exit(EXIT_SUCCESS);
	if (in != STDIN_FILENO)
		close(in);
	if (out != STDOUT_FILENO)
		close(out);	
	for (t = 0; t < P->ntasks; t++) {
		waitpid(pid[t], NULL, 0);
	}
	free(pid);
}

int main (int argc, char** argv)
{
    char* cmdline;
    Parse* P;
	char *prompt_path;
    print_banner ();

    while (1) {
		prompt_path = build_prompt();
        cmdline = readline (prompt_path);
		free(prompt_path);
        if (!cmdline)       /* EOF (ex: ctrl-d) */
            exit (EXIT_SUCCESS);

        P = parse_cmdline (cmdline);
        if (!P)
            goto next;

        if (P->invalid_syntax) {
            printf ("pssh: invalid syntax\n");
            goto next;
        }

#if DEBUG_PARSE
        parse_debug (P);
#endif

        execute_tasks (P);

    next:
        parse_destroy (&P);
        free(cmdline);
    }
}
