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
#include <condition_variable>
#include <iostream>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

using Socket = int;

void
echoHandler(Socket socket, short& finished)
{
    int done, n;
    char str[100];
    done = 0;
    do {
        n = recv(socket, str, 100, 0);
        if (n <= 0) {
            if (n < 0) perror("recv");
            done = 1;
        }

        if (!done) 
            if (send(socket, str, n, 0) < 0) {
                perror("send");
                done = 1;
            }
    } while (!done);

    close(socket);
    finished = true;
}


class DesignHandler
{
    using IOThreads = std::vector<std::thread>; 

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
        for(;;)
        {
            removeFinishedIOThreads();

            Socket newConnection;
            while( (newConnection = getNewConnection() ) != -1)
            {
                addNewDesign(newConnection);

                ioThreadStatus.emplace_back(false);
                ioThreads.emplace_back(echoHandler, newConnection, std::ref(ioThreadStatus.back()));
            }

            // Compile new FPGA design...
            // Stop IO Threads from
            // Update FPGA
            // Let IO Threads continue..

            std::unique_lock<std::mutex> wakeUpLock(wakeUpMutex);
            wakeUp.wait(wakeUpLock);
        }
    }

    void
    newConnection(Socket s)
    {
        {
        std::lock_guard<std::mutex> lock(unhandledConnectionsMutex);
        unhandledConnections.push_back(s);
        }

        wakeUp.notify_one();
    }

private:
    Socket
    getNewConnection()
    {
        std::lock_guard<std::mutex> lock(unhandledConnectionsMutex);
        if(unhandledConnections.empty())
            return -1;
        else
        {
            Socket newConnection = unhandledConnections.back();
            unhandledConnections.pop_back();
            return newConnection;
        }
    }

    void
    addNewDesign(Socket newConnection)
    {
        std::cout << "Got new design connection from " << newConnection << std::endl;
    }

    void
    removeFinishedIOThreads()
    {
        const bool finished = true;
        for(size_t i = 0; i < ioThreads.size(); ++i)
        {
            if(ioThreadStatus[i] == finished)
            {
                ioThreads[i].join();
                ioThreads.erase(ioThreads.begin() + i);
                ioThreadStatus.erase(ioThreadStatus.begin() + i);
                std::cout << "Removed connection " << i << std::endl;
            }
        }
    }

    std::vector<Socket> unhandledConnections;
    std::mutex          unhandledConnectionsMutex;

    IOThreads  ioThreads;
    std::vector<short> ioThreadStatus; // false == running, true == finished

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
