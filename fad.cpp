#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include<ctime>

using namespace std;

#define DAEMON_NAME "fad"

void process(){
//    syslog (LOG_NOTICE, "Writing to Syslog");
    system("cat /home/aniket/fad/blackhole/*");
}   

int main(int argc, char *argv[]) {
     
    //Set our Logging Mask and open the Log
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);

    syslog(LOG_NOTICE, "Entering Daemon");

    pid_t pid, sid;

   //Fork the Parent Process to make 'process' autonomous (i.e. start-stop-daemon process in fad script ends)
    pid = fork();
//    syslog(LOG_NOTICE, "pid: %d",pid);

    if (pid < 0) {syslog(LOG_NOTICE, "Daemon couldn't create child process"); exit(EXIT_FAILURE); }

    //We got a good pid, Close the Parent Process
    if (pid > 0) {syslog(LOG_NOTICE, "Daemon created a autonomous child process"); exit(EXIT_SUCCESS); }

    if (pid == 0) { //child process
	struct stat sb;
        if (!(stat("/home/aniket/fad/blackhole", &sb) == 0 && S_ISDIR(sb.st_mode))){
        	system("mkdir /home/aniket/fad/blackhole");
        	system("chmod 777 /home/aniket/fad/blackhole"); 
	}
	
	std::clock_t start = std::clock();
	while(std::clock()-start < 2*60*50){ //minutes<change it accordingly>*seconds*clocks_per_sec
		sleep(5);    //Sleep for 5 seconds
		process();    //Run our Process
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
