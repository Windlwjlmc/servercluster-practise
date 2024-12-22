#ifndef CHATSERVER_H
#define CHATSERVER_H


#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
#include <signal.h>

using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;

class ChatServer
{
public:
   ChatServer(EventLoop* loop,  //事件循环
              const InetAddress& listenAddr, //ip+端口
              const string& nameArg); //服务器名称（给线程绑定一个名字）

   //开启事件循环
   void start();
   
private:
   //专门处理用户的连接创建和断开
   void onConnection(const TcpConnectionPtr& conn);

   static void reset(int);

   //专门处理用户的读写事件
   void onMessage(const TcpConnectionPtr& conn, //连接
                  Buffer* buffer,               //缓冲区
                  Timestamp time);              //接受到数据的时间信息
   

   TcpServer _server; //定义一个server
   EventLoop *_loop;  //可看作epoll事件循环
};

#endif