#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "/home/aniket/fad/design_socket"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

using Socket    = int;
using LockGuard = std::lock_guard<std::mutex>;

class IO_Handler
{
public:

    IO_Handler(std::string designPath_, Socket socket_)
    : designPath(designPath_), socket(socket_)
    {}

    void
    passOutput(std::string const& output)
    {
        std::cout << "Passing output...\n";
        if (send(socket, output.c_str(), output.size(), MSG_NOSIGNAL) < 0)
        {
            perror("send");
        }
        close(socket);
    }
    std::string designPath;
private:    
    Socket socket;
};

class DesignHandler
{
    using IO_Handlers = std::vector<IO_Handler>;

public:
    DesignHandler()
    {}

    std::thread
    spawn()
    {
        return std::thread( [this] { this->run(); });
    }

    void
    run()
    {
        char designPath[100];

        for(;;)
        {
            Socket newConnection;
            while( (newConnection = getNewConnection() ) != -1)
            {
                std::cout << "Got new design connection from " << newConnection << std::endl;
                int n;
                n = recv(newConnection, designPath, 100, 0);
                if(n <= 0)
                {
                    if(n < 0) perror("design recv");
                    continue;
                }

                // Validate design name
                std::cout << "Design path is: " << designPath << std::endl;

                ioHandlers.emplace_back(std::string(designPath), newConnection); 
                // False delay...
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }

            if(!ioHandlers.empty())
            {
                compileNewDesign();
                spreadOutput();
                ioHandlers.clear();
            }

            if(!areThereNewConnections())
            {
                std::unique_lock<std::mutex> wakeUpLock(wakeUpMutex);
                wakeUp.wait(wakeUpLock);
            }
        }
    }

    void
    newConnection(Socket s)
    {
        {
        LockGuard lock(unhandledConnectionsMutex);
        unhandledConnections.push_back(s);
        }

        wakeUp.notify_one();
    }

private:

    Socket
    getNewConnection()
    {
        LockGuard lock(unhandledConnectionsMutex);
        if(unhandledConnections.empty())
            return -1;
        else
        {
            Socket newConnection = unhandledConnections.back();
            unhandledConnections.pop_back();
            return newConnection;
        }
    }

    bool
    areThereNewConnections()
    {
        LockGuard lock(unhandledConnectionsMutex);
        return !unhandledConnections.empty();
    }

    void
    compileNewDesign()
    {
	std::cout << "Starting to compile new design for " << ioHandlers.size() << " clients" << std::endl;
	system("rm -f " BLACKHOLE_PATH "/seeds/*");
	for(int cl = 0; cl < ioHandlers.size(); cl++) 	
	{
	    std::string command = "cp "+ioHandlers[cl].designPath+" " BLACKHOLE_PATH "/seeds/";
	    system(command.c_str());   
	}
	
	{
		std::string command = "python " FAD_PATH "/merge.py -c " + std::to_string(ioHandlers.size());
		system(command.c_str());                       //Merge programs
	}
	system("rm -rf " VERILOG_SRC_PATH "/*");                      //Clear old verilog files
	system("cp " BLACKHOLE_PATH "/offspring.c " VERILOG_SRC_PATH); //Copy new program files
	system("cp " BLACKHOLE_PATH "/Makefile " VERILOG_SRC_PATH);    //Copy Makefile 
	system("make -C " VERILOG_SRC_PATH "/ -s");                  //Generate verilog for ModelSim to execute
	system("make v -C " VERILOG_SRC_PATH "/ > " BLACKHOLE_PATH "/aftermath -s"); //Synthesize/Run on ModelSim
	{
		std::string command = "python " FAD_PATH "/extract.py -c " + std::to_string(ioHandlers.size());
		system(command.c_str());                       //Merge programs
	}
	
	std::this_thread::sleep_for(std::chrono::seconds(5));
	std::cout << "Finished Compiling new design!\n";
    }

    void
    spreadOutput()
    {
        for(auto& handle : ioHandlers)
        {
            std::string out = "Here you go!";
            handle.passOutput(out);
        }
    }


    std::vector<Socket> unhandledConnections;
    std::mutex          unhandledConnectionsMutex;

    IO_Handlers ioHandlers;

    std::condition_variable wakeUp;
    std::mutex              wakeUpMutex;
};


int executeServer()
{
    int s, s2, len;
    socklen_t t;

    struct sockaddr_un local, remote;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(s, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(s, 5) == -1) {
        perror("listen");
        exit(1);
    }

    DesignHandler handler;
    std::thread handlerThread = handler.spawn();
    for(;;) {
        printf("Waiting for a connection...\n");
        t = sizeof(remote);
        if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
            perror("accept");
            exit(1);
        }

        printf("Connected.\n");
        handler.newConnection(s2); 
    }

    return 0;
}
