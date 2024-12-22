#ifndef PUBLIC_H
#define PUBLIC_H

enum enMsgType
{
    LOGIN_MSG = 1, //登陆信息
    LOGIN_MSG_ACK,
    QUIT_MSG,

    REG_MSG,       //注册信息
    REG_MSG_ACK,

    SINGLE_CHAT_MSG,//chatmsg

    ADD_FRIEND_MSG,

    CREATE_GROUP_MSG,
    JOIN_GROUP_MSG,
    GROUP_CHAT_MSG
};

#endif