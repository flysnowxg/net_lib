/********************************************************************
	创建时间:	2012-2-24  11:06
	创建者:		薛钢 email:308821698@qq.com
	文件描述:	
*********************************************************************/
#pragma once
#include <windows.h>
//[2012/2/22 author:xuegang email:308821698@qq.com] 
//锁定动作
class auto_lock_cs
{
public:
	auto_lock_cs(CRITICAL_SECTION& cs):m_cs(cs),m_b_locked(false){lock();}
	auto_lock_cs(auto_lock_cs& other):m_cs(other.m_cs),m_b_locked(false){lock();}
	~auto_lock_cs(){unlock();}
	void lock(){EnterCriticalSection(&m_cs);; m_b_locked=true;}
	void unlock()
	{
		if(m_b_locked) LeaveCriticalSection(&m_cs); 
		m_b_locked=false;
	}
private:
	auto_lock_cs operator=(const auto_lock_cs& other);
	CRITICAL_SECTION& m_cs;
	bool m_b_locked;
};
