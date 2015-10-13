/********************************************************************
创建时间:	2012-4-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#pragma once
#include "msg_parse.h"

#define  ReserveMsg(msg_id)\
	const uint32_t msg_netlib_reserve####msg_id##_id=msg_id;\
struct msg_netlib_reserve##msg_id\
{\
};\
RegisterMsg3(msg_netlib_reserve####msg_id##_id,msg_netlib_reserve##msg_id);

ReserveMsg(3);
ReserveMsg(4);
ReserveMsg(5);
ReserveMsg(6);
ReserveMsg(7);
ReserveMsg(8);
ReserveMsg(9);
ReserveMsg(10);

const uint32_t msg_authentication_id=1;
const uint32_t msg_session_keep_alive_id=2;
struct msg_authentication
{
};
struct msg_session_keep_alive
{
};
RegisterMsg3(msg_authentication_id,msg_authentication);
RegisterMsg3(msg_session_keep_alive_id,msg_session_keep_alive);




