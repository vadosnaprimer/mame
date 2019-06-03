// license:BSD-3-Clause
// copyright-holders:feos
/***************************************************************************

    exports.h

    API for using MAME as a shared library.

***************************************************************************/


// MAME headers
#include "osdcore.h"


//**************************************************************************
//  OUTPUT REDIRECTION
//**************************************************************************

class export_output : public osd_output
{
public:
	virtual void output_callback(osd_output_channel channel, const char *msg, va_list args) override;
};
