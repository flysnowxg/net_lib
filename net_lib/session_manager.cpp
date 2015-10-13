/********************************************************************
创建时间:	2012-4-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#include "session_manager.h"
#include "net_msg_call.h"

static const int g_session_check_time=10;

session::session()
:m_protocol_id(0),
m_login_error(e_authentication_connect_fail)
{
	InitializeCriticalSection(&m_cs);
	m_last_recv_msg_time=time(NULL);
}

session::~session()
{
	clear_all_per_session_data();
}

void  session::attach_socket(shared_ptr<socket_rw> sp_socket)
{
	auto_lock_cs lock(m_cs);
	m_sp_callee_socket=sp_socket;
}

bool session::connect(ip_port addr,const string& account_id,const string& password,uint32_t protocol_id)
{
	auto_lock_cs lock(m_cs);
	m_remote_lisen_addr=addr;
	m_redential.m_account_id=account_id;
	m_redential.m_password=password;
	m_protocol_id=protocol_id;
	return reconnect();
}

bool session::reconnect()
{
	auto_lock_cs lock(m_cs);
	disconnect();
	if(m_redential.m_account_id.empty()) 
	{
		m_login_error=e_authentication_invalid_userid_or_pwd;
		return false;
	}
	auto_ptr<sync_socket_rw> ap(sync_socket_rw::create(m_remote_lisen_addr));
	if(!ap.get()) 
	{
		m_login_error=e_authentication_connect_fail;
		return false;
	}
	m_sp_caller_socket.reset(ap.release());

	auto_ptr<authentication_protocol> ap_protocol=get_authentication_protocol_factory().create(m_protocol_id);
	if(ap_protocol.get()==0) return false;

	auto_buffer package_out;
	package_out<<ap_protocol->get_protocol_id();
	ap_protocol->initial_credential(m_redential,package_out);
	while(!ap_protocol->is_over()) 
	{
		uint32_t recved_id=0;
		auto_buffer recv_buffer=msg_call(this,msg_authentication_id,package_out.get(),package_out.size(),recved_id);
		if(recved_id!=msg_authentication_id) break;
		package_out.reset();
		uint32_t protocol_id=0;
		recv_buffer>>protocol_id;
		if(protocol_id!=ap_protocol->get_protocol_id()) break;
		package_out<<ap_protocol->get_protocol_id();
		ap_protocol->process_package(recv_buffer,package_out);
	}
	if(ap_protocol.get()) m_login_error=ap_protocol->get_error();
	else m_login_error=e_authentication_fail;
	if(m_login_error==e_authentication_success)
	{
		m_encrpyt_key=ap_protocol->get_encrypt_key();
		m_local_authorization=ap_protocol->get_local_authorization();
		m_remote_authorization=ap_protocol->get_remote_authorization();

		ip_port local_addr=m_sp_caller_socket->get_local_addr();
		ip_port remote_addr=m_sp_caller_socket->get_remote_addr();
		printf("connect success  local %s:%d . remote %s:%d\n",
			local_addr.m_ip.get_string().c_str(),local_addr.m_port,remote_addr.m_ip.get_string().c_str(),remote_addr.m_port);
		return true;
	}
	return false;
}

void session::disconnect(bool b_clear_credential)
{
	auto_lock_cs lock(m_cs);
	m_sp_caller_socket.reset();
	m_encrpyt_key.clear();
	m_local_authorization.m_account_id.clear();
	m_local_authorization.m_groups.clear();
	m_remote_authorization.m_account_id.clear();
	m_remote_authorization.m_groups.clear();
	if(b_clear_credential)
	{
		m_redential.m_account_id.clear();
		m_redential.m_password.clear();
	}
	clear_all_per_session_data();
}

shared_ptr<sync_socket_rw> session::get_caller_socket()
{
	auto_lock_cs lock(m_cs);
	return m_sp_caller_socket;
}

shared_ptr<socket_rw> session::get_callee_socket()
{
	auto_lock_cs lock(m_cs);
	return m_sp_callee_socket;
}

authorization session::get_local_authorization()
{
	auto_lock_cs lock(m_cs);
	return m_local_authorization;
}

authorization session::get_remote_authorization()
{
	auto_lock_cs lock(m_cs);
	return m_remote_authorization;
}

void session::set_local_authorization_info(const authorization& info)
{
	auto_lock_cs lock(m_cs);
	m_local_authorization=info;
}

void session::set_remote_authorization_info(const authorization& info)
{
	auto_lock_cs lock(m_cs);
	m_remote_authorization=info;
}

bool session::is_verifyed()
{
	auto_lock_cs lock(m_cs);
	if(m_remote_authorization.m_account_id.empty()&&m_local_authorization.m_account_id.empty()) return false;
	else return true;
}

uint32_t session::add_per_session_data(void* p_data,per_session_data::destroy_fun_t fun,const string& type_name)
{
	auto_lock_cs lock(m_cs);
	static uint32_t s_countor=1;
	s_countor++;
	m_per_session_datas[s_countor]=per_session_data(p_data,fun,type_name);
	return s_countor;
}

bool session::delete_per_session_data(uint32_t index)
{
	auto_lock_cs lock(m_cs);
	map<uint32_t,per_session_data>::iterator it=m_per_session_datas.find(index);
	if(it==m_per_session_datas.end()) return true;
	it->second.m_destroy(it->second.m_p_data);
	m_per_session_datas.erase(it);
	return true;
}

void session::clear_all_per_session_data()
{
	auto_lock_cs lock(m_cs);
	for(map<uint32_t,per_session_data>::iterator it=m_per_session_datas.begin();it!=m_per_session_datas.end();++it)
	{
		it->second.m_destroy(it->second.m_p_data);
	}
	m_per_session_datas.clear();
}

void session::keep_session_longer()
{
	auto_lock_cs lock(m_cs);
	m_last_recv_msg_time=time(NULL)+20*60;//20分钟
}

void session::updata_last_send_recv_time()
{
	auto_lock_cs lock(m_cs);
	m_last_recv_msg_time=time(NULL);
}

bool session::is_half_session()
{
	auto_lock_cs lock(m_cs);
	time_t current_time=time(NULL);
	if((current_time-m_last_recv_msg_time)>g_session_check_time) return true;
	return false;
}

void session::post_keep_alive_msg()
{
	auto_lock_cs lock(m_cs);
	time_t current_time=time(NULL);
	if((current_time-m_last_recv_msg_time)>(g_session_check_time/2))
	{
		msg_session_keep_alive msg;
		msg_post(this,msg_session_keep_alive_id,&msg,sizeof(msg_session_keep_alive));
	}
}

session_manager::session_manager()
{
	InitializeCriticalSection(&m_cs);
	m_login_notify_fun=NULL;
	m_timer_srv.start_sevice();
	time_event timer(0,-1,5);
	m_timer_srv.add_time_event(timer,half_session_check);
}

shared_ptr<session> session_manager::get_session(shared_ptr<socket_rw> sp_socket)
{
	auto_lock_cs lock(m_cs);
	shared_ptr<session> sp_session=m_sessions[sp_socket.get()];
	if(!sp_session.get())
	{
		sp_session=shared_ptr<session>(new session);
		sp_session->attach_socket(sp_socket);
		m_sessions[sp_socket.get()]=sp_session;
	}
	return sp_session;
}

void session_manager::delete_session_by_socket(shared_ptr<socket_rw> sp_socket)
{
	auto_lock_cs lock(m_cs);
	map<socket_rw*,shared_ptr<session> >::iterator it=m_sessions.find(sp_socket.get());
	if(it==m_sessions.end()) return;
	get_authentication_sevice().session_colse_callback(it->second);
	if(m_login_notify_fun!=NULL) m_login_notify_fun(it->second,false);
	m_sessions.erase(it);
}

void session_manager::close_all_session()
{
	auto_lock_cs lock(m_cs);
	map<socket_rw*,shared_ptr<session> >::iterator it=m_sessions.begin();
	for(;it!=m_sessions.end();++it)
	{
		shared_ptr<socket_rw> sp_socket=it->second->get_callee_socket();
		if(sp_socket.get()) sp_socket->close();
	}
}

void session_manager::clean_half_session()
{
	auto_lock_cs lock(m_cs);
	map<socket_rw*,shared_ptr<session> >::iterator it=m_sessions.begin();
	for(;it!=m_sessions.end();++it)
	{
		if(it->second->is_half_session())
		{
			shared_ptr<socket_rw> sp_socket=it->second->get_callee_socket();
			printf("clean_half_session remote %s\n",sp_socket->get_remote_addr().m_ip.get_string().c_str());
			if(sp_socket.get()) sp_socket->close();
		}
	}
}
time_event_result session_manager::half_session_check(time_event*)
{
	get_session_manager().clean_half_session();
	return time_e_loop;
}

void msg_session_keep_alive_handler(shared_ptr<session> sp_session,auto_buffer recved_buffer,msg_session_keep_alive*,out_msg_content& out_content)
{
}

bool session_manager::inital()
{
	register_msg_handler(msg_session_keep_alive_handler,false);
	return true;
}

session_manager& get_session_manager()
{
	static session_manager s_session_manager;
	return s_session_manager;
}

void socket_close_notify(shared_ptr<socket_rw> sp_socket_rw)
{
	get_session_manager().delete_session_by_socket(sp_socket_rw);
}

bool remote_user_is_in_group(shared_ptr<session> sp_session,const string& group)
{
	authorization authz=sp_session->get_remote_authorization();
	vector<string>::iterator it=find(authz.m_groups.begin(),authz.m_groups.end(),group);
	if(it!=authz.m_groups.end()) return true;
	else return false;
}