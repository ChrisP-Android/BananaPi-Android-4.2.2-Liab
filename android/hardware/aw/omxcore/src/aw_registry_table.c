
/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

  This module contains the registry table for the QCOM's OpenMAX core.

*//*========================================================================*/


#include "aw_omx_core.h"

omx_core_cb_type core[] =
{
	{
		"OMX.allwinner.video.decoder.avc",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVdec.so",
		{
			"video_decoder.avc"
		}
	},


	{
		"OMX.allwinner.video.decoder.mpeg4",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVdec.so",
		{
			"video_decoder.mpeg4"
		}
	},


	{
		"OMX.allwinner.video.decoder.vc1",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVdec.so",
		{
			"video_decoder.vc1"
		}
	},


	{
		"OMX.allwinner.video.decoder.h263",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVdec.so",
		{
			"video_decoder.h263"
		}
	},


    {
		"OMX.allwinner.video.decoder.mpeg2",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVdec.so",
		{
			"video_decoder.mpeg2"
		}
	},

    {
		"OMX.allwinner.video.decoder.vp8",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVdec.so",
		{
			"video_decoder.vp8"
		}
	},

	{
		"OMX.allwinner.video.encoder.avc",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVenc.so",
		{
			"video_encode.avc"
		}
	},
	{
		"OMX.allwinner.video.encoder.h263",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVenc.so",
		{
			"video_encode.h263"
		}
	},
	{
		"OMX.allwinner.video.encoder.mpeg4",
		NULL, // Create instance function
		// Unique instance handle
		{
			NULL,
			NULL,
			NULL,
			NULL
		},
		NULL,   // Shared object library handle
		"libOmxVenc.so",
		{
			"video_encode.mpeg4"
		}
	},
};

const unsigned int SIZE_OF_CORE = sizeof(core) / sizeof(omx_core_cb_type);


