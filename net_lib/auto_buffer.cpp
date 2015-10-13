/********************************************************************
	创建时间:	2012-4-25  18:00
	创建者:		薛钢 email:308821698@qq.com
	文件描述:	
*********************************************************************/
#include "auto_buffer.h"

auto_buffer::auto_buffer(uint32_t buf_size)
:m_buf_size(buf_size),
m_data_size(0),
m_read_point(0),
m_buf(NULL)
{
	if(buf_size) m_buf=(uint8_t*)malloc(buf_size);
}

auto_buffer::auto_buffer(auto_buffer& other)
:m_buf_size(other.m_buf_size),
m_data_size(other.m_data_size),
m_read_point(0),
m_buf(NULL)
{
	m_buf=other.release();
}

auto_buffer& auto_buffer::operator=(auto_buffer& other)
{
	reset();
	m_buf_size=other.m_buf_size;
	m_data_size=other.m_data_size;
	m_buf=other.release();
	return *this;
}

auto_buffer::~auto_buffer()
{
	free(m_buf);
}

bool auto_buffer::write(void* p_data,uint32_t size)
{
	if((m_buf_size-m_data_size)<size)
	{
		m_buf_size=m_buf_size+max(m_buf_size,size*2);
		m_buf=(uint8_t*)realloc(m_buf,m_buf_size);
	}
	memcpy(&m_buf[m_data_size],p_data,size);
	m_data_size+=size;
	return true;
}

uint8_t* auto_buffer::release()
{
	uint8_t* p_buf=m_buf;
	m_buf=NULL;	
	reset(NULL,0);
	return p_buf;
}

bool auto_buffer::set_size(uint32_t data_size)
{
	if(data_size>m_buf_size) return false;
	m_data_size=data_size;
	return true;
}

void auto_buffer::reserve(uint32_t size)
{
	if(m_buf_size<size)
	{
		m_buf_size=size;
		m_buf=(uint8_t*)realloc(m_buf,size);
	}
}

void auto_buffer::reset(uint8_t* p_data,uint32_t data_size)
{
	free(m_buf);
	m_buf=p_data;
	m_data_size=data_size;
	m_buf_size=data_size;
	m_read_point=0;
}

uint8_t* auto_buffer::alloc_space(uint32_t size)
{
	if((m_buf_size-m_data_size)<size)
	{
		m_buf_size=m_buf_size+max(m_buf_size,size*2);
		m_buf=(uint8_t*)realloc(m_buf,m_buf_size);
	}
	uint8_t* p_write_point=m_buf+m_data_size;
	m_data_size+=size;
	return p_write_point;
}

auto_buffer& operator<<(auto_buffer& out,const string& data)
{
	uint32_t str_size=data.size();
	uint8_t* p_write_point=out.alloc_space(sizeof(uint32_t)+str_size+1);
	*(uint32_t*)p_write_point=str_size;
	p_write_point+=4;
	strcpy_s((char*)p_write_point,str_size+1,data.c_str());
	return out;
}

auto_buffer& operator>>(auto_buffer& in,string& data)
{
	uint8_t* p_read_point=in.m_buf+in.m_read_point;
	in.m_read_point+=4;
	if(in.m_read_point>in.m_data_size)
	{
		return in;//error
	}
	uint32_t str_size=*(uint32_t*)p_read_point;

	p_read_point=in.m_buf+in.m_read_point;
	in.m_read_point+=(str_size+1);
	if(in.m_read_point>in.m_data_size)
	{
		return in;//error
	}
	if(p_read_point[str_size]!=0) 
	{
		return in;//error
	}
	data=(char*)p_read_point;
	return in;

}
