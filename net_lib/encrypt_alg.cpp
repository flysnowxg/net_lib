#include "encrypt_alg.h"
#include <wincrypt.h>
#pragma comment(lib,"advapi32.lib")

string calulate_md5(const string& data)
{
	HCRYPTPROV h_crypt_prov;

	if(CryptAcquireContext(&h_crypt_prov,    _T("net_lib_encrypt_context"),MS_ENHANCED_PROV,PROV_RSA_FULL,0)==FALSE)
	{
		if(CryptAcquireContext(&h_crypt_prov, _T("net_lib_encrypt_context"),MS_ENHANCED_PROV,
			PROV_RSA_FULL,CRYPT_NEWKEYSET)==FALSE)  
		{
			DWORD error= GetLastError();
			return "";
		}
	}

	HCRYPTHASH hHash = NULL;
	/// 创建一个hash
	if (!CryptCreateHash(h_crypt_prov, CALG_MD5, 0, 0,&hHash))
	{
		DWORD error= GetLastError();
		CryptDestroyHash(hHash);
		CryptReleaseContext(h_crypt_prov, 0);
		return "";
	}

	if (!CryptHashData(hHash, (BYTE *)data.c_str(), data.size(),0))
	{
		DWORD error= GetLastError();
		CryptDestroyHash(hHash);
		CryptReleaseContext(h_crypt_prov, 0);
		return "";
	}

	// 获取hash值的字节数
	DWORD dwHashLen = 0;
	if (!CryptGetHashParam(hHash, HP_HASHVAL, NULL, &dwHashLen, 0)) 
	{
		DWORD error= GetLastError();
		CryptDestroyHash(hHash);
		CryptReleaseContext(h_crypt_prov, 0);
		return "";
	}
	std::vector<BYTE> vecHash(dwHashLen, 0);
	if (!CryptGetHashParam(hHash, HP_HASHVAL, &vecHash[0], &dwHashLen, 0)) 
	{
		DWORD error= GetLastError();
		CryptDestroyHash(hHash);
		CryptReleaseContext(h_crypt_prov, 0);
		return "";
	}

	// 将hash值转换为十六进制字符串
	char hash_hex[256];
	memset(hash_hex,0,sizeof(hash_hex));
	for(DWORD i=0; i<dwHashLen; i++) 
	{
		char* p_cusor=hash_hex+i*2;
		sprintf_s(p_cusor,4,"%02X",vecHash[i]);
	}

	/// 删除Hash
	CryptDestroyHash(hHash);
	CryptReleaseContext(h_crypt_prov, 0);
	return hash_hex;
}

bool encrypt_data(uint8_t* p_data,uint32_t size,const string& password)
{
	if(size==0) return true;
	for(uint32_t count_i=0;count_i<password.size();count_i++)
	{
		int32_t count_j=count_i%size;
		p_data[count_j]^=password[count_i];
	}
	return true;
}

bool decrypt_data(uint8_t* p_data,uint32_t size,const string& password)
{
	if(size==0) return true;
	for(uint32_t count_i=0;count_i<password.size();count_i++)
	{
		int32_t count_j=count_i%size;
		p_data[count_j]^=password[count_i];
	}
	return true;
}
