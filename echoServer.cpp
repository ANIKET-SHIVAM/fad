#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "echo_socket"

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

struct PCI_Connection
{
    void
    inputOutput()
    {
        LockGuard lock(inUseMutex);
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    void
    reconfigure()
    {
        LockGuard lock(inUseMutex);
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    std::mutex inUseMutex;
};

class IO_Handler
{
    enum Status { NOT_STARTED, RUNNING, FINISHED};
public:

    IO_Handler(std::string designPath_, Socket socket_, PCI_Connection& pciConnection_)
    : designPath(designPath_), status(Status::NOT_STARTED), socket(socket_), pciConnection(pciConnection_)
    {}

    std::thread
    spawn()
    {
        status = Status::RUNNING;
        return std::thread( [this] { this->run(); });
    }

    void
    run()
    {
        int done, n;
        char str[100];
        done = 0;
        try{
            do {
                n = recv(socket, str, 100, 0);
                if (n <= 0) {
                    if (n < 0) perror("recv");
                    done = 1;
                }

                if (!done) 
                    pciConnection.inputOutput();
                    if (send(socket, str, n, MSG_NOSIGNAL) < 0) {
                        perror("send");
                        done = 1;
                    }
            } while (!done);
        }
        catch(std::exception const& )
        {
            std::cout << "Lost connection!!\n";
        }

        {
        LockGuard lock(statusMutex);
        close(socket);
        status = Status::FINISHED;
        }
    }

    bool
    notStarted()
    {
        LockGuard lock(statusMutex);
        return status == Status::NOT_STARTED;
    }

    bool
    finished()
    {
        LockGuard lock(statusMutex);
        return status == Status::FINISHED;
    }

private:
    std::string designPath;
    Status status;
    std::mutex statusMutex;
    Socket socket;
    PCI_Connection& pciConnection;
};

class DesignHandler
{
    using IOThreads   = std::vector<std::thread*>; 
    using IO_Handlers = std::vector<IO_Handler*>;

public:
    DesignHandler()
    : fpgaIsReady(false)
    {}

    std::thread
    spawn()
    {
        return std::thread( [this] { this->run(); });
    }

    void
    run()
    {
        PCI_Connection pciConnection; // Open the connection...
        char designPath[100];

        for(;;)
        {
            removeFinishedIOThreads();

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

                ioHandlers.push_back(new IO_Handler(std::string(designPath), newConnection, pciConnection)); 
            }

            removeFinishedIOThreads();

            if(!ioHandlers.empty())
            {
                compileNewDesign();
                reconfigureFPGA(pciConnection);
                startNewIoThreads();
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
    removeFinishedIOThreads()
    {
        for(size_t i = 0; i < ioHandlers.size(); /*increment inside*/)
        {
            if(ioHandlers[i]->finished())
            {
                ioThreads[i]->join();
                delete ioHandlers[i];
                ioHandlers.erase(ioHandlers.begin() + i);
                delete ioThreads[i];
                ioThreads.erase(ioThreads.begin() + i);
                std::cout << "Removed connection " << i << std::endl;
                // No Increment
            }
            else
            {
                ++i;
            }
        }
    }

    // Lock on IOThreads must be taken
    void
    compileNewDesign()
    {
        std::cout << "Starting to compile new design for " << ioHandlers.size() << " clients" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "Finished Compiling new design!\n";
    }

    void
    startNewIoThreads()
    {
        //std::cout << "Starting new IO Threads " 
        //          << ioHandlers.size() << " " << ioThreads.size() << std::endl;
        for(auto& handler : ioHandlers)
        {
            if(handler->notStarted())
            {
                ioThreads.push_back(new std::thread(handler->spawn()));
            }
        }
        //std::cout << "Finished starting new threads" 
        //          << ioHandlers.size() << " " << ioThreads.size() << std::endl;
    }

    void
    reconfigureFPGA(PCI_Connection& pciConnection)
    {
        pciConnection.reconfigure();   
    }

    std::vector<Socket> unhandledConnections;
    std::mutex          unhandledConnectionsMutex;

    IO_Handlers ioHandlers;
    IOThreads   ioThreads;

    std::condition_variable wakeUp;
    std::mutex              wakeUpMutex;

    bool fpgaIsReady;
};


int main(void)
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
