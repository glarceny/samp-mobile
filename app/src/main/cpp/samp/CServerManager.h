#pragma once
#include "main.h"

#include <cstdint>
#include <cstring>
#include "vendor/obfuscate/str_obfuscator_no_template.hpp"

class CServerInstance
{
public:
    static void initConnection(int id);
    static int iServer;

public:
	class CServerInstanceEncrypted
	{
	public:
		constexpr CServerInstanceEncrypted(const char* str, size_t len, int sum, int port, bool isVoice) :
			_buffer{}, _decrypted{ false }, _key{ static_cast<const char>(len % 255) }, _length{ len }, _sum{ sum }, _port{ port }, _isVoice{ isVoice }
		{
            for (size_t i = 0; i < _length && i < 30; i++) {
                _buffer[i] = str[i] ^ _key;
            }
            _buffer[_length] = '\0';
		}
#ifdef _WIN32
		__declspec(noinline)
#elif __GNUC__
		__attribute__((noinline))
#endif
			const char* decrypt() const {
			if (_decrypted) {
				return _buffer;
			}

			for (size_t i = 0; i < _length; i++) {
				_buffer[i] ^= _key;
			}

			_decrypted = true;

			return _buffer;
		}

		char getKey() const {
			return _key;
		}

		const char* getBuffer() const {
			return _buffer;
		}

		int getSum() const
		{
			return _sum;
		}
		int isVoice() const
		{
			return _isVoice;
		}
		int getPort() const
		{
			return _port;
		}

	private:
		mutable char _buffer[31];
		mutable bool _decrypted;
		const char _key;
		size_t _length;
		int _sum;
		int _isVoice;
		int _port;
	};

    static constexpr auto create(const char* str, int sum, size_t len, int port, bool isVoice) {
        return CServerInstanceEncrypted(str, len, sum, port, isVoice);
    }
};

#define NUM_TO_STR_IP(a1, a2, a3, a4) \
    #a1 "." #a2 "." #a3 "." #a4, (a1) + (a2) + (a3) + (a4)

extern const CServerInstance::CServerInstanceEncrypted g_sEncryptedAddresses[];
extern const char* g_szServerNames[];