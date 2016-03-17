#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <ctime>


#define DAEMON_NAME "fad"

#define FAD_PATH "/home/aniket/fad"

#define BLACKHOLE_PATH "/home/aniket/fad/blackhole"

#define VERILOG_SRC_PATH "/home/aniket/fad/blackhole/verilog"

#include "designServer.cpp"

using namespace std;

/*void process(){
//    syslog (LOG_NOTICE, "Writing to Syslog");
//    system("cat " BLACKHOLE_PATH "/*");
    system("python " FAD_PATH "/merge.py");                       //Merge programs
    system("rm -rf " VERILOG_SRC_PATH "/*");                      //Clear old verilog files
    system("cp " BLACKHOLE_PATH "/offspring.c " VERILOG_SRC_PATH); //Copy new program files
    system("cp " BLACKHOLE_PATH "/Makefile " VERILOG_SRC_PATH);    //Copy Makefile 
    system("make -C " VERILOG_SRC_PATH "/ -s");                  //Generate verilog for ModelSim to execute
    system("make v -C " VERILOG_SRC_PATH "/ > " BLACKHOLE_PATH "/aftermath -s"); //Synthesize/Run on ModelSim
    system("python " FAD_PATH "/extract.py");
}*/   

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
        if (!(stat(BLACKHOLE_PATH, &sb) == 0 && S_ISDIR(sb.st_mode))){
        	system("mkdir " BLACKHOLE_PATH);
        	system("chmod 777 " BLACKHOLE_PATH); 
	}
	
       executeServer();

       /*
	std::clock_t start = std::clock();
	while(std::clock()-start < 1*60){ //minutes<change it accordingly>*seconds
		process();    //Run our Process
		sleep(60);
	}
       */
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
