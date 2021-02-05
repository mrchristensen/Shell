#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	char *scenario = argv[1];
	int pid = atoi(argv[2]);

	switch (scenario[0]) {
	case '1':
		break;
	case '2':
		kill(pid, 1);
		sleep(5);
;		break;
	case '3':
		kill(pid, 1);
		sleep(5);
		kill(pid, 1);
		sleep(5);
		break;
	case '4':
		kill(pid, 1);
		sleep(1);
		kill(pid, 2);
		sleep(5);
		break;
	case '5':
		kill(pid, 1); //prints 1 and 2
		sleep(5);
		kill(pid, 10); //prints 7
		sleep(1);
		kill(pid, 16); //prints 10
		sleep(1);
		break;
	case '6':
		kill(pid, 31); //block
		sleep(1);
		kill(pid, 10); //fork and set foo to child pd
		sleep(1);
		kill(pid, 30); //sets foo to 6
		sleep(1);
		kill(pid, 1); //prints 1 and 2
		sleep(5);
		kill(pid, SIGTERM); //prints foo
		break;
	case '7':
		kill(pid, 31); //block
		sleep(1);
		kill(pid, SIGQUIT);
		sleep(5);
		kill(pid, 1);
		sleep(5);
		break;

	}
}
