/********************************************************************
创建时间:	2012-4-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#pragma once
#include "msg_parse.h"
#include "net_msg_callback.h"
#include "socket_service.h"
#include "session_manager.h"

//xuegang 当一段代码在执行时有一个进程和线程环境 ， 同理，当一个消息被处理时也有一个会话环境（代码还未完成）

//xuegang 消息的原型大概如下， MsgType为被处理消息的类型
// void net_msg_handler(shared_ptr<session> sp_session,auto_buffer,MsgType* p_msg,out_msg_content& out_content);

typedef void (*net_msg_handler_t)(shared_ptr<session> sp_session,auto_buffer,void* p_msg,out_msg_content& out_content);
typedef void (*sevice_msg_call_t)(shared_ptr<session> sp_session,auto_buffer recved_msg_body,out_msg_content&,net_msg_handler_t);

template<typename MsgType>
void sevice_msg_call(shared_ptr<session> sp_session,auto_buffer recved_msg_body,out_msg_content& out_content,net_msg_handler_t msg_handler)
{
	if(msg_type<MsgType>::model!=3)
	{
		auto_ptr_m<MsgType> ap_msg=msg_load<MsgType>(recved_msg_body);
		if(ap_msg.get()) msg_handler(sp_session,recved_msg_body,ap_msg.get(),out_content);
		else sp_session->get_callee_socket()->close();
	}
	else msg_handler(sp_session,recved_msg_body,NULL,out_content);
}

struct msg_handler_item
{
	msg_handler_item(uint32_t msg_id=mid_msg_invalid,sevice_msg_call_t service_call=NULL,
		net_msg_handler_t msg_handler=NULL,bool b_need_authentication=true);
	uint32_t m_msg_id;
	sevice_msg_call_t m_service_call;
	net_msg_handler_t m_msg_handler;
	bool m_b_need_authentication;
};

//[2012/4/24 author:xuegang email:308821698@qq.com] 
//  消息处理函数的管理
class msg_handler_manager
{
public:
	static msg_handler_item get_handler(uint32_t msg_id);
	static bool register_handler(uint32_t msg_id,sevice_msg_call_t service_call,net_msg_handler_t msg_handler,bool b_need_authentication);
private:
	static hash_map<uint32_t,msg_handler_item> sm_handlers;
};

//xuegang 注册一个消息处理函数
template<typename MsgType>
bool register_msg_handler(void (*p_net_msg_handler)(shared_ptr<session>,auto_buffer,MsgType*,out_msg_content&),bool b_need_authentication=true)
{
	uint32_t msg_id=msg_type<MsgType>::to_id;//如果消息id和消息类型未关联，这一句会报错
	sevice_msg_call_t call_entry=(sevice_msg_call<MsgType>);
	net_msg_handler_t p_handler=(net_msg_handler_t)p_net_msg_handler;
	return msg_handler_manager::register_handler(msg_id,call_entry,p_handler,b_need_authentication);
}










