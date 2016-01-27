#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

using namespace std;

#define DAEMON_NAME "fad"

void process(int i, int pid){
    syslog (LOG_NOTICE, "Writing to %d's %dth Syslog",pid,i);
}   

int main(int argc, char *argv[]) {
     
    //Set our Logging Mask and open the Log
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);

    syslog(LOG_NOTICE, "Entering Daemon");

    pid_t pid, sid;

   //Fork the Parent Process to make 'process' autonomous (i.e. start-stop-daemon process in fad script ends)
    pid = fork();
    syslog(LOG_NOTICE, "pid: %d",pid);

    if (pid < 0) {syslog(LOG_NOTICE, "Daemon couldn't create child process"); exit(EXIT_FAILURE); }

    //We got a good pid, Close the Parent Process
    if (pid > 0) {syslog(LOG_NOTICE, "Daemon created a child process"); exit(EXIT_SUCCESS); }

    if (pid == 0) { //child process
    	int i = 1;
    	while(i<6){
        	sleep(5);    //Sleep for 60 seconds
		process(i,pid);    //Run our Process
        	i++;
	}

    }
    //Change File Mask
    umask(0);

    //Create a new Signature Id for our child
    sid = setsid();
    syslog(LOG_NOTICE, "sid: %d",sid);

    if (sid < 0) { exit(EXIT_FAILURE); }

    //Change Directory
    //If we cant find the directory we exit with failure.
    if ((chdir("/")) < 0) { exit(EXIT_FAILURE); }

    //Close Standard File Descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //Close the log
    closelog ();
}
