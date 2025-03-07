/*
 * Implementation of the SmartTee filter
 *
 * Copyright 2015 Damjan Jovanovic
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct {
    struct strmbase_filter filter;
    BaseInputPin sink;
    struct strmbase_source capture, preview;
} SmartTeeFilter;

static inline SmartTeeFilter *impl_from_strmbase_filter(struct strmbase_filter *filter)
{
    return CONTAINING_RECORD(filter, SmartTeeFilter, filter);
}

static inline SmartTeeFilter *impl_from_strmbase_pin(struct strmbase_pin *pin)
{
    return impl_from_strmbase_filter(pin->filter);
}

static const IBaseFilterVtbl SmartTeeFilterVtbl = {
    BaseFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    BaseFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    BaseFilterImpl_Stop,
    BaseFilterImpl_Pause,
    BaseFilterImpl_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static IPin *smart_tee_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    SmartTeeFilter *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin.IPin_iface;
    else if (index == 1)
        return &filter->capture.pin.IPin_iface;
    else if (index == 2)
        return &filter->preview.pin.IPin_iface;
    return NULL;
}

static void smart_tee_destroy(struct strmbase_filter *iface)
{
    SmartTeeFilter *filter = impl_from_strmbase_filter(iface);

    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->capture);
    strmbase_source_cleanup(&filter->preview);
    strmbase_filter_cleanup(&filter->filter);
    CoTaskMemFree(filter);
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = smart_tee_get_pin,
    .filter_destroy = smart_tee_destroy,
};

static const IPinVtbl SmartTeeFilterInputVtbl = {
    BasePinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseInputPinImpl_Connect,
    BaseInputPinImpl_ReceiveConnection,
    BasePinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseInputPinImpl_EndOfStream,
    BaseInputPinImpl_BeginFlush,
    BaseInputPinImpl_EndFlush,
    BaseInputPinImpl_NewSegment
};

static HRESULT sink_query_accept(struct strmbase_pin *base, const AM_MEDIA_TYPE *pmt)
{
    SmartTeeFilter *This = impl_from_strmbase_pin(base);
    TRACE("(%p, AM_MEDIA_TYPE(%p))\n", This, pmt);
    if (!pmt)
        return VFW_E_TYPE_NOT_ACCEPTED;
    /* We'll take any media type, but the output pins will later
     * struggle to connect downstream. */
    return S_OK;
}

