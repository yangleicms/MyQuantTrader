

#ifndef BINACPP_UTILS
#define BINACPP_UTILS

#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <string.h>
#include <sys/time.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <iostream>
#include <mbedtls/sha256.h>
#include <mbedtls/md.h>

using namespace std;

void split_string( string &s, char delim, vector <string> &result);
bool replace_string( string& str, const char *from, const char *to);
int replace_string_once( string& str, const char *from, const char *to , int offset);


string b2a_hex( char *byte_arr, int n );
time_t get_current_epoch();
unsigned long get_current_ms_epoch();


//--------------------
inline bool file_exists (const std::string& name) {
 	 return ( access( name.c_str(), F_OK ) != -1 );
}

string hmac_sha256( const char *key, const char *data);
string sha256( const char *data );
void string_toupper( string &src);
string string_toupper( const char *cstr );

namespace pionex_fastAPI {

	static const std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	static unsigned char ToHex(unsigned char x)
	{
		return  x > 9 ? x + 55 : x + 48;
	}

	static std::string UrlEncode(const std::string& str)
	{
		std::string strTemp = "";
		size_t length = str.length();
		for (size_t i = 0; i < length; i++)
		{
			if (isalnum((unsigned char)str[i]) ||
				(str[i] == '-') ||
				(str[i] == '_') ||
				(str[i] == '.') ||
				(str[i] == '~'))
				strTemp += str[i];
			else if (str[i] == ' ')
				strTemp += "+";
			else
			{
				strTemp += '%';
				strTemp += ToHex((unsigned char)str[i] >> 4);
				strTemp += ToHex((unsigned char)str[i] % 16);
			}
		}
		return strTemp;
	}

	static std::string base64_encode(const char* bytes_to_encode, unsigned int in_len) {
		std::string ret;
		int i = 0;
		int j = 0;
		unsigned char char_array_3[3];  // store 3 byte of bytes_to_encode
		unsigned char char_array_4[4];  // store encoded character to 4 bytes

		while (in_len--) {
			char_array_3[i++] = *(bytes_to_encode++);  // get three bytes (24 bits)
			if (i == 3) {
				// eg. we have 3 bytes as ( 0100 1101, 0110 0001, 0110 1110) --> (010011, 010110, 000101, 101110)
				char_array_4[0] = (char_array_3[0] & 0xfc) >> 2; // get first 6 bits of first byte,
				char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4); // get last 2 bits of first byte and first 4 bit of second byte
				char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6); // get last 4 bits of second byte and first 2 bits of third byte
				char_array_4[3] = char_array_3[2] & 0x3f; // get last 6 bits of third byte

				for (i = 0; (i < 4); i++)
					ret += base64_chars[char_array_4[i]];
				i = 0;
			}
		}

		if (i)
		{
			for (j = i; j < 3; j++)
				char_array_3[j] = '\0';

			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

			for (j = 0; (j < i + 1); j++)
				ret += base64_chars[char_array_4[j]];

			while ((i++ < 3))
				ret += '=';

		}

		return ret;
	}

	static std::string get_okex_sign(const char* key, const char* data)
	{
		unsigned char digest[32];
		mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
			reinterpret_cast<const unsigned char*>(key), strlen(key),
			reinterpret_cast<const unsigned char*>(data), strlen(data),
			digest);
		std::string b64 = base64_encode((const char*)digest, 32);
		//std::string urlcode = UrlEncode(b64);
		return b64;
	}

	static uint64_t pionex_get_current_ms_epoch()
	{
#ifdef _WIN32
		time_t clock;
		struct tm tm;
		SYSTEMTIME wtm;
		GetLocalTime(&wtm);
		tm.tm_year = wtm.wYear - 1900;
		tm.tm_mon = wtm.wMonth - 1;
		tm.tm_mday = wtm.wDay;
		tm.tm_hour = wtm.wHour;
		tm.tm_min = wtm.wMinute;
		tm.tm_sec = wtm.wSecond;
		tm.tm_isdst = -1;
		clock = mktime(&tm);
		long long num1 = clock * 1000;
		long long num2 = wtm.wMilliseconds;
		long long res = (num1 + num2);
		return res;
#else
		struct timeval tv;
		gettimeofday(&tv, NULL);

		return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
	}
}

std::string get_utc_time()
{
	struct tm* local;
	time_t t;
	t = time(NULL);
	local = gmtime(&t);

	int year = local->tm_year + 1900;
	int mon = local->tm_mon + 1;
	int day = local->tm_mday;
	int hour = local->tm_hour;
	int min = local->tm_min;
	int sec = local->tm_sec;

	char buf[32] = { 0 };
	sprintf(buf, "%d-%02d-%02dT%02d:%02d:%02d", year, mon, day, hour, min, sec);
	std::string res = buf;
	return res;
}
#endif
