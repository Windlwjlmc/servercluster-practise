#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include <iostream>
#include <functional>
#include <mutex>
#include <vector>

#include "redis.hpp"
#include "json.hpp"
#include "public.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

using namespace muduo::net;
using namespace muduo;
using namespace std::placeholders;

using json = nlohmann::json;

using msgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

class ChatService
 {
public:
    //获取单例实体
    static ChatService* instance();

    //处理登陆业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //聊天业务
    void singleChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //添加好友
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //quit
    void quit(const TcpConnectionPtr &conn, json &js, Timestamp time);

    msgHandler getHandler(int msgid);

    void handleRedisSubscribeMsg(int id, std::string msg);

    //意外退出
    void clientCloseUnexpect(const TcpConnectionPtr &conn);

    //resetall
    void reset();

private:
    ChatService();

    //存储对应服务的map
    std::unordered_map<int, msgHandler> msgHandler_;

    std::unordered_map<int, TcpConnectionPtr> userConnMap_;

    //保护线程安全的互斥锁
    std::mutex connMtx_;

    UserModel usermodel_;

    OfflineMessageModel OfflineMessageModel_;

    FriendModel friendModel_;

    GroupModel groupModel_;

    Redis redis_;
 };

#endif