#include "./headers/Server.hpp"

void Server::initialize()
{
    ML::initialize("socketChat.log");

    ML::log_info("Socket Chat loggin system initialized", TARGET_ALL);

    threadPool = new ConnectionThreadPool();
    
    serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
}

int Server::bindAndListen()
{
    int temp = 1;
    if (serverSocketFD < 0 || setsockopt(serverSocketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &temp, sizeof(temp)) < 0)
    {
        ML::log_fatal(std::string("Cannot create new socket with current IP or port !"), TARGET_ALL);
        return -1;
    }

    ML::log_info("Server socket created", TARGET_ALL);

    listeningAddress.sin_addr.s_addr = INADDR_ANY;
    listeningAddress.sin_port = htons(port);
    listeningAddress.sin_family = AF_INET;

    if (bind(serverSocketFD, (struct sockaddr *)&listeningAddress, sizeof(listeningAddress)) < 0 || listen(serverSocketFD, 8) < 0)
    {
        ML::log_fatal("Binding or Listening failed", TARGET_ALL);
        return -1;
    }

    ML::log_info(std::string("Listening for connections at port ") + std::to_string(port), TARGET_ALL);

    return 1;
}

void Server::startAccept()
{
    int addressSize = sizeof(listeningAddress);
    while (1)
    {

        int clientSocket = accept(serverSocketFD, (struct sockaddr *)&listeningAddress, (socklen_t *)&addressSize);
        fcntl(clientSocket, F_SETFL, O_NONBLOCK);

        if (clientSocket > 0)
        {
            threadPool->addConnectionThread(clientSocket);
        }
    }
}

void *chatRoutine(void *connectionThreadPool)
{
    ConnectionThreadPool *connThPool = (ConnectionThreadPool *)connectionThreadPool;

    while (1)
    {
        if(connThPool->incomingMessagges.size() > 0){
            for (int _index = 0; _index < connThPool->sockets.size(); _index++)
            {
                connThPool->broadcastMessages.push_back(connThPool->incomingMessagges.front());
            }
            connThPool->incomingMessagges.erase(connThPool->incomingMessagges.begin());
        }
    }
}
