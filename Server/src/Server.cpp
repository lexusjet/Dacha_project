#include <Server/Server.h>
#include <Logger.h>

#include <iostream>

using namespace dachaServer;

Server::Server(
    int port,
    int backlog,
    const ServerStateChangedCallback& onServerStateChangedCallback,
    const ErrorCallBack& onErorrCallback,
    const DataReciveCallBack& onDataRecivedCallback
    ) 
    : 
        listeningSocket(std::move(socket_wrapper::Socket(AF_INET, SOCK_STREAM, 0))),
        onServerStateChangedCallback(onServerStateChangedCallback),
        onErrorCallback(onErorrCallback),
        onDataRecivedCallback(onDataRecivedCallback),
        isStop(false),
        m_state(Constructing)

{
    addres.sin_family = AF_INET;
    addres.sin_port = htons(port);
    addres.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(listeningSocket,(struct sockaddr*) &addres, sizeof(addres)) < 0){
        onErorrCallback(std::string("Server bind erorr"));
        LOG("Server Bind error when creating server. Port " << port << " is occupied");
        return;
    }

    if(listen(listeningSocket, backlog) == -1){
        onErorrCallback(std::string("Server listen start eror"));
        LOG("");
        return;
    }

    pollStruct.events = POLLIN;
    pollStruct.fd = static_cast<int>(listeningSocket);
    m_state = Created;
    onServerStateChangedCallback(m_state);
    LOG("Server created on port " << port);
}

Server::~Server(){
    isStop = true;
    if(listenerThread.joinable())
        listenerThread.join();
    std::unique_lock lock(mutexForMap);
    condtion.wait(lock, [&](){return connectionThreadsMap.empty();}); 
    m_state = Destroyed;
    onServerStateChangedCallback(m_state);
    LOG("Server successfully destroyed");
}

void Server::acceptLoop()
{
    LOG("Server start accepting connections on socket #" << static_cast<int>(listeningSocket));
    m_state = Started;
    onServerStateChangedCallback(m_state);
    while(!isStop){
        if(m_state != StartListen){
            m_state = StartListen;
            onServerStateChangedCallback(m_state);
        }
        if(poll(&pollStruct, 1, 1000) == 0) 
            continue;
        SocketDescriptorType newConnection = accept(listeningSocket, nullptr, nullptr);
        //TODO: check connection errors

        std::lock_guard<std::mutex> lock(mutexForMap);
        LOG("Server accept new connection on socket #" << newConnection);
        connectionThreadsMap.emplace(newConnection,
            std::thread(
                &Server::reciveData,
                this,
                newConnection
            )
        );
        
        pollStruct.revents = 0;
    }
    m_state = ListenStopped;
    onServerStateChangedCallback(m_state);
    LOG("Server stop accepting connections");
}

void dachaServer::Server::start()
{
    isStop = false;
    listenerThread = std::thread(&Server::acceptLoop, this);
    LOG("Server started");
}

void dachaServer::Server::stop()
{
    isStop = true;
    m_state = Stopped;
    onServerStateChangedCallback(Stopped);
    LOG("Server stoped");
}


void Server::reciveData(SocketDescriptorType newConnection)
{
    
    socket_wrapper::Socket connection(newConnection);
    MessageHeader message;
    const int timeout = 1000;
    size_t recived = 0;

    int pollResult = 0;
    struct pollfd pfd;
    pfd.events = POLLIN;
    pfd.fd = static_cast<int>(connection);

    LOG("Server start reciving data on socket #" << newConnection);
    while(recived < sizeof(MessageHeader) && !isStop){
        pollResult =  poll(&pfd, 1, timeout);
        if(pollResult != 1) 
            break;
        int bytesRead = recv(connection,
            &message+recived,
            sizeof(MessageHeader) - recived,
            0
        );
        if(bytesRead <= 0){
            pollResult = -1;
            break;
        }
        recived += bytesRead;
    }

    if(!isStop){
        if(pollResult != 1){
            std::string errorMassage;
            if(pollResult == 0){
                errorMassage = "poll timeout";
                LOG("Server poll timeout on socket #" << newConnection);
            }
            else{
                errorMassage = "connection error ";
                LOG("Server Connection error on socket #" << newConnection);
            }
            onErrorCallback(errorMassage);
        }
        else{
            LOG("data successfully read from the socket #" << newConnection);
            onDataRecivedCallback(message);
        }
    }
    
    std::lock_guard lock(mutexForMap);
    connectionThreadsMap[connection].detach();
    connectionThreadsMap.erase(connection);
    condtion.notify_all();
}