
#ifndef __DE_TYPE_H__
#define __DE_TYPE_H__


//**************************************************
//display struct typedef
typedef struct {unsigned char  alpha;unsigned char red;unsigned char green; unsigned char blue; }__color_t;         /* 32-bit (ARGB) color                  */
typedef struct {signed int x; signed int y; unsigned int width; unsigned int height;}__rect_t;          /* rect attrib                          */
typedef struct {signed int x; signed int y;                           }__pos_t;        /* coordinate (x, y)                    */
typedef struct {unsigned int width;      unsigned int height;             }__rectsz_t;          /* rect size                            */



typedef enum
{
    BT601 = 0,
	BT709,
	YCC,
	VXYCC
}__cs_mode_t;

typedef enum __PIXEL_RGBFMT                         /* pixel format(rgb)                                            */
{ 
    PIXEL_MONO_1BPP=0,                              /* only used in normal framebuffer                              */
    PIXEL_MONO_2BPP,
    PIXEL_MONO_4BPP,
    PIXEL_MONO_8BPP,
    PIXEL_COLOR_RGB655,
    PIXEL_COLOR_RGB565,
    PIXEL_COLOR_RGB556,
    PIXEL_COLOR_ARGB1555,
    PIXEL_COLOR_RGBA5551,
    PIXEL_COLOR_RGB0888,                            /* used in normal framebuffer and scaler framebuffer            */
    PIXEL_COLOR_ARGB8888,
}__pixel_rgbfmt_t;
typedef enum __PIXEL_YUVFMT                         /* pixel format(yuv)                                            */
{ 
    PIXEL_YUV444 = 0x10,                            /* only used in scaler framebuffer                              */
    PIXEL_YUV422,
    PIXEL_YUV420,
    PIXEL_YUV411,
    PIXEL_CSIRGB,
    PIXEL_OTHERFMT
}__pixel_yuvfmt_t;
typedef enum __YUV_MODE                             /* Frame buffer data mode definition                            */
{
    YUV_MOD_INTERLEAVED=0,
    YUV_MOD_NON_MB_PLANAR,                          /* 无宏块平面模式                                               */
    YUV_MOD_MB_PLANAR,                              /* 宏块平面模式                                                 */
    YUV_MOD_UV_NON_MB_COMBINED,                     /* 无宏块UV打包模式                                             */
    YUV_MOD_UV_MB_COMBINED                          /* 宏块UV打包模式                                               */
}__yuv_mod_t;
typedef enum                                        /* yuv seq                                                      */
{
    YUV_SEQ_UYVY=0,                                 /* 以下4种指适合于yuv422 加 interleaved的组合方式               */
    YUV_SEQ_YUYV,
    YUV_SEQ_VYUY,
    YUV_SEQ_YVYU,
    YUV_SEQ_AYUV=0x10,                              /* 以下2种只适合于yuv444 加 interleaved的组合方式               */
    YUV_SEQ_VUYA,
    YUV_SEQ_UVUV=0x20,                              /* 以下2种只适合于yuv420 加 uv_combined的组合方式               */
    YUV_SEQ_VUVU,
    YUV_SEQ_OTHRS=0xff,                             /* 其他的pixelfmt和mod的组合方式                                */
}__yuv_seq_t;
typedef enum
{
    FB_TYPE_RGB=0,
    FB_TYPE_YUV=1
}__fb_type_t;

typedef struct
{
    __fb_type_t                 type;               /* 0 rgb, 1 yuv                                                 */
    union
    {
        struct
        {
            __pixel_rgbfmt_t    pixelfmt;           /* 像素的格式                                                   */
            unsigned char               br_swap;            /* blue red color swap flag                                     */
            unsigned char                pixseq;             /* 图像流的存储顺序                                             */
            struct                                  /* 调色板                                                       */
            {       
                void          * addr;               /* 如果pixel为非bpp格式，调色板指针为0                          */
                unsigned int           size;               
            }palette;
        }rgb;
        struct
        {
            __pixel_yuvfmt_t    pixelfmt;           /* 像素的格式                                                   */
            __yuv_mod_t         mod;                /* 图像的格式                                                   */
            __yuv_seq_t         yuvseq;             /* yuv的排列顺序                                                */
        }yuv;
    }fmt;
    __cs_mode_t					cs_mode;
}__fb_format_t;

typedef struct __FB                                 /* frame buffer                                                 */
{
    __rectsz_t size;               /* frame buffer的长宽                                           */
    void * addr[3];            /* frame buffer的内容地址，对于rgb类型，只有addr[0]有效         */
    __fb_format_t fmt;
}FB;



#endif
