/********************************************************************
创建时间:	2012-4-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
//#include "stdafx.h"
#include "msg_buffer.h"

msg_buffer::msg_buffer()
:m_p_buf(NULL),
m_size(0)
{
}

//xuegang 用free释放内存可调用该函数
msg_buffer::msg_buffer(uint8_t* p_buf,uint32_t size)
:m_p_buf(p_buf),
m_size(size)
{
	m_ap_fun.reset(new free_fun_wrap<free_fun_t>(::free));
}

msg_buffer::msg_buffer(auto_buffer out_buffer)
{
	m_size=out_buffer.size();
	m_p_buf=out_buffer.release();
	m_ap_fun.reset(new free_fun_wrap<free_fun_t>(::free));
}

msg_buffer::msg_buffer(msg_buffer& other)
:m_p_buf(other.m_p_buf),
m_size(other.m_size),
m_ap_fun(other.m_ap_fun)
{
	other.m_p_buf=NULL;
	other.m_size=0;
}

msg_buffer& msg_buffer::operator=(msg_buffer& other)
{
	clear();
	m_p_buf=other.m_p_buf;
	m_size=other.m_size;
	m_ap_fun=other.m_ap_fun;
	other.m_p_buf=NULL;
	other.m_size=0;
	return *this;
}
msg_buffer::~msg_buffer()
{
	clear();
}
uint8_t* msg_buffer::get()
{
	return m_p_buf;
}
uint32_t msg_buffer::size()
{
	return m_size;
}
void msg_buffer::set_with_no_free(uint8_t* p_buf,uint32_t size)
{
	clear();
	m_p_buf=p_buf;
	m_size=size;
}

void msg_buffer::clear()
{
	if(m_ap_fun.get())
	{
		m_ap_fun->free(m_p_buf);
		m_ap_fun.reset();
	}
	m_p_buf=NULL;
	m_size=0;
}


msg_buffers::msg_buffers()
:m_size(0)
{
}
msg_buffers::iterator msg_buffers::begin()
{
	return &buffers[0];
}
msg_buffers::iterator msg_buffers::end()
{
	return &buffers[m_size];
}
msg_buffer& msg_buffers::operator[](uint32_t index)
{
	return buffers[index];
}
uint32_t msg_buffers::size()
{
	return m_size;
}
bool msg_buffers::push_back(msg_buffer& buffer)
{
	if(m_size>=fix_buffer_size) return false;
	buffers[m_size]=buffer;
	m_size++;
	return true;
}

bool msg_buffers::push_back(auto_buffer& buffer)
{
	if(m_size>=fix_buffer_size) return false;
	buffers[m_size]=msg_buffer(buffer);
	m_size++;
	return true;
}