/********************************************************************
	创建时间:	2012-4-27  11:56
	创建者:		薛钢 email:308821698@qq.com
	文件描述:	
*********************************************************************/
#pragma once

//xuegang 认证数据包的格式
struct authentication_package
{
	uint32_t protocol_id;
	char authentication_data[1];
};

//xuegang 登录凭证 基于用户名密码的凭证
class password_credential
{
public:
	string m_account_id;
	string m_password;
};

//xuegang 权限信息 用户的身份和所属用户组
class authorization
{
public:
	string m_account_id;
	vector<string> m_groups;
};

enum e_authentication_error
{
	e_authentication_success,
	e_authentication_fail,
	e_authentication_invalid_userid_or_pwd,
	e_authentication_protocol_error,
	e_authentication_connect_fail,
};

//xuegang 认证协议基类
class authentication_protocol
{
public:
	authentication_protocol();
	virtual void initial_credential(const password_credential& credential,auto_buffer& out_package)=0;//xuegang 在客户端调用
	virtual void process_package(auto_buffer& recved_package,auto_buffer& out_package)=0;//xuegang 返回true表示要继续发包
	bool need_response(){return m_need_response;}//xuegang 认证是否结束
	authorization get_local_authorization(){return m_local_authorization;}//xuegang 本地权限
	authorization get_remote_authorization(){return m_remote_authorization;}//xuegang 对端权限
	string get_encrypt_key(){return m_encrypt_key;}
	e_authentication_error get_error(){return m_auth_state;}//如果失败，给出失败的原因
	uint32_t get_protocol_id(){return m_protocol_id;}
	bool is_over(){return m_is_over;}
protected:
	uint32_t m_protocol_id;
	string m_encrypt_key;
	authorization m_local_authorization;
	authorization m_remote_authorization;
	bool m_need_response;
	bool m_is_over;
	e_authentication_error m_auth_state;
};

//xuegang 注册和创建协议认证处理对象
class authentication_protocol_factory
{
public:
	authentication_protocol_factory(){}
	template<typename CreatorT>
	bool register_protocol(uint32_t protocol_id,CreatorT& creator)
	{
		m_protocols[protocol_id]=shared_ptr<hodler_base>(new hodler<CreatorT>(creator));
		return true;
	}
	auto_ptr<authentication_protocol> create(uint32_t protocol_id);
private:
	authentication_protocol_factory(authentication_protocol_factory&);
	authentication_protocol_factory& operator=(authentication_protocol_factory&);
	struct hodler_base
	{
		virtual authentication_protocol* create()=0;
	};
	template<typename T>
	struct hodler:public hodler_base
	{
		hodler(T& creator):m_creator(creator){}
		virtual authentication_protocol* create()
		{
			return m_creator();
		}
		T m_creator;
	};
	hash_map<uint32_t,shared_ptr<hodler_base> > m_protocols;
};
authentication_protocol_factory& get_authentication_protocol_factory();

class session;
class authentication_sevice
{
public:
	authentication_sevice();
	void initial();//xuegang 在服务启动的时候需要调用
	bool process(shared_ptr<session> sp_session,auto_buffer package_in,auto_buffer& package_out);//返回true表示要继续发包
	void session_colse_callback(shared_ptr<session> sp_session);
private:
	authentication_sevice(authentication_sevice&);
	struct authentication_context
	{
		shared_ptr<session> sp_session;
		shared_ptr<authentication_protocol> sp_protocol;
	};
	CRITICAL_SECTION m_cs;
	map<session*,authentication_context > m_datas;
};
authentication_sevice& get_authentication_sevice();


//////////////////////////////////////////////////////////////////////////
// xuegang 缺省的认证协议只能服务端认证客户端，不能客户端认证服务端
//1 客户端发送一个认证请求
//2 服务端返回一个随机数
//3 客户端返回用用户名 和用password-md5后作为密码加密的随机数
//4 服务端读取密码表 用password-md5后作为密码加密那个随机数 把它和客户端传过来的加密数据比较 ，如果相同 ，表示认证成功 ，
//   返回一个用password-md5后作为密码j加密的会话密钥，和权限信息 ，认证结束
//5 客户端收到加密密钥和权限信息 认证结束

const uint32_t default_authentication_protocol_id=1;
struct check_password_result
{
	authorization authz;
	string encrypt_key;
	string encrypted_encrtypt_key;
};
typedef check_password_result (*check_password_fun_t)(const string& account_id,uint32_t rand_num,uint32_t encrpyted_rand_num);


class default_authentication_protocol
	:public authentication_protocol
{
public:
	template<typename CheckPasswordFun>
	default_authentication_protocol(CheckPasswordFun checker):m_next_package(1),m_rand_num(0)
	{
		m_ap_checker.reset(new hodler<CheckPasswordFun>(checker));
		m_protocol_id=default_authentication_protocol_id;
	}
	virtual void initial_credential(const password_credential& credential,auto_buffer& out_package);//xuegang 在客户端调用
	virtual void process_package(auto_buffer& recved_package,auto_buffer& out_package);//xuegang 返回true表示要继续发包
private:
	void process_protocol_error_helper(auto_buffer& out_package);
	void package1_process(auto_buffer& authentication_package_in,auto_buffer& out_package);//xuegang 返回true表示要继续发包
	void package2_process(auto_buffer& authentication_package_in,auto_buffer& out_package);
	void package3_process(auto_buffer& authentication_package_in,auto_buffer& out_package);
	void package4_process(auto_buffer& authentication_package_in,auto_buffer& out_package);
	void package5_process(auto_buffer& authentication_package_in,auto_buffer& out_package);
	struct hodler_base
	{
		virtual check_password_result check_password(const string& account_id,uint32_t rand_num,uint32_t encrpyted_rand_num)=0;
	};
	template<typename T>
	struct hodler:public hodler_base
	{
		hodler(T& check_fun):m_check_fun(check_fun){}
		virtual check_password_result check_password(const string& account_id,uint32_t rand_num,uint32_t encrpyted_rand_num)
		{
			return m_check_fun(account_id,rand_num,encrpyted_rand_num);
		}
		T m_check_fun;
	};
private:
	default_authentication_protocol(default_authentication_protocol&);
	default_authentication_protocol& operator=(default_authentication_protocol&);
	int m_next_package;
	auto_ptr<hodler_base> m_ap_checker;
	password_credential m_credential;
	uint32_t m_rand_num;
};

//xuegang 返回认证信息有用户id表示认证成功
static check_password_result invalid_checker(const string& account_id,uint32_t rand_num,uint32_t encrpyted_rand_num)
{
	check_password_result result;
	return result;
}

template<typename Checker>
class dap_creator
{
public:
	dap_creator(Checker checker):m_checker(checker){}
	authentication_protocol* operator()(){return new default_authentication_protocol(m_checker);}
private:
	Checker m_checker;
};
template<typename Checker> 
dap_creator<Checker> make_dap_creator(Checker checker)
{
	return dap_creator<Checker>(checker);
}
