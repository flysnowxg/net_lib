/********************************************************************
	创建时间:	2012-5-2  12:42
	创建者:		薛钢 email:308821698@qq.com
	文件描述:	
*********************************************************************/
#pragma once
#include <string>
using namespace std;

string calulate_md5(const string&);
 bool encrypt_data(uint8_t* p_data,uint32_t size,const string& password); 
 bool decrypt_data(uint8_t* p_data,uint32_t size,const string& password);
