/********************************************************************
创建时间:	2012-2-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#include "net_msg_call.h"

static uint32_t new_response_id()
{
	static long response_id_countor=1;
	return InterlockedIncrement(&response_id_countor);
}

static uint32_t msg_send_internal(sync_socket_rw* p_socket,uint32_t msg_id,void* p_msg,uint32_t msg_size)
{
	msg_head send_head;
	send_head.msg_tag=0x12345678;
	send_head.response_id=new_response_id();
	send_head.msg_id=msg_id;
	send_head.msg_body_size=msg_size;
	WSABUF wsa_buf[2];
	wsa_buf[0].buf=(char*)&send_head;
	wsa_buf[0].len=sizeof(msg_head);
	wsa_buf[1].buf=(char*)p_msg;
	wsa_buf[1].len=msg_size;
	if(!p_socket->sync_write(wsa_buf,2)) return 0;
	return send_head.response_id;
}

bool msg_post(session* p_session,uint32_t msg_id,void* p_msg,uint32_t msg_size)
{
	p_session->updata_last_send_recv_time();
	shared_ptr<sync_socket_rw> sp_socket=p_session->get_caller_socket();
	if(!sp_socket.get()) return false;
	auto_lock_cs lock(sp_socket->lock());
	return 0!=msg_send_internal(sp_socket.get(),msg_id,p_msg,msg_size);
}

//xuegang 网络消息调用
auto_buffer msg_call(session* p_session,uint32_t msg_id,void* p_msg,uint32_t msg_size,uint32_t& recv_msg_id)
{
	p_session->updata_last_send_recv_time();
	recv_msg_id=0;
	auto_buffer recved_buffer;
	shared_ptr<sync_socket_rw> sp_socket=p_session->get_caller_socket();
	if(!sp_socket.get()) 
	{
		p_session->reconnect();
		if(!sp_socket.get()) recved_buffer;
	}
	auto_lock_cs lock(sp_socket->lock());

	uint32_t reponse_id=msg_send_internal(sp_socket.get(),msg_id,p_msg,msg_size);
	if(reponse_id==0) return recved_buffer;

	msg_head recved_head;
	memset(&recved_head,0,sizeof(recved_head));
	if(!sp_socket->sync_read((uint8_t*)&recved_head,sizeof(recved_head))) return recved_buffer;
	if(recved_head.msg_tag!=0x12345678) return recved_buffer;
	if(reponse_id!=recved_head.response_id) return recved_buffer;

	recved_buffer.reserve(recved_head.msg_body_size+1);
	if(!sp_socket->sync_read(recved_buffer.get(),recved_head.msg_body_size)) return recved_buffer;
	recved_buffer.set_size(recved_head.msg_body_size);
	recv_msg_id=recved_head.msg_id;
	return recved_buffer;
}

auto_buffer msg_call(session* p_session,uint32_t msg_id,msg_buffers& buffers,uint32_t& recv_msg_id)
{
	p_session->updata_last_send_recv_time();
	auto_buffer recved_buffer;
	shared_ptr<sync_socket_rw> sp_socket=p_session->get_caller_socket();
	if(!sp_socket.get()) return recved_buffer;
	auto_lock_cs lock(sp_socket->lock());

	msg_head send_head;
	send_head.msg_tag=0x12345678;
	send_head.response_id=new_response_id();
	send_head.msg_id=msg_id;
	send_head.msg_body_size=0;
	WSABUF wsa_buf[msg_buffers::fix_buffer_size+1];
	wsa_buf[0].buf=(char*)&send_head;
	wsa_buf[0].len=sizeof(msg_head);
	int count_i=1;
	for(msg_buffers::iterator it=buffers.begin();it!=buffers.end();++it)
	{
		wsa_buf[count_i].buf=(char*)it->get();
		wsa_buf[count_i].len=it->size();
		send_head.msg_body_size+=it->size();
		count_i++;
	}
	sp_socket->sync_write(wsa_buf,count_i);

	msg_head recved_head;
	memset(&recved_head,0,sizeof(recved_head));
	if(!sp_socket->sync_read((uint8_t*)&recved_head,sizeof(recved_head))) return recved_buffer;
	if(recved_head.msg_tag!=0x12345678) return recved_buffer;
	if(send_head.response_id!=recved_head.response_id) return recved_buffer;

	recved_buffer.reserve(recved_head.msg_body_size+1);
	if(!sp_socket->sync_read(recved_buffer.get(),recved_head.msg_body_size)) return recved_buffer;
	recved_buffer.set_size(recved_head.msg_body_size);
	recv_msg_id=recved_head.msg_id;
	return recved_buffer;
}
