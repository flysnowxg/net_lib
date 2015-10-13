/********************************************************************
	创建时间:	2012-4-23  18:42
	创建者:		薛钢 email:308821698@qq.com
	文件描述:	
*********************************************************************/
//#include "stdafx.h"
#include "net_msg_callback.h"

msg_handler_item::msg_handler_item(uint32_t msg_id,sevice_msg_call_t service_call,
	net_msg_handler_t msg_handler,bool b_need_authentication)
:m_msg_id(msg_id),
m_service_call(service_call),
m_msg_handler(msg_handler),
m_b_need_authentication(b_need_authentication)
{
}

hash_map<uint32_t,msg_handler_item> msg_handler_manager::sm_handlers;

msg_handler_item msg_handler_manager::get_handler(uint32_t msg_id)
{
	hash_map<uint32_t,msg_handler_item>::iterator it=sm_handlers.find(msg_id);
	if(it!=sm_handlers.end()) return it->second;
	return msg_handler_item();
}

bool msg_handler_manager::register_handler(uint32_t msg_id,sevice_msg_call_t service_call,
	net_msg_handler_t msg_handler,bool b_need_authentication)
{
	msg_handler_item check_item=get_handler(msg_id);
	if(check_item.m_service_call!=NULL)
	{
		printf_s("注册消息失败 消息id %d 已经被注册\n",msg_id);
		return false;
	}
	msg_handler_item new_item(msg_id,service_call,msg_handler,b_need_authentication);
	sm_handlers[msg_id]=new_item;
	return true;
}

//xuegang 消息分发的起点 在socket_service中被调用
void recved_msg_process(shared_ptr<socket_rw> sp_socket,msg_head& recved_head,auto_buffer recved_body,out_msg_content& out_content)
{
	msg_handler_item msg_handler=msg_handler_manager::get_handler(recved_head.msg_id);
	if(msg_handler.m_service_call==NULL) 
	{
		return;
	}
	shared_ptr<session> sp_session=get_session_manager().get_session(sp_socket);
	if((!msg_handler.m_b_need_authentication)||sp_session->is_verifyed())
	{
		sp_session->keep_session_longer();
		(*msg_handler.m_service_call)(sp_session,recved_body,out_content,msg_handler.m_msg_handler);
		sp_session->updata_last_send_recv_time();
	}
	else sp_socket->close();
}


