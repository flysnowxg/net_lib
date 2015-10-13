#include "stdafx.h"
#include "../../net_lib/net_lib.h"
#include "../server/msg_define/test_msg_define.h"

int main(int argc, char* argv[])
{
	net_lib_initial();
	session session_inst;
	string str_ip("192.168.50.174");
	while(true)
	{
		ip_port addr(str_ip,23456);
		if(session_inst.connect(addr,"admin","123",default_authentication_protocol_id)) break;
		cout<<"·þÎñÆ÷ip:";
		cin>>str_ip;
	}

	time_t last_time=time(NULL);
	msg_fix_string msg_out;
	while(true)
	{
		auto_ptr_m<msg_int> ap_msg_in=msg_call<msg_int>(&session_inst,&msg_out);
		if(!ap_msg_in.get()) return 0;

		if((ap_msg_in->count%10000)==0) 
		{
			time_t current_time=time(NULL);
			printf("after %lld sec  recv_msg.count=%d\n",long long(current_time-last_time),ap_msg_in->count);
			last_time=current_time;
		}
	}
	return 0;
}
