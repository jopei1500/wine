/*
 * Video Mixing Renderer 7 unit tests
 *
 * Copyright 2018 Zebediah Figura
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

#define COBJMACROS
#include "dshow.h"
#include "d3d9.h"
#include "vmr9.h"
#include "wine/strmbase.h"
#include "wine/test.h"

static IBaseFilter *create_vmr7(DWORD mode)
{
    IBaseFilter *filter = NULL;
    IVMRFilterConfig *config;
    HRESULT hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    if (mode)
    {
        hr = IBaseFilter_QueryInterface(filter, &IID_IVMRFilterConfig, (void **)&config);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IVMRFilterConfig_SetRenderingMode(config, mode);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        IVMRFilterConfig_Release(config);
    }
    return filter;
}

static HRESULT set_mixing_mode(IBaseFilter *filter)
{
    IVMRFilterConfig *config;
    HRESULT hr;

    hr = IBaseFilter_QueryInterface(filter, &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_SetNumberOfStreams(config, 2);
    todo_wine ok(hr == VFW_E_DDRAW_CAPS_NOT_SUITABLE || hr == S_OK, "Got hr %#x.\n", hr);

    IVMRFilterConfig_Release(config);
    return hr;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
        && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static IFilterGraph2 *create_graph(void)
{
    IFilterGraph2 *ret;
    HRESULT hr;
    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterGraph2, (void **)&ret);
    ok(hr == S_OK, "Failed to create FilterGraph: %#x\n", hr);
    return ret;
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

static void test_filter_config(void)
{
    IVMRFilterConfig *config;
    DWORD count, mode;
    HRESULT hr;
    ULONG ref;

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMRMode_Windowed, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowed);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMRMode_Windowed, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowed);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowless);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMRMode_Windowless, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowed);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Renderless);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(mode == VMRMode_Renderless, "Got mode %#x.\n", mode);

    hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowless);
    ok(hr == VFW_E_WRONG_STATE, "Got hr %#x.\n", hr);

    ref = IVMRFilterConfig_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER,
            &IID_IVMRFilterConfig, (void **)&config);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_GetNumberOfStreams(config, &count);
    todo_wine ok(hr == VFW_E_VMR_NOT_IN_MIXER_MODE, "Got hr %#x.\n", hr);

    hr = IVMRFilterConfig_SetNumberOfStreams(config, 3);
    if (hr != VFW_E_DDRAW_CAPS_NOT_SUITABLE)
    {
        todo_wine ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IVMRFilterConfig_GetNumberOfStreams(config, &count);
        todo_wine {
            ok(hr == S_OK, "Got hr %#x.\n", hr);
            ok(count == 3, "Got count %u.\n", count);
        }

        hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(mode == VMRMode_Windowed, "Got mode %#x.\n", mode);

        /* Despite MSDN, you can still change the rendering mode after setting the
         * stream count. */
        hr = IVMRFilterConfig_SetRenderingMode(config, VMRMode_Windowless);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IVMRFilterConfig_GetRenderingMode(config, &mode);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(mode == VMRMode_Windowless, "Got mode %#x.\n", mode);

        hr = IVMRFilterConfig_GetNumberOfStreams(config, &count);
        todo_wine {
            ok(hr == S_OK, "Got hr %#x.\n", hr);
            ok(count == 3, "Got count %u.\n", count);
        }
    }
    else
        skip("Mixing mode is not supported.\n");

    ref = IVMRFilterConfig_Release(config);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#x, expected %#x.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static void test_interfaces(void)
{
    WCHAR sink_id[] = {'V','M','R',' ','I','n','p','u','t','0',0};
    IBaseFilter *filter = create_vmr7(0);
    ULONG ref;
    IPin *pin;

    check_interface(filter, &IID_IAMCertifiedOutputProtection, TRUE);
    check_interface(filter, &IID_IAMFilterMiscFlags, TRUE);
    check_interface(filter, &IID_IBaseFilter, TRUE);
    check_interface(filter, &IID_IBasicVideo, TRUE);
    todo_wine check_interface(filter, &IID_IBasicVideo2, TRUE);
    todo_wine check_interface(filter, &IID_IKsPropertySet, TRUE);
    check_interface(filter, &IID_IMediaFilter, TRUE);
    check_interface(filter, &IID_IMediaPosition, TRUE);
    check_interface(filter, &IID_IMediaSeeking, TRUE);
    check_interface(filter, &IID_IPersist, TRUE);
    check_interface(filter, &IID_IQualityControl, TRUE);
    todo_wine check_interface(filter, &IID_IQualProp, TRUE);
    check_interface(filter, &IID_IUnknown, TRUE);
    check_interface(filter, &IID_IVideoWindow, TRUE);
    todo_wine check_interface(filter, &IID_IVMRAspectRatioControl, TRUE);
    todo_wine check_interface(filter, &IID_IVMRDeinterlaceControl, TRUE);
    check_interface(filter, &IID_IVMRFilterConfig, TRUE);
    todo_wine check_interface(filter, &IID_IVMRMixerBitmap, TRUE);
    check_interface(filter, &IID_IVMRMonitorConfig, TRUE);

    check_interface(filter, &IID_IBasicAudio, FALSE);
    check_interface(filter, &IID_IDirectDrawVideo, FALSE);
    check_interface(filter, &IID_IPersistPropertyBag, FALSE);
    check_interface(filter, &IID_IPin, FALSE);
    check_interface(filter, &IID_IReferenceClock, FALSE);
    check_interface(filter, &IID_IVMRAspectRatioControl9, FALSE);
    check_interface(filter, &IID_IVMRDeinterlaceControl9, FALSE);
    todo_wine check_interface(filter, &IID_IVMRFilterConfig9, FALSE);
    check_interface(filter, &IID_IVMRMixerBitmap9, FALSE);
    check_interface(filter, &IID_IVMRMixerControl, FALSE);
    check_interface(filter, &IID_IVMRMixerControl9, FALSE);
    todo_wine check_interface(filter, &IID_IVMRMonitorConfig9, FALSE);
    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify, FALSE);
    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify9, FALSE);
    check_interface(filter, &IID_IVMRWindowlessControl, FALSE);
    check_interface(filter, &IID_IVMRWindowlessControl9, FALSE);

    IBaseFilter_FindPin(filter, sink_id, &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IOverlay, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    IBaseFilter_Release(filter);
    filter = create_vmr7(VMRMode_Windowless);

    check_interface(filter, &IID_IVMRMonitorConfig, TRUE);
    check_interface(filter, &IID_IVMRWindowlessControl, TRUE);

    todo_wine check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IBasicVideo2, FALSE);
    todo_wine check_interface(filter, &IID_IVideoWindow, FALSE);
    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify, FALSE);
    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify9, FALSE);
    check_interface(filter, &IID_IVMRMixerControl, FALSE);
    check_interface(filter, &IID_IVMRMixerControl9, FALSE);
    todo_wine check_interface(filter, &IID_IVMRMonitorConfig9, FALSE);
    todo_wine check_interface(filter, &IID_IVMRWindowlessControl9, FALSE);

    IBaseFilter_FindPin(filter, sink_id, &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IOverlay, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    IBaseFilter_Release(filter);
    filter = create_vmr7(VMRMode_Renderless);

    check_interface(filter, &IID_IVMRSurfaceAllocatorNotify, TRUE);

    todo_wine check_interface(filter, &IID_IBasicVideo, FALSE);
    check_interface(filter, &IID_IBasicVideo2, FALSE);
    todo_wine check_interface(filter, &IID_IVideoWindow, FALSE);
    check_interface(filter, &IID_IVMRMixerControl, FALSE);
    todo_wine check_interface(filter, &IID_IVMRMonitorConfig, FALSE);
    todo_wine check_interface(filter, &IID_IVMRMonitorConfig9, FALSE);
    todo_wine check_interface(filter, &IID_IVMRSurfaceAllocatorNotify9, FALSE);
    check_interface(filter, &IID_IVMRWindowlessControl, FALSE);
    check_interface(filter, &IID_IVMRWindowlessControl9, FALSE);

    IBaseFilter_FindPin(filter, sink_id, &pin);

    check_interface(pin, &IID_IMemInputPin, TRUE);
    check_interface(pin, &IID_IOverlay, TRUE);
    check_interface(pin, &IID_IPin, TRUE);
    todo_wine check_interface(pin, &IID_IQualityControl, TRUE);
    check_interface(pin, &IID_IUnknown, TRUE);

    check_interface(pin, &IID_IKsPropertySet, FALSE);
    check_interface(pin, &IID_IMediaPosition, FALSE);
    check_interface(pin, &IID_IMediaSeeking, FALSE);

    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static const GUID test_iid = {0x33333333};
static LONG outer_ref = 1;

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IBaseFilter)
            || IsEqualGUID(iid, &test_iid))
    {
        *out = (IUnknown *)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&outer_ref);
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return InterlockedDecrement(&outer_ref);
}