static HRESULT sink_get_media_type(struct strmbase_pin *base,
        unsigned int iPosition, AM_MEDIA_TYPE *amt)
{
    SmartTeeFilter *This = impl_from_strmbase_pin(base);
    HRESULT hr;
    TRACE("(%p)->(%d, %p)\n", This, iPosition, amt);
    if (iPosition)
        return S_FALSE;
    EnterCriticalSection(&This->filter.csFilter);
    if (This->sink.pin.peer)
    {
        CopyMediaType(amt, &This->sink.pin.mtCurrent);
        hr = S_OK;
    }
    else
        hr = S_FALSE;
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static HRESULT sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    SmartTeeFilter *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT copy_sample(IMediaSample *inputSample, IMemAllocator *allocator, IMediaSample **pOutputSample)
{
    REFERENCE_TIME startTime, endTime;
    BOOL haveStartTime = TRUE, haveEndTime = TRUE;
    IMediaSample *outputSample = NULL;
    BYTE *ptrIn, *ptrOut;
    AM_MEDIA_TYPE *mediaType = NULL;
    HRESULT hr;

    hr = IMediaSample_GetTime(inputSample, &startTime, &endTime);
    if (hr == S_OK)
        ;
    else if (hr == VFW_S_NO_STOP_TIME)
        haveEndTime = FALSE;
    else if (hr == VFW_E_SAMPLE_TIME_NOT_SET)
        haveStartTime = haveEndTime = FALSE;
    else
        goto end;

    hr = IMemAllocator_GetBuffer(allocator, &outputSample,
            haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL, 0);
    if (FAILED(hr)) goto end;
    if (IMediaSample_GetSize(outputSample) < IMediaSample_GetActualDataLength(inputSample)) {
        ERR("insufficient space in sample\n");
        hr = VFW_E_BUFFER_OVERFLOW;
        goto end;
    }

    hr = IMediaSample_SetTime(outputSample, haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_GetPointer(inputSample, &ptrIn);
    if (FAILED(hr)) goto end;
    hr = IMediaSample_GetPointer(outputSample, &ptrOut);
    if (FAILED(hr)) goto end;
    memcpy(ptrOut, ptrIn, IMediaSample_GetActualDataLength(inputSample));
    IMediaSample_SetActualDataLength(outputSample, IMediaSample_GetActualDataLength(inputSample));

    hr = IMediaSample_SetDiscontinuity(outputSample, IMediaSample_IsDiscontinuity(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

    haveStartTime = haveEndTime = TRUE;
    hr = IMediaSample_GetMediaTime(inputSample, &startTime, &endTime);
    if (hr == S_OK)
        ;
    else if (hr == VFW_S_NO_STOP_TIME)
        haveEndTime = FALSE;
    else if (hr == VFW_E_MEDIA_TIME_NOT_SET)
        haveStartTime = haveEndTime = FALSE;
    else
        goto end;
    hr = IMediaSample_SetMediaTime(outputSample, haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_GetMediaType(inputSample, &mediaType);
    if (FAILED(hr)) goto end;
    if (hr == S_OK) {
        hr = IMediaSample_SetMediaType(outputSample, mediaType);
        if (FAILED(hr)) goto end;
    }

    hr = IMediaSample_SetPreroll(outputSample, IMediaSample_IsPreroll(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_SetSyncPoint(outputSample, IMediaSample_IsSyncPoint(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

end:
    if (mediaType)
        DeleteMediaType(mediaType);
    if (FAILED(hr) && outputSample) {
        IMediaSample_Release(outputSample);
        outputSample = NULL;
    }
    *pOutputSample = outputSample;
    return hr;
}

static HRESULT WINAPI SmartTeeFilterInput_Receive(BaseInputPin *base, IMediaSample *inputSample)
{
    SmartTeeFilter *This = impl_from_strmbase_pin(&base->pin);
    IMediaSample *captureSample = NULL;
    IMediaSample *previewSample = NULL;
    HRESULT hrCapture = VFW_E_NOT_CONNECTED, hrPreview = VFW_E_NOT_CONNECTED;

    TRACE("(%p)->(%p)\n", This, inputSample);

    /* Modifying the image coming out of one pin doesn't modify the image
     * coming out of the other. MSDN claims the filter doesn't copy,
     * but unless it somehow uses copy-on-write, I just don't see how
     * that's possible. */

    /* FIXME: we should ideally do each of these in a separate thread */
    EnterCriticalSection(&This->filter.csFilter);
    if (This->capture.pin.peer)
        hrCapture = copy_sample(inputSample, This->capture.pAllocator, &captureSample);
    LeaveCriticalSection(&This->filter.csFilter);
    if (SUCCEEDED(hrCapture))
        hrCapture = BaseOutputPinImpl_Deliver(&This->capture, captureSample);
    if (captureSample)
        IMediaSample_Release(captureSample);

    EnterCriticalSection(&This->filter.csFilter);
    if (This->preview.pin.peer)
        hrPreview = copy_sample(inputSample, This->preview.pAllocator, &previewSample);
    LeaveCriticalSection(&This->filter.csFilter);
    /* No timestamps on preview stream: */
    if (SUCCEEDED(hrPreview))
        hrPreview = IMediaSample_SetTime(previewSample, NULL, NULL);
    if (SUCCEEDED(hrPreview))
        hrPreview = BaseOutputPinImpl_Deliver(&This->preview, previewSample);
    if (previewSample)
        IMediaSample_Release(previewSample);

    /* FIXME: how to merge the HRESULTs from the 2 pins? */
    if (SUCCEEDED(hrCapture))
        return hrCapture;
    else
        return hrPreview;
}

static const BaseInputPinFuncTable SmartTeeFilterInputFuncs =
{
    .base.pin_query_accept = sink_query_accept,
    .base.pin_get_media_type = sink_get_media_type,
    .base.pin_query_interface = sink_query_interface,
    .pfnReceive = SmartTeeFilterInput_Receive,
};

static const IPinVtbl SmartTeeFilterCaptureVtbl = {
    BasePinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BaseOutputPinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static HRESULT capture_query_accept(struct strmbase_pin *base, const AM_MEDIA_TYPE *amt)
{
    FIXME("(%p) stub\n", base);
    return S_OK;
}

static HRESULT source_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    SmartTeeFilter *filter = impl_from_strmbase_pin(iface);
    HRESULT hr = S_OK;

    EnterCriticalSection(&filter->filter.csFilter);

    if (!filter->sink.pin.peer)
        hr = VFW_E_NOT_CONNECTED;
    else if (!index)
        CopyMediaType(mt, &filter->sink.pin.mtCurrent);
    else
        hr = VFW_S_NO_MORE_ITEMS;

    LeaveCriticalSection(&filter->filter.csFilter);
    return hr;
}

static HRESULT WINAPI SmartTeeFilterCapture_DecideAllocator(struct strmbase_source *base,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    SmartTeeFilter *This = impl_from_strmbase_pin(&base->pin);
    TRACE("(%p, %p, %p)\n", This, pPin, pAlloc);
    *pAlloc = This->sink.pAllocator;
    IMemAllocator_AddRef(This->sink.pAllocator);
    return IMemInputPin_NotifyAllocator(pPin, This->sink.pAllocator, TRUE);
}

static const struct strmbase_source_ops capture_ops =
{
    .base.pin_query_accept = capture_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = SmartTeeFilterCapture_DecideAllocator,
};

static const IPinVtbl SmartTeeFilterPreviewVtbl = {
    BasePinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BaseOutputPinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static HRESULT preview_query_accept(struct strmbase_pin *base, const AM_MEDIA_TYPE *amt)
{
    FIXME("(%p) stub\n", base);
    return S_OK;
}

static HRESULT WINAPI SmartTeeFilterPreview_DecideAllocator(struct strmbase_source *base,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    SmartTeeFilter *This = impl_from_strmbase_pin(&base->pin);
    TRACE("(%p, %p, %p)\n", This, pPin, pAlloc);
    *pAlloc = This->sink.pAllocator;
    IMemAllocator_AddRef(This->sink.pAllocator);
    return IMemInputPin_NotifyAllocator(pPin, This->sink.pAllocator, TRUE);
}

static const struct strmbase_source_ops preview_ops =
{
    .base.pin_query_accept = preview_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = SmartTeeFilterPreview_DecideAllocator,
};

IUnknown* WINAPI QCAP_createSmartTeeFilter(IUnknown *outer, HRESULT *phr)
{
    static const WCHAR captureW[] = {'C','a','p','t','u','r','e',0};
    static const WCHAR previewW[] = {'P','r','e','v','i','e','w',0};
    static const WCHAR inputW[] = {'I','n','p','u','t',0};
    SmartTeeFilter *object;
    HRESULT hr;

    if (!(object = CoTaskMemAlloc(sizeof(*object))))
    {
        *phr = E_OUTOFMEMORY;
        return NULL;
    }
    memset(object, 0, sizeof(*object));

    strmbase_filter_init(&object->filter, &SmartTeeFilterVtbl, outer, &CLSID_SmartTee, &filter_ops);
    strmbase_sink_init(&object->sink, &SmartTeeFilterInputVtbl, &object->filter,
            inputW, &SmartTeeFilterInputFuncs, NULL);
    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void **)&object->sink.pAllocator);
    if (FAILED(hr))
    {
        *phr = hr;
        strmbase_filter_cleanup(&object->filter);
        return NULL;
    }

    strmbase_source_init(&object->capture, &SmartTeeFilterCaptureVtbl,
            &object->filter, captureW, &capture_ops);
    strmbase_source_init(&object->preview, &SmartTeeFilterPreviewVtbl,
            &object->filter, previewW, &preview_ops);

    *phr = S_OK;
    return &object->filter.IUnknown_inner;
}
