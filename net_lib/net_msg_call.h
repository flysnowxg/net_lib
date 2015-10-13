/********************************************************************
	创建时间:	2012-2-23  9:28
	创建者:		薛钢 email:308821698@qq.com
	文件描述:	
*********************************************************************/
#pragma once
#include "auto_lock_cs.h"
#include "session_manager.h"
#include "msg_parse.h"

//xuegang 网络消息调用 不等待返回消息
bool msg_post(session* p_session,uint32_t msg_id,void* p_msg,uint32_t msg_size);

//xuegang 网络消息调用 不等待返回消息
template<typename SendType>
bool msg_post(session* p_session,SendType* p_msg)
{
	if(msg_type<SendType>::model==1)
	{
		return msg_post(p_session,msg_type<SendType>::to_id,p_msg,sizeof(SendType));
	}
	else
	{
		auto_buffer out_buffer=msg_save(p_msg);
		return msg_post(p_session,msg_type<SendType>::to_id,out_buffer.get(),out_buffer.size())
	}
	return false;
}

//xuegang 网络消息调用
auto_buffer msg_call(session* p_session,uint32_t msg_id,void* p_msg,uint32_t msg_size,uint32_t& recv_msg_id);
auto_buffer msg_call(session* p_session,uint32_t msg_id,msg_buffers&,uint32_t& recv_msg_id);

//xuegang 网络消息调用
template<typename SendType>
auto_buffer msg_call(session* p_session,SendType* p_msg,uint32_t& recv_msg_id)
{
	if(msg_type<SendType>::model==1)
	{
		return msg_call(p_session,msg_type<SendType>::to_id,p_msg,sizeof(SendType),recv_msg_id);
	}
	else if(msg_type<SendType>::model==2)
	{
		auto_buffer out_buffer=msg_save(p_msg);
		return msg_call(p_session,msg_type<SendType>::to_id,out_buffer.get(),out_buffer.size(),recv_msg_id);
	}
	else return auto_buffer(0);
}

//xuegang 网络消息调用
template<typename RecvType,typename SendType>
auto_ptr_m<RecvType> msg_call(session* p_session,SendType* p_msg)
{
	if(msg_type<SendType>::model==1)
	{
		uint32_t recv_msg_id=0;
		auto_buffer recved_buffer=msg_call(p_session,msg_type<SendType>::to_id,p_msg,sizeof(SendType),recv_msg_id);
		if(msg_type<RecvType>::to_id!=recv_msg_id) return auto_ptr_m<RecvType>(0);
		return msg_load<RecvType>(recved_buffer);
	}
	else if(msg_type<SendType>::model==2)
	{
		uint32_t recv_msg_id=0;
		auto_buffer out_buffer=msg_save(p_msg);
		auto_buffer recved_buffer=msg_call(p_session,msg_type<SendType>::to_id,out_buffer.get(),out_buffer.size(),recv_msg_id);
		if(msg_type<RecvType>::to_id!=recv_msg_id) return auto_ptr_m<RecvType>(0);
		return msg_load<RecvType>(recved_buffer);
	}
	else return auto_ptr_m<RecvType>(0);
}

//xuegang 网络消息调用
template<typename MsgType>
inline auto_ptr_m<MsgType> msg_callx(session* p_session,MsgType* p_msg)
{
	return msg_call<MsgType>(p_session,p_msg);
}



