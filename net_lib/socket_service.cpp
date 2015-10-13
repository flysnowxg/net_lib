/********************************************************************
创建时间:	2012-4-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
//#include "stdafx.h"
#include "socket_service.h"

bool socket_lib_initial()
{
	WSADATA wsaData;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData )== 0 ) return true;
	else return false;
}

class peer_io_data
{
public:
	peer_io_data()
	{
		memset(&m_overlapped,0,sizeof(m_overlapped));
	}
	virtual ~peer_io_data(){}
	virtual void proccess(io_service* p_io_service,DWORD size_transfered)=0;//xuegang 调用该函数以后不要再去接触该对象
	virtual void register_request(io_service* p_io_service)=0;//xuegang 调用该函数以后不要再去接触该对象
	OVERLAPPED m_overlapped;
};

class peer_io_data_write:public peer_io_data
{
public:
	static bool create(shared_ptr<socket_rw> sp_socket,msg_head& head,msg_buffers& buffers,io_service* p_io_service)
	{
		peer_io_data_write* p_peer_io=new peer_io_data_write(sp_socket,head,buffers);
		p_io_service->increment_wait_io();
		p_peer_io->register_request(p_io_service);
		return true;
	}
	virtual ~peer_io_data_write(){}
	virtual void proccess(io_service* p_io_service,DWORD size_transfered)
	{
		if(size_transfered!=(sizeof(m_head)+m_head.msg_body_size)) return on_socket_error(p_io_service);
		p_io_service->decrement_wait_io();
		delete this;
	}
	virtual void register_request(io_service* p_io_service)
	{
		DWORD size_transfered=0;
		DWORD flags = 0;
		uint32_t wsa_buf_size=sizeof(m_rw_data)/sizeof(WSABUF);
		if(m_buffers.size()<wsa_buf_size)
		{
			m_rw_data[0].buf=(char*)&m_head;
			m_rw_data[0].len=sizeof(m_head);
			m_head.msg_body_size=0;
			for(uint32_t count_i=0;count_i<=m_buffers.size();++count_i)
			{
				m_rw_data[count_i+1].buf=(char*)m_buffers[count_i].get();
				m_rw_data[count_i+1].len=m_buffers[count_i].size();
				m_head.msg_body_size+=m_rw_data[count_i+1].len;
			}
			::WSASend(m_sp_socket->get_handle(),m_rw_data,m_buffers.size()+1,&size_transfered,flags,&m_overlapped,NULL);
			DWORD error=GetLastError();
			if((error==0)||(error==ERROR_IO_PENDING)) return;
		}
		on_socket_error(p_io_service);
	}

private:
	peer_io_data_write(shared_ptr<socket_rw> sp_socket,msg_head& head,msg_buffers& buffers)
		:m_sp_socket(sp_socket),m_head(head),m_buffers(buffers)
	{
		memset(m_rw_data,0,sizeof(m_rw_data));
	}
	void on_socket_error(io_service* p_io_service)//xuegang 调用该函数以后不要再去接触该对象
	{
		p_io_service->unbind(m_sp_socket.get());
		p_io_service->decrement_wait_io();
		socket_close_notify(m_sp_socket);
		delete this;
	}
	shared_ptr<socket_rw> m_sp_socket;
	WSABUF m_rw_data[msg_buffers::fix_buffer_size+1];
	msg_head m_head;
	msg_buffers m_buffers;
};

class peer_io_data_read:public peer_io_data
{
public:
	static bool create(shared_ptr<socket_rw> sp_socket,io_service* p_io_service)
	{
		peer_io_data_read* p_peer_io=new peer_io_data_read(sp_socket);
		p_io_service->increment_wait_io();
		p_peer_io->register_request(p_io_service);
		return true;
	}
	virtual ~peer_io_data_read(){}
	virtual void proccess(io_service* p_io_service,DWORD size_transfered)
	{
		bool b_need_close_socket=false;
		if(size_transfered==0) b_need_close_socket=true;
		else if(m_msg_head.msg_tag!=0x012345678) b_need_close_socket=true;
		else if((!m_b_recved_head)&&(size_transfered!=sizeof(m_msg_head))) b_need_close_socket=true; 
		if(!b_need_close_socket)
		{
			if(!m_b_recved_head)
			{
				m_b_recved_head=true;
				if(m_msg_head.msg_body_size!=0) 
				{
					//xuegang 收到一个消息头 还需要接收消息体
					m_msg_data.reserve(m_msg_head.msg_body_size);
					m_rw_data.buf=(char*)m_msg_data.get();
					m_rw_data.len=m_msg_head.msg_body_size;
					register_request(p_io_service);
					return;
				}
			}
			else
			{
				m_already_recved_size+=size_transfered;
				if(m_already_recved_size!=m_msg_head.msg_body_size)
				{
					m_rw_data.buf=(char*)m_msg_data.get()+m_already_recved_size;
					m_rw_data.len=m_msg_head.msg_body_size-m_already_recved_size;
					register_request(p_io_service);
					return;
				}
			}
			//xuegang 收到一个完整的消息
			m_msg_data.set_size(m_msg_head.msg_body_size);
			//xuegang 为处理消息准备一些数据
			shared_ptr<socket_rw> sp_socket_rw(m_sp_socket);
			msg_head recved_head=m_msg_head;
			auto_buffer recved_msg(m_msg_data);
			//xuegang 去接收下一个消息
			reset_buffer();
			register_request(p_io_service);
			//xuegang 消息的处理 不要再接触this指针指向的内容
			out_msg_content out_content;
			recved_msg_process(sp_socket_rw,recved_head,recved_msg,out_content);
			if(out_content.msg_id!=0)
			{
				msg_head out_head=recved_head;
				out_head.msg_id=out_content.msg_id;
				peer_io_data_write::create(sp_socket_rw,out_head,out_content.out,p_io_service);
			}
		}
		if(b_need_close_socket) on_socket_error(p_io_service);
	}
	virtual void register_request(io_service* p_io_service)
	{
		DWORD size_transfered=0;
		DWORD flags = 0;
		::WSARecv(m_sp_socket->get_handle(),&m_rw_data,1,&size_transfered,&flags,&m_overlapped,NULL);
		DWORD error=GetLastError();
		if((error==0)||(error==ERROR_IO_PENDING)) return;
		on_socket_error(p_io_service);
	}

private:
	peer_io_data_read(shared_ptr<socket_rw> sp_socket)
		:m_sp_socket(sp_socket)
	{
		reset_buffer();
	}
	void reset_buffer()
	{
		m_already_recved_size=0;
		m_b_recved_head=false;
		memset(&m_msg_head,0,sizeof(m_msg_head));
		m_rw_data.len=sizeof(m_msg_head);
		m_rw_data.buf=(char*)&m_msg_head;
	}
	void on_socket_error(io_service* p_io_service)//xuegang 调用该函数以后不要再去接触该对象
	{
		p_io_service->unbind(m_sp_socket.get());
		p_io_service->decrement_wait_io();
		socket_close_notify(m_sp_socket);
		delete this;
	}
public:
	shared_ptr<socket_rw> m_sp_socket;
	uint32_t m_already_recved_size;
	bool m_b_recved_head;
	msg_head m_msg_head;
	WSABUF m_rw_data;
	auto_buffer m_msg_data;
};

class peer_io_data_lisen:public peer_io_data
{
public:
	static bool create(shared_ptr<socket_lisen> sp_socket,io_service* p_io_service)
	{
		peer_io_data_lisen* p_peer_io=new peer_io_data_lisen(sp_socket);
		p_io_service->increment_wait_io();
		p_peer_io->m_haccept=socket(AF_INET,SOCK_STREAM,0);
		if(p_peer_io->m_haccept<=0) 
		{
			p_peer_io->on_socket_error(p_io_service);
			return false;
		}
		p_peer_io->register_request(p_io_service);
		return true;
	}
	virtual ~peer_io_data_lisen()
	{
		closesocket(m_haccept);
	}
	virtual void proccess(io_service* p_io_service,DWORD size_transfered)
	{
		//创建一个socket_rw对象，并将套接字关联到完成端口
		shared_ptr<socket_rw> sp_socket_rw=socket_rw::create(m_haccept,p_io_service,addr_info_buf);
		printf("accept a connect . from %s:%d  to %s:%d\n",
			sp_socket_rw->get_remote_addr().m_ip.get_string().c_str(),sp_socket_rw->get_remote_addr().m_port,
			sp_socket_rw->get_local_addr().m_ip.get_string().c_str(),sp_socket_rw->get_local_addr().m_port);
		//监听下一个连接
		m_haccept=socket(AF_INET,SOCK_STREAM,0);
		memset(&addr_info_buf,0,sizeof(addr_info_buf));
		register_request(p_io_service);
	}
	virtual void register_request(io_service* p_io_service)
	{
		DWORD transfered=0;
		::AcceptEx(m_sp_socket->get_handle(),m_haccept,&addr_info_buf,0,sizeof(SOCKADDR_IN)+16,
			sizeof(SOCKADDR_IN)+16,&transfered,&m_overlapped);
		DWORD error=GetLastError();
		if((error==0)||(error==ERROR_IO_PENDING)) return;
		on_socket_error(p_io_service);
	}
public:
	peer_io_data_lisen(shared_ptr<socket_lisen> sp_socket)
		:m_sp_socket(sp_socket),m_haccept(NULL)
	{
		memset(&addr_info_buf,0,sizeof(addr_info_buf));
	}
	void on_socket_error(io_service* p_io_service)//xuegang 调用该函数以后不要再去接触该对象
	{
		p_io_service->unbind(m_sp_socket.get());
		p_io_service->decrement_wait_io();
		delete this;
	}
public:
	shared_ptr<socket_lisen> m_sp_socket;
	SOCKET m_haccept;
	uint8_t addr_info_buf[(sizeof(SOCKADDR_IN)+16)*2];
};

socket_base::socket_base(SOCKET hsocket)
:m_hsocket(hsocket)
{
	if(hsocket!=NULL) m_hsocket=hsocket;
	InitializeCriticalSection(&m_cs);
}

void socket_base::close()
{
	auto_lock_cs lock(m_cs);
	if(m_hsocket!=0) closesocket(m_hsocket);
	m_hsocket=0;
}

shared_ptr<socket_rw> socket_rw::create(SOCKET hsocket,io_service* p_io_service,uint8_t* p_addr_buf)
{
	shared_ptr<socket_rw> sp_socket(new socket_rw(hsocket));
	sp_socket->m_local_addr.m_port=ntohs(*(uint32_t*)(p_addr_buf+12));
	sp_socket->m_local_addr.m_ip=ip_address(*(uint32_t*)(p_addr_buf+14),false);
	sp_socket->m_remote_addr.m_port=ntohs(*(uint32_t*)(p_addr_buf+40));
	sp_socket->m_remote_addr.m_ip=ip_address(*(uint32_t*)(p_addr_buf+42),false);
	BOOL nodelay= TRUE; 
	setsockopt(sp_socket->m_hsocket, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay));
	p_io_service->bind(sp_socket);
	//xuegang 发送一个读数据请求
	peer_io_data_read::create(sp_socket,p_io_service);
	return sp_socket;
}

auto_ptr<sync_socket_rw> sync_socket_rw::create(const ip_port& addr)
{
	auto_ptr<sync_socket_rw> ap_socket(new sync_socket_rw);
	SOCKADDR_IN server_addr;
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(addr.m_port);
	server_addr.sin_addr.s_addr=addr.m_ip.get_net_order();
	ap_socket->m_hsocket = socket(AF_INET,SOCK_STREAM,0);
	unsigned long no_wait = 1;
	ioctlsocket(ap_socket->m_hsocket, FIONBIO, &no_wait);
	bool b_link_suc=true;
	if(SOCKET_ERROR==::connect(ap_socket->m_hsocket,(struct sockaddr *)&server_addr,sizeof(server_addr)))
	{
		b_link_suc=false;
		timeval time_out;
		time_out.tv_sec=2;
		time_out.tv_usec= 0;
		fd_set socket_set;
		FD_ZERO(&socket_set);
		FD_SET(ap_socket->m_hsocket, &socket_set);
		if(select(0, NULL, &socket_set, NULL, &time_out) > 0)          
		{
			if(FD_ISSET(ap_socket->m_hsocket,&socket_set)) b_link_suc=true;
		}
	}
	if(b_link_suc)
	{
		no_wait=0;
		ioctlsocket(ap_socket->m_hsocket, FIONBIO, &no_wait);
		BOOL nodelay= TRUE; 
		setsockopt(ap_socket->m_hsocket, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay));
		return ap_socket;
	}
	else return  auto_ptr<sync_socket_rw>();
}

ip_port sync_socket_rw::get_remote_addr()
{
	auto_lock_cs lock(m_cs);
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(addr));
	int32_t addr_size=sizeof(addr);
	getpeername(m_hsocket,(sockaddr*)(&addr),&addr_size);
	ip_port result;
	result.m_ip=ntohl(addr.sin_addr.s_addr);
	result.m_port=ntohs(addr.sin_port);
	return result;
}

ip_port  sync_socket_rw::get_local_addr()
{
	auto_lock_cs lock(m_cs);
	SOCKADDR_IN addr;
	memset(&addr,0,sizeof(addr));
	int32_t addr_size=sizeof(addr);
	getsockname(m_hsocket,(sockaddr*)(&addr),&addr_size);
	ip_port result;
	result.m_ip=ntohl(addr.sin_addr.s_addr);
	result.m_port=ntohs(addr.sin_port);
	return result;
}

bool sync_socket_rw::sync_write(uint8_t* p_buf,uint32_t out_size)
{
	auto_lock_cs lock(m_cs);
	if(get_handle()==NULL) return false;
	uint32_t total_writed_size=0;
	while(total_writed_size<out_size)
	{
		int writed_size=::send(m_hsocket,(char*)(p_buf+total_writed_size),out_size-total_writed_size,0);
		DWORD error=GetLastError();
		if(writed_size<=0) 
		{
			close();
			return false;
		}
		total_writed_size+=writed_size;
	}
	return true;
}

bool sync_socket_rw::sync_write(WSABUF* buf,uint32_t count)
{
	auto_lock_cs lock(m_cs);
	if(get_handle()==NULL) return false;
	DWORD need_write=0;
	for(uint32_t count_i=0;count_i<count;++count_i) 
		need_write+=buf[count_i].len;
	DWORD writed_size=0;
	WSASend(m_hsocket,buf,count,&writed_size,0,NULL,NULL);
	DWORD error=GetLastError();
	if(need_write==writed_size) return true;
	close();
	return false;
}

bool  sync_socket_rw::sync_read(uint8_t* p_buf,uint32_t read_size)
{
	auto_lock_cs lock(m_cs);
	if(get_handle()==NULL) return false;
	auto_buffer read_buffer(read_size);
	uint32_t total_readed_size=0;
	while(total_readed_size<read_size)
	{
		int readed_size=::recv(m_hsocket,(char*)(p_buf+total_readed_size),read_size-total_readed_size,0);
		DWORD error=GetLastError();
		if(readed_size<=0) 
		{
			close();
			return false;
		}
		total_readed_size+=readed_size;
	}
	return true;
}

bool sync_socket_rw::good()
{
	auto_lock_cs lock(m_cs);
	if(get_handle()==NULL) return false;
	uint32_t old_time_out=m_msec;
	set_time_out(1);
	DWORD read_ed=::recv(m_hsocket,NULL,0,0);
	DWORD error=GetLastError();
	if((read_ed==0)||(error==10054))
	{
		close();
		return false;
	}
	set_time_out(old_time_out);
	return true;
}

void sync_socket_rw::set_time_out(uint32_t msec)
{
	auto_lock_cs lock(m_cs);
	m_msec=msec;
	setsockopt(m_hsocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&m_msec, sizeof(m_msec));
	DWORD error=GetLastError();
}

shared_ptr<socket_lisen> socket_lisen::create(uint16_t port,io_service* p_io_service)
{
	shared_ptr<socket_lisen> sp_socket(new socket_lisen);
	sp_socket->m_hsocket=socket(PF_INET,SOCK_STREAM,0);
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = ::htons(port);
	uint32_t open_flag=1;
	if(SOCKET_ERROR==::setsockopt(sp_socket->m_hsocket,SOL_SOCKET,SO_REUSEADDR,(char*)&open_flag,sizeof(open_flag)))  return shared_ptr<socket_lisen>();
	if(SOCKET_ERROR==::bind(sp_socket->m_hsocket, (sockaddr*)&addr, sizeof(addr))) return shared_ptr<socket_lisen>();
	if(SOCKET_ERROR==::listen(sp_socket->m_hsocket, 10)) return shared_ptr<socket_lisen>();
	DWORD error=GetLastError();

	p_io_service->bind(sp_socket);
	//xuegang 发送一个等待连接完成的请求
	peer_io_data_lisen::create(sp_socket,p_io_service);
	return sp_socket;
}

io_service_thread::io_service_thread(shared_ptr<io_service> sp_io_service)
:m_sp_io_service(sp_io_service),
m_hthread(NULL),
m_b_notify_exit(false)
{
}

shared_ptr<io_service_thread> io_service_thread::create(shared_ptr<io_service> sp_service)
{
	shared_ptr<io_service_thread>sp_thread(new io_service_thread(sp_service));
	sp_thread->m_hthread = reinterpret_cast<HANDLE>(CreateThread(NULL,0,thread_proc,sp_thread.get(),0,NULL));
	if(sp_thread->m_hthread!=0)
	{
		sp_service->add_thread(sp_thread);
		return sp_thread;
	}
	return shared_ptr<io_service_thread>();
}

DWORD WINAPI io_service_thread::thread_proc(void* vThis)
{
	io_service_thread* pThis=(io_service_thread*)vThis;
	pThis->run();
	pThis->m_sp_io_service->delete_thread(pThis);
	return 0;
}

void io_service_thread::run()
{
	while(true)
	{
		//xuegang 在关联到此完成端口的所有套节字上等待I/O完成
		DWORD size_transfered=0;
		ULONG_PTR key=0;
		OVERLAPPED *p_overlapped=NULL;
		BOOL b_result=::GetQueuedCompletionStatus(m_sp_io_service->m_hcomplete,&size_transfered, &key, &p_overlapped, 1000);
		DWORD error=GetLastError();
		if(p_overlapped) 
		{
			peer_io_data* p_peer_io_data=(peer_io_data*)(((uint8_t*)p_overlapped)-offsetof(peer_io_data,m_overlapped));
			p_peer_io_data->proccess(m_sp_io_service.get(),size_transfered);
		}
		if(m_b_notify_exit) return;
	} 
}

io_service::io_service()
:m_hcomplete(NULL),
m_wait_complete_event_count(0)
{
	InitializeCriticalSection(&m_cs);
}

io_service::~io_service()
{
	close();
	CloseHandle(m_hcomplete);
}

shared_ptr<io_service> io_service::create(uint32_t thread_num,uint16_t port)
{
	shared_ptr<io_service> ap_ios(new io_service);
	ap_ios->m_hcomplete=CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,NULL,0);
	if(ap_ios->m_hcomplete==INVALID_HANDLE_VALUE) return shared_ptr<io_service>();
	for(uint32_t i=0;i<thread_num;++i)
	{
		shared_ptr<io_service_thread> sp_thread=io_service_thread::create(ap_ios);
		if(!sp_thread.get()) return shared_ptr<io_service>();
	}
	//创建监听套接字
	if(port) socket_lisen::create(port,ap_ios.get());
	return ap_ios;
}

void io_service::close()
{
	auto_lock_cs lock(m_cs);
	for(map<socket_rw*,shared_ptr<socket_rw> >::iterator it=m_rw_sockets.begin();it!=m_rw_sockets.end();++it)
	{
		it->second->close();
	}
	for(map<socket_lisen*,shared_ptr<socket_lisen> >::iterator it=m_lisen_sockets.begin();it!=m_lisen_sockets.end();++it)
	{
		it->second->close();
	}
	lock.unlock();
	while(true)
	{
		//等待所有未决io完成
		if(m_wait_complete_event_count==0) break;
		Sleep(1000);
	}
	lock.lock();
	for(map<io_service_thread*,shared_ptr<io_service_thread> >::iterator it=m_io_threads.begin();it!=m_io_threads.end();++it)
	{
		it->second->notify_exit();
	}
	lock.unlock();
	while(true)
	{
		if(m_io_threads.size()==0) break;
		Sleep(1000);
	}
}

bool io_service::bind(shared_ptr<socket_rw> sp_socket)
{
	auto_lock_cs lock(m_cs);
	m_rw_sockets[sp_socket.get()]=sp_socket;
	CreateIoCompletionPort((HANDLE)sp_socket->get_handle(),m_hcomplete,(ULONG_PTR)sp_socket.get(),0);
	DWORD error=GetLastError();
	return true;
}

void io_service::unbind(socket_rw* p_socket)
{
	auto_lock_cs lock(m_cs);
	p_socket->close();
	m_rw_sockets.erase(p_socket);
}

bool io_service::bind(shared_ptr<socket_lisen> sp_socket)
{
	auto_lock_cs lock(m_cs);
	m_lisen_sockets[sp_socket.get()]=sp_socket;
	CreateIoCompletionPort((HANDLE)sp_socket->get_handle(),m_hcomplete,(ULONG_PTR)sp_socket.get(),0);
	DWORD error=GetLastError();
	return true;
}

void io_service::unbind(socket_lisen* p_socket)
{
	auto_lock_cs lock(m_cs);
	p_socket->close();
	m_lisen_sockets.erase(p_socket);
}

void io_service::add_thread(shared_ptr<io_service_thread> sp_thread)
{
	auto_lock_cs lock(m_cs);
	m_io_threads[sp_thread.get()]=sp_thread;
}

void io_service::delete_thread(io_service_thread* p_thread)
{
	auto_lock_cs lock(m_cs);
	m_io_threads.erase(p_thread);
}
