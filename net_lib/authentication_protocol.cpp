/********************************************************************
创建时间:	2012-4-27  11:56
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#include "authentication_protocol.h"
#include "session_manager.h"
#include "default_msg_define.h"
#include "encrypt_alg.h"

authentication_protocol::authentication_protocol()
:m_protocol_id(0),
m_need_response(true),
m_is_over(false),
m_auth_state(e_authentication_fail)
{
}

authentication_protocol_factory& get_authentication_protocol_factory()
{
	static authentication_protocol_factory factory;
	return factory;
}

auto_ptr<authentication_protocol> authentication_protocol_factory::create(uint32_t protocol_id)
{
	hash_map<uint32_t,shared_ptr<hodler_base> >::iterator it=m_protocols.find(protocol_id);
	if(it==m_protocols.end()) return auto_ptr<authentication_protocol>();
	return auto_ptr<authentication_protocol>(it->second->create());
}

authentication_sevice& get_authentication_sevice()
{
	static authentication_sevice sevice;
	return sevice;
}

authentication_sevice::authentication_sevice()
{
	InitializeCriticalSection(&m_cs);
}

bool authentication_sevice::process(shared_ptr<session> sp_session,auto_buffer package_in,auto_buffer& package_out)
{
	auto_lock_cs lock(m_cs);
	uint32_t protocol_id=0;
	package_in>>protocol_id;
	if(package_in.good()) 
	{
		authentication_context& context=m_datas[sp_session.get()];
		if(!context.sp_session.get())
		{
			context.sp_protocol.reset(get_authentication_protocol_factory().create(protocol_id).release());
			if(context.sp_protocol.get()) context.sp_session=sp_session;
		}
		if(context.sp_session.get())
		{
			package_out<<protocol_id;
			context.sp_protocol->process_package(package_in,package_out);
			if(context.sp_protocol->get_error()==e_authentication_success)
			{
				sp_session->set_local_authorization_info(context.sp_protocol->get_local_authorization());
				sp_session->set_remote_authorization_info(context.sp_protocol->get_remote_authorization());
				login_notify_callback_t notfiy_fun=get_session_manager().get_login_callback_fun();
				if(notfiy_fun) notfiy_fun(sp_session,true);
			}
			if(context.sp_protocol->need_response()) return true;
			else return false;
		}
	}
 	m_datas.erase(sp_session.get());
 	sp_session->get_callee_socket()->close();
	return false;
}

void authentication_sevice::session_colse_callback(shared_ptr<session> sp_session)
{
	auto_lock_cs lock(m_cs);
	m_datas.erase(sp_session.get());
}

void msg_authentication_handler(shared_ptr<session> sp_session,auto_buffer recved_buffer,msg_authentication*,out_msg_content& out_content)
{
	authentication_sevice& service=get_authentication_sevice();
	auto_buffer package_out;
	bool b_need_response=service.process(sp_session,recved_buffer,package_out);
	if(b_need_response)
	{
		out_content.msg_id=msg_authentication_id;
		out_content.out.push_back(package_out);
	}
}

void authentication_sevice::initial()
{
	register_msg_handler(msg_authentication_handler,false);
}


//////////////////////////////////////////////////////////////////////////
//xuegang 缺省认证协议的协议包
struct dap_package1
{
	int step;
};

struct dap_package2
{
	int step;
	int rand_num;
};

struct dap_package3
{
	int step;
	string account_id;
	int encrypted_rand_num;
};

struct dap_package4
{
	int step;
	string encrpyt_key;//用password-md5进行了加密
	authorization client_authorization_info;
};

struct dap_package5
{
	int step;//5 认证失败
	int reason;//失败原因
};

//{{0,1},{0,2},{1,3},{2,4},{3,5}}

//xuegang 返回一个编码的进行下一步认证的数据包
void default_authentication_protocol::initial_credential(const password_credential& credential,auto_buffer& out_package)
{
	if(m_next_package==1)
	{
		m_credential=credential;
		out_package<<uint32_t(1);
		m_next_package=2;
	}
	else
	{
		process_protocol_error_helper(out_package);
	}
}

void default_authentication_protocol::process_package(auto_buffer& recved_package,auto_buffer& out_package)
{
	int step=0;
	recved_package>>step;
	if(m_next_package==step)
	{
		switch(step)
		{
		case 1:
			package1_process(recved_package,out_package);
			break;
		case 2:
			package2_process(recved_package,out_package);
			break;
		case 3:
			package3_process(recved_package,out_package);
			break;
		case 4:
			package4_process(recved_package,out_package);
			break;
		case 5:
			package5_process(recved_package,out_package);
			break;
		}
	}
	else
	{
		process_protocol_error_helper(out_package);
	}
}

void default_authentication_protocol::process_protocol_error_helper(auto_buffer& out_package)
{
	m_is_over=true;
	m_next_package=5;
	m_auth_state=e_authentication_protocol_error;
	out_package<<uint32_t(5)<<uint32_t(e_authentication_protocol_error);
	m_need_response=true;
}

void default_authentication_protocol::package1_process(auto_buffer& recv_buffer,auto_buffer& out_package)
{
	if(!recv_buffer.good())
	{
		process_protocol_error_helper(out_package);
		return;
	}
	m_next_package=3;
	m_rand_num=rand();
	out_package<< uint32_t(2)<<m_rand_num;
	m_need_response=true;
}

void default_authentication_protocol::package2_process(auto_buffer& recv_buffer,auto_buffer& out_package)
{
	recv_buffer>>m_rand_num;
	if(!recv_buffer.good())
	{
		process_protocol_error_helper(out_package);
		return;
	}
	m_next_package=4;
	string pwd_hash=calulate_md5(m_credential.m_password);
	uint32_t rand_num=m_rand_num;
	encrypt_data((uint8_t*)&rand_num,sizeof(rand_num),pwd_hash);
	out_package<< uint32_t(3)<<m_credential.m_account_id<<rand_num;
	m_need_response=true;
}

void default_authentication_protocol::package3_process(auto_buffer& recv_buffer,auto_buffer& out_package)
{
	string account_id;
	int encrypt_rand_num;
	recv_buffer>>account_id>>encrypt_rand_num;
	if(!recv_buffer.good())
	{
		process_protocol_error_helper(out_package);
		return;
	}
	m_next_package=5;
	check_password_result result=m_ap_checker->check_password(account_id,m_rand_num,encrypt_rand_num);
	m_remote_authorization=result.authz;
	m_encrypt_key=result.encrypt_key;
	m_auth_state=(m_remote_authorization.m_account_id.size()>0)?e_authentication_success:e_authentication_invalid_userid_or_pwd;

	out_package<< uint32_t(4)<<result.encrypted_encrtypt_key<<m_remote_authorization.m_account_id<<m_remote_authorization.m_groups;
	m_need_response=true;
	m_is_over=true;
}

void default_authentication_protocol::package4_process(auto_buffer& recv_buffer,auto_buffer& out_package)
{
	recv_buffer>>m_encrypt_key>>m_local_authorization.m_account_id>>m_local_authorization.m_groups;
	if(!recv_buffer.good()) 
	{
		process_protocol_error_helper(out_package);
		return;
	}
	m_next_package=5;

	string pwd_hash=calulate_md5(m_credential.m_password);
	char sz_encrypted_encrypt_key[256];
	sprintf_s(sz_encrypted_encrypt_key,"%s",m_encrypt_key.c_str());
	encrypt_data((uint8_t*)sz_encrypted_encrypt_key,strlen(sz_encrypted_encrypt_key),pwd_hash);
	m_encrypt_key=sz_encrypted_encrypt_key;

	m_auth_state=(m_local_authorization.m_account_id.size()>0)?e_authentication_success:e_authentication_invalid_userid_or_pwd;
	m_need_response=false;
	m_is_over=true;
}

void default_authentication_protocol::package5_process(auto_buffer& recv_buffer,auto_buffer& out_package)
{
	uint32_t fail_reason=0;
	recv_buffer>>fail_reason;
	m_auth_state=e_authentication_error(fail_reason);
	process_protocol_error_helper(out_package);
}
