// license:BSD-3-Clause
// copyright-holders:feos
/***************************************************************************

    exports.cpp

    API for using MAME as a shared library.

***************************************************************************/


#include "exports.h"


//**************************************************************************
//  MACROS
//**************************************************************************

#ifdef MAME_SHARED_LIB
#ifdef WIN32
#define MAME_EXPORT extern "C" __declspec(dllexport)
#else
#define MAME_EXPORT extern "C" __attribute__((visibility("default"))) 
#endif
#else
#define MAME_EXPORT
#endif


//**************************************************************************
//  GLOBAL FUNCTIONS
//**************************************************************************

void (*log_callback)(osd_output_channel channel, int size, char *buffer) = nullptr;

extern int main(int argc, char *argv[]);


//**************************************************************************
//  OUTPUT REDIRECTION
//**************************************************************************

void export_output::output_callback(osd_output_channel channel, const char *msg, va_list args)
{
	if (!log_callback)
	{
		chain_output(channel, msg, args);
		return;
	}

	std::vector<char> buffer(1 + std::vsnprintf(nullptr, 0, msg, args));
	std::vsnprintf(buffer.data(), buffer.size(), msg, args);

	log_callback(channel, buffer.size(), buffer.data());
};


//**************************************************************************
//  API
//**************************************************************************

MAME_EXPORT int mame_launch(int argc, char *argv[])
{
	return main(argc, argv);
}

MAME_EXPORT void mame_set_log_callback(void (*callback)(osd_output_channel channel, int size, char *buffer))
{
	log_callback = callback;
}