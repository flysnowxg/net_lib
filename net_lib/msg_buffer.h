/********************************************************************
	创建时间:	2012-4-24  9:05
	创建者:		薛钢 email:308821698@qq.com
	文件描述:	
*********************************************************************/
#include "auto_buffer.h"
#include <memory>
using namespace std;
#pragma once
typedef void (cdecl *free_fun_t)(void*);
class msg_buffer
{
public:
	//xuegang 请确保p_buf 在 fun 被调用以前能被独占访问 , fun 是类似void(void*)的函数和仿函数
	template<typename FreeFunc>
	msg_buffer(uint8_t* p_buf,uint32_t size,FreeFunc fun)
		:m_p_buf(p_buf),m_size(size)
	{
		m_ap_fun.reset(new free_fun_wrap<FreeFunc>(fun));
	}
	msg_buffer();
	//xuegang 用free释放内存可调用该函数 p_buf的所有权将转移给msg_buffer
	msg_buffer(uint8_t* p_buf,uint32_t size);
	msg_buffer(auto_buffer out_buffer);
	msg_buffer(msg_buffer& other);
	msg_buffer& operator=(msg_buffer& other);
	~msg_buffer();

	uint8_t* get();
	uint32_t size();
	void set_with_no_free(uint8_t* p_buf,uint32_t size);
private:
	void clear();
	class free_fun_base
	{
	public:
		virtual void free(void*)=0;
	};

	template<typename Functor>
	class free_fun_wrap:public free_fun_base
	{
	public:
		free_fun_wrap(){}
		free_fun_wrap(Functor fun)
		{
			m_fun=fun;
		}
		virtual void free(void* p_buf)
		{
			m_fun(p_buf);
		}
		Functor m_fun;
	};
	uint8_t* m_p_buf;
	uint32_t m_size;
	auto_ptr<free_fun_base> m_ap_fun;
};

class msg_buffers
{
public:
	static const int fix_buffer_size=10;
	typedef msg_buffer* iterator;
	msg_buffers();
	iterator begin();
	iterator end();
	msg_buffer& operator[](uint32_t index);
	uint32_t size();
	bool push_back(msg_buffer& buffer);
	bool push_back(auto_buffer& buffer);
private:
	msg_buffer buffers[fix_buffer_size];
	uint32_t m_size;
};
