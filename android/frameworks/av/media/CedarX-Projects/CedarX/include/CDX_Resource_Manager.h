#ifndef CDXRESOURCEMANAGE_H
#define CDXRESOURCEMANAGE_H


#ifdef __cplusplus
extern "C"
{
#endif

typedef enum CEDARV_USAGE
{
	CEDARV_UNKNOWN           = 0,
	CEDARV_ENCODE_BACKGROUND = 1,
	CEDARV_DECODE_BACKGROUND = 2,
	CEDARV_ENCODE            = 3,
	CEDARV_DECODE            = 4,
}cedarv_usage_t;

typedef void* ve_mutex_t;

int  ve_mutex_init(ve_mutex_t* mutex, cedarv_usage_t usage);
void ve_mutex_destroy(ve_mutex_t* mutex);
int  ve_mutex_lock(ve_mutex_t* mutex);
int  ve_mutex_timed_lock(ve_mutex_t* mutex, int64_t timeout_us);
void ve_mutex_unlock(ve_mutex_t* mutex);

//int RequestCedarVResource(CEDARV_REQUEST_CONTEXT *req_ctx);
//int ReleaseCedarVResource(CEDARV_REQUEST_CONTEXT *req_ctx);
//int RequestCedarVFrameLevel(CEDARV_REQUEST_CONTEXT *request_ctx);
//int ReleaseCedarVFrameLevel(CEDARV_REQUEST_CONTEXT *req_ctx);
//void CedarVMayReset();

#ifdef __cplusplus
}
#endif

#endif
