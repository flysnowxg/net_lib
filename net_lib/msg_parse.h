/********************************************************************
创建时间:	2012-2-23  18:36
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#pragma once
#include "auto_buffer.h"
#include "msg_buffer.h"
const int mid_msg_invalid=0;
//[2012/2/24 author:xuegang email:308821698@qq.com] 
//消息的内存分配 释放 智能指针
template<class T>
class auto_ptr_m
{
public:
	explicit auto_ptr_m(int buf_size=sizeof(T)):m_p(NULL)
	{
		if(buf_size>=sizeof(T)) 
		{
			m_p=(T*)malloc(buf_size);
			new(m_p) T;
		}
	}
	auto_ptr_m(auto_ptr_m<T>& other)
	{
		m_p=other.m_p;
		other.m_p=0;
	}
	auto_ptr_m<T>& operator=(auto_ptr_m<T>& other)
	{
		destroy();
		m_p=other.m_p;
		other.m_p=0;
		return *this;
	}
	~auto_ptr_m()
	{
		destroy();
	}
	T *operator->() const{return m_p;}
	T *get() const{return m_p;}
	bool set(auto_buffer& buffer)
	{
		if(buffer.size()<sizeof(T)) return false;
		destroy();
		m_p=(T*)buffer.release();
		return true;
	}
	T* release()
	{
		T* result=m_p;
		m_p=0;
		return result;
	}
private:
	void destroy()
	{
		if(m_p) 
		{
			m_p->~T();
			free(m_p);
		}
		m_p=0;
	}
	T *m_p;	
};

//缺省的消息序列化方法
template<typename MsgType>
auto_ptr_m<MsgType> msg_load(auto_buffer& buffer)
{
	if(buffer.size()<sizeof(MsgType)) return auto_ptr_m<MsgType>(0);
	auto_ptr_m<MsgType>  ap_msg;
	ap_msg.set(buffer);
	return ap_msg;
}

//xuegang 缺省的消息序列化方法 
template<typename MsgType>
auto_buffer msg_save(MsgType* p_msg)
{
	auto_buffer out_buffer;
	out_buffer.write(p_msg,sizeof(MsgType));
	return out_buffer;
}

//[2012/2/23 author:xuegang email:308821698@qq.com] 
//消息类型到消息id的映射
template<typename msg_type> 
struct msg_type
{
	/*enum {to_id= ?};*/
	/*enum {model= ?};*/
};

//[2012/2/23 author:xuegang email:308821698@qq.com] 
//使用缺省的消息序列化方法 用这个宏
#define RegisterMsgImpl(msg_id_param,msg_type_param,model_param) \
	template<> struct msg_type<msg_type_param>\
{\
	enum {to_id= msg_id_param};\
	enum {model= model_param};\
};

//[2012/4/23 author:xuegang email:308821698@qq.com] 
//使用缺省的消息序列化方法 用这个宏
#define RegisterMsg1(msg_id_param,msg_type_param) \
	RegisterMsgImpl(msg_id_param,msg_type_param,1) 

//xuegang 自定义消息序列化方法 用这个宏
#define RegisterMsg2(msg_id_param,msg_type_param) \
	RegisterMsgImpl(msg_id_param,msg_type_param,2) \
	template<> auto_ptr_m<msg_type_param> msg_load<msg_type_param>(auto_buffer& buffer);\
	template<> auto_buffer msg_save<msg_type_param>(msg_type_param* p_msg);

//xuegang 手动处理收到的消息缓冲区 用这个宏
#define RegisterMsg3(msg_id_param,msg_type_param) \
	RegisterMsgImpl(msg_id_param,msg_type_param,3) 

