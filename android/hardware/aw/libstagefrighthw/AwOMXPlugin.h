
#ifndef AW_OMX_PLUGIN_H_

#define AW_OMX_PLUGIN_H_

#include <OMXPluginBase.h>

namespace android
{

struct AwOMXPlugin : public OMXPluginBase
{
	AwOMXPlugin();
    virtual ~AwOMXPlugin();

    virtual OMX_ERRORTYPE makeComponentInstance(const char *name, const OMX_CALLBACKTYPE *callbacks, OMX_PTR appData, OMX_COMPONENTTYPE **component);
    virtual OMX_ERRORTYPE destroyComponentInstance(OMX_COMPONENTTYPE *component);
    virtual OMX_ERRORTYPE enumerateComponents(OMX_STRING name, size_t size, OMX_U32 index);
    virtual OMX_ERRORTYPE getRolesOfComponent(const char *name, Vector<String8> *roles);

private:
    void *mLibHandle;

    typedef OMX_ERRORTYPE (*InitFunc)();
    typedef OMX_ERRORTYPE (*DeinitFunc)();
    typedef OMX_ERRORTYPE (*ComponentNameEnumFunc)(OMX_STRING, OMX_U32, OMX_U32);
    typedef OMX_ERRORTYPE (*GetHandleFunc)(OMX_HANDLETYPE *, OMX_STRING, OMX_PTR, OMX_CALLBACKTYPE *);
    typedef OMX_ERRORTYPE (*FreeHandleFunc)(OMX_HANDLETYPE *);
    typedef OMX_ERRORTYPE (*GetRolesOfComponentFunc)(OMX_STRING, OMX_U32 *, OMX_U8 **);

    InitFunc                    mInit;
    DeinitFunc                  mDeinit;
    ComponentNameEnumFunc       mComponentNameEnum;
    GetHandleFunc               mGetHandle;
    FreeHandleFunc              mFreeHandle;
    GetRolesOfComponentFunc     mGetRolesOfComponentHandle;

    AwOMXPlugin(const AwOMXPlugin &);
    AwOMXPlugin &operator=(const AwOMXPlugin &);
};

}  // namespace android

#endif  // QCOM_OMX_PLUGIN_H_
