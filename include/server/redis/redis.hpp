#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

class Redis
{
public:
    Redis();
    ~Redis();

    bool connect();

    //向redis指定通道发布消息
    bool publish(int channel, std::string msg);

    //向redis指定通道subscribe
    bool subscribe(int channel);

    //向redis指定通道unsubscribe
    bool unsubscribe(int channel);

    //独立thread中接受channel消息
    void observer_channel_msg();

    //call back obj
    void init_notify_handler(std::function<void(int, std::string)> func);

private:
    //同步上下文obj 负责publish
    redisContext* publish_context_;

    //同步上下文obj 负责subscribe
    redisContext* subscribe_context_;

    //callback report subscribe msg to service
    std::function<void(int, std::string)> notify_msg_handler_;

};

#endif