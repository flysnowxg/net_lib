/********************************************************************
创建时间:	2012-4-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#pragma once
#include "mswsock.h"
#include "base_define.h"
#include "auto_lock_cs.h"
#include "auto_buffer.h"
#include "msg_buffer.h"

class ip_address;
class ip_prot;

class peer_io_data;
class peer_io_data_write;
class peer_io_data_read;
class peer_io_data_lisen;

class socket_base;
class socket_rw;
class sync_socket_rw;
class socket_lisen;

class io_service_thread;
class io_service;


// #define Msg_Flag_Signatured 0x0001
// #define Msg_Flag_Encrpyted 0x0002
// unsigned int signature;
// unsigned short mark;//0xF1F2
// unsigned short flags;
struct msg_head
{
	unsigned int msg_tag;//请求消息时为0x012345678
	unsigned int response_id;//当消息处理函数处理一个消息后，返回的消息要把该值copy到返回消息中
	unsigned int msg_id;//消息id,定义在e_msg_id中
	unsigned int msg_body_size;//消息体长度
};

class out_msg_content
{
public:
	out_msg_content():msg_id(0){}
	uint32_t msg_id;//被返回的消息的id
	msg_buffers out;//out用来处理分散集中io，避免不必要的缓冲区复制
};


class ip_address
{
public:
	ip_address(uint32_t ip_addr=0,bool b_host_order=true)
		:m_ip(ip_addr)
	{
		if(!b_host_order) m_ip=ntohl(ip_addr);
	}
	ip_address(const string& str_ip)
	{
		m_ip=inet_addr(str_ip.c_str());
		m_ip=ntohl(m_ip);
	}
	ip_address(const ip_address& other):m_ip(other.m_ip){}
	string get_string()
	{
		char ip_str[32];
		uint8_t* p_byte=(uint8_t*)&m_ip;
		sprintf_s(ip_str,"%d.%d.%d.%d",p_byte[3],p_byte[2],p_byte[1],p_byte[0]);
		return ip_str;
	}
	uint32_t get_host_order()const{return m_ip;}
	uint32_t get_net_order()const{return htonl(m_ip);}
private:
	uint32_t m_ip;//主机字节顺序
};

class ip_port
{
public:
	ip_address m_ip;
	uint16_t m_port;//主机字节顺序
	ip_port(uint32_t ip_addr=0,uint16_t port=0,bool b_host_order=true)
		:m_ip(ip_addr,b_host_order),m_port(port)
	{
		if(!b_host_order) m_port=ntohs(port);
	}
	ip_port(const string& str_ip,uint16_t port,bool b_host_order=true)
		:m_ip(str_ip),m_port(port)
	{
		if(!b_host_order) m_port=ntohs(port);
	}
};

bool socket_lib_initial();

class socket_base
{
public:
	socket_base(SOCKET hsocket=NULL);
	virtual ~socket_base(){close();}
	SOCKET get_handle(){return m_hsocket;}
	void close();
	auto_lock_cs lock(){ return auto_lock_cs(m_cs);}
protected:
	SOCKET m_hsocket;
	CRITICAL_SECTION m_cs;
};

class socket_rw:public socket_base
{
public:
	~socket_rw()
	{
		printf("~socket_rw   local=%s:%d remote=%s:%d \n",
			m_local_addr.m_ip.get_string().c_str(),m_local_addr.m_port,
			m_remote_addr.m_ip.get_string().c_str(),m_remote_addr.m_port);
	}
	static shared_ptr<socket_rw> create(SOCKET hsocket,io_service* p_io_service,uint8_t* p_addr_buf);
	ip_port get_remote_addr(){return m_remote_addr;}
	ip_port get_local_addr(){return m_local_addr;}
private:
	socket_rw(SOCKET hsocket):socket_base(hsocket){}
	ip_port m_remote_addr;
	ip_port m_local_addr;
};

class sync_socket_rw:public socket_base
{
public:
	static auto_ptr<sync_socket_rw> create(const ip_port& addr);
	ip_port get_remote_addr();
	ip_port get_local_addr();
	bool sync_write(uint8_t* p_buf,uint32_t out_size);
	bool sync_write(WSABUF* buf,uint32_t count);
	bool sync_read(uint8_t* p_buf,uint32_t read_size);
	bool good();
	void set_time_out(uint32_t msec);//毫秒
private:
	sync_socket_rw():m_msec(5000){}
	uint32_t m_msec;
};

class socket_lisen:public socket_base
{
public:
	//xuegang 通过调用该函数可以在完成端口上监听多个端口
	static shared_ptr<socket_lisen> create(uint16_t port,io_service* p_io_service);
private:
	socket_lisen(){}
};

class io_service_thread
{
public:
	~io_service_thread(){CloseHandle(m_hthread);}
	static shared_ptr<io_service_thread> create(shared_ptr<io_service> sp_io_service);
	void notify_exit(){m_b_notify_exit=true;}
private:
	io_service_thread(shared_ptr<io_service> sp_service);
	static DWORD WINAPI thread_proc(void* vThis);
	void run();

	shared_ptr<io_service> m_sp_io_service;
	HANDLE m_hthread;
	bool m_b_notify_exit;
};

class io_service
{
public:
	~io_service();
	//xuegang port等于0将不创建监听套接字 thread_num指明用多少个线程来处理请求
	static shared_ptr<io_service> create(uint32_t thread_num=10,uint16_t port=0);
	void close();//xuegang 关闭服务，会等待所有服务线程退出
public:
	//xuegang 以下函数仅仅在socket_base派生类 peer_io_data派生类 io_service_thread 中需要调用
	bool bind(shared_ptr<socket_rw> sp_socket);
	void unbind(socket_rw* p_socket);
	bool bind(shared_ptr<socket_lisen> sp_socket);
	void unbind(socket_lisen* p_socket);
	void increment_wait_io(){InterlockedIncrement((LONG*)&m_wait_complete_event_count);}
	void decrement_wait_io(){InterlockedDecrement((LONG*)&m_wait_complete_event_count);}
	void add_thread(shared_ptr<io_service_thread> sp_thread);
	void delete_thread(io_service_thread* p_thread);
private:
	io_service();

	CRITICAL_SECTION m_cs;
	HANDLE m_hcomplete;
	uint32_t m_wait_complete_event_count;
	map<socket_lisen*,shared_ptr<socket_lisen> > m_lisen_sockets;
	map<socket_rw*,shared_ptr<socket_rw> > m_rw_sockets;
	map<io_service_thread*,shared_ptr<io_service_thread> > m_io_threads;
	friend io_service_thread;
};

void recved_msg_process(shared_ptr<socket_rw> sp_socket,msg_head& recved_head,auto_buffer recved_body,out_msg_content& out_content);
void socket_close_notify(shared_ptr<socket_rw> sp_socket_rw);






