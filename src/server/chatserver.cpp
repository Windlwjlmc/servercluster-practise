#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"


using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop,  //事件循环
              const InetAddress& listenAddr, //ip+端口
              const string& nameArg) //服务器名称（给线程绑定一个名字）
         :_server(loop,listenAddr,nameArg),_loop(loop)
{
    ::signal(SIGINT, reset);

    //给服务器注册用户连接的创建和断开回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    //给服务器注册用户读写事件回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    //设置服务器线程数量 1个IO线程，3个worker线程
    _server.setThreadNum(4);
}



void ChatServer::start()
{
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseUnexpect(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn,
                            Buffer* buffer,
                            Timestamp receiveTime)
{
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);
    auto handler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    handler(conn, js, receiveTime);
}

void ChatServer::reset(int)
{
    ChatService::instance()->reset();
    exit(0);
}