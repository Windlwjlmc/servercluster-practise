#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>

using json = nlohmann::json;


#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

User currentUser_;

std::vector<User> currrentUserFriendList_;

std::vector<Group> currentUserGroupList_;

sem_t rwsem;

std::atomic_bool isLogin = false;

//control the chat page
bool isMainMenuRunning = false;

void readTaskHandler(int clientfd);

std::string getCurrentTime();

void showCurrentUserData();

void mainMenu(int clientfd);

int main(int argc, char **argv)
{
    if(argc<3)
    {
        std::cerr<<"command invalid!"<<std::endl<<"example: ./ChatClient 127.0.0.1 6000"<<std::endl;
    }

    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd)
    {
        std::cerr<<"socket create error"<<std::endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0 ,sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if(-1 == connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)))
    {
        std::cerr<<"connect server error"<<std::endl;
        close(clientfd);
        exit(-1);
    } 

    //init sem of communication of r&&w thread
    sem_init(&rwsem, 0, 0);
    //connect success, start recv thread for recv data;
    //this thread will start only once;

    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    for(;;)
    {
        std::cout<<"\\\\\\\\\\\\\\\\\\\\\\\\\\\\"<<std::endl;
        std::cout<<"1.login"<<std::endl;
        std::cout<<"2.register"<<std::endl;
        std::cout<<"3.quit"<<std::endl;
        std::cout<<"\\\\\\\\\\\\\\\\\\\\\\\\\\\\"<<std::endl;
        std::cout<<"choice";
        int choice = 0;
        std::cin>>choice;
        std::cin.get(); //read enter

        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[50] = {0};
            std::cout<<"userid:";
            std::cin>>id;
            std::cin.get();
            std::cout<<"userpassword:";
            std::cin.getline(pwd, 50);
            
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            std::string request = js.dump();

            isLogin = false;

            int len = ::send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
            if(len == -1)
            {
                std::cerr<<"send login msg error:"<<request<<std::endl;
            }

            //after child thread finish handling response, notice here
            sem_wait(&rwsem);

            if(isLogin)
            {
                //chat menu
                isMainMenuRunning = true;
                mainMenu(clientfd);   
            }     
        }
            
            break;

        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            std::cout<<"username:";
            std::cin.getline(name, 50);
            std::cout<<"userpassword:";
            std::cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            std::string request = js.dump();
            
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if(len == -1)
            {
                std::cerr<<"send reg msg error:"<<request<<std::endl;
            }
    
            char buffer[1024] = {0};
            len = ::recv(clientfd, buffer,1024, 0);
            if(-1 == len)
            {
                std::cerr<<"recv reg response error"<<std::endl;
            }
            sem_wait(&rwsem);
    
            break;
        }

        case 3:
        {
            ::close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        }
            
        
        default:
            std::cerr<<"invalid input!"<<std::endl;
            break;
        }
    }

    return 0;
}

void loginResponse(json &responsejs)
{
    if(0 != responsejs["errno"].get<int>())
    {
        std::cerr<<responsejs["errmsg"]<<std::endl;
        isLogin = false;
        return;
    }
    else
    {
        currentUser_.setId(responsejs["id"].get<int>());
        currentUser_.setName(responsejs["name"]);

        if(responsejs.contains("friend"))
        {
            //init
            currrentUserFriendList_.clear();

            std::vector<std::string> vec = responsejs["friend"];
            for(std::string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                currrentUserFriendList_.push_back(user);
            }
        }

        if(responsejs.contains("groups"))
        {
            //init
            currentUserGroupList_.clear();

            std::vector<std::string> vec1 = responsejs["groups"];
            for(std::string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setID(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                std::vector<std::string> vec2 = grpjs["users"];
                //coding......
                for(std::string &userstr: vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }

                currentUserGroupList_.push_back(group);
            }
        }
        showCurrentUserData();

        if(responsejs.contains("offlinemsg"))
        {
            std::vector<std::string> vec = responsejs["offlinemsg"];
            for(std::string &str : vec)
            {
                json js = json::parse(str);
                int msgtype = js["msgid"].get<int>();
                if(SINGLE_CHAT_MSG == msgtype)
                {
                    std::cout<<js["time"].get<std::string>()<<"["<<js["id"]<<"]"<<js["name"].get<std::string>()
                        <<" said: "<<js["msg"].get<std::string>()<<std::endl;
                }
                else if(GROUP_CHAT_MSG == msgtype)
                {
                    std::cout<<"groupmsg["<<js["groupid"]<<"]:"<<js["time"].get<std::string>()<<"["<<js["id"]
                        <<"]"<<js["name"].get<std::string>()<<" said: "<<js["msg"].get<std::string>()<<std::endl;
                }
            }
        }
    }
    std::cout<<"=================================================="<<std::endl;
    isLogin = true;
}

void regResponse(json &responsejs)
{
    if(0!=responsejs["errno"].get<int>())
    {
        //failed
        std::cerr<<" name is already exist, register error!"<<std::endl;
    }
    else
    {
        std::cout<<" register successfully, userid is " << responsejs["id"]
            <<" , please save it carefully."<<std::endl;
    }
}

//child thread
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024] = {0};
        int len = ::recv(clientfd, buffer, 1024, 0);
        if(-1 == len || 0 ==len)
        {
            close(clientfd);
            exit(-1);
        }
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(SINGLE_CHAT_MSG == msgtype)
        {
            std::cout<<js["time"].get<std::string>()<<"["<<js["id"]<<"]"<<js["name"].get<std::string>()
                <<" said: "<<js["msg"].get<std::string>()<<std::endl;
                continue;
        }
        else if(GROUP_CHAT_MSG == msgtype)
        {
            std::cout<<"groupmsg["<<js["groupid"]<<"]:"<<js["time"].get<std::string>()<<"["<<js["id"]
                <<"]"<<js["name"].get<std::string>()<<" said: "<<js["msg"].get<std::string>()<<std::endl;
                continue;
        }

        if(LOGIN_MSG_ACK == msgtype)
        {
            loginResponse(js);
            sem_post(&rwsem);
            continue;
        }

        if(REG_MSG_ACK == msgtype)
        {
            regResponse(js);
            sem_post(&rwsem);
            continue;
        }
    }
}

