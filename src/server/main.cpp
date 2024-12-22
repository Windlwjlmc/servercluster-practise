#include "chatserver.hpp"
#include <iostream>


int main(int argc, char **argv)
{
   if(argc < 3)
   {
      std::cerr<<"comman invalid! example: ./ChatServer 127.0.0.1 6000" <<std::endl;
      exit(-1);
   }

   EventLoop loop; //非常像创建一个epoll
   char* ip = argv[1]; 
   uint16_t port = atoi(argv[2]);
   InetAddress addr(ip, port);
   ChatServer server(&loop, addr, "ChatServer");

   server.start();//listenfd  通过epoll_ctl添加到epoll上
   loop.loop(); //epoll_wait,以阻塞方式等待新用户连接，已连接用户的读写事件等

   return 0;
}