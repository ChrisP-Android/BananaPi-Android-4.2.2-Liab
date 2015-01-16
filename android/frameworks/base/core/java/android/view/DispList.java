/**
 * This file is used to show display format toast for user.
 * When the toast hides, something should be done.
 * This file is created by Gary on 2011-10-25 16:32:13.
 */
package android.view;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.util.Config;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;
import android.os.Handler;
import android.os.Message;
import android.view.DisplayManagerAw;
import android.os.Looper;
import java.util.HashMap;
import java.util.ArrayList;
import android.os.SystemProperties;


public class DispList extends Handler{
    public static final String TAG = "DispList";

    /**
     * display format list
     */
    public static final DispFormat DISP_FORMAT_NTSC             = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_NTSC);
    public static final DispFormat DISP_FORMAT_PAL              = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_PAL);
    public static final DispFormat DISP_FORMAT_YPBPR_480I       = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_480I );
    public static final DispFormat DISP_FORMAT_YPBPR_480P       = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_480P );
    public static final DispFormat DISP_FORMAT_YPBPR_576I       = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_576I );
    public static final DispFormat DISP_FORMAT_YPBPR_576P       = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_576P );
    public static final DispFormat DISP_FORMAT_YPBPR_720P_50HZ  = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_720P_50HZ );
    public static final DispFormat DISP_FORMAT_YPBPR_720P_60HZ  = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_720P_60HZ );
    public static final DispFormat DISP_FORMAT_YPBPR_1080P_50HZ = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_1080P_50HZ );
    public static final DispFormat DISP_FORMAT_YPBPR_1080P_60HZ = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV  , DisplayManagerAw.DISPLAY_TVFORMAT_1080P_60HZ );
    public static final DispFormat DISP_FORMAT_HDMI_480I        = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_480I );
    public static final DispFormat DISP_FORMAT_HDMI_480P        = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_480P );
    public static final DispFormat DISP_FORMAT_HDMI_576I        = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_576I );
    public static final DispFormat DISP_FORMAT_HDMI_576P        = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_576P );
    public static final DispFormat DISP_FORMAT_HDMI_720P_50HZ   = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_720P_50HZ );
    public static final DispFormat DISP_FORMAT_HDMI_720P_60HZ   = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_720P_60HZ );
    public static final DispFormat DISP_FORMAT_HDMI_1080I_50HZ  = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_1080I_50HZ);
    public static final DispFormat DISP_FORMAT_HDMI_1080I_60HZ  = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_1080I_60HZ);
    public static final DispFormat DISP_FORMAT_HDMI_1080P_24HZ  = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_1080P_24HZ);
    public static final DispFormat DISP_FORMAT_HDMI_1080P_50HZ  = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_1080P_50HZ);
    public static final DispFormat DISP_FORMAT_HDMI_1080P_60HZ  = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI, DisplayManagerAw.DISPLAY_TVFORMAT_1080P_60HZ);
    public static final DispFormat DISP_FORMAT_VGA_640_480      = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H640_V480  );
    public static final DispFormat DISP_FORMAT_VGA_800_600      = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H800_V600  );
    public static final DispFormat DISP_FORMAT_VGA_1024_768     = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H1024_V768 );
    public static final DispFormat DISP_FORMAT_VGA_1280_720     = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H1280_V720 );
    public static final DispFormat DISP_FORMAT_VGA_1280_1024    = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H1280_V1024);
    public static final DispFormat DISP_FORMAT_VGA_1360_768     = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H1360_V768 );
    public static final DispFormat DISP_FORMAT_VGA_1440_900     = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H1440_V900 );
    public static final DispFormat DISP_FORMAT_VGA_1680_1050    = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H1680_V1050);
    public static final DispFormat DISP_FORMAT_VGA_1920_1080    = new DispFormat(DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA , DisplayManagerAw.DISPLAY_VGA_H1920_V1080);

    public static final String DISP_FORMAT_NAME_NTSC             = "NTSC";
    public static final String DISP_FORMAT_NAME_PAL              = "PAL";
    public static final String DISP_FORMAT_NAME_YPBPR_480I       = "YPbPr 480I";
    public static final String DISP_FORMAT_NAME_YPBPR_480P       = "YPbPr 480P";
    public static final String DISP_FORMAT_NAME_YPBPR_576I       = "YPbPr 576I";
    public static final String DISP_FORMAT_NAME_YPBPR_576P       = "YPbPr 576P";
    public static final String DISP_FORMAT_NAME_YPBPR_720P_50HZ  = "YPbPr 720P 50HZ";
    public static final String DISP_FORMAT_NAME_YPBPR_720P_60HZ  = "YPbPr 720P 60HZ";
    public static final String DISP_FORMAT_NAME_YPBPR_1080P_50HZ = "YPbPr 1080P 50HZ";
    public static final String DISP_FORMAT_NAME_YPBPR_1080P_60HZ = "YPbPr 1080P 60HZ";
    public static final String DISP_FORMAT_NAME_HDMI_480I        = "HDMI 480I";
    public static final String DISP_FORMAT_NAME_HDMI_480P        = "HDMI 480P";
    public static final String DISP_FORMAT_NAME_HDMI_576I        = "HDMI 576I";
    public static final String DISP_FORMAT_NAME_HDMI_576P        = "HDMI 576P";
    public static final String DISP_FORMAT_NAME_HDMI_720P_50HZ   = "HDMI 720P 50HZ";
    public static final String DISP_FORMAT_NAME_HDMI_720P_60HZ   = "HDMI 720P 60HZ";
    public static final String DISP_FORMAT_NAME_HDMI_1080I_50HZ  = "HDMI 1080I 50HZ";
    public static final String DISP_FORMAT_NAME_HDMI_1080I_60HZ  = "HDMI 1080I 60HZ";
    public static final String DISP_FORMAT_NAME_HDMI_1080P_24HZ  = "HDMI 1080P 24HZ";
    public static final String DISP_FORMAT_NAME_HDMI_1080P_50HZ  = "HDMI 1080P 50HZ";
    public static final String DISP_FORMAT_NAME_HDMI_1080P_60HZ  = "HDMI 1080P 60HZ";
    public static final String DISP_FORMAT_NAME_VGA_640_480      = "VGA 640 x 480";
    public static final String DISP_FORMAT_NAME_VGA_800_600      = "VGA 800 x 600";
    public static final String DISP_FORMAT_NAME_VGA_1024_768     = "VGA 1024 x 768";
    public static final String DISP_FORMAT_NAME_VGA_1280_720     = "VGA 1280 x 720";
    public static final String DISP_FORMAT_NAME_VGA_1280_1024    = "VGA 1280 x 1024";
    public static final String DISP_FORMAT_NAME_VGA_1360_768     = "VGA 1360 x 768";
    public static final String DISP_FORMAT_NAME_VGA_1440_900     = "VGA 1440 x 900";
    public static final String DISP_FORMAT_NAME_VGA_1680_1050    = "VGA 1680 x 1050";
    public static final String DISP_FORMAT_NAME_VGA_1920_1080    = "VGA 1920 x 1080";

    /* some default display format */
    public static final DispFormat HDMI_DEFAULT_FORMAT  = DISP_FORMAT_HDMI_720P_60HZ;
    public static final DispFormat CVBS_DEFAULT_FORMAT  = DISP_FORMAT_NTSC;
    public static final DispFormat YPBPR_DEFAULT_FORMAT = DISP_FORMAT_YPBPR_720P_60HZ;
    public static final DispFormat VGA_DEFAULT_FORMAT   = DISP_FORMAT_VGA_1024_768;

    public static final DispFormat SYS_DISPLAY_DEFAULT_FORMAT = CVBS_DEFAULT_FORMAT;

    public static final int ADVANCED_DISPLAY_TYPE_UNKNOWN   = 0;
    public static final int ADVANCED_DISPLAY_TYPE_HDMI      = 1;
    public static final int ADVANCED_DISPLAY_TYPE_CVBS      = 2;
    public static final int ADVANCED_DISPLAY_TYPE_YPBPR     = 3;
    public static final int ADVANCED_DISPLAY_TYPE_VGA       = 4;

    private static HashMap<DispFormat, Integer>  mStrMap;
    private static ArrayList<DispFormat>         mItemArray;
    private static ArrayList<Integer>            mShortCutArray;
    private static HashMap<Integer, Integer>     mShortCutStrMap;
    private static HashMap<Integer, DispFormat>  mShortCutFormatMap;
    private static HashMap<DispFormat, String>   mCode2NameMap;
    private static HashMap<String, DispFormat>   mName2CodeMap;

    static {
        // create a HashMap for string and add items to it
        mStrMap = new HashMap<DispFormat, Integer>();
        mStrMap.put(DISP_FORMAT_NTSC            , new Integer(com.android.internal.R.string.disp_format_ntsc            ));
        mStrMap.put(DISP_FORMAT_PAL             , new Integer(com.android.internal.R.string.disp_format_pal             ));
        mStrMap.put(DISP_FORMAT_YPBPR_480I      , new Integer(com.android.internal.R.string.disp_format_ypbpr_480i      ));
        mStrMap.put(DISP_FORMAT_YPBPR_480P      , new Integer(com.android.internal.R.string.disp_format_ypbpr_480p      ));
        mStrMap.put(DISP_FORMAT_YPBPR_576I      , new Integer(com.android.internal.R.string.disp_format_ypbpr_576i      ));
        mStrMap.put(DISP_FORMAT_YPBPR_576P      , new Integer(com.android.internal.R.string.disp_format_ypbpr_576p      ));
        mStrMap.put(DISP_FORMAT_YPBPR_720P_50HZ , new Integer(com.android.internal.R.string.disp_format_ypbpr_720p_50hz ));
        mStrMap.put(DISP_FORMAT_YPBPR_720P_60HZ , new Integer(com.android.internal.R.string.disp_format_ypbpr_720p_60hz ));
        mStrMap.put(DISP_FORMAT_YPBPR_1080P_60HZ, new Integer(com.android.internal.R.string.disp_format_ypbpr_1080p_60hz));
        mStrMap.put(DISP_FORMAT_YPBPR_1080P_50HZ, new Integer(com.android.internal.R.string.disp_format_ypbpr_1080p_50hz));
        mStrMap.put(DISP_FORMAT_HDMI_480I       , new Integer(com.android.internal.R.string.disp_format_hdmi_480i       ));
        mStrMap.put(DISP_FORMAT_HDMI_480P       , new Integer(com.android.internal.R.string.disp_format_hdmi_480p       ));
        mStrMap.put(DISP_FORMAT_HDMI_576I       , new Integer(com.android.internal.R.string.disp_format_hdmi_576i       ));
        mStrMap.put(DISP_FORMAT_HDMI_576P       , new Integer(com.android.internal.R.string.disp_format_hdmi_576p       ));
        mStrMap.put(DISP_FORMAT_HDMI_720P_50HZ  , new Integer(com.android.internal.R.string.disp_format_hdmi_720p_50hz  ));
        mStrMap.put(DISP_FORMAT_HDMI_720P_60HZ  , new Integer(com.android.internal.R.string.disp_format_hdmi_720p_60hz  ));
        mStrMap.put(DISP_FORMAT_HDMI_1080I_50HZ , new Integer(com.android.internal.R.string.disp_format_hdmi_1080i_50hz ));
        mStrMap.put(DISP_FORMAT_HDMI_1080I_60HZ , new Integer(com.android.internal.R.string.disp_format_hdmi_1080i_60hz ));
        mStrMap.put(DISP_FORMAT_HDMI_1080P_24HZ , new Integer(com.android.internal.R.string.disp_format_hdmi_1080p_24hz ));
        mStrMap.put(DISP_FORMAT_HDMI_1080P_50HZ , new Integer(com.android.internal.R.string.disp_format_hdmi_1080p_50hz ));
        mStrMap.put(DISP_FORMAT_HDMI_1080P_60HZ , new Integer(com.android.internal.R.string.disp_format_hdmi_1080p_60hz ));
        mStrMap.put(DISP_FORMAT_VGA_640_480     , new Integer(com.android.internal.R.string.disp_format_vga_640_480     ));
        mStrMap.put(DISP_FORMAT_VGA_800_600     , new Integer(com.android.internal.R.string.disp_format_vga_800_600     ));
        mStrMap.put(DISP_FORMAT_VGA_1024_768    , new Integer(com.android.internal.R.string.disp_format_vga_1024_768    ));
        mStrMap.put(DISP_FORMAT_VGA_1280_720    , new Integer(com.android.internal.R.string.disp_format_vga_1280_720    ));
        mStrMap.put(DISP_FORMAT_VGA_1280_1024   , new Integer(com.android.internal.R.string.disp_format_vga_1280_1024   ));
        mStrMap.put(DISP_FORMAT_VGA_1360_768    , new Integer(com.android.internal.R.string.disp_format_vga_1360_768    ));
        mStrMap.put(DISP_FORMAT_VGA_1440_900    , new Integer(com.android.internal.R.string.disp_format_vga_1440_900    ));
        mStrMap.put(DISP_FORMAT_VGA_1680_1050   , new Integer(com.android.internal.R.string.disp_format_vga_1680_1050   ));
        mStrMap.put(DISP_FORMAT_VGA_1920_1080   , new Integer(com.android.internal.R.string.disp_format_vga_1920_1080   ));

        // create a list mapping format code to name
        mCode2NameMap = new HashMap<DispFormat, String>();
        mCode2NameMap.put(DISP_FORMAT_NTSC            , DISP_FORMAT_NAME_NTSC            );
        mCode2NameMap.put(DISP_FORMAT_PAL             , DISP_FORMAT_NAME_PAL             );
        mCode2NameMap.put(DISP_FORMAT_YPBPR_480I      , DISP_FORMAT_NAME_YPBPR_480I      );
        mCode2NameMap.put(DISP_FORMAT_YPBPR_480P      , DISP_FORMAT_NAME_YPBPR_480P      );
        mCode2NameMap.put(DISP_FORMAT_YPBPR_576I      , DISP_FORMAT_NAME_YPBPR_576I      );
        mCode2NameMap.put(DISP_FORMAT_YPBPR_576P      , DISP_FORMAT_NAME_YPBPR_576P      );
        mCode2NameMap.put(DISP_FORMAT_YPBPR_720P_50HZ , DISP_FORMAT_NAME_YPBPR_720P_50HZ );
        mCode2NameMap.put(DISP_FORMAT_YPBPR_720P_60HZ , DISP_FORMAT_NAME_YPBPR_720P_60HZ );
        mCode2NameMap.put(DISP_FORMAT_YPBPR_1080P_60HZ, DISP_FORMAT_NAME_YPBPR_1080P_50HZ);
        mCode2NameMap.put(DISP_FORMAT_YPBPR_1080P_50HZ, DISP_FORMAT_NAME_YPBPR_1080P_60HZ);
        mCode2NameMap.put(DISP_FORMAT_HDMI_480I       , DISP_FORMAT_NAME_HDMI_480I       );
        mCode2NameMap.put(DISP_FORMAT_HDMI_480P       , DISP_FORMAT_NAME_HDMI_480P       );
        mCode2NameMap.put(DISP_FORMAT_HDMI_576I       , DISP_FORMAT_NAME_HDMI_576I       );
        mCode2NameMap.put(DISP_FORMAT_HDMI_576P       , DISP_FORMAT_NAME_HDMI_576P       );
        mCode2NameMap.put(DISP_FORMAT_HDMI_720P_50HZ  , DISP_FORMAT_NAME_HDMI_720P_50HZ  );
        mCode2NameMap.put(DISP_FORMAT_HDMI_720P_60HZ  , DISP_FORMAT_NAME_HDMI_720P_60HZ  );
        mCode2NameMap.put(DISP_FORMAT_HDMI_1080I_50HZ , DISP_FORMAT_NAME_HDMI_1080I_50HZ );
        mCode2NameMap.put(DISP_FORMAT_HDMI_1080I_60HZ , DISP_FORMAT_NAME_HDMI_1080I_60HZ );
        mCode2NameMap.put(DISP_FORMAT_HDMI_1080P_24HZ , DISP_FORMAT_NAME_HDMI_1080P_24HZ );
        mCode2NameMap.put(DISP_FORMAT_HDMI_1080P_50HZ , DISP_FORMAT_NAME_HDMI_1080P_50HZ );
        mCode2NameMap.put(DISP_FORMAT_HDMI_1080P_60HZ , DISP_FORMAT_NAME_HDMI_1080P_60HZ );
        mCode2NameMap.put(DISP_FORMAT_VGA_640_480     , DISP_FORMAT_NAME_VGA_640_480     );
        mCode2NameMap.put(DISP_FORMAT_VGA_800_600     , DISP_FORMAT_NAME_VGA_800_600     );
        mCode2NameMap.put(DISP_FORMAT_VGA_1024_768    , DISP_FORMAT_NAME_VGA_1024_768    );
        mCode2NameMap.put(DISP_FORMAT_VGA_1280_720    , DISP_FORMAT_NAME_VGA_1280_720    );
        mCode2NameMap.put(DISP_FORMAT_VGA_1280_1024   , DISP_FORMAT_NAME_VGA_1280_1024   );
        mCode2NameMap.put(DISP_FORMAT_VGA_1360_768    , DISP_FORMAT_NAME_VGA_1360_768    );
        mCode2NameMap.put(DISP_FORMAT_VGA_1440_900    , DISP_FORMAT_NAME_VGA_1440_900    );
        mCode2NameMap.put(DISP_FORMAT_VGA_1680_1050   , DISP_FORMAT_NAME_VGA_1680_1050   );
        mCode2NameMap.put(DISP_FORMAT_VGA_1920_1080   , DISP_FORMAT_NAME_VGA_1920_1080  );

        // create a list mapping format name to code
        mName2CodeMap = new HashMap<String, DispFormat>();
        mName2CodeMap.put(DISP_FORMAT_NAME_NTSC            , DISP_FORMAT_NTSC            );
        mName2CodeMap.put(DISP_FORMAT_NAME_PAL             , DISP_FORMAT_PAL             );
        mName2CodeMap.put(DISP_FORMAT_NAME_YPBPR_480I      , DISP_FORMAT_YPBPR_480I      );
        mName2CodeMap.put(DISP_FORMAT_NAME_YPBPR_480P      , DISP_FORMAT_YPBPR_480P      );
        mName2CodeMap.put(DISP_FORMAT_NAME_YPBPR_576I      , DISP_FORMAT_YPBPR_576I      );
        mName2CodeMap.put(DISP_FORMAT_NAME_YPBPR_576P      , DISP_FORMAT_YPBPR_576P      );
        mName2CodeMap.put(DISP_FORMAT_NAME_YPBPR_720P_50HZ , DISP_FORMAT_YPBPR_720P_50HZ );
        mName2CodeMap.put(DISP_FORMAT_NAME_YPBPR_720P_60HZ , DISP_FORMAT_YPBPR_720P_60HZ );
        mName2CodeMap.put(DISP_FORMAT_NAME_YPBPR_1080P_50HZ, DISP_FORMAT_YPBPR_1080P_60HZ);
        mName2CodeMap.put(DISP_FORMAT_NAME_YPBPR_1080P_60HZ, DISP_FORMAT_YPBPR_1080P_50HZ);
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_480I       , DISP_FORMAT_HDMI_480I       );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_480P       , DISP_FORMAT_HDMI_480P       );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_576I       , DISP_FORMAT_HDMI_576I       );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_576P       , DISP_FORMAT_HDMI_576P       );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_720P_50HZ  , DISP_FORMAT_HDMI_720P_50HZ  );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_720P_60HZ  , DISP_FORMAT_HDMI_720P_60HZ  );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_1080I_50HZ , DISP_FORMAT_HDMI_1080I_50HZ );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_1080I_60HZ , DISP_FORMAT_HDMI_1080I_60HZ );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_1080P_24HZ , DISP_FORMAT_HDMI_1080P_24HZ );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_1080P_50HZ , DISP_FORMAT_HDMI_1080P_50HZ );
        mName2CodeMap.put(DISP_FORMAT_NAME_HDMI_1080P_60HZ , DISP_FORMAT_HDMI_1080P_60HZ );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_640_480     , DISP_FORMAT_VGA_640_480     );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_800_600     , DISP_FORMAT_VGA_800_600     );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_1024_768    , DISP_FORMAT_VGA_1024_768    );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_1280_720    , DISP_FORMAT_VGA_1280_720    );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_1280_1024   , DISP_FORMAT_VGA_1280_1024   );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_1360_768    , DISP_FORMAT_VGA_1360_768    );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_1440_900    , DISP_FORMAT_VGA_1440_900    );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_1680_1050   , DISP_FORMAT_VGA_1680_1050   );
        mName2CodeMap.put(DISP_FORMAT_NAME_VGA_1920_1080   , DISP_FORMAT_VGA_1920_1080   );

        // create a ArrayList and add items to it
        mItemArray = new ArrayList<DispFormat>();

//        mItemArray.add(DISP_FORMAT_NTSC            );
//        mItemArray.add(DISP_FORMAT_PAL             );
        mItemArray.add(DISP_FORMAT_NTSC            );
        mItemArray.add(DISP_FORMAT_PAL             );
//        mItemArray.add(DISP_FORMAT_YPBPR_480I      );
//        mItemArray.add(DISP_FORMAT_YPBPR_480P      );
//        mItemArray.add(DISP_FORMAT_YPBPR_576I      );
//        mItemArray.add(DISP_FORMAT_YPBPR_576P      );
//        mItemArray.add(DISP_FORMAT_YPBPR_720P_50HZ );
//        mItemArray.add(DISP_FORMAT_YPBPR_720P_60HZ );
//        mItemArray.add(DISP_FORMAT_YPBPR_1080P_50HZ);
//        mItemArray.add(DISP_FORMAT_YPBPR_1080P_60HZ);
        mItemArray.add(DISP_FORMAT_HDMI_480I       );
        mItemArray.add(DISP_FORMAT_HDMI_480P       );
        mItemArray.add(DISP_FORMAT_HDMI_576I       );
        mItemArray.add(DISP_FORMAT_HDMI_576P       );
        mItemArray.add(DISP_FORMAT_HDMI_720P_50HZ  );
        mItemArray.add(DISP_FORMAT_HDMI_720P_60HZ  );
        mItemArray.add(DISP_FORMAT_HDMI_1080I_50HZ );
        mItemArray.add(DISP_FORMAT_HDMI_1080I_60HZ );
        mItemArray.add(DISP_FORMAT_HDMI_1080P_24HZ );
        mItemArray.add(DISP_FORMAT_HDMI_1080P_50HZ );
        mItemArray.add(DISP_FORMAT_HDMI_1080P_60HZ );
        mItemArray.add(DISP_FORMAT_VGA_640_480     );
        mItemArray.add(DISP_FORMAT_VGA_800_600     );
        mItemArray.add(DISP_FORMAT_VGA_1024_768    );
        mItemArray.add(DISP_FORMAT_VGA_1280_720    );
        mItemArray.add(DISP_FORMAT_VGA_1280_1024   );
        mItemArray.add(DISP_FORMAT_VGA_1360_768    );
        mItemArray.add(DISP_FORMAT_VGA_1440_900    );
        mItemArray.add(DISP_FORMAT_VGA_1680_1050   );
        mItemArray.add(DISP_FORMAT_VGA_1920_1080   );

        // create a ArrayList and add items to it
        mShortCutArray = new ArrayList<Integer>();
        mShortCutArray.add(new Integer(ADVANCED_DISPLAY_TYPE_HDMI ));
        mShortCutArray.add(new Integer(ADVANCED_DISPLAY_TYPE_CVBS ));
        //mShortCutArray.add(new Integer(ADVANCED_DISPLAY_TYPE_YPBPR));
        mShortCutArray.add(new Integer(ADVANCED_DISPLAY_TYPE_VGA  ));

        mShortCutStrMap = new HashMap<Integer, Integer>();
        mShortCutStrMap.put(new Integer(ADVANCED_DISPLAY_TYPE_HDMI ), new Integer(com.android.internal.R.string.display_type_hmdi ));
        mShortCutStrMap.put(new Integer(ADVANCED_DISPLAY_TYPE_CVBS ), new Integer(com.android.internal.R.string.display_type_cvbs ));
        mShortCutStrMap.put(new Integer(ADVANCED_DISPLAY_TYPE_YPBPR), new Integer(com.android.internal.R.string.display_type_ypbpr));
        mShortCutStrMap.put(new Integer(ADVANCED_DISPLAY_TYPE_VGA  ), new Integer(com.android.internal.R.string.display_type_vga  ));

        mShortCutFormatMap = new HashMap<Integer, DispFormat>();
        mShortCutFormatMap.put(new Integer(ADVANCED_DISPLAY_TYPE_HDMI ), HDMI_DEFAULT_FORMAT );
        mShortCutFormatMap.put(new Integer(ADVANCED_DISPLAY_TYPE_CVBS ), CVBS_DEFAULT_FORMAT );
        mShortCutFormatMap.put(new Integer(ADVANCED_DISPLAY_TYPE_YPBPR), YPBPR_DEFAULT_FORMAT);
        mShortCutFormatMap.put(new Integer(ADVANCED_DISPLAY_TYPE_VGA  ), VGA_DEFAULT_FORMAT  );
    }

    public static ArrayList<DispFormat> getDispList(){
        return mItemArray;
    }

    public static HashMap<DispFormat, Integer> getItemStringIdList(){
        return mStrMap;
    }

    public static String ItemCode2Name(DispFormat item){
        return mCode2NameMap.get(item);
    }

    public static DispFormat ItemName2Code(String name){
        return mName2CodeMap.get(name);
    }

    private static final int DURATION = Toast.LENGTH_SHORT;

    /**
     * message list
     */
    private static final int MSG_SHOW_ITEM  = 0;
    private static final int MSG_SHOW_NEXT  = 1;

    /**
     * member variables
     */
    private MyToast                       mToast;
    private int                           mCurItem;
    private Context                       mContext;
    private boolean                       mToastIsInited = false;

    public DispList(Context context){
        mContext = context;
        mCurItem = ADVANCED_DISPLAY_TYPE_UNKNOWN;

        // create a toast
        mToast = new MyToast( context );
//        initToast();
    }

    public void showItem(DispFormat item){
        initToast();
        obtainMessage(MSG_SHOW_ITEM, item).sendToTarget();
    }

    public void showNext(){
        initToast();
        obtainMessage(MSG_SHOW_NEXT).sendToTarget();
    }

    public boolean isShowing(){
        initToast();
        return mToast.isShowing();
    }

    /**
     * a callback method. When the toast hides, the method is called.
     *
     * @param item  input. the current showing item.
     */
    public void onHide(DispFormat item){
        Log.d(TAG, "on Hide");
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {

            case MSG_SHOW_ITEM: {
                onShowItem((DispFormat)msg.obj);
                break;
            }
            case MSG_SHOW_NEXT: {
                onShowNext();
                break;
            }
            default: {
                Log.w(TAG, "unsupported message = " + msg.what);
                break;
            }
        }
    }

    public static boolean isHDMI(DispFormat format){
        if(format.mOutputType == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI)
            return true;
        else
            return false;
    }

    public static boolean isCVBS(DispFormat format){
        if(    format.equals(DispList.DISP_FORMAT_NTSC)
            || format.equals(DispList.DISP_FORMAT_PAL))
            return true;
        else
            return false;
    }

    public static boolean isYPbPr(DispFormat format){
        if(   format.equals(DispList.DISP_FORMAT_YPBPR_480I      )
           || format.equals(DispList.DISP_FORMAT_YPBPR_480P      )
           || format.equals(DispList.DISP_FORMAT_YPBPR_576I      )
           || format.equals(DispList.DISP_FORMAT_YPBPR_576P      )
           || format.equals(DispList.DISP_FORMAT_YPBPR_720P_50HZ )
           || format.equals(DispList.DISP_FORMAT_YPBPR_720P_60HZ )
           || format.equals(DispList.DISP_FORMAT_YPBPR_1080P_50HZ)
           || format.equals(DispList.DISP_FORMAT_YPBPR_1080P_60HZ))
            return true;
        else
            return false;
    }


    public static boolean isVGA(DispFormat format){
        if(format.mOutputType == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA)
            return true;
        else
            return false;
    }

    public static int getAdvancedDisplayType(DispFormat format){
        if(isHDMI(format)){
            return ADVANCED_DISPLAY_TYPE_HDMI;
        }else if(isCVBS(format)){
            return ADVANCED_DISPLAY_TYPE_CVBS;
        }else if(isYPbPr(format)){
            return ADVANCED_DISPLAY_TYPE_YPBPR;
        }else if(isVGA(format)){
            return ADVANCED_DISPLAY_TYPE_VGA;
        }else{
            return ADVANCED_DISPLAY_TYPE_UNKNOWN;
        }
    }

    protected void onShowItem(DispFormat item){
        // get a item's string id
        int advType = getAdvancedDisplayType(item);
        Integer str_id = mShortCutStrMap.get(advType);
        if( str_id == null ){
            Log.w(TAG, "Fail in searching item = " + item);
            return;
        }

        // redraw TextView and show toast
        mToast.setText(str_id.intValue());
        mToast.show();
        // record the current showing item
        mCurItem = advType;
    }

    protected void onShowNext(){
        // get the current showing item's index in the array.
        int index = mShortCutArray.indexOf(mCurItem);
        Log.d(TAG,"current index = " + index);
        if( index == -1 )
            return;

        // get the index of the next item to be shown
        index++;
        if( index == mShortCutArray.size() )
            index = 0;

        onShowItem(mShortCutFormatMap.get(mShortCutArray.get(index)));
    }

    private void initToast(){
        if(!mToastIsInited){
            LayoutInflater inflate = LayoutInflater.from(mContext);
            View v = inflate.inflate(com.android.internal.R.layout.transient_notification, null);

            mToast.setGravity(Gravity.TOP | Gravity.LEFT, 100, 0 );
            mToast.setView(v);
            mToast.setDuration(DURATION);
            mToastIsInited = true;
        }
    }


    class MyToast extends Toast{

        public MyToast( Context context ){
            super( context );
        }

        public void onHide(){
            Log.d(TAG, "on Hide in MyToast.\n");
            // call DispList's method
            DispList.this.onHide(mShortCutFormatMap.get(mCurItem));
        }
    }

    static public class DispFormat {
        public int mOutputType;
        public int mFormat;

        public DispFormat(int type, int format){
            mOutputType = type;
            mFormat = format;
        }

        @Override
        public boolean equals(Object obj){
            if(obj == null)
                return false;
            if(obj.getClass() != this.getClass())
                return false;

            DispFormat n = (DispFormat)obj;
            return (n.mOutputType == this.mOutputType) && (n.mFormat == this.mFormat);
        }

        @Override
        public int hashCode(){
            return mOutputType;
        }

        @Override
        public String toString(){
            return new String("type = " + mOutputType + " format = " + mFormat);
        }
    }
}