void showCurrentUserData()
{
    std::cout<<"====================login user===================="<<std::endl;
    std::cout<<"current login user -> id:"<<currentUser_.getId()<<" name:"<<currentUser_.getName()<<std::endl;
    std::cout<<"====================friendlist===================="<<std::endl;
    if(!currrentUserFriendList_.empty())
    {
        for(User &user : currrentUserFriendList_)
        {
            std::cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<std::endl;
        }
    }
    std::cout<<"====================group list===================="<<std::endl;
    if(!currentUserGroupList_.empty())
    {
        for(Group &group : currentUserGroupList_)
        {
            std::cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<std::endl;
            for(GroupUser &user : group.getUsers())
            {
                std::cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()
                    <<" " <<user.getRole()<<std::endl;
            }
        }
    }
    std::cout<<"=================================================="<<std::endl;
}

std::string getCurrentTime()
{
    auto now = std::chrono::system_clock::now();  // 获取当前时间点
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    char* time_str = std::ctime(&now_c);
    std::string currenttime = time_str;
    currenttime.erase(currenttime.size()-1);
    return currenttime;
}

void help(int fd = 0, std::string str ="");

void chat(int,std::string);

void addfriend(int,std::string);

void creategroup(int,std::string);

void joingroup(int,std::string);

void groupchat(int,std::string);

void quit(int,std::string);

std::unordered_map<std::string,std::string> commandMap =
{
    {"help", "all command that server supported to display"},
    {"chat", "format->chat:friendid:message"},
    {"addfriend", "format->addfriend:friendid"},
    {"creategroup", "format->creategroup:groupname:groupdesc"},
    {"joingroup", "format->joingroup:groupid"},
    {"groupchat", "format->groupchat:groupid:message"},
    {"quit", "format->quit"}
};

std::unordered_map<std::string,std::function<void(int, std::string)>> commandHandlerMap =
{
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"joingroup", joingroup},
    {"groupchat", groupchat},
    {"quit", quit}
};


void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer);
        std::string command;
        int idx = commandbuf.find(":");
        if(-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end())
        {
            std::cerr<<"invalid input command!"<<std::endl;
            continue;
        }

        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

void help(int, std::string)
{
    std::cout<<"show command list >>" <<std::endl;
    for(auto &p : commandMap)
    {
        std::cout<<p.first<<" : "<<p.second<<std::endl;
    }
    std::cout<<std::endl;
}

void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = currentUser_.getId();
    js["friendid"] = friendid;
    std::string buffer = js.dump();

    int len = ::send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        std::cerr<<"send addfriend msg error -> "<<buffer<<std::endl;
    }
}

void chat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if(-1 == idx)
    {
        std::cerr<<"chat command invalid!"<<std::endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = SINGLE_CHAT_MSG;
    js["id"] = currentUser_.getId();
    js["name"] = currentUser_.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len)
    {
        std::cerr << "chat msg error ->" << buffer << std::endl;
    }
}

void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if(-1 == idx)
    {
        std::cerr<<"chat command invalid!"<<std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = currentUser_.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    std::string buffer = js.dump();

    int len = ::send(clientfd,buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        std::cerr <<"send creategroup msg error -> "<< buffer <<std::endl;
    }
}

void joingroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = JOIN_GROUP_MSG;
    js["id"] = currentUser_.getId();
    js["groupid"] = groupid;
    std::string buffer = js.dump();

    int len = ::send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        std::cerr <<"send joingroup msg error -> "<<buffer<<std::endl;
    }
}

void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if(-1 == idx)
    {
        std::cerr << "groupchat command invalid" <<std::endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx+1, str.size()-idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = currentUser_.getId();
    js["name"] = currentUser_.getName(); 
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    std::string buffer = js.dump();

    int len = ::send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        std::cerr << "send groupchat msg error -> "<<buffer<<std::endl;
    }
}

void quit(int clientfd, std::string str)
{
    json js;
    js["msgid"] = QUIT_MSG;
    js["id"] = currentUser_.getId();
    std::string buffer = js.dump();

    int len = ::send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len)
    {
        std::cerr << "send quit msg error -> "<<buffer<<std::endl;
    }
    isMainMenuRunning = false;
}