static const IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown test_outer = {&outer_vtbl};

static void test_aggregation(void)
{
    IBaseFilter *filter, *filter2;
    IUnknown *unk, *unk2;
    HRESULT hr;
    ULONG ref;

    filter = (IBaseFilter *)0xdeadbeef;
    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!filter, "Got interface %p.\n", filter);

    hr = CoCreateInstance(&CLSID_VideoMixingRenderer, &test_outer, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
    ok(unk != &test_outer, "Returned IUnknown should not be outer IUnknown.\n");
    ref = get_refcount(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    ref = IUnknown_AddRef(unk);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    ref = IUnknown_Release(unk);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == unk, "Got unexpected IUnknown %p.\n", unk2);
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_QueryInterface(filter, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &IID_IBaseFilter, (void **)&filter2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(filter2 == (IBaseFilter *)0xdeadbeef, "Got unexpected IBaseFilter %p.\n", filter2);

    hr = IUnknown_QueryInterface(unk, &test_iid, (void **)&unk2);
    ok(hr == E_NOINTERFACE, "Got hr %#x.\n", hr);
    ok(!unk2, "Got unexpected IUnknown %p.\n", unk2);

    hr = IBaseFilter_QueryInterface(filter, &test_iid, (void **)&unk2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(unk2 == (IUnknown *)0xdeadbeef, "Got unexpected IUnknown %p.\n", unk2);

    IBaseFilter_Release(filter);
    ref = IUnknown_Release(unk);
    ok(!ref, "Got unexpected refcount %d.\n", ref);
    ok(outer_ref == 1, "Got unexpected refcount %d.\n", outer_ref);
}

static void test_enum_pins(void)
{
    IBaseFilter *filter = create_vmr7(0);
    IEnumPins *enum1, *enum2;
    ULONG count, ref;
    IPin *pins[3];
    HRESULT hr;

    ref = get_refcount(filter);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IBaseFilter_EnumPins(filter, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IBaseFilter_EnumPins(filter, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, NULL, NULL);
    ok(hr == E_POINTER, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ref = get_refcount(filter);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(pins[0]);
    ok(ref == 3, "Got unexpected refcount %d.\n", ref);
    ref = get_refcount(enum1);
    ok(ref == 1, "Got unexpected refcount %d.\n", ref);
    IPin_Release(pins[0]);
    ref = get_refcount(filter);
    ok(ref == 2, "Got unexpected refcount %d.\n", ref);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Next(enum1, 1, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 2, pins, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(count == 1, "Got count %u.\n", count);
    IPin_Release(pins[0]);

    hr = IEnumPins_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 2);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumPins_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum1, 1, pins, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumPins_Next(enum2, 1, pins, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    IPin_Release(pins[0]);

    IEnumPins_Release(enum2);

    if (SUCCEEDED(set_mixing_mode(filter)))
    {
        hr = IEnumPins_Next(enum1, 1, pins, NULL);
        todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);

        hr = IEnumPins_Reset(enum1);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IEnumPins_Next(enum1, 1, pins, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        IPin_Release(pins[0]);

        hr = IEnumPins_Next(enum1, 1, pins, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        IPin_Release(pins[0]);

        hr = IEnumPins_Next(enum1, 1, pins, NULL);
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);

        hr = IEnumPins_Reset(enum1);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IEnumPins_Next(enum1, 2, pins, &count);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(count == 2, "Got count %u.\n", count);
        IPin_Release(pins[0]);
        IPin_Release(pins[1]);

        hr = IEnumPins_Reset(enum1);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IEnumPins_Next(enum1, 3, pins, &count);
        ok(hr == S_FALSE, "Got hr %#x.\n", hr);
        ok(count == 2, "Got count %u.\n", count);
        IPin_Release(pins[0]);
        IPin_Release(pins[1]);
    }
    else
        skip("Mixing mode is not supported.\n");

    IEnumPins_Release(enum1);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_find_pin(void)
{
    static const WCHAR input_pinW[] = {'i','n','p','u','t',' ','p','i','n',0};
    static const WCHAR inW[] = {'I','n',0};

    WCHAR sink_id[] = {'V','M','R',' ','I','n','p','u','t','0',0};
    IBaseFilter *filter = create_vmr7(0);
    IEnumPins *enum_pins;
    IPin *pin, *pin2;
    HRESULT hr;
    ULONG ref;

    IBaseFilter_EnumPins(filter, &enum_pins);

    hr = IBaseFilter_FindPin(filter, input_pinW, &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, inW, &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    hr = IBaseFilter_FindPin(filter, sink_id, &pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(pin == pin2, "Pins did not match.\n");
    IPin_Release(pin);
    IPin_Release(pin2);

    sink_id[9] = '1';
    hr = IBaseFilter_FindPin(filter, sink_id, &pin);
    ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);

    if (SUCCEEDED(set_mixing_mode(filter)))
    {
        IEnumPins_Reset(enum_pins);

        sink_id[9] = '0';
        hr = IBaseFilter_FindPin(filter, sink_id, &pin);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(pin == pin2, "Pins did not match.\n");
        IPin_Release(pin);
        IPin_Release(pin2);

        sink_id[9] = '1';
        hr = IBaseFilter_FindPin(filter, sink_id, &pin);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IEnumPins_Next(enum_pins, 1, &pin2, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(pin == pin2, "Pins did not match.\n");
        IPin_Release(pin);
        IPin_Release(pin2);

        sink_id[9] = '2';
        hr = IBaseFilter_FindPin(filter, sink_id, &pin);
        ok(hr == VFW_E_NOT_FOUND, "Got hr %#x.\n", hr);
    }
    else
        skip("Mixing mode is not supported.\n");

    IEnumPins_Release(enum_pins);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_pin_info(void)
{
    WCHAR sink_id[] = {'V','M','R',' ','I','n','p','u','t','0',0};
    IBaseFilter *filter = create_vmr7(0);
    PIN_DIRECTION dir;
    ULONG count, ref;
    PIN_INFO info;
    HRESULT hr;
    WCHAR *id;
    IPin *pin;

    IBaseFilter_FindPin(filter, sink_id, &pin);
    hr = IPin_QueryPinInfo(pin, &info);
    ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
    ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
    ok(!lstrcmpW(info.achName, sink_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
    IBaseFilter_Release(info.pFilter);

    hr = IPin_QueryDirection(pin, &dir);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

    hr = IPin_QueryId(pin, &id);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(!lstrcmpW(id, sink_id), "Got id %s.\n", wine_dbgstr_w(id));
    CoTaskMemFree(id);

    hr = IPin_QueryInternalConnections(pin, NULL, &count);
    ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

    IPin_Release(pin);

    if (SUCCEEDED(set_mixing_mode(filter)))
    {
        sink_id[9] = '1';

        IBaseFilter_FindPin(filter, sink_id, &pin);
        hr = IPin_QueryPinInfo(pin, &info);
        ok(info.pFilter == filter, "Expected filter %p, got %p.\n", filter, info.pFilter);
        ok(info.dir == PINDIR_INPUT, "Got direction %d.\n", info.dir);
        ok(!lstrcmpW(info.achName, sink_id), "Got name %s.\n", wine_dbgstr_w(info.achName));
        IBaseFilter_Release(info.pFilter);

        hr = IPin_QueryDirection(pin, &dir);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(dir == PINDIR_INPUT, "Got direction %d.\n", dir);

        hr = IPin_QueryId(pin, &id);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(!lstrcmpW(id, sink_id), "Got id %s.\n", wine_dbgstr_w(id));
        CoTaskMemFree(id);

        hr = IPin_QueryInternalConnections(pin, NULL, &count);
        ok(hr == E_NOTIMPL, "Got hr %#x.\n", hr);

        IPin_Release(pin);
    }
    else
        skip("Mixing mode is not supported.\n");

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_media_types(void)
{
    WCHAR sink_id[] = {'V','M','R',' ','I','n','p','u','t','0',0};
    IBaseFilter *filter = create_vmr7(0);
    AM_MEDIA_TYPE *mt, req_mt = {{0}};
    VIDEOINFOHEADER vih =
    {
        {0}, {0}, 0, 0, 0,
        {sizeof(BITMAPINFOHEADER), 32, 24, 1, 0, BI_RGB}
    };
    IEnumMediaTypes *enummt;
    unsigned int i;
    HRESULT hr;
    ULONG ref;
    IPin *pin;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_NULL,
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
    };

    IBaseFilter_FindPin(filter, sink_id, &pin);

    hr = IPin_EnumMediaTypes(pin, &enummt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enummt, 1, &mt, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enummt);

    req_mt.majortype = MEDIATYPE_Video;
    req_mt.formattype = FORMAT_VideoInfo;
    req_mt.cbFormat = sizeof(VIDEOINFOHEADER);
    req_mt.pbFormat = (BYTE *)&vih;

    for (i = 0; i < ARRAY_SIZE(subtype_tests); ++i)
    {
        req_mt.subtype = *subtype_tests[i];
        hr = IPin_QueryAccept(pin, &req_mt);
        ok(hr == S_OK, "Got hr %#x for subtype %s.\n", hr, wine_dbgstr_guid(subtype_tests[i]));
    }

    req_mt.subtype = MEDIASUBTYPE_RGB8;
    hr = IPin_QueryAccept(pin, &req_mt);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    req_mt.subtype = MEDIASUBTYPE_RGB24;

    req_mt.majortype = MEDIATYPE_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    req_mt.majortype = MEDIATYPE_Video;

    req_mt.formattype = FORMAT_None;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    req_mt.formattype = GUID_NULL;
    hr = IPin_QueryAccept(pin, &req_mt);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_enum_media_types(void)
{
    WCHAR sink_id[] = {'V','M','R',' ','I','n','p','u','t','0',0};
    IBaseFilter *filter = create_vmr7(0);
    IEnumMediaTypes *enum1, *enum2;
    AM_MEDIA_TYPE *mts[2];
    ULONG ref, count;
    HRESULT hr;
    IPin *pin;

    IBaseFilter_FindPin(filter, sink_id, &pin);

    hr = IPin_EnumMediaTypes(pin, &enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, &count);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(!count, "Got count %u.\n", count);

    hr = IEnumMediaTypes_Reset(enum1);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum1, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Clone(enum1, &enum2);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Skip(enum1, 1);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    hr = IEnumMediaTypes_Next(enum2, 1, mts, NULL);
    ok(hr == S_FALSE, "Got hr %#x.\n", hr);

    IEnumMediaTypes_Release(enum1);
    IEnumMediaTypes_Release(enum2);
    IPin_Release(pin);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_unconnected_filter_state(void)
{
    IBaseFilter *filter = create_vmr7(0);
    FILTER_STATE state;
    HRESULT hr;
    ULONG ref;

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Pause(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Paused, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    hr = IBaseFilter_Run(filter, 0);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Running, "Got state %u.\n", state);

    hr = IBaseFilter_Stop(filter);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IBaseFilter_GetState(filter, 0, &state);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(state == State_Stopped, "Got state %u.\n", state);

    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

struct testfilter
{
    struct strmbase_filter filter;
    struct strmbase_source source;
};

static const IBaseFilterVtbl testfilter_vtbl =
{
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
    BaseFilterImpl_QueryVendorInfo,
};

static inline struct testfilter *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct testfilter, filter);
}

static IPin *testfilter_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    if (!index)
        return &filter->source.pin.IPin_iface;
    return NULL;
}

static void testfilter_destroy(struct strmbase_filter *iface)
{
    struct testfilter *filter = impl_from_strmbase_filter(iface);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
}

static const struct strmbase_filter_ops testfilter_ops =
{
    .filter_get_pin = testfilter_get_pin,
    .filter_destroy = testfilter_destroy,
};

static const IPinVtbl testsource_vtbl =
{
    BasePinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BasePinImpl_Disconnect,
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
    BasePinImpl_NewSegment,
};

static HRESULT testsource_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    return S_OK;
}

static HRESULT WINAPI testsource_AttemptConnection(struct strmbase_source *iface,
        IPin *peer, const AM_MEDIA_TYPE *mt)
{
    HRESULT hr;

    iface->pin.peer = peer;
    IPin_AddRef(peer);
    CopyMediaType(&iface->pin.mtCurrent, mt);

    if (FAILED(hr = IPin_ReceiveConnection(peer, &iface->pin.IPin_iface, mt)))
    {
        ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
        IPin_Release(peer);
        iface->pin.peer = NULL;
        FreeMediaType(&iface->pin.mtCurrent);
    }

    return hr;
}

static const struct strmbase_source_ops testsource_ops =
{
    .base.pin_query_accept = testsource_query_accept,
    .base.pin_get_media_type = strmbase_pin_get_media_type,
    .pfnAttemptConnection = testsource_AttemptConnection,
};

static void testfilter_init(struct testfilter *filter)
{
    static const GUID clsid = {0xabacab};
    strmbase_filter_init(&filter->filter, &testfilter_vtbl, NULL, &clsid, &testfilter_ops);
    strmbase_source_init(&filter->source, &testsource_vtbl, &filter->filter, L"", &testsource_ops);
}

static void test_connect_pin(void)
{
    VIDEOINFOHEADER vih =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 32,
        .bmiHeader.biHeight = 16,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biCompression = BI_RGB,
    };
    AM_MEDIA_TYPE req_mt =
    {
        .majortype = MEDIATYPE_Video,
        .subtype = MEDIASUBTYPE_WAVE,
        .formattype = FORMAT_VideoInfo,
        .cbFormat = sizeof(vih),
        .pbFormat = (BYTE *)&vih,
    };
    IBaseFilter *filter = create_vmr7(VMRMode_Windowed);
    IFilterGraph2 *graph = create_graph();
    struct testfilter source;
    AM_MEDIA_TYPE mt;
    IPin *pin, *peer;
    unsigned int i;
    HRESULT hr;
    ULONG ref;

    static const GUID *subtype_tests[] =
    {
        &MEDIASUBTYPE_RGB565,
        &MEDIASUBTYPE_RGB24,
        &MEDIASUBTYPE_RGB32,
        &MEDIASUBTYPE_WAVE,
    };

    testfilter_init(&source);

    IFilterGraph2_AddFilter(graph, &source.filter.IBaseFilter_iface, NULL);
    IFilterGraph2_AddFilter(graph, filter, NULL);

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);

    vih.bmiHeader.biBitCount = 16;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    todo_wine ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    if (hr == S_OK)
    {
        IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
        IFilterGraph2_Disconnect(graph, pin);
    }
    vih.bmiHeader.biBitCount = 32;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    if (hr == VFW_E_TYPE_NOT_ACCEPTED) /* w7u */
    {
        vih.bmiHeader.biBitCount = 24;
        hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    }
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(subtype_tests); ++i)
    {
        req_mt.subtype = *subtype_tests[i];
        hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
        ok(hr == S_OK, "Got hr %#x for subtype %s.\n", hr, wine_dbgstr_guid(subtype_tests[i]));

        hr = IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        hr = IFilterGraph2_Disconnect(graph, pin);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
    }

    req_mt.formattype = FORMAT_None;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    req_mt.formattype = GUID_NULL;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    req_mt.formattype = FORMAT_VideoInfo;

    req_mt.subtype = MEDIASUBTYPE_RGB8;
    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    todo_wine ok(hr == VFW_E_TYPE_NOT_ACCEPTED, "Got hr %#x.\n", hr);
    if (hr == S_OK)
    {
        IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);
        IFilterGraph2_Disconnect(graph, pin);
    }
    req_mt.subtype = MEDIASUBTYPE_RGB32;

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    hr = IFilterGraph2_ConnectDirect(graph, &source.source.pin.IPin_iface, pin, &req_mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(peer == &source.source.pin.IPin_iface, "Got peer %p.\n", peer);
    IPin_Release(peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    ok(compare_media_types(&mt, &req_mt), "Media types didn't match.\n");

    hr = IFilterGraph2_Disconnect(graph, pin);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    hr = IFilterGraph2_Disconnect(graph, pin);
    todo_wine ok(hr == S_FALSE, "Got hr %#x.\n", hr);
    ok(source.source.pin.peer == pin, "Got peer %p.\n", peer);
    IFilterGraph2_Disconnect(graph, &source.source.pin.IPin_iface);

    peer = (IPin *)0xdeadbeef;
    hr = IPin_ConnectedTo(pin, &peer);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);
    ok(!peer, "Got peer %p.\n", peer);

    hr = IPin_ConnectionMediaType(pin, &mt);
    ok(hr == VFW_E_NOT_CONNECTED, "Got hr %#x.\n", hr);

    IPin_Release(pin);
    ref = IFilterGraph2_Release(graph);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
    ref = IBaseFilter_Release(&source.filter.IBaseFilter_iface);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

static void test_overlay(void)
{
    IBaseFilter *filter = create_vmr7(0);
    IOverlay *overlay;
    HRESULT hr;
    ULONG ref;
    IPin *pin;
    HWND hwnd;

    IBaseFilter_FindPin(filter, L"VMR Input0", &pin);

    hr = IPin_QueryInterface(pin, &IID_IOverlay, (void **)&overlay);
    ok(hr == S_OK, "Got hr %#x.\n", hr);

    hwnd = (HWND)0xdeadbeef;
    hr = IOverlay_GetWindowHandle(overlay, &hwnd);
    ok(hr == S_OK, "Got hr %#x.\n", hr);
    todo_wine ok(hwnd && hwnd != (HWND)0xdeadbeef, "Got invalid window %p.\n", hwnd);

    IOverlay_Release(overlay);
    IPin_Release(pin);
    ref = IBaseFilter_Release(filter);
    ok(!ref, "Got outstanding refcount %d.\n", ref);
}

START_TEST(vmr7)
{
    CoInitialize(NULL);

    test_filter_config();
    test_interfaces();
    test_aggregation();
    test_enum_pins();
    test_find_pin();
    test_pin_info();
    test_media_types();
    test_enum_media_types();
    test_unconnected_filter_state();
    test_connect_pin();
    test_overlay();

    CoUninitialize();
}
