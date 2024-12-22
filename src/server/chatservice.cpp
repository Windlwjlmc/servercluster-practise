#include "chatservice.hpp"

ChatService::ChatService()
{
    msgHandler_.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    msgHandler_.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    msgHandler_.insert({SINGLE_CHAT_MSG, std::bind(&ChatService::singleChat, this, _1, _2, _3)});
    msgHandler_.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    msgHandler_.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    msgHandler_.insert({JOIN_GROUP_MSG, std::bind(&ChatService::joinGroup, this, _1, _2, _3)});
    msgHandler_.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    msgHandler_.insert({QUIT_MSG, std::bind(&ChatService::quit, this, _1, _2, _3)});

    if(redis_.connect())
    {
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMsg, this, _1, _2));
    }
}

ChatService* ChatService::instance()
{
    static ChatService chatService;
    return &chatService;
}

msgHandler ChatService::getHandler(int msgid)
{
    auto it = msgHandler_.find(msgid);
    if(it == msgHandler_.end())
    {   
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp){
            LOG_ERROR<<"msgid: "<<msgid <<"can't find handler!";
        };
    }
    else
    {
        return msgHandler_[msgid];
    }
}

//处理登陆业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO<<"do login service!";
    int id = js["id"].get<int>();
    std::string password = js["password"];
    User user = usermodel_.query(id);
    if(id == user.getId() && password == user.getPassword())
    {
        if(user.getState() == "online")
        {
            json js;
            js["msgid"] = LOGIN_MSG_ACK;
            js["errno"] = 2;
            js["errmsg"] = "CAN'T LOGIN REPEATEDLY!";
            conn->send(js.dump());
            return;
        }
        else
        {
            {
                std::lock_guard<std::mutex> lock(connMtx_);
                userConnMap_.insert({id, conn});
            }

            //subscribe channel named by id；
            redis_.subscribe(id);

            user.setState("online");
            usermodel_.updateState(user);
            json js;
            js["msgid"] = LOGIN_MSG_ACK;
            js["errno"] = 0;
            js["id"] = user.getId();
            js["name"] = user.getName();
            
            //查询是否有离线消息
            std::vector<std::string> offlinemessage = OfflineMessageModel_.queryOfflineMessage(id);
            if(!offlinemessage.empty())
            {
                js["offlinemsg"] = offlinemessage;
                OfflineMessageModel_.removeOfflineMessage(id);
            }

            //friend info
            std::vector<User> uservec = friendModel_.query(id);
            std::vector<std::string> friends;
            if(!uservec.empty())
            {       
                for(int i = 0; i < uservec.size(); ++i)
                {
                    json friendjs;
                    friendjs["id"] = uservec[i].getId();
                    friendjs["name"] = uservec[i].getName();
                    friendjs["state"] = uservec[i].getState();
                    friends.push_back(friendjs.dump());
                }
                js["friend"] = friends;
            }

            std::vector<Group> groupvec = groupModel_.queryGroups(id);
            std::vector<std::string> groups;
            if(!groupvec.empty())
            {
                for(int i = 0; i < groupvec.size(); ++i)
                {
                    json groupjs;
                    groupjs["id"] = groupvec[i].getId();
                    groupjs["groupname"] = groupvec[i].getName();
                    groupjs["groupdesc"] = groupvec[i].getDesc();
                    std::vector<std::string> groupusers;
                    //后续可添加功能，人数太多就太长了 算了
                    for(int j = 0; j < groupvec[i].getUsers().size(); ++j)
                    {
                        json groupuserjs;
                        groupuserjs["id"] = groupvec[i].getUsers()[j].getId();
                        groupuserjs["name"] = groupvec[i].getUsers()[j].getName();
                        groupuserjs["state"] = groupvec[i].getUsers()[j].getState();
                        groupuserjs["role"] = groupvec[i].getUsers()[j].getRole();
                        groupusers.push_back(groupuserjs.dump());
                    }
                    groupjs["users"] = groupusers;
                    groups.push_back(groupjs.dump());
                }
                js["groups"] = groups;
            }

            conn->send(js.dump());
        }
    }

    else
    {
        json js;
        js["msgid"] = LOGIN_MSG_ACK;
        js["errno"] = 1;
        js["errmsg"] = "NAME OR PASSWORD MISTAKE, PLEASE ENTER AGAIN!";
        conn->send(js.dump());
    }
}

//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO<<"do reg service!";
    std::string name = js["name"];
    std::string password = js["password"];
    User user;
    user.setName(name);
    user.setPassword(password);
    if(usermodel_.insert(user))
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::singleChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int destiId = js["to"].get<int>();
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        auto it = userConnMap_.find(destiId);
        if(it != userConnMap_.end())
        {
            it->second->send(js.dump());
            return;
        }
    }
    //offlineMsg
    User user = usermodel_.query(destiId);
    if(user.getState()=="online")
    {
        redis_.publish(destiId, js.dump());
        return;
    }

    OfflineMessageModel_.insertOfflineMessage(destiId, js.dump());
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    friendModel_.insert(id, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if(groupModel_.createGroup(group) != -1)
    {
        groupModel_.joinGroup(id, group.getId(), "creator");
    }
}

void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    int gid = js["groupid"].get<int>();
    groupModel_.joinGroup(id, gid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    int gid = js["groupid"].get<int>();
    std::vector<int> guvec = groupModel_.queryGroupUsers(id, gid);
    std::lock_guard<std::mutex> lock(connMtx_);
    for(int id: guvec)
    {
        auto it = userConnMap_.find(id);
        if(it!=userConnMap_.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            User user = usermodel_.query(id);
            if(user.getState() == "online")
            {
                redis_.publish(id, js.dump());
            }
            else
            {
              OfflineMessageModel_.insertOfflineMessage(id, js.dump());
            }
        }
    }
}

void ChatService::quit(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        auto it = userConnMap_.find(id);
        if(it != userConnMap_.end())
        {
            userConnMap_.erase(it);
        }
    }

    redis_.unsubscribe(id);

    User user(id, "", "", "offline");
    usermodel_.updateState(user);
}

void ChatService::handleRedisSubscribeMsg(int id, std::string msg)
{
    std::lock_guard<std::mutex> lock(connMtx_);

    //刚拿到消息 对方下线了？
    auto it = userConnMap_.find(id);
    if(it!=userConnMap_.end())
    {
        it->second->send(msg);
        return;
    }

    OfflineMessageModel_.insertOfflineMessage(id, msg);
}

void ChatService::clientCloseUnexpect(const TcpConnectionPtr &conn)
{
    User user;
    {
        std::lock_guard<std::mutex> lock(connMtx_);
        for(auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it)
        {
            if(it->second == conn)
            {
                user.setId(it->first); 
                userConnMap_.erase(it);
                break;
            }
        }
    }
    
    redis_.unsubscribe(user.getId());

    if(user.getId() != -1)
    {
        user.setState("offline");
        usermodel_.updateState(user);
    }
}

void ChatService::reset()
{
    usermodel_.resetAll();
}