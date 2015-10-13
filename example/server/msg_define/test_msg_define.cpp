/********************************************************************
创建时间:	2012-4-25  17:30
创建者:		薛钢 email:308821698@qq.com
文件描述:	
*********************************************************************/
#include "stdafx.h"
#include "test_msg_define.h"

template<>
auto_ptr_m<msg_var_string> msg_load<msg_var_string>(auto_buffer& buffer)
{
	auto_ptr_m<msg_var_string>  ap_msg;
	buffer>>ap_msg->data;
	return ap_msg;
}

template<>
auto_buffer msg_save<msg_var_string>(msg_var_string* p_msg)
{
	auto_buffer out_buffer;
	out_buffer<<p_msg->data;
	return out_buffer;
}
