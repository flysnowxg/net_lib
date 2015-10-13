/********************************************************************
创建时间:	2012-4-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#include "stdafx.h"
#include "net_lib.h"
#include "service_base.cpp"
#include "auto_buffer.cpp"
#include "msg_buffer.cpp"
#include "socket_service.cpp"
#include "net_msg_call.cpp"
#include "net_msg_callback.cpp"
#include "authentication_protocol.cpp"
#include "session_manager.cpp"
#include "default_msg_define.cpp"
#include "encrypt_alg.cpp"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")

bool net_lib_initial()
{
	locale::global(locale("chs"));
	socket_lib_initial();
	get_authentication_sevice().initial();
	get_authentication_protocol_factory().register_protocol(default_authentication_protocol_id,make_dap_creator(invalid_checker));
	get_session_manager().inital();
	return true;
}