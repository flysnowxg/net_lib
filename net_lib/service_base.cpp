/********************************************************************
创建时间:	2012-4-23  18:42
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
//#include "stdafx.h"
#include "service_base.h"

time_event::time_event(int  begin_time_sec,int end_time_sec,int per_sec)
:m_begin_time(0),
m_end_time(0),
m_per_sec(per_sec)
{
	time_t current_time=time(NULL);
	m_begin_time=current_time+begin_time_sec;
	if(end_time_sec==-1) m_end_time=0;
	else m_end_time=current_time+end_time_sec;
}

time_event::time_event(time_t begin_time_t,int end_time_sec,int per_sec)
:m_begin_time(begin_time_t),
m_end_time(0),
m_per_sec(per_sec)
{
	time_t current_time=time(NULL);
	if(end_time_sec==-1) m_end_time=0;
	else m_end_time=current_time+end_time_sec;
}

bool time_event::is_over()
{
	if(m_end_time==0) return false;
	return (m_end_time<m_begin_time)?true:false;
}

time_event_mgr::time_event_mgr()
{
	InitializeCriticalSection(&m_cs);
}

bool time_event_mgr::add_event(const time_event& timer,time_event_result(* functor)(time_event*))
{
	shared_ptr<time_event_fun_wrap_base> sp_event_fun(
		new time_event_fun_wrap<unary_function_x<time_event*, time_event_result> >
		(timer,make_unary_function(functor)));
	auto_lock_cs lock(m_cs);
	m_events.insert(make_pair(sp_event_fun->m_timer.get_begin_time(),sp_event_fun));
	return true;
}

void time_event_mgr::fire_event()
{
	time_t current_time=time(NULL);
	while(true)
	{
		auto_lock_cs lock(m_cs);
		multimap<time_t,shared_ptr<time_event_fun_wrap_base> >::iterator it=m_events.begin();
		if((it!=m_events.end()) && current_time>it->first) 
		{
			shared_ptr<time_event_fun_wrap_base> sp_event_fun=it->second;
			m_events.erase(it);
			sp_event_fun->m_timer.plus_begin_time();
			time_event_result result=sp_event_fun->fire();
			if((!sp_event_fun->m_timer.is_over())&&(result==time_e_loop))
			{
				m_events.insert(make_pair(sp_event_fun->m_timer.get_begin_time(),sp_event_fun));
			}
		}
		else break;
	}
}

msg_driven_service::msg_driven_service()
:m_semaphore(NULL),
m_hthread(NULL),
m_b_notify_exit(false),
m_timer(NULL),
m_ap_event_mgr(NULL)
{
	InitializeCriticalSection(&m_cs);
	m_semaphore=CreateSemaphoreA(NULL,0,1000,NULL);
	m_timer=CreateWaitableTimerA(NULL,FALSE,NULL);
	LARGE_INTEGER start_time;
	start_time.QuadPart=-1;
	SetWaitableTimer(m_timer,&start_time,1000,NULL,NULL,TRUE);
	m_ap_event_mgr.reset(new time_event_mgr);
}

msg_driven_service::~msg_driven_service()
{
}

bool msg_driven_service::start_sevice()
{
	auto_lock_cs lock(m_cs);
	if(int(m_hthread)<=0) 
	{
		m_hthread = reinterpret_cast<HANDLE>(CreateThread(NULL,0,thread_proc,this,0,NULL));
		m_b_notify_exit=false;
	}
	return int(m_hthread)>0;
}

bool msg_driven_service::stop_sevice()
{
	auto_lock_cs lock(m_cs);
	if(m_hthread>0) 
	{
		m_b_notify_exit=true;
		if(WaitForSingleObject(m_hthread,INFINITE)==WAIT_OBJECT_0)
		{
			m_hthread=NULL;
			return true;
		}
		return false;
	}
	return true;
}

DWORD WINAPI msg_driven_service::thread_proc(void* vThis)
{
	locale::global(locale("chs"));
	msg_driven_service* This=(msg_driven_service*)vThis;
	This->initial();
	while(true)
	{
		try
		{
			HANDLE wait_handle[2];
			wait_handle[0]=This->m_semaphore;
			wait_handle[1]=This->m_timer;
			DWORD result=WaitForMultipleObjects(2,wait_handle,FALSE,10000);
			if(result==WAIT_OBJECT_0)
			{
				while (true)
				{
					auto_lock_cs lock(This->m_cs);
					if(This->m_msg_list.size()==0) break;
					list<shared_ptr<msg_base> >::iterator it=This->m_msg_list.begin();
					shared_ptr<msg_base> sp_msg(*it);
					This->m_msg_list.pop_front();
					lock.unlock();	
					This->msg_dispatch(sp_msg.get());
				}

			}
			else if(result=(WAIT_OBJECT_0+1))
			{
				This->fire_timer_events();
			}
		}
		catch (std::exception& e)
		{
			printf_s("%s %d %s",__FUNCTION__,__LINE__,e.what());
		}
		if(This->m_b_notify_exit) return 0;
	}
	return 0;
}

void msg_driven_service::fire_timer_events()
{ 
	m_ap_event_mgr->fire_event();
}
