

//#define LOG_NDEBUG 0
#define LOG_TAG "AwOMXPlugin"
#include <utils/Log.h>
#include "AWOmxDebug.h"

#include "AwOMXPlugin.h"

#include <dlfcn.h>

#include <HardwareAPI.h>

namespace android
{

extern "C" OMXPluginBase* createOMXPlugin()
{
    return new AwOMXPlugin;
}

extern "C" void destroyOMXPlugin(OMXPluginBase *plugin) {
    delete plugin;
}


AwOMXPlugin::AwOMXPlugin()
    : mLibHandle(dlopen("libOmxCore.so", RTLD_NOW)),
      mInit(NULL),
      mDeinit(NULL),
      mComponentNameEnum(NULL),
      mGetHandle(NULL),
      mFreeHandle(NULL),
      mGetRolesOfComponentHandle(NULL)
{
    if (mLibHandle != NULL)
    {
        mInit                      = (InitFunc)dlsym(mLibHandle, "OMX_Init");
        mDeinit                    = (DeinitFunc)dlsym(mLibHandle, "OMX_Deinit");
        mComponentNameEnum         = (ComponentNameEnumFunc)dlsym(mLibHandle, "OMX_ComponentNameEnum");
        mGetHandle                 = (GetHandleFunc)dlsym(mLibHandle, "OMX_GetHandle");
        mFreeHandle                = (FreeHandleFunc)dlsym(mLibHandle, "OMX_FreeHandle");
        mGetRolesOfComponentHandle = (GetRolesOfComponentFunc)dlsym(mLibHandle, "OMX_GetRolesOfComponent");

        (*mInit)();
    }
}


AwOMXPlugin::~AwOMXPlugin()
{
    if(mLibHandle != NULL)
    {
        (*mDeinit)();
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

OMX_ERRORTYPE AwOMXPlugin::makeComponentInstance(const char* name, const OMX_CALLBACKTYPE* callbacks, OMX_PTR appData, OMX_COMPONENTTYPE** component)
{
	ALOGV("step 1.");
    if (mLibHandle == NULL)
    {
        return OMX_ErrorUndefined;
    }

	ALOGV("step 2.");

    return (*mGetHandle)(reinterpret_cast<OMX_HANDLETYPE *>(component),
                         const_cast<char *>(name),
                         appData,
                         const_cast<OMX_CALLBACKTYPE *>(callbacks));
}


OMX_ERRORTYPE AwOMXPlugin::destroyComponentInstance(OMX_COMPONENTTYPE* component)
{
    if (mLibHandle == NULL)
    {
        return OMX_ErrorUndefined;
    }

    return (*mFreeHandle)(reinterpret_cast<OMX_HANDLETYPE *>(component));
}


OMX_ERRORTYPE AwOMXPlugin::enumerateComponents(OMX_STRING name, size_t size, OMX_U32 index)
{
    if (mLibHandle == NULL)
    {
        return OMX_ErrorUndefined;
    }

    OMX_ERRORTYPE res = (*mComponentNameEnum)(name, size, index);

    if (res != OMX_ErrorNone)
    {
        return res;
    }

    return OMX_ErrorNone;
}



OMX_ERRORTYPE AwOMXPlugin::getRolesOfComponent(const char *name, Vector<String8> *roles)
{
    roles->clear();

    if (mLibHandle == NULL)
    {
        return OMX_ErrorUndefined;
    }

    OMX_U32 numRoles;
    OMX_ERRORTYPE err = (*mGetRolesOfComponentHandle)(const_cast<OMX_STRING>(name), &numRoles, NULL);

    if (err != OMX_ErrorNone)
    {
        return err;
    }

    if (numRoles > 0)
    {
        OMX_U8** array = new OMX_U8 *[numRoles];
        for (OMX_U32 i = 0; i < numRoles; ++i)
        {
            array[i] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
        }

        OMX_U32 numRoles2 = numRoles;
        err = (*mGetRolesOfComponentHandle)(const_cast<OMX_STRING>(name), &numRoles2, array);

        if (err == OMX_ErrorNone && numRoles != numRoles2)
        {
            err = OMX_ErrorUndefined;
        }

        for (OMX_U32 i = 0; i < numRoles; ++i)
        {
            if (err == OMX_ErrorNone)
            {
                String8 s((const char *)array[i]);
                roles->push(s);
            }

            delete[] array[i];
            array[i] = NULL;
        }

        delete[] array;
        array = NULL;
    }

    return err;
}

}  // namespace android
