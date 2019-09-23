#include "./Includer.hpp"

#define MAXCONNECTIONS 255
#define PORT 45678

typedef struct sockaddr_in SOCKETADDRIN;

void initialize();
int bindAndListen(int *socketFD, SOCKETADDRIN *address);
void *handleConnection(void *socket);
void addMessageToThreads(std::string message);
std::string getConnectionIPAndPort(int socket);

std::map<int, std::vector<std::string>> threadData = {};

int main()
{
    initialize();

    int threadCounter = 0;
    std::array<pthread_t, MAXCONNECTIONS> threads;

    int serverSocketFD;
    SOCKETADDRIN listeningAddress;
    int clientSockets[MAXCONNECTIONS];
    int addressSize = sizeof(listeningAddress);

    serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);

    if (bindAndListen(&serverSocketFD, &listeningAddress) < 0)
    {
        ML::log_error("Error while binding or listening to the socket...exiting", TARGET_ALL);
    }

    while (1)
    {
        clientSockets[threadCounter] = accept(serverSocketFD, (struct sockaddr *)&listeningAddress, (socklen_t *)&addressSize);
        fcntl(clientSockets[threadCounter], F_SETFL, O_NONBLOCK);

        if (clientSockets[threadCounter] > 0)
        {
            if (pthread_create(&threads[threadCounter], NULL, handleConnection, &clientSockets[threadCounter]) == 0)
            {
                threadData[clientSockets[threadCounter]] = {};
                threadCounter++;
            }
            else
            {
                ML::log_error("Error while accepting connection", TARGET_ALL);
            }
        }
    }

    return 0;
}

void initialize()
{
    ML::initialize("socketChat.log");

    ML::log_info("Socket Chat loggin system initialized", TARGET_ALL);
}

// Bind and start listening to a specific SOCKET with a certain address
// Return 1 on success , -1 otherwise
int bindAndListen(int *socketFD, SOCKETADDRIN *address)
{
    int temp = 1;
    if (*socketFD < 0 || setsockopt(*socketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &temp, sizeof(temp)) < 0)
    {
        ML::log_fatal(std::string("Cannot create new socket with current IP or port !"), TARGET_ALL);
        return -1;
    }

    ML::log_info("Server socket created", TARGET_ALL);

    // Building address (SOCKETADDRIN) for listeninig
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);
    address->sin_family = AF_INET;

    if (bind(*socketFD, (struct sockaddr *)address, sizeof(*address)) < 0 || listen(*socketFD, 8) < 0)
    {
        ML::log_fatal("Binding or Listening failed", TARGET_ALL);
        return -1;
    }

    ML::log_info(std::string("Listening for connections at port ") + std::to_string(PORT), TARGET_ALL);

    return 1;
}

void *handleConnection(void *socket)
{
    char buffer[10];
    int clientSocket = *((int *)socket);

    ML::log_info(std::string("Client connected ") + getConnectionIPAndPort(clientSocket), TARGET_ALL);

    std::string receivedMessage;

    while (1)
    {
        receivedMessage.clear();

        bool isReceiving = recv(clientSocket, &buffer, 1, 0);

        if(isReceiving){
            receivedMessage += buffer[0];
        }
        
        while (isReceiving)
        {
            while (int size = recv(clientSocket, &buffer, 1, 0) > 0)
            {
                if (size == 1)
                {
                    receivedMessage += buffer[0];
                }
                isReceiving = receivedMessage.find("&(end)&") == std::string::npos;
            }
        }

        if (!receivedMessage.empty())
        {
            ML::log_info(receivedMessage, TARGET_ALL);
            addMessageToThreads(receivedMessage);
        }

        while (threadData[clientSocket].size() > 0)
        {
            send(clientSocket, threadData[clientSocket].front().c_str(), strlen(threadData[clientSocket].front().c_str()), 0);
            threadData[clientSocket].erase(threadData[clientSocket].begin());
        }
    }
}

void addMessageToThreads(std::string message)
{
    for (std::map<int, std::vector<std::string>>::iterator it = threadData.begin(); it != threadData.end(); it++)
    {
        it->second.push_back(message);
    }
}

std::string getConnectionIPAndPort(int socket)
{
    socklen_t len;
    struct sockaddr_storage address;
    char IP[INET_ADDRSTRLEN];
    int port;

    len = sizeof address;
    getpeername(socket, (struct sockaddr *)&address, &len);

    struct sockaddr_in *s = (struct sockaddr_in *)&address;
    port = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, IP, sizeof(IP));

    return std::string(std::string(IP) + std::string(" :: ") + std::to_string(s->sin_port));
}