/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>
#include <utils/Timers.h>

#include <binder/Parcel.h>
#include <binder/IInterface.h>

#include <gui/ISurfaceTexture.h>
#include <hardware/hwcomposer.h>

namespace android {
// ----------------------------------------------------------------------------

enum {
    REQUEST_BUFFER = IBinder::FIRST_CALL_TRANSACTION,
    SET_BUFFER_COUNT,
    DEQUEUE_BUFFER,
    QUEUE_BUFFER,
    CANCEL_BUFFER,
    QUERY,
    SET_SYNCHRONOUS_MODE,
    CONNECT,
    DISCONNECT,
    SET_PARAMETER,
    GET_PARAMETER,
    SET_CROP,
    SET_TRANSFORM,
    SET_SCALINGMODE,
    SET_TIMESTEAP,
};


class BpSurfaceTexture : public BpInterface<ISurfaceTexture>
{
public:
    BpSurfaceTexture(const sp<IBinder>& impl)
        : BpInterface<ISurfaceTexture>(impl)
    {
    }

    virtual status_t requestBuffer(int bufferIdx, sp<GraphicBuffer>* buf) {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(bufferIdx);
        status_t result =remote()->transact(REQUEST_BUFFER, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        bool nonNull = reply.readInt32();
        if (nonNull) {
            *buf = new GraphicBuffer();
            reply.read(**buf);
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setBufferCount(int bufferCount)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(bufferCount);
        status_t result =remote()->transact(SET_BUFFER_COUNT, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t dequeueBuffer(int *buf, sp<Fence>& fence,
            uint32_t w, uint32_t h, uint32_t format, uint32_t usage) {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(w);
        data.writeInt32(h);
        data.writeInt32(format);
        data.writeInt32(usage);
        status_t result = remote()->transact(DEQUEUE_BUFFER, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        *buf = reply.readInt32();
        fence.clear();
        bool hasFence = reply.readInt32();
        if (hasFence) {
            fence = new Fence();
            reply.read(*fence.get());
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t queueBuffer(int buf,
            const QueueBufferInput& input, QueueBufferOutput* output) {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(buf);
        data.write(input);
        status_t result = remote()->transact(QUEUE_BUFFER, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        memcpy(output, reply.readInplace(sizeof(*output)), sizeof(*output));
        result = reply.readInt32();
        return result;
    }

    virtual void cancelBuffer(int buf, sp<Fence> fence) {
        Parcel data, reply;
        bool hasFence = fence.get() && fence->isValid();
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(buf);
        data.writeInt32(hasFence);
        if (hasFence) {
            data.write(*fence.get());
        }
        remote()->transact(CANCEL_BUFFER, data, &reply);
    }

    virtual int query(int what, int* value) {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(what);
        status_t result = remote()->transact(QUERY, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        value[0] = reply.readInt32();
        result = reply.readInt32();
        return result;
    }

    virtual status_t setSynchronousMode(bool enabled) {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(enabled);
        status_t result = remote()->transact(SET_SYNCHRONOUS_MODE, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t connect(int api, QueueBufferOutput* output) {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(api);
        status_t result = remote()->transact(CONNECT, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        memcpy(output, reply.readInplace(sizeof(*output)), sizeof(*output));
        result = reply.readInt32();
        return result;
    }

    virtual status_t disconnect(int api) {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(api);
        status_t result =remote()->transact(DISCONNECT, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

	virtual int setParameter(uint32_t cmd,uint32_t value) 
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(cmd);
        if(cmd == HWC_LAYER_SETINITPARA)
        {
        	layerinitpara_t  *layer_info = (layerinitpara_t  *)value;	        	
        	data.write((void *)value,sizeof(layerinitpara_t));
        }
        else if(cmd == HWC_LAYER_SETFRAMEPARA)
        {
        	data.write((void *)value,sizeof(libhwclayerpara_t));
        }
        else if(cmd == HWC_LAYER_SET3DMODE)
        {
        	data.write((void *)value,sizeof(video3Dinfo_t));
        }
        else
        {
        	data.writeInt32(value);
    	}
        status_t result = remote()->transact(SET_PARAMETER, data, &reply);
        if (result != NO_ERROR) 
        {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual uint32_t getParameter(uint32_t cmd) 
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(cmd);
        status_t result =remote()->transact(GET_PARAMETER, data, &reply);
        if (result != NO_ERROR) 
        {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setCrop(const Rect& reg) {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeFloat(reg.left);
        data.writeFloat(reg.top);
        data.writeFloat(reg.right);
        data.writeFloat(reg.bottom);
        status_t result = remote()->transact(SET_CROP, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setCurrentTransform(uint32_t transfrom)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(transfrom);
        status_t result =remote()->transact(SET_TRANSFORM, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setCurrentScalingMode(int mode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt32(mode);
        status_t result =remote()->transact(SET_SCALINGMODE, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }

    virtual status_t setTimestamp(int64_t timestamp)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceTexture::getInterfaceDescriptor());
        data.writeInt64(timestamp);
        status_t result =remote()->transact(SET_TIMESTEAP, data, &reply);
        if (result != NO_ERROR) {
            return result;
        }
        result = reply.readInt32();
        return result;
    }
};

IMPLEMENT_META_INTERFACE(SurfaceTexture, "android.gui.SurfaceTexture");

// ----------------------------------------------------------------------

status_t BnSurfaceTexture::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case REQUEST_BUFFER: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            int bufferIdx   = data.readInt32();
            sp<GraphicBuffer> buffer;
            int result = requestBuffer(bufferIdx, &buffer);
            reply->writeInt32(buffer != 0);
            if (buffer != 0) {
                reply->write(*buffer);
            }
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_BUFFER_COUNT: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            int bufferCount = data.readInt32();
            int result = setBufferCount(bufferCount);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case DEQUEUE_BUFFER: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            uint32_t w      = data.readInt32();
            uint32_t h      = data.readInt32();
            uint32_t format = data.readInt32();
            uint32_t usage  = data.readInt32();
            int buf;
            sp<Fence> fence;
            int result = dequeueBuffer(&buf, fence, w, h, format, usage);
            bool hasFence = fence.get() && fence->isValid();
            reply->writeInt32(buf);
            reply->writeInt32(hasFence);
            if (hasFence) {
                reply->write(*fence.get());
            }
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case QUEUE_BUFFER: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            int buf = data.readInt32();
            QueueBufferInput input(data);
            QueueBufferOutput* const output =
                    reinterpret_cast<QueueBufferOutput *>(
                            reply->writeInplace(sizeof(QueueBufferOutput)));
            status_t result = queueBuffer(buf, input, output);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case CANCEL_BUFFER: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            int buf = data.readInt32();
            sp<Fence> fence;
            bool hasFence = data.readInt32();
            if (hasFence) {
                fence = new Fence();
                data.read(*fence.get());
            }
            cancelBuffer(buf, fence);
            return NO_ERROR;
        } break;
        case QUERY: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            int value;
            int what = data.readInt32();
            int res = query(what, &value);
            reply->writeInt32(value);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case SET_SYNCHRONOUS_MODE: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            bool enabled = data.readInt32();
            status_t res = setSynchronousMode(enabled);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case CONNECT: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            int api = data.readInt32();
            QueueBufferOutput* const output =
                    reinterpret_cast<QueueBufferOutput *>(
                            reply->writeInplace(sizeof(QueueBufferOutput)));
            status_t res = connect(api, output);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case DISCONNECT: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            int api = data.readInt32();
            status_t res = disconnect(api);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
		case SET_CROP: {
            Rect reg;
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            reg.left = data.readFloat();
            reg.top = data.readFloat();
            reg.right = data.readFloat();
            reg.bottom = data.readFloat();
            status_t result = setCrop(reg);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_TRANSFORM: {
            uint32_t transform;
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            transform = data.readInt32();
            status_t result = setCurrentTransform(transform);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_SCALINGMODE: {
            uint32_t scalingmode;
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            scalingmode = data.readInt32();
            status_t result = setCurrentScalingMode(scalingmode);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_TIMESTEAP: {
            uint32_t timestamp;
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            timestamp = data.readInt64();
            status_t result = setTimestamp(timestamp);
            reply->writeInt32(result);
            return NO_ERROR;
        } break;
        case SET_PARAMETER: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            uint32_t cmd    = (uint32_t)data.readInt32();
            uint32_t value;
           	if(cmd == HWC_LAYER_SETINITPARA)
	        {
	        	layerinitpara_t  layer_info;
	        	
	        	data.read((void *)&layer_info,sizeof(layerinitpara_t));
	        	
	        	value = (uint32_t)&layer_info;
	        }
	        else if(cmd == HWC_LAYER_SETFRAMEPARA)
	        {
	        	libhwclayerpara_t  frame_info;
	        	
	        	data.read((void *)&frame_info,sizeof(libhwclayerpara_t));
	        	
	        	value = (uint32_t)&frame_info;
	        }
	        else if(cmd == HWC_LAYER_SET3DMODE)
	        {
	        	video3Dinfo_t _3d_info;
	        	data.read((void *)&_3d_info, sizeof(video3Dinfo_t));
	        	value = (uint32_t)&_3d_info;
	        }
	        else
	        {
	        	value    = (uint32_t)data.readInt32();
	        }
            int res = setParameter(cmd,value);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case GET_PARAMETER: {
            CHECK_INTERFACE(ISurfaceTexture, data, reply);
            uint32_t cmd    = (uint32_t)data.readInt32();
            uint32_t res = getParameter(cmd);
            reply->writeInt32((int32_t)res);
            return NO_ERROR;
        } break;
    }
    return BBinder::onTransact(code, data, reply, flags);
}

// ----------------------------------------------------------------------------

static bool isValid(const sp<Fence>& fence) {
    return fence.get() && fence->isValid();
}

ISurfaceTexture::QueueBufferInput::QueueBufferInput(const Parcel& parcel) {
    parcel.read(*this);
}

size_t ISurfaceTexture::QueueBufferInput::getFlattenedSize() const
{
    return sizeof(timestamp)
         + sizeof(crop)
         + sizeof(scalingMode)
         + sizeof(transform)
         + sizeof(bool)
         + (isValid(fence) ? fence->getFlattenedSize() : 0);
}

size_t ISurfaceTexture::QueueBufferInput::getFdCount() const
{
    return isValid(fence) ? fence->getFdCount() : 0;
}

status_t ISurfaceTexture::QueueBufferInput::flatten(void* buffer, size_t size,
        int fds[], size_t count) const
{
    status_t err = NO_ERROR;
    bool haveFence = isValid(fence);
    char* p = (char*)buffer;
    memcpy(p, &timestamp,   sizeof(timestamp));   p += sizeof(timestamp);
    memcpy(p, &crop,        sizeof(crop));        p += sizeof(crop);
    memcpy(p, &scalingMode, sizeof(scalingMode)); p += sizeof(scalingMode);
    memcpy(p, &transform,   sizeof(transform));   p += sizeof(transform);
    memcpy(p, &haveFence,   sizeof(haveFence));   p += sizeof(haveFence);
    if (haveFence) {
        err = fence->flatten(p, size - (p - (char*)buffer), fds, count);
    }
    return err;
}

status_t ISurfaceTexture::QueueBufferInput::unflatten(void const* buffer,
        size_t size, int fds[], size_t count)
{
    status_t err = NO_ERROR;
    bool haveFence;
    const char* p = (const char*)buffer;
    memcpy(&timestamp,   p, sizeof(timestamp));   p += sizeof(timestamp);
    memcpy(&crop,        p, sizeof(crop));        p += sizeof(crop);
    memcpy(&scalingMode, p, sizeof(scalingMode)); p += sizeof(scalingMode);
    memcpy(&transform,   p, sizeof(transform));   p += sizeof(transform);
    memcpy(&haveFence,   p, sizeof(haveFence));   p += sizeof(haveFence);
    if (haveFence) {
        fence = new Fence();
        err = fence->unflatten(p, size - (p - (const char*)buffer), fds, count);
    }
    return err;
}

}; // namespace android
