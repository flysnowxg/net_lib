#include "stdafx.h"
#include "../../net_lib/net_lib.h"
#include "msg_define/test_msg_define.h"

void debug_print(const char* format,...)
{
	return;
	va_list arg_list;
	va_start(arg_list,format);
	vprintf_s(format,arg_list);
	va_end(arg_list);
}

check_password_result test_pwd_check(const string& user_id,uint32_t rand_num,uint32_t encrpyted_rand_num)
{
	check_password_result result;
	if(user_id=="admin")
	{
		string password="123";
		string pwd_hash=calulate_md5(password);
		decrypt_data((uint8_t*)&rand_num,sizeof(rand_num),pwd_hash);
		if(rand_num==encrpyted_rand_num)
		{
			result.authz.m_account_id=user_id;
			result.authz.m_groups.push_back(string("administrator"));

			uint32_t session_key=rand();
			char sz_session_key[64];
			memset(sz_session_key,0,sizeof(sz_session_key));
			sprintf_s(sz_session_key,"%d",session_key);
			result.encrypt_key=sz_session_key;
			encrypt_data((uint8_t*)sz_session_key,strlen(sz_session_key),pwd_hash);
			result.encrypted_encrtypt_key=sz_session_key;
		}
	}
	return result;
}

void msg_statistics()
{
	static long s_count=0;
	InterlockedIncrement(&s_count);
	static time_t last_time=time(NULL);
	if(s_count%100000==0) 
	{
		time_t current_time=time(NULL);
		printf("last_time=%#llX,after %lld sec  s_count=%d\n",long long(last_time),long long(current_time-last_time),s_count);
		last_time=current_time;
	}
}

void msg_int_handler(shared_ptr<session> sp_session,auto_buffer,msg_int* p_msg_recved,out_msg_content& out_content)
{
	shared_ptr<socket_rw> sp_socket=sp_session->get_callee_socket();
	debug_print("msg_int_handler %s\n",sp_socket->get_remote_addr().m_ip.get_string().c_str());
	out_content.msg_id=msg_int_id;
	out_content.out.push_back(msg_save(p_msg_recved));
	msg_statistics();
}

void msg_fix_string_handler(shared_ptr<session> sp_session,auto_buffer,msg_fix_string* p_msg_recved,out_msg_content& out_content)
{
	shared_ptr<socket_rw> sp_socket=sp_session->get_callee_socket();
	debug_print("msg_fix_string_handler %s\n",sp_socket->get_remote_addr().m_ip.get_string().c_str());
	out_content.msg_id=msg_int_id;
	out_content.out.push_back(msg_save(p_msg_recved));
	msg_statistics();
}

void msg_var_string_handler(shared_ptr<session> sp_session,auto_buffer,msg_var_string* p_msg_recved,out_msg_content& out_content)
{
	debug_print("msg_var_string_handler\n");
	out_content.msg_id=msg_var_string_id;
	msg_var_string msg;
	msg.data="msg_var_string_handler";
	out_content.out.push_back(msg_save(&msg));
	msg_statistics();
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	net_lib_initial();
	get_authentication_protocol_factory().register_protocol(default_authentication_protocol_id,make_dap_creator(test_pwd_check));
	register_msg_handler(msg_fix_string_handler,false);
	register_msg_handler(msg_int_handler,false);
	register_msg_handler(msg_var_string_handler,false);

	shared_ptr<io_service> sp=io_service::create(5,23456);//监听端口23456，并创建5个处理线程
	socket_lisen::create(23457,sp.get());//监听端口23457
	while(true)
	{
		if(getchar()=='e')
		{
			sp->close();
			//重启服务
			socket_lisen::create(23456,sp.get());
			socket_lisen::create(23457,sp.get());
			io_service_thread::create(sp);
			io_service_thread::create(sp);
		}
	}
	return 0;
}


