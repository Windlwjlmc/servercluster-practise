#include "redis.hpp"
#include <iostream>

Redis::Redis()
    :publish_context_(nullptr)
    ,subscribe_context_(nullptr)
{}

Redis::~Redis()
{
    if(publish_context_ != nullptr)
    {
        redisFree(publish_context_);
    }

    if(subscribe_context_ != nullptr)
    {
        redisFree(subscribe_context_);
    }
}

bool Redis::connect()
{
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if(nullptr == publish_context_)
    {
        std::cerr<<"redis connection failed!"<<std::endl;
        return false;
    }

    subscribe_context_ = redisConnect("127.0.0.1", 6379);
    if(nullptr == subscribe_context_)
    {
        std::cerr<<"redis connection failed!"<<std::endl;
        return false;
    }

    std::thread t([&](){
        observer_channel_msg();
    });
    t.detach();

    std::cout<<"connect redis_server successfully!"<<std::endl;

    return true;
}

bool Redis::publish(int channel, std::string msg)
{
    redisReply* reply = (redisReply*)redisCommand(publish_context_, 
        "PUBLISH %d %s", channel, msg.c_str());
    if(nullptr == reply)
    {
        std::cerr<<"publish failed!"<<std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel)
{
    //和publish中方法不同    非阻塞模式
    if(REDIS_ERR == redisAppendCommand(this->subscribe_context_, "SUBSCRIBE %d", channel))
    {
        std::cerr<<"subscribe failed!"<<std::endl;
        return false;
    }

    int done = 0;
    //send command to redis server, wait for reply in blocking mode. 
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->subscribe_context_, &done))
        {
            std::cerr<<"subscribe failed!"<<std::endl;
            return false;
        }
    }

    return true;
}

bool Redis::unsubscribe(int channel)
{
    //和publish中方法不同    非阻塞模式
    if(REDIS_ERR == redisAppendCommand(this->subscribe_context_, "UNSUBSCRIBE %d", channel))
    {
        std::cerr<<"unsubscribe failed!"<<std::endl;
        return false;
    }

    int done = 0;
    //send command to redis server, wait for reply in blocking mode. 
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->subscribe_context_, &done))
        {
            std::cerr<<"unsubscribe failed!"<<std::endl;
            return false;
        }
    }

    return true;
}

void Redis::observer_channel_msg()
{
    redisReply* reply = nullptr;
    while(REDIS_OK == redisGetReply(this->subscribe_context_, (void**)&reply))
    {
        if(reply != nullptr && reply->element[2] != nullptr &&reply->element[2]->str!=nullptr)
        {
            notify_msg_handler_(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr<<"observer_channel_msg quit"<<std::endl;
}

void Redis::init_notify_handler(std::function<void(int, std::string)> func)
{
    this->notify_msg_handler_ = func;
}