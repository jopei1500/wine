/*              DirectShow FilterGraph object (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2004 Christian Costa
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
#include "winuser.h"
#include "winreg.h"
#include "shlwapi.h"
#include "dshow.h"
#include "wine/debug.h"
#include "quartz_private.h"
#include "ole2.h"
#include "olectl.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "evcode.h"
#include "wine/heap.h"
#include "wine/list.h"


WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct {
    HWND     hWnd;      /* Target window */
    UINT     msg;       /* User window message */
    LONG_PTR instance;  /* User data */
    int      disabled;  /* Disabled messages posting */
} WndNotify;

typedef struct {
    LONG lEventCode;   /* Event code */
    LONG_PTR lParam1;  /* Param1 */
    LONG_PTR lParam2;  /* Param2 */
} Event;

/* messages ring implementation for queuing events (taken from winmm) */
#define EVENTS_RING_BUFFER_INCREMENT      64
typedef struct {
    Event* messages;
    int ring_buffer_size;
    int msg_tosave;
    int msg_toget;
    CRITICAL_SECTION msg_crst;
    HANDLE msg_event; /* Signaled for no empty queue */
} EventsQueue;

static int EventsQueue_Init(EventsQueue* omr)
{
    omr->msg_toget = 0;
    omr->msg_tosave = 0;
    omr->msg_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    omr->ring_buffer_size = EVENTS_RING_BUFFER_INCREMENT;
    omr->messages = CoTaskMemAlloc(omr->ring_buffer_size * sizeof(Event));
    ZeroMemory(omr->messages, omr->ring_buffer_size * sizeof(Event));

    InitializeCriticalSection(&omr->msg_crst);
    omr->msg_crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": EventsQueue.msg_crst");
    return TRUE;
}

static int EventsQueue_Destroy(EventsQueue* omr)
{
    CloseHandle(omr->msg_event);
    CoTaskMemFree(omr->messages);
    omr->msg_crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&omr->msg_crst);
    return TRUE;
}

static BOOL EventsQueue_PutEvent(EventsQueue* omr, const Event* evt)
{
    EnterCriticalSection(&omr->msg_crst);
    if (omr->msg_toget == ((omr->msg_tosave + 1) % omr->ring_buffer_size))
    {
	int old_ring_buffer_size = omr->ring_buffer_size;
	omr->ring_buffer_size += EVENTS_RING_BUFFER_INCREMENT;
	TRACE("omr->ring_buffer_size=%d\n",omr->ring_buffer_size);
	omr->messages = CoTaskMemRealloc(omr->messages, omr->ring_buffer_size * sizeof(Event));
	/* Now we need to rearrange the ring buffer so that the new
	   buffers just allocated are in between omr->msg_tosave and
	   omr->msg_toget.
	*/
	if (omr->msg_tosave < omr->msg_toget)
	{
	    memmove(&(omr->messages[omr->msg_toget + EVENTS_RING_BUFFER_INCREMENT]),
		    &(omr->messages[omr->msg_toget]),
		    sizeof(Event)*(old_ring_buffer_size - omr->msg_toget)
		    );
	    omr->msg_toget += EVENTS_RING_BUFFER_INCREMENT;
	}
    }
    omr->messages[omr->msg_tosave] = *evt;
    SetEvent(omr->msg_event);
    omr->msg_tosave = (omr->msg_tosave + 1) % omr->ring_buffer_size;
    LeaveCriticalSection(&omr->msg_crst);
    return TRUE;
}

static BOOL EventsQueue_GetEvent(EventsQueue* omr, Event* evt, LONG msTimeOut)
{
    if (WaitForSingleObject(omr->msg_event, msTimeOut) != WAIT_OBJECT_0)
	return FALSE;
	
    EnterCriticalSection(&omr->msg_crst);

    if (omr->msg_toget == omr->msg_tosave) /* buffer empty ? */
    {
        LeaveCriticalSection(&omr->msg_crst);
	return FALSE;
    }

    *evt = omr->messages[omr->msg_toget];
    omr->msg_toget = (omr->msg_toget + 1) % omr->ring_buffer_size;

    /* Mark the buffer as empty if needed */
    if (omr->msg_toget == omr->msg_tosave) /* buffer empty ? */
	ResetEvent(omr->msg_event);

    LeaveCriticalSection(&omr->msg_crst);
    return TRUE;
}

#define MAX_ITF_CACHE_ENTRIES 3
typedef struct _ITF_CACHE_ENTRY {
   const IID* riid;
   IBaseFilter* filter;
   IUnknown* iface;
} ITF_CACHE_ENTRY;

struct filter
{
    struct list entry;
    IBaseFilter *filter;
    WCHAR *name;
};

typedef struct _IFilterGraphImpl {
    IUnknown IUnknown_inner;
    IFilterGraph2 IFilterGraph2_iface;
    IMediaControl IMediaControl_iface;
    IMediaSeeking IMediaSeeking_iface;
    IBasicAudio IBasicAudio_iface;
    IBasicVideo2 IBasicVideo2_iface;
    IVideoWindow IVideoWindow_iface;
    IMediaEventEx IMediaEventEx_iface;
    IMediaFilter IMediaFilter_iface;
    IMediaEventSink IMediaEventSink_iface;
    IGraphConfig IGraphConfig_iface;
    IMediaPosition IMediaPosition_iface;
    IObjectWithSite IObjectWithSite_iface;
    IGraphVersion IGraphVersion_iface;
    /* IAMGraphStreams */
    /* IAMStats */
    /* IFilterChain */
    /* IFilterMapper2 */
    /* IQueueCommand */
    /* IRegisterServiceProvider */
    /* IResourceMananger */
    /* IServiceProvider */
    /* IVideoFrameStep */

    IUnknown *outer_unk;
    LONG ref;
    IUnknown *punkFilterMapper2;
    struct list filters;
    LONG nameIndex;
    IReferenceClock *refClock;
    IBaseFilter *refClockProvider;
    EventsQueue evqueue;
    HANDLE hEventCompletion;
    int CompletionStatus;
    WndNotify notif;
    int nRenderers;
    int EcCompleteCount;
    int HandleEcComplete;
    int HandleEcRepaint;
    int HandleEcClockChanged;
    OAFilterState state;
    CRITICAL_SECTION cs;
    ITF_CACHE_ENTRY ItfCacheEntries[MAX_ITF_CACHE_ENTRIES];
    int nItfCacheEntries;
    BOOL defaultclock;
    GUID timeformatseek;
    REFERENCE_TIME start_time;
    REFERENCE_TIME pause_time;
    LONGLONG stop_position;
    LONG recursioncount;
    IUnknown *pSite;
    LONG version;

    HANDLE message_thread, message_thread_ret;
    DWORD message_thread_id;
} IFilterGraphImpl;

struct enum_filters
{
    IEnumFilters IEnumFilters_iface;
    LONG ref;
    IFilterGraphImpl *graph;
    LONG version;
    struct list *cursor;
};

static HRESULT create_enum_filters(IFilterGraphImpl *graph, struct list *cursor, IEnumFilters **out);

static inline struct enum_filters *impl_from_IEnumFilters(IEnumFilters *iface)
{
    return CONTAINING_RECORD(iface, struct enum_filters, IEnumFilters_iface);
}

static HRESULT WINAPI EnumFilters_QueryInterface(IEnumFilters *iface, REFIID iid, void **out)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);
    TRACE("enum_filters %p, iid %s, out %p.\n", enum_filters, qzdebugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumFilters))
    {
        IEnumFilters_AddRef(*out = iface);
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", qzdebugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumFilters_AddRef(IEnumFilters *iface)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);
    ULONG ref = InterlockedIncrement(&enum_filters->ref);

    TRACE("%p increasing refcount to %u.\n", enum_filters, ref);

    return ref;
}

static ULONG WINAPI EnumFilters_Release(IEnumFilters *iface)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);
    ULONG ref = InterlockedDecrement(&enum_filters->ref);

    TRACE("%p decreasing refcount to %u.\n", enum_filters, ref);

    if (!ref)
    {
        IUnknown_Release(enum_filters->graph->outer_unk);
        heap_free(enum_filters);
    }

    return ref;
}

static HRESULT WINAPI EnumFilters_Next(IEnumFilters *iface, ULONG count,
        IBaseFilter **filters, ULONG *fetched)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);
    unsigned int i = 0;

    TRACE("enum_filters %p, count %u, filters %p, fetched %p.\n",
            enum_filters, count, filters, fetched);

    if (enum_filters->version != enum_filters->graph->version)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (!filters)
        return E_POINTER;

    for (i = 0; i < count; ++i)
    {
        struct filter *filter = LIST_ENTRY(enum_filters->cursor, struct filter, entry);

        if (!enum_filters->cursor)
            break;

        IBaseFilter_AddRef(filters[i] = filter->filter);
        enum_filters->cursor = list_next(&enum_filters->graph->filters, enum_filters->cursor);
    }

    if (fetched)
        *fetched = i;

    return (i == count) ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumFilters_Skip(IEnumFilters *iface, ULONG count)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);

    TRACE("enum_filters %p, count %u.\n", enum_filters, count);

    if (!enum_filters->cursor)
        return S_FALSE;

    while (count--)
    {
        if (!(enum_filters->cursor = list_next(&enum_filters->graph->filters, enum_filters->cursor)))
            return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI EnumFilters_Reset(IEnumFilters *iface)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);

    TRACE("enum_filters %p.\n", enum_filters);

    enum_filters->cursor = list_head(&enum_filters->graph->filters);
    enum_filters->version = enum_filters->graph->version;
    return S_OK;
}

static HRESULT WINAPI EnumFilters_Clone(IEnumFilters *iface, IEnumFilters **out)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);

    TRACE("enum_filters %p, out %p.\n", enum_filters, out);

    return create_enum_filters(enum_filters->graph, enum_filters->cursor, out);
}

static const IEnumFiltersVtbl EnumFilters_vtbl =
{
    EnumFilters_QueryInterface,
    EnumFilters_AddRef,
    EnumFilters_Release,
    EnumFilters_Next,
    EnumFilters_Skip,
    EnumFilters_Reset,
    EnumFilters_Clone,
};

static HRESULT create_enum_filters(IFilterGraphImpl *graph, struct list *cursor, IEnumFilters **out)
{
    struct enum_filters *enum_filters;

    if (!(enum_filters = heap_alloc(sizeof(*enum_filters))))
        return E_OUTOFMEMORY;

    enum_filters->IEnumFilters_iface.lpVtbl = &EnumFilters_vtbl;
    enum_filters->ref = 1;
    enum_filters->cursor = cursor;
    enum_filters->graph = graph;
    IUnknown_AddRef(graph->outer_unk);
    enum_filters->version = graph->version;

    *out = &enum_filters->IEnumFilters_iface;
    return S_OK;
}

static inline IFilterGraphImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IUnknown_inner);
}

static HRESULT WINAPI FilterGraphInner_QueryInterface(IUnknown *iface, REFIID riid, void **ppvObj)
{
    IFilterGraphImpl *This = impl_from_IUnknown(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObj);

    if (IsEqualGUID(&IID_IUnknown, riid)) {
        *ppvObj = &This->IUnknown_inner;
        TRACE("   returning IUnknown interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IFilterGraph, riid) ||
	IsEqualGUID(&IID_IFilterGraph2, riid) ||
	IsEqualGUID(&IID_IGraphBuilder, riid)) {
        *ppvObj = &This->IFilterGraph2_iface;
        TRACE("   returning IGraphBuilder interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaControl, riid)) {
        *ppvObj = &This->IMediaControl_iface;
        TRACE("   returning IMediaControl interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaSeeking, riid)) {
        *ppvObj = &This->IMediaSeeking_iface;
        TRACE("   returning IMediaSeeking interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IBasicAudio, riid)) {
        *ppvObj = &This->IBasicAudio_iface;
        TRACE("   returning IBasicAudio interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IBasicVideo, riid) ||
               IsEqualGUID(&IID_IBasicVideo2, riid)) {
        *ppvObj = &This->IBasicVideo2_iface;
        TRACE("   returning IBasicVideo2 interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IVideoWindow, riid)) {
        *ppvObj = &This->IVideoWindow_iface;
        TRACE("   returning IVideoWindow interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaEvent, riid) ||
	   IsEqualGUID(&IID_IMediaEventEx, riid)) {
        *ppvObj = &This->IMediaEventEx_iface;
        TRACE("   returning IMediaEvent(Ex) interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaFilter, riid) ||
          IsEqualGUID(&IID_IPersist, riid)) {
        *ppvObj = &This->IMediaFilter_iface;
        TRACE("   returning IMediaFilter interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaEventSink, riid)) {
        *ppvObj = &This->IMediaEventSink_iface;
        TRACE("   returning IMediaEventSink interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IGraphConfig, riid)) {
        *ppvObj = &This->IGraphConfig_iface;
        TRACE("   returning IGraphConfig interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaPosition, riid)) {
        *ppvObj = &This->IMediaPosition_iface;
        TRACE("   returning IMediaPosition interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IObjectWithSite, riid)) {
        *ppvObj = &This->IObjectWithSite_iface;
        TRACE("   returning IObjectWithSite interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IFilterMapper, riid)) {
        TRACE("   requesting IFilterMapper interface from aggregated filtermapper (%p)\n", *ppvObj);
        return IUnknown_QueryInterface(This->punkFilterMapper2, riid, ppvObj);
    } else if (IsEqualGUID(&IID_IFilterMapper2, riid)) {
        TRACE("   returning IFilterMapper2 interface from aggregated filtermapper (%p)\n", *ppvObj);
        return IUnknown_QueryInterface(This->punkFilterMapper2, riid, ppvObj);
    } else if (IsEqualGUID(&IID_IFilterMapper3, riid)) {
        TRACE("   returning IFilterMapper3 interface from aggregated filtermapper (%p)\n", *ppvObj);
        return IUnknown_QueryInterface(This->punkFilterMapper2, riid, ppvObj);
    } else if (IsEqualGUID(&IID_IGraphVersion, riid)) {
        *ppvObj = &This->IGraphVersion_iface;
        TRACE("   returning IGraphVersion interface (%p)\n", *ppvObj);
    } else {
        *ppvObj = NULL;
	FIXME("unknown interface %s\n", debugstr_guid(riid));
	return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppvObj);
    return S_OK;
}

static ULONG WINAPI FilterGraphInner_AddRef(IUnknown *iface)
{
    IFilterGraphImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI FilterGraphInner_Release(IUnknown *iface)
{
    IFilterGraphImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    struct filter *filter, *next;

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (ref == 0) {
        int i;

        This->ref = 1; /* guard against reentrancy (aggregation). */

        IMediaControl_Stop(&This->IMediaControl_iface);

        LIST_FOR_EACH_ENTRY_SAFE(filter, next, &This->filters, struct filter, entry)
        {
            IFilterGraph2_RemoveFilter(&This->IFilterGraph2_iface, filter->filter);
        }

        if (This->refClock)
            IReferenceClock_Release(This->refClock);

        for (i = 0; i < This->nItfCacheEntries; i++)
        {
            if (This->ItfCacheEntries[i].iface)
                IUnknown_Release(This->ItfCacheEntries[i].iface);
        }

        IUnknown_Release(This->punkFilterMapper2);

        if (This->pSite) IUnknown_Release(This->pSite);

	CloseHandle(This->hEventCompletion);
	EventsQueue_Destroy(&This->evqueue);
        This->cs.DebugInfo->Spare[0] = 0;
        if (This->message_thread)
        {
            PostThreadMessageW(This->message_thread_id, WM_USER + 1, 0, 0);
            WaitForSingleObject(This->message_thread, INFINITE);
            CloseHandle(This->message_thread);
            CloseHandle(This->message_thread_ret);
        }
	DeleteCriticalSection(&This->cs);
	CoTaskMemFree(This);
    }
    return ref;
}

static inline IFilterGraphImpl *impl_from_IFilterGraph2(IFilterGraph2 *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IFilterGraph2_iface);
}

static HRESULT WINAPI FilterGraph2_QueryInterface(IFilterGraph2 *iface, REFIID riid, void **ppvObj)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI FilterGraph2_AddRef(IFilterGraph2 *iface)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI FilterGraph2_Release(IFilterGraph2 *iface)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);
}

static IBaseFilter *find_filter_by_name(IFilterGraphImpl *graph, const WCHAR *name)
{
    struct filter *filter;

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        if (!wcscmp(filter->name, name))
            return filter->filter;
    }

    return NULL;
}

/*** IFilterGraph methods ***/
static HRESULT WINAPI FilterGraph2_AddFilter(IFilterGraph2 *iface, IBaseFilter *pFilter,
        LPCWSTR pName)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    struct filter *entry;
    HRESULT hr;
    int j;
    WCHAR* wszFilterName = NULL;
    BOOL duplicate_name = FALSE;

    TRACE("(%p/%p)->(%p, %s (%p))\n", This, iface, pFilter, debugstr_w(pName), pName);

    if (!pFilter)
        return E_POINTER;

    wszFilterName = CoTaskMemAlloc( (pName ? lstrlenW(pName) + 6 : 5) * sizeof(WCHAR) );

    if (pName && find_filter_by_name(This, pName))
        duplicate_name = TRUE;

    /* If no name given or name already existing, generate one */
    if (!pName || duplicate_name)
    {
	static const WCHAR wszFmt1[] = {'%','s',' ','%','0','4','d',0};
	static const WCHAR wszFmt2[] = {'%','0','4','d',0};

	for (j = 0; j < 10000 ; j++)
	{
	    /* Create name */
	    if (pName)
		swprintf(wszFilterName, pName ? lstrlenW(pName) + 6 : 5, wszFmt1, pName, This->nameIndex);
	    else
		swprintf(wszFilterName, pName ? lstrlenW(pName) + 6 : 5, wszFmt2, This->nameIndex);
	    TRACE("Generated name %s\n", debugstr_w(wszFilterName));

	    if (This->nameIndex++ == 10000)
		This->nameIndex = 1;

            if (!find_filter_by_name(This, wszFilterName))
                break;
	}
	/* Unable to find a suitable name */
	if (j == 10000)
	{
	    CoTaskMemFree(wszFilterName);
	    return VFW_E_DUPLICATE_NAME;
	}
    }
    else
	memcpy(wszFilterName, pName, (lstrlenW(pName) + 1) * sizeof(WCHAR));

    hr = IBaseFilter_JoinFilterGraph(pFilter, (IFilterGraph *)&This->IFilterGraph2_iface, wszFilterName);
    if (FAILED(hr))
    {
        CoTaskMemFree(wszFilterName);
        return hr;
    }

    if (!(entry = heap_alloc(sizeof(*entry))))
    {
        CoTaskMemFree(wszFilterName);
        return E_OUTOFMEMORY;
    }

    IBaseFilter_AddRef(entry->filter = pFilter);
    entry->name = wszFilterName;
    list_add_head(&This->filters, &entry->entry);
    This->version++;

    return duplicate_name ? VFW_S_DUPLICATE_NAME : hr;
}

static HRESULT WINAPI FilterGraph2_RemoveFilter(IFilterGraph2 *iface, IBaseFilter *pFilter)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    struct filter *entry;
    int i;
    HRESULT hr = E_FAIL;

    TRACE("(%p/%p)->(%p)\n", This, iface, pFilter);

    /* FIXME: check graph is stopped */

    LIST_FOR_EACH_ENTRY(entry, &This->filters, struct filter, entry)
    {
        if (entry->filter == pFilter)
        {
            IEnumPins *penumpins = NULL;
            FILTER_STATE state;

            if (This->defaultclock && This->refClockProvider == pFilter)
            {
                IMediaFilter_SetSyncSource(&This->IMediaFilter_iface, NULL);
                This->defaultclock = TRUE;
            }

            TRACE("Removing filter %s.\n", debugstr_w(entry->name));
            IBaseFilter_GetState(pFilter, 0, &state);
            if (state == State_Running)
                IBaseFilter_Pause(pFilter);
            if (state != State_Stopped)
                IBaseFilter_Stop(pFilter);

            hr = IBaseFilter_EnumPins(pFilter, &penumpins);
            if (SUCCEEDED(hr)) {
                IPin *ppin;
                while(IEnumPins_Next(penumpins, 1, &ppin, NULL) == S_OK)
                {
                    IPin *victim = NULL;
                    HRESULT h;
                    IPin_ConnectedTo(ppin, &victim);
                    if (victim)
                    {
                        h = IPin_Disconnect(victim);
                        TRACE("Disconnect other side: %08x\n", h);
                        if (h == VFW_E_NOT_STOPPED)
                        {
                            PIN_INFO pinfo;
                            IPin_QueryPinInfo(victim, &pinfo);

                            IBaseFilter_GetState(pinfo.pFilter, 0, &state);
                            if (state == State_Running)
                                IBaseFilter_Pause(pinfo.pFilter);
                            IBaseFilter_Stop(pinfo.pFilter);
                            IBaseFilter_Release(pinfo.pFilter);
                            h = IPin_Disconnect(victim);
                            TRACE("Disconnect retry: %08x\n", h);
                        }
                        IPin_Release(victim);
                    }
                    h = IPin_Disconnect(ppin);
                    TRACE("Disconnect 2: %08x\n", h);

                    IPin_Release(ppin);
                }
                IEnumPins_Release(penumpins);
            }

            hr = IBaseFilter_JoinFilterGraph(pFilter, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                IBaseFilter_SetSyncSource(pFilter, NULL);
                IBaseFilter_Release(pFilter);
                list_remove(&entry->entry);
                CoTaskMemFree(entry->name);
                heap_free(entry);
                This->version++;
                /* Invalidate interfaces in the cache */
                for (i = 0; i < This->nItfCacheEntries; i++)
                    if (pFilter == This->ItfCacheEntries[i].filter)
                    {
                        IUnknown_Release(This->ItfCacheEntries[i].iface);
                        This->ItfCacheEntries[i].iface = NULL;
                        This->ItfCacheEntries[i].filter = NULL;
                    }
                return S_OK;
            }
            break;
        }
    }

    return hr; /* FIXME: check this error code */
}

static HRESULT WINAPI FilterGraph2_EnumFilters(IFilterGraph2 *iface, IEnumFilters **out)
{
    IFilterGraphImpl *graph = impl_from_IFilterGraph2(iface);

    TRACE("graph %p, out %p.\n", graph, out);

    return create_enum_filters(graph, list_head(&graph->filters), out);
}

static HRESULT WINAPI FilterGraph2_FindFilterByName(IFilterGraph2 *iface,
        const WCHAR *name, IBaseFilter **filter)
{
    IFilterGraphImpl *graph = impl_from_IFilterGraph2(iface);

    TRACE("graph %p, name %s, filter %p.\n", graph, debugstr_w(name), filter);

    if (!filter)
        return E_POINTER;

    if ((*filter = find_filter_by_name(graph, name)))
    {
        IBaseFilter_AddRef(*filter);
        return S_OK;
    }

    return VFW_E_NOT_FOUND;
}

/* Don't allow a circular connection to form, return VFW_E_CIRCULAR_GRAPH if this would be the case.
 * A circular connection will be formed if from the filter of the output pin, the input pin can be reached
 */
static HRESULT CheckCircularConnection(IFilterGraphImpl *This, IPin *out, IPin *in)
{
#if 1
    HRESULT hr;
    PIN_INFO info_out, info_in;

    hr = IPin_QueryPinInfo(out, &info_out);
    if (FAILED(hr))
        return hr;
    if (info_out.dir != PINDIR_OUTPUT)
    {
        IBaseFilter_Release(info_out.pFilter);
        return VFW_E_CANNOT_CONNECT;
    }

    hr = IPin_QueryPinInfo(in, &info_in);
    if (SUCCEEDED(hr))
        IBaseFilter_Release(info_in.pFilter);
    if (FAILED(hr))
        goto out;
    if (info_in.dir != PINDIR_INPUT)
    {
        hr = VFW_E_CANNOT_CONNECT;
        goto out;
    }

    if (info_out.pFilter == info_in.pFilter)
        hr = VFW_E_CIRCULAR_GRAPH;
    else
    {
        IEnumPins *enumpins;
        IPin *test;

        hr = IBaseFilter_EnumPins(info_out.pFilter, &enumpins);
        if (FAILED(hr))
            goto out;

        IEnumPins_Reset(enumpins);
        while ((hr = IEnumPins_Next(enumpins, 1, &test, NULL)) == S_OK)
        {
            PIN_DIRECTION dir = PINDIR_OUTPUT;
            IPin_QueryDirection(test, &dir);
            if (dir == PINDIR_INPUT)
            {
                IPin *victim = NULL;
                IPin_ConnectedTo(test, &victim);
                if (victim)
                {
                    hr = CheckCircularConnection(This, victim, in);
                    IPin_Release(victim);
                    if (FAILED(hr))
                    {
                        IPin_Release(test);
                        break;
                    }
                }
            }
            IPin_Release(test);
        }
        IEnumPins_Release(enumpins);
    }

out:
    IBaseFilter_Release(info_out.pFilter);
    if (FAILED(hr))
        ERR("Checking filtergraph returned %08x, something's not right!\n", hr);
    return hr;
#else
    /* Debugging filtergraphs not enabled */
    return S_OK;
#endif
}


/* NOTE: despite the implication, it doesn't matter which
 * way round you put in the input and output pins */
static HRESULT WINAPI FilterGraph2_ConnectDirect(IFilterGraph2 *iface, IPin *ppinIn, IPin *ppinOut,
        const AM_MEDIA_TYPE *pmt)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    PIN_DIRECTION dir;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p)\n", This, iface, ppinIn, ppinOut, pmt);
    strmbase_dump_media_type(pmt);

    /* FIXME: check pins are in graph */

    if (TRACE_ON(quartz))
    {
        PIN_INFO PinInfo;

        hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning ppinIn(%p) => %p\n", ppinIn, PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning ppinOut(%p) => %p\n", ppinOut, PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    hr = IPin_QueryDirection(ppinIn, &dir);
    if (SUCCEEDED(hr))
    {
        if (dir == PINDIR_INPUT)
        {
            hr = CheckCircularConnection(This, ppinOut, ppinIn);
            if (SUCCEEDED(hr))
                hr = IPin_Connect(ppinOut, ppinIn, pmt);
        }
        else
        {
            hr = CheckCircularConnection(This, ppinIn, ppinOut);
            if (SUCCEEDED(hr))
                hr = IPin_Connect(ppinIn, ppinOut, pmt);
        }
    }

    return hr;
}

static HRESULT WINAPI FilterGraph2_Reconnect(IFilterGraph2 *iface, IPin *ppin)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    IPin *pConnectedTo = NULL;
    HRESULT hr;
    PIN_DIRECTION pindir;

    IPin_QueryDirection(ppin, &pindir);
    hr = IPin_ConnectedTo(ppin, &pConnectedTo);

    TRACE("(%p/%p)->(%p) -- %p\n", This, iface, ppin, pConnectedTo);

    if (FAILED(hr)) {
        TRACE("Querying connected to failed: %x\n", hr);
        return hr; 
    }
    IPin_Disconnect(ppin);
    IPin_Disconnect(pConnectedTo);
    if (pindir == PINDIR_INPUT)
        hr = IPin_Connect(pConnectedTo, ppin, NULL);
    else
        hr = IPin_Connect(ppin, pConnectedTo, NULL);
    IPin_Release(pConnectedTo);
    if (FAILED(hr))
        WARN("Reconnecting pins failed, pins are not connected now..\n");
    TRACE("-> %08x\n", hr);
    return hr;
}

static HRESULT WINAPI FilterGraph2_Disconnect(IFilterGraph2 *iface, IPin *ppin)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppin);

    if (!ppin)
       return E_POINTER;

    return IPin_Disconnect(ppin);
}

static HRESULT WINAPI FilterGraph2_SetDefaultSyncSource(IFilterGraph2 *iface)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    IReferenceClock *pClock = NULL;
    struct filter *filter;
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->() live sources not handled properly!\n", This, iface);

    EnterCriticalSection(&This->cs);

    LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
    {
        if (IBaseFilter_QueryInterface(filter->filter, &IID_IReferenceClock, (void **)&pClock) == S_OK)
            break;
    }

    if (!pClock)
    {
        hr = CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER, &IID_IReferenceClock, (LPVOID*)&pClock);
        This->refClockProvider = NULL;
    }
    else
    {
        filter = LIST_ENTRY(list_tail(&This->filters), struct filter, entry);
        This->refClockProvider = filter->filter;
    }

    if (SUCCEEDED(hr))
    {
        hr = IMediaFilter_SetSyncSource(&This->IMediaFilter_iface, pClock);
        This->defaultclock = TRUE;
        IReferenceClock_Release(pClock);
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT GetFilterInfo(IMoniker* pMoniker, VARIANT* pvar)
{
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
    IPropertyBag * pPropBagCat = NULL;
    HRESULT hr;

    VariantInit(pvar);

    hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBagCat);

    if (SUCCEEDED(hr))
        hr = IPropertyBag_Read(pPropBagCat, wszFriendlyName, pvar, NULL);

    if (SUCCEEDED(hr))
        TRACE("Moniker = %s\n", debugstr_w(V_BSTR(pvar)));

    if (pPropBagCat)
        IPropertyBag_Release(pPropBagCat);

    return hr;
}

struct filter_create_params
{
    HRESULT hr;
    IMoniker *moniker;
    IBaseFilter *filter;
};

static DWORD WINAPI message_thread_run(void *ctx)
{
    IFilterGraphImpl *graph = ctx;
    MSG msg;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    for (;;)
    {
        GetMessageW(&msg, NULL, 0, 0);

        if (!msg.hwnd && msg.message == WM_USER)
        {
            struct filter_create_params *params = (struct filter_create_params *)msg.wParam;

            params->hr = IMoniker_BindToObject(params->moniker, NULL, NULL,
                    &IID_IBaseFilter, (void **)&params->filter);
            SetEvent(graph->message_thread_ret);
        }
        else if (!msg.hwnd && msg.message == WM_USER + 1)
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    CoUninitialize();
    return 0;
}

static HRESULT create_filter(IFilterGraphImpl *graph, IMoniker *moniker, IBaseFilter **filter)
{
    if (graph->message_thread)
    {
        struct filter_create_params params;

        params.moniker = moniker;
        PostThreadMessageW(graph->message_thread_id, WM_USER, (WPARAM)&params, 0);
        WaitForSingleObject(graph->message_thread_ret, INFINITE);
        *filter = params.filter;
        return params.hr;
    }
    else
        return IMoniker_BindToObject(moniker, NULL, NULL, &IID_IBaseFilter, (void **)filter);
}

/* Attempt to connect one of the output pins on filter to sink. Helper for
 * FilterGraph2_Connect(). */
static HRESULT connect_output_pin(IFilterGraphImpl *graph, IBaseFilter *filter, IPin *sink)
{
    IEnumPins *enumpins;
    PIN_INFO info;
    HRESULT hr;
    IPin *pin;

    hr = IBaseFilter_EnumPins(filter, &enumpins);
    if (FAILED(hr))
        return hr;

    while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
    {
        IPin_QueryPinInfo(pin, &info);
        IBaseFilter_Release(info.pFilter);
        if (info.dir == PINDIR_OUTPUT)
        {
            if (info.achName[0] == '~')
            {
                TRACE("Skipping non-rendered pin %s.\n", debugstr_w(info.achName));
                IPin_Release(pin);
                continue;
            }

            if (SUCCEEDED(IFilterGraph2_Connect(&graph->IFilterGraph2_iface, pin, sink)))
            {
                IPin_Release(pin);
                IEnumPins_Release(enumpins);
                return S_OK;
            }
        }
        IPin_Release(pin);
    }

    IEnumPins_Release(enumpins);
    return VFW_E_CANNOT_CONNECT;
}

/*** IGraphBuilder methods ***/
static HRESULT WINAPI FilterGraph2_Connect(IFilterGraph2 *iface, IPin *ppinOut, IPin *ppinIn)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    struct filter *filter;
    HRESULT hr;
    IPin *pin;
    AM_MEDIA_TYPE* mt = NULL;
    IEnumMediaTypes* penummt = NULL;
    ULONG nbmt;
    IEnumPins* penumpins;
    IEnumMoniker* pEnumMoniker;
    GUID tab[2];
    IMoniker* pMoniker;
    PIN_INFO PinInfo;
    PIN_DIRECTION dir;
    IFilterMapper2 *pFilterMapper2 = NULL;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, ppinOut, ppinIn);

    if(!ppinOut || !ppinIn)
        return E_POINTER;

    if (TRACE_ON(quartz))
    {
        hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning ppinIn(%p) => %p\n", ppinIn, PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning ppinOut(%p) => %p\n", ppinOut, PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    EnterCriticalSection(&This->cs);
    ++This->recursioncount;
    if (This->recursioncount >= 5)
    {
        WARN("Recursion count has reached %d\n", This->recursioncount);
        hr = VFW_E_CANNOT_CONNECT;
        goto out;
    }

    hr = IPin_QueryDirection(ppinOut, &dir);
    if (FAILED(hr))
        goto out;

    if (dir == PINDIR_INPUT)
    {
        IPin *temp;

        TRACE("Directions seem backwards, swapping pins\n");

        temp = ppinIn;
        ppinIn = ppinOut;
        ppinOut = temp;
    }

    hr = CheckCircularConnection(This, ppinOut, ppinIn);
    if (FAILED(hr))
        goto out;

    /* Try direct connection first */
    hr = IPin_Connect(ppinOut, ppinIn, NULL);

    /* If direct connection succeeded, we should propagate that return value.
     * If it returned VFW_E_NOT_CONNECTED or VFW_E_NO_AUDIO_HARDWARE, then don't
     * even bother trying intermediate filters, since they won't succeed. */
    if (SUCCEEDED(hr) || hr == VFW_E_NOT_CONNECTED || hr == VFW_E_NO_AUDIO_HARDWARE)
        goto out;

    TRACE("Direct connection failed, trying to render using extra filters\n");

    LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
    {
        hr = IBaseFilter_EnumPins(filter->filter, &penumpins);
        if (FAILED(hr))
            goto out;

        while (IEnumPins_Next(penumpins, 1, &pin, NULL) == S_OK)
        {
            IPin_QueryDirection(pin, &dir);
            if (dir == PINDIR_INPUT && SUCCEEDED(IFilterGraph2_ConnectDirect(iface,
                    ppinOut, pin, NULL)))
            {
                if (SUCCEEDED(hr = connect_output_pin(This, filter->filter, ppinIn)))
                {
                    IPin_Release(pin);
                    IEnumPins_Release(penumpins);
                    goto out;
                }

                IFilterGraph2_Disconnect(iface, pin);
                IFilterGraph2_Disconnect(iface, ppinOut);
            }
            IPin_Release(pin);
        }

        IEnumPins_Release(penumpins);
    }

    /* Find the appropriate transform filter than can transform the minor media type of output pin of the upstream 
     * filter to the minor mediatype of input pin of the renderer */
    hr = IPin_EnumMediaTypes(ppinOut, &penummt);
    if (FAILED(hr))
    {
        WARN("EnumMediaTypes (%x)\n", hr);
        goto out;
    }

    hr = IEnumMediaTypes_Next(penummt, 1, &mt, &nbmt);
    if (FAILED(hr)) {
        WARN("IEnumMediaTypes_Next (%x)\n", hr);
        goto out;
    }

    if (!nbmt)
    {
        WARN("No media type found!\n");
        hr = VFW_E_INVALIDMEDIATYPE;
        goto out;
    }
    TRACE("MajorType %s\n", debugstr_guid(&mt->majortype));
    TRACE("SubType %s\n", debugstr_guid(&mt->subtype));

    hr = IUnknown_QueryInterface(This->punkFilterMapper2, &IID_IFilterMapper2, (void**)&pFilterMapper2);
    if (FAILED(hr)) {
        WARN("Unable to get IFilterMapper2 (%x)\n", hr);
        goto out;
    }

    /* Try to find a suitable filter that can connect to the pin to render */
    tab[0] = mt->majortype;
    tab[1] = mt->subtype;
    hr = IFilterMapper2_EnumMatchingFilters(pFilterMapper2, &pEnumMoniker, 0, FALSE, MERIT_UNLIKELY, TRUE, 1, tab, NULL, NULL, FALSE, FALSE, 0, NULL, NULL, NULL);
    if (FAILED(hr)) {
        WARN("Unable to enum filters (%x)\n", hr);
        goto out;
    }

    hr = VFW_E_CANNOT_RENDER;
    while (IEnumMoniker_Next(pEnumMoniker, 1, &pMoniker, NULL) == S_OK)
    {
        VARIANT var;
        IPin* ppinfilter = NULL;
        IBaseFilter* pfilter = NULL;
        IAMGraphBuilderCallback *callback = NULL;

        hr = GetFilterInfo(pMoniker, &var);
        if (FAILED(hr)) {
            WARN("Unable to retrieve filter info (%x)\n", hr);
            goto error;
        }

        hr = create_filter(This, pMoniker, &pfilter);
        IMoniker_Release(pMoniker);
        if (FAILED(hr)) {
            WARN("Unable to create filter (%x), trying next one\n", hr);
            goto error;
        }

        if (This->pSite)
        {
            IUnknown_QueryInterface(This->pSite, &IID_IAMGraphBuilderCallback, (LPVOID*)&callback);
            if (callback)
            {
                HRESULT rc;
                rc = IAMGraphBuilderCallback_SelectedFilter(callback, pMoniker);
                if (FAILED(rc))
                {
                    TRACE("Filter rejected by IAMGraphBuilderCallback_SelectedFilter\n");
                    IAMGraphBuilderCallback_Release(callback);
                    goto error;
                }
            }
        }

        if (callback)
        {
            HRESULT rc;
            rc = IAMGraphBuilderCallback_CreatedFilter(callback, pfilter);
            IAMGraphBuilderCallback_Release(callback);
            if (FAILED(rc))
            {
                IBaseFilter_Release(pfilter);
                pfilter = NULL;
                TRACE("Filter rejected by IAMGraphBuilderCallback_CreatedFilter\n");
                goto error;
            }
        }

        hr = IFilterGraph2_AddFilter(iface, pfilter, V_BSTR(&var));
        if (FAILED(hr)) {
            WARN("Unable to add filter (%x)\n", hr);
            IBaseFilter_Release(pfilter);
            pfilter = NULL;
            goto error;
        }

        VariantClear(&var);

        hr = IBaseFilter_EnumPins(pfilter, &penumpins);
        if (FAILED(hr)) {
            WARN("Enumpins (%x)\n", hr);
            goto error;
        }

        hr = IEnumPins_Next(penumpins, 1, &ppinfilter, NULL);
        IEnumPins_Release(penumpins);

        if (FAILED(hr)) {
            WARN("Obtaining next pin: (%x)\n", hr);
            goto error;
        }
        if (hr == S_FALSE) {
            WARN("Cannot use this filter: no pins\n");
            goto error;
        }

        hr = IPin_Connect(ppinOut, ppinfilter, NULL);
        if (FAILED(hr)) {
            TRACE("Cannot connect to filter (%x), trying next one\n", hr);
            goto error;
        }
        TRACE("Successfully connected to filter, follow chain...\n");

        if (SUCCEEDED(hr = connect_output_pin(This, pfilter, ppinIn)))
        {
            IPin_Release(ppinfilter);
            IBaseFilter_Release(pfilter);
            break;
        }

error:
        VariantClear(&var);
        if (ppinfilter) IPin_Release(ppinfilter);
        if (pfilter) {
            IFilterGraph2_RemoveFilter(iface, pfilter);
            IBaseFilter_Release(pfilter);
        }
    }

    if (FAILED(hr))
        hr = VFW_E_CANNOT_CONNECT;

    IEnumMoniker_Release(pEnumMoniker);

out:
    if (pFilterMapper2)
        IFilterMapper2_Release(pFilterMapper2);
    if (penummt)
        IEnumMediaTypes_Release(penummt);
    if (mt)
        DeleteMediaType(mt);
    --This->recursioncount;
    LeaveCriticalSection(&This->cs);
    TRACE("--> %08x\n", hr);
    return hr;
}

/* Render all output pins of the given filter. Helper for FilterGraph2_Render(). */
static HRESULT render_output_pins(IFilterGraphImpl *graph, IBaseFilter *filter)
{
    BOOL renderany = FALSE;
    BOOL renderall = TRUE;
    IEnumPins *enumpins;
    IPin *pin, *peer;
    PIN_INFO info;

    IBaseFilter_EnumPins(filter, &enumpins);
    while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
    {
        IPin_QueryPinInfo(pin, &info);
        IBaseFilter_Release(info.pFilter);
        if (info.dir == PINDIR_OUTPUT)
        {
            if (info.achName[0] == '~')
            {
                TRACE("Skipping non-rendered pin %s.\n", debugstr_w(info.achName));
                IPin_Release(pin);
                continue;
            }

            if (IPin_ConnectedTo(pin, &peer) == VFW_E_NOT_CONNECTED)
            {
                HRESULT hr;
                hr = IFilterGraph2_Render(&graph->IFilterGraph2_iface, pin);
                if (SUCCEEDED(hr))
                    renderany = TRUE;
                else
                    renderall = FALSE;
            }
            else
                IPin_Release(peer);
        }

        IPin_Release(pin);
    }

    IEnumPins_Release(enumpins);

    if (renderall)
        return S_OK;

    if (renderany)
        return VFW_S_PARTIAL_RENDER;

    return VFW_E_CANNOT_RENDER;
}

/* Ogg hates me if I create a direct rendering method
 *
 * It can only connect to a pin properly once, so use a recursive method that does
 *
 *  +----+ --- (PIN 1) (Render is called on this pin)
 *  |    |
 *  +----+ --- (PIN 2)
 *
 *  Enumerate possible renderers that EXACTLY match the requested type
 *
 *  If none is available, try to add intermediate filters that can connect to the input pin
 *  then call Render on that intermediate pin's output pins
 *  if it succeeds: Render returns success, if it doesn't, the intermediate filter is removed,
 *  and another filter that can connect to the input pin is tried
 *  if we run out of filters that can, give up and return VFW_E_CANNOT_RENDER
 *  It's recursive, but fun!
 */

static HRESULT WINAPI FilterGraph2_Render(IFilterGraph2 *iface, IPin *ppinOut)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    IEnumMediaTypes* penummt;
    struct filter *filter;
    AM_MEDIA_TYPE* mt;
    ULONG nbmt;
    HRESULT hr;

    IEnumMoniker* pEnumMoniker;
    GUID tab[4];
    ULONG nb;
    IMoniker* pMoniker;
    IFilterMapper2 *pFilterMapper2 = NULL;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppinOut);

    if (TRACE_ON(quartz))
    {
        PIN_INFO PinInfo;

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning pin => %p\n", PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    /* Try to find out if there is a renderer for the specified subtype already, and use that
     */
    EnterCriticalSection(&This->cs);
    LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
    {
        IEnumPins *enumpins = NULL;
        IPin *pin = NULL;

        hr = IBaseFilter_EnumPins(filter->filter, &enumpins);

        if (FAILED(hr) || !enumpins)
            continue;

        IEnumPins_Reset(enumpins);
        while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
        {
            IPin *to = NULL;
            PIN_DIRECTION dir = PINDIR_OUTPUT;

            IPin_QueryDirection(pin, &dir);
            if (dir != PINDIR_INPUT)
            {
                IPin_Release(pin);
                continue;
            }
            IPin_ConnectedTo(pin, &to);

            if (to == NULL)
            {
                hr = FilterGraph2_ConnectDirect(iface, ppinOut, pin, NULL);
                if (SUCCEEDED(hr))
                {
                    TRACE("Connected successfully %p/%p, %08x look if we should render more!\n", ppinOut, pin, hr);
                    IPin_Release(pin);

                    hr = render_output_pins(This, filter->filter);
                    if (FAILED(hr))
                    {
                        IPin_Disconnect(ppinOut);
                        IPin_Disconnect(pin);
                        continue;
                    }
                    IEnumPins_Release(enumpins);
                    LeaveCriticalSection(&This->cs);
                    return hr;
                }
                WARN("Could not connect!\n");
            }
            else
                IPin_Release(to);

            IPin_Release(pin);
        }
        IEnumPins_Release(enumpins);
    }

    LeaveCriticalSection(&This->cs);

    hr = IPin_EnumMediaTypes(ppinOut, &penummt);
    if (FAILED(hr)) {
        WARN("EnumMediaTypes (%x)\n", hr);
        return hr;
    }

    IEnumMediaTypes_Reset(penummt);

    /* Looks like no existing renderer of the kind exists
     * Try adding new ones
     */
    tab[0] = tab[1] = GUID_NULL;
    while (SUCCEEDED(hr))
    {
        hr = IEnumMediaTypes_Next(penummt, 1, &mt, &nbmt);
        if (FAILED(hr)) {
            WARN("IEnumMediaTypes_Next (%x)\n", hr);
            break;
        }
        if (!nbmt)
        {
            hr = VFW_E_CANNOT_RENDER;
            break;
        }
        else
        {
            TRACE("MajorType %s\n", debugstr_guid(&mt->majortype));
            TRACE("SubType %s\n", debugstr_guid(&mt->subtype));

            /* Only enumerate once, this doesn't account for all previous ones, but this should be enough nonetheless */
            if (IsEqualIID(&tab[0], &mt->majortype) && IsEqualIID(&tab[1], &mt->subtype))
            {
                DeleteMediaType(mt);
                continue;
            }

            if (pFilterMapper2 == NULL)
            {
                hr = IUnknown_QueryInterface(This->punkFilterMapper2, &IID_IFilterMapper2, (void**)&pFilterMapper2);
                if (FAILED(hr))
                {
                    WARN("Unable to query IFilterMapper2 (%x)\n", hr);
                    break;
                }
            }

            /* Try to find a suitable renderer with the same media type */
            tab[0] = mt->majortype;
            tab[1] = mt->subtype;
            hr = IFilterMapper2_EnumMatchingFilters(pFilterMapper2, &pEnumMoniker, 0, FALSE, MERIT_UNLIKELY, TRUE, 1, tab, NULL, NULL, FALSE, FALSE, 0, NULL, NULL, NULL);
            if (FAILED(hr))
            {
                WARN("Unable to enum filters (%x)\n", hr);
                break;
            }
        }
        hr = E_FAIL;

        while (IEnumMoniker_Next(pEnumMoniker, 1, &pMoniker, &nb) == S_OK)
        {
            VARIANT var;
            IPin* ppinfilter;
            IBaseFilter* pfilter = NULL;
            IEnumPins* penumpins = NULL;
            ULONG pin;

            hr = GetFilterInfo(pMoniker, &var);
            if (FAILED(hr)) {
                WARN("Unable to retrieve filter info (%x)\n", hr);
                goto error;
            }

            hr = create_filter(This, pMoniker, &pfilter);
            IMoniker_Release(pMoniker);
            if (FAILED(hr))
            {
                WARN("Unable to create filter (%x), trying next one\n", hr);
                goto error;
            }

            hr = IFilterGraph2_AddFilter(iface, pfilter, V_BSTR(&var));
            if (FAILED(hr)) {
                WARN("Unable to add filter (%x)\n", hr);
                IBaseFilter_Release(pfilter);
                pfilter = NULL;
                goto error;
            }

            hr = IBaseFilter_EnumPins(pfilter, &penumpins);
            if (FAILED(hr)) {
                WARN("Splitter Enumpins (%x)\n", hr);
                goto error;
            }

            while ((hr = IEnumPins_Next(penumpins, 1, &ppinfilter, &pin)) == S_OK)
            {
                PIN_DIRECTION dir;

                if (pin == 0) {
                    WARN("No Pin\n");
                    hr = E_FAIL;
                    goto error;
                }

                hr = IPin_QueryDirection(ppinfilter, &dir);
                if (FAILED(hr)) {
                    IPin_Release(ppinfilter);
                    WARN("QueryDirection failed (%x)\n", hr);
                    goto error;
                }
                if (dir != PINDIR_INPUT) {
                    IPin_Release(ppinfilter);
                    continue; /* Wrong direction */
                }

                /* Connect the pin to the "Renderer" */
                hr = IPin_Connect(ppinOut, ppinfilter, NULL);
                IPin_Release(ppinfilter);

                if (FAILED(hr)) {
                    WARN("Unable to connect %s to renderer (%x)\n", debugstr_w(V_BSTR(&var)), hr);
                    goto error;
                }
                TRACE("Connected, recursing %s\n",  debugstr_w(V_BSTR(&var)));

                VariantClear(&var);

                hr = render_output_pins(This, pfilter);
                if (FAILED(hr)) {
                    WARN("Unable to connect recursively (%x)\n", hr);
                    goto error;
                }
                IBaseFilter_Release(pfilter);
                break;
            }
            if (SUCCEEDED(hr)) {
                IEnumPins_Release(penumpins);
                break; /* out of IEnumMoniker_Next loop */
            }

            /* IEnumPins_Next failed, all other failure case caught by goto error */
            WARN("IEnumPins_Next (%x)\n", hr);
            /* goto error */

error:
            VariantClear(&var);
            if (penumpins)
                IEnumPins_Release(penumpins);
            if (pfilter) {
                IFilterGraph2_RemoveFilter(iface, pfilter);
                IBaseFilter_Release(pfilter);
            }
            if (SUCCEEDED(hr)) DebugBreak();
        }

        IEnumMoniker_Release(pEnumMoniker);
        if (nbmt)
            DeleteMediaType(mt);
        if (SUCCEEDED(hr))
            break;
        hr = S_OK;
    }

    if (pFilterMapper2)
        IFilterMapper2_Release(pFilterMapper2);

    IEnumMediaTypes_Release(penummt);
    return hr;
}

static HRESULT WINAPI FilterGraph2_RenderFile(IFilterGraph2 *iface, LPCWSTR lpcwstrFile,
        LPCWSTR lpcwstrPlayList)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    static const WCHAR string[] = {'R','e','a','d','e','r',0};
    IBaseFilter* preader = NULL;
    IPin* ppinreader = NULL;
    IEnumPins* penumpins = NULL;
    struct filter *filter;
    HRESULT hr;
    BOOL partial = FALSE;
    BOOL any = FALSE;

    TRACE("(%p/%p)->(%s, %s)\n", This, iface, debugstr_w(lpcwstrFile), debugstr_w(lpcwstrPlayList));

    if (lpcwstrPlayList != NULL)
        return E_INVALIDARG;

    hr = IFilterGraph2_AddSourceFilter(iface, lpcwstrFile, string, &preader);
    if (FAILED(hr))
        return hr;

    hr = IBaseFilter_EnumPins(preader, &penumpins);
    if (SUCCEEDED(hr))
    {
        while (IEnumPins_Next(penumpins, 1, &ppinreader, NULL) == S_OK)
        {
            PIN_DIRECTION dir;

            IPin_QueryDirection(ppinreader, &dir);
            if (dir == PINDIR_OUTPUT)
            {
                hr = IFilterGraph2_Render(iface, ppinreader);

                TRACE("Filters in chain:\n");
                LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
                    TRACE("- %s.\n", debugstr_w(filter->name));

                if (SUCCEEDED(hr))
                    any = TRUE;
                if (hr != S_OK)
                    partial = TRUE;
            }
            IPin_Release(ppinreader);
        }
        IEnumPins_Release(penumpins);

        if (!any)
            hr = VFW_E_CANNOT_RENDER;
        else if (partial)
            hr = VFW_S_PARTIAL_RENDER;
        else
            hr = S_OK;
    }
    IBaseFilter_Release(preader);

    TRACE("--> %08x\n", hr);
    return hr;
}

static HRESULT WINAPI FilterGraph2_AddSourceFilter(IFilterGraph2 *iface,
        const WCHAR *filename, const WCHAR *filter_name, IBaseFilter **ret_filter)
{
    IFilterGraphImpl *graph = impl_from_IFilterGraph2(iface);
    IFileSourceFilter *filesource;
    IBaseFilter *filter;
    HRESULT hr;
    GUID clsid;

    TRACE("graph %p, filename %s, filter_name %s, ret_filter %p.\n",
            graph, debugstr_w(filename), debugstr_w(filter_name), ret_filter);

    if (!get_media_type(filename, NULL, NULL, &clsid))
        clsid = CLSID_AsyncReader;
    TRACE("Using source filter %s.\n", debugstr_guid(&clsid));

    if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter)))
    {
        WARN("Failed to create filter, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource)))
    {
        WARN("Failed to get IFileSourceFilter, hr %#x.\n", hr);
        IBaseFilter_Release(filter);
        return hr;
    }

    hr = IFileSourceFilter_Load(filesource, filename, NULL);
    IFileSourceFilter_Release(filesource);
    if (FAILED(hr))
    {
        WARN("Failed to load file, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = IFilterGraph2_AddFilter(iface, filter, filter_name)))
    {
        IBaseFilter_Release(filter);
        return hr;
    }

    if (ret_filter)
        *ret_filter = filter;
    return S_OK;
}

static HRESULT WINAPI FilterGraph2_SetLogFile(IFilterGraph2 *iface, DWORD_PTR hFile)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(%08x): stub !!!\n", This, iface, (DWORD) hFile);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_Abort(IFilterGraph2 *iface)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_ShouldOperationContinue(IFilterGraph2 *iface)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

/*** IFilterGraph2 methods ***/
static HRESULT WINAPI FilterGraph2_AddSourceFilterForMoniker(IFilterGraph2 *iface,
        IMoniker *pMoniker, IBindCtx *pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);
    HRESULT hr;
    IBaseFilter* pfilter;

    TRACE("(%p/%p)->(%p %p %s %p)\n", This, iface, pMoniker, pCtx, debugstr_w(lpcwstrFilterName), ppFilter);

    hr = IMoniker_BindToObject(pMoniker, pCtx, NULL, &IID_IBaseFilter, (void**)&pfilter);
    if(FAILED(hr)) {
        WARN("Unable to bind moniker to filter object (%x)\n", hr);
        return hr;
    }

    hr = IFilterGraph2_AddFilter(iface, pfilter, lpcwstrFilterName);
    if (FAILED(hr)) {
        WARN("Unable to add filter (%x)\n", hr);
        IBaseFilter_Release(pfilter);
        return hr;
    }

    if(ppFilter)
        *ppFilter = pfilter;
    else IBaseFilter_Release(pfilter);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_ReconnectEx(IFilterGraph2 *iface, IPin *ppin,
        const AM_MEDIA_TYPE *pmt)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(%p %p): stub !!!\n", This, iface, ppin, pmt);
    strmbase_dump_media_type(pmt);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_RenderEx(IFilterGraph2 *iface, IPin *pPinOut, DWORD dwFlags,
        DWORD *pvContext)
{
    IFilterGraphImpl *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(%p %08x %p): stub !!!\n", This, iface, pPinOut, dwFlags, pvContext);

    return S_OK;
}


static const IFilterGraph2Vtbl IFilterGraph2_VTable =
{
    FilterGraph2_QueryInterface,
    FilterGraph2_AddRef,
    FilterGraph2_Release,
    FilterGraph2_AddFilter,
    FilterGraph2_RemoveFilter,
    FilterGraph2_EnumFilters,
    FilterGraph2_FindFilterByName,
    FilterGraph2_ConnectDirect,
    FilterGraph2_Reconnect,
    FilterGraph2_Disconnect,
    FilterGraph2_SetDefaultSyncSource,
    FilterGraph2_Connect,
    FilterGraph2_Render,
    FilterGraph2_RenderFile,
    FilterGraph2_AddSourceFilter,
    FilterGraph2_SetLogFile,
    FilterGraph2_Abort,
    FilterGraph2_ShouldOperationContinue,
    FilterGraph2_AddSourceFilterForMoniker,
    FilterGraph2_ReconnectEx,
    FilterGraph2_RenderEx
};

static inline IFilterGraphImpl *impl_from_IMediaControl(IMediaControl *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IMediaControl_iface);
}

static HRESULT WINAPI MediaControl_QueryInterface(IMediaControl *iface, REFIID riid, void **ppvObj)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI MediaControl_AddRef(IMediaControl *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MediaControl_Release(IMediaControl *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);

}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaControl_GetTypeInfoCount(IMediaControl *iface, UINT *pctinfo)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI MediaControl_GetTypeInfo(IMediaControl *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI MediaControl_GetIDsOfNames(IMediaControl *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->(%s, %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), rgszNames,
            cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI MediaControl_Invoke(IMediaControl *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExepInfo,
        UINT *puArgErr)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->(%d, %s, %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember,
            debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

typedef HRESULT(WINAPI *fnFoundFilter)(IBaseFilter *, DWORD_PTR data);

static BOOL has_output_pins(IBaseFilter *filter)
{
    IEnumPins *enumpins;
    PIN_DIRECTION dir;
    IPin *pin;

    if (FAILED(IBaseFilter_EnumPins(filter, &enumpins)))
        return FALSE;

    while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
    {
        IPin_QueryDirection(pin, &dir);
        IPin_Release(pin);
        if (dir == PINDIR_OUTPUT)
        {
            IEnumPins_Release(enumpins);
            return TRUE;
        }
    }

    IEnumPins_Release(enumpins);
    return FALSE;
}

static HRESULT ExploreGraph(IFilterGraphImpl* pGraph, IPin* pOutputPin, fnFoundFilter FoundFilter, DWORD_PTR data)
{
    IAMFilterMiscFlags *flags;
    IMediaSeeking *seeking;
    IEnumPins *enumpins;
    PIN_DIRECTION dir;
    HRESULT hr;
    IPin* pInputPin;
    PIN_INFO PinInfo;
    IPin *pin;

    TRACE("%p %p\n", pGraph, pOutputPin);
    PinInfo.pFilter = NULL;

    hr = IPin_ConnectedTo(pOutputPin, &pInputPin);

    if (SUCCEEDED(hr))
    {
        hr = IPin_QueryPinInfo(pInputPin, &PinInfo);
        IPin_Release(pInputPin);
    }

    if (SUCCEEDED(hr))
        hr = IBaseFilter_EnumPins(PinInfo.pFilter, &enumpins);

    if (SUCCEEDED(hr))
    {
        while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
        {
            IPin_QueryDirection(pin, &dir);
            if (dir == PINDIR_OUTPUT)
                ExploreGraph(pGraph, pin, FoundFilter, data);
            IPin_Release(pin);
        }

        IEnumPins_Release(enumpins);
        TRACE("Doing stuff with filter %p\n", PinInfo.pFilter);

        if (SUCCEEDED(IBaseFilter_QueryInterface(PinInfo.pFilter,
                &IID_IAMFilterMiscFlags, (void **)&flags)))
        {
            if (IAMFilterMiscFlags_GetMiscFlags(flags) & AM_FILTER_MISC_FLAGS_IS_RENDERER)
                pGraph->nRenderers++;
            IAMFilterMiscFlags_Release(flags);
        }
        else if (SUCCEEDED(IBaseFilter_QueryInterface(PinInfo.pFilter,
                &IID_IMediaSeeking, (void **)&seeking)))
        {
            if (!has_output_pins(PinInfo.pFilter))
                pGraph->nRenderers++;
            IMediaSeeking_Release(seeking);
        }

        FoundFilter(PinInfo.pFilter, data);
    }

    if (PinInfo.pFilter) IBaseFilter_Release(PinInfo.pFilter);
    return hr;
}

static HRESULT WINAPI SendRun(IBaseFilter *pFilter, DWORD_PTR data)
{
    REFERENCE_TIME time = *(REFERENCE_TIME*)data;
    return IBaseFilter_Run(pFilter, time);
}

static HRESULT WINAPI SendPause(IBaseFilter *pFilter, DWORD_PTR data)
{
    return IBaseFilter_Pause(pFilter);
}

static HRESULT WINAPI SendStop(IBaseFilter *pFilter, DWORD_PTR data)
{
    return IBaseFilter_Stop(pFilter);
}

static HRESULT WINAPI SendGetState(IBaseFilter *pFilter, DWORD_PTR data)
{
    FILTER_STATE state;
    DWORD time_end = data;
    DWORD time_now = GetTickCount();
    LONG wait;

    if (time_end == INFINITE)
    {
        wait = INFINITE;
    }
    else if (time_end > time_now)
    {
        wait = time_end - time_now;
    }
    else
        wait = 0;

    return IBaseFilter_GetState(pFilter, wait, &state);
}


static HRESULT SendFilterMessage(IFilterGraphImpl *This, fnFoundFilter FoundFilter, DWORD_PTR data)
{
    struct filter *filter;
    IEnumPins* pEnum;
    HRESULT hr;
    IPin* pPin;
    DWORD dummy;
    PIN_DIRECTION dir;

    TRACE("(%p)->()\n", This);

    /* Explorer the graph from source filters to renderers, determine renderers
     * number and run filters from renderers to source filters */
    This->nRenderers = 0;
    ResetEvent(This->hEventCompletion);

    LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
    {
        BOOL source = TRUE;
        hr = IBaseFilter_EnumPins(filter->filter, &pEnum);
        if (hr != S_OK)
        {
            WARN("Enum pins failed %x\n", hr);
            continue;
        }
        /* Check if it is a source filter */
        while(IEnumPins_Next(pEnum, 1, &pPin, &dummy) == S_OK)
        {
            IPin_QueryDirection(pPin, &dir);
            IPin_Release(pPin);
            if (dir == PINDIR_INPUT)
            {
                source = FALSE;
                break;
            }
        }
        if (source)
        {
            TRACE("Found source filter %p.\n", filter->filter);
            IEnumPins_Reset(pEnum);
            while(IEnumPins_Next(pEnum, 1, &pPin, &dummy) == S_OK)
            {
                /* Explore the graph downstream from this pin */
                ExploreGraph(This, pPin, FoundFilter, data);
                IPin_Release(pPin);
            }
            FoundFilter(filter->filter, data);
        }
        IEnumPins_Release(pEnum);
    }

    return S_FALSE;
}

static HRESULT WINAPI MediaControl_Run(IMediaControl *iface)
{
    IFilterGraphImpl *graph = impl_from_IMediaControl(iface);

    return IMediaFilter_Run(&graph->IMediaFilter_iface, 0);
}

static HRESULT WINAPI MediaControl_Pause(IMediaControl *iface)
{
    IFilterGraphImpl *graph = impl_from_IMediaControl(iface);

    return IMediaFilter_Pause(&graph->IMediaFilter_iface);
}

static HRESULT WINAPI MediaControl_Stop(IMediaControl *iface)
{
    IFilterGraphImpl *graph = impl_from_IMediaControl(iface);

    return IMediaFilter_Stop(&graph->IMediaFilter_iface);
}

static HRESULT WINAPI MediaControl_GetState(IMediaControl *iface, LONG timeout, OAFilterState *state)
{
    IFilterGraphImpl *graph = impl_from_IMediaControl(iface);

    if (timeout < 0) timeout = INFINITE;

    return IMediaFilter_GetState(&graph->IMediaFilter_iface, timeout, (FILTER_STATE *)state);
}

static HRESULT WINAPI MediaControl_RenderFile(IMediaControl *iface, BSTR strFilename)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strFilename), strFilename);

    return IFilterGraph2_RenderFile(&This->IFilterGraph2_iface, strFilename, NULL);
}

static HRESULT WINAPI MediaControl_AddSourceFilter(IMediaControl *iface, BSTR strFilename,
        IDispatch **ppUnk)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    FIXME("(%p/%p)->(%s (%p), %p): stub !!!\n", This, iface, debugstr_w(strFilename), strFilename, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_get_FilterCollection(IMediaControl *iface, IDispatch **ppUnk)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_get_RegFilterCollection(IMediaControl *iface, IDispatch **ppUnk)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_StopWhenReady(IMediaControl *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaControl(iface);

    FIXME("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}


static const IMediaControlVtbl IMediaControl_VTable =
{
    MediaControl_QueryInterface,
    MediaControl_AddRef,
    MediaControl_Release,
    MediaControl_GetTypeInfoCount,
    MediaControl_GetTypeInfo,
    MediaControl_GetIDsOfNames,
    MediaControl_Invoke,
    MediaControl_Run,
    MediaControl_Pause,
    MediaControl_Stop,
    MediaControl_GetState,
    MediaControl_RenderFile,
    MediaControl_AddSourceFilter,
    MediaControl_get_FilterCollection,
    MediaControl_get_RegFilterCollection,
    MediaControl_StopWhenReady
};

static inline IFilterGraphImpl *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IMediaSeeking_iface);
}

static HRESULT WINAPI MediaSeeking_QueryInterface(IMediaSeeking *iface, REFIID riid, void **ppvObj)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI MediaSeeking_AddRef(IMediaSeeking *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MediaSeeking_Release(IMediaSeeking *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);
}

typedef HRESULT (WINAPI *fnFoundSeek)(IFilterGraphImpl *This, IMediaSeeking*, DWORD_PTR arg);

static HRESULT all_renderers_seek(IFilterGraphImpl *This, fnFoundSeek FoundSeek, DWORD_PTR arg) {
    BOOL allnotimpl = TRUE;
    HRESULT hr, hr_return = S_OK;
    struct filter *filter;

    TRACE("(%p)->(%p %08lx)\n", This, FoundSeek, arg);
    /* Send a message to all renderers, they are responsible for broadcasting it further */

    LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
    {
        IMediaSeeking *seek = NULL;

        IBaseFilter_QueryInterface(filter->filter, &IID_IMediaSeeking, (void **)&seek);
        if (!seek)
            continue;
        hr = FoundSeek(This, seek, arg);
        IMediaSeeking_Release(seek);
        if (hr_return != E_NOTIMPL)
            allnotimpl = FALSE;
        if (hr_return == S_OK || (FAILED(hr) && hr != E_NOTIMPL && SUCCEEDED(hr_return)))
            hr_return = hr;
    }

    if (allnotimpl)
        return E_NOTIMPL;
    return hr_return;
}

static HRESULT WINAPI FoundCapabilities(IFilterGraphImpl *This, IMediaSeeking *seek, DWORD_PTR pcaps)
{
    HRESULT hr;
    DWORD caps = 0;

    hr = IMediaSeeking_GetCapabilities(seek, &caps);
    if (FAILED(hr))
        return hr;

    /* Only add common capabilities everything supports */
    *(DWORD*)pcaps &= caps;

    return hr;
}

/*** IMediaSeeking methods ***/
static HRESULT WINAPI MediaSeeking_GetCapabilities(IMediaSeeking *iface, DWORD *pCapabilities)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    *pCapabilities = 0xffffffff;

    hr = all_renderers_seek(This, FoundCapabilities, (DWORD_PTR)pCapabilities);
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaSeeking_CheckCapabilities(IMediaSeeking *iface, DWORD *pCapabilities)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);
    DWORD originalcaps;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    originalcaps = *pCapabilities;
    hr = all_renderers_seek(This, FoundCapabilities, (DWORD_PTR)pCapabilities);
    LeaveCriticalSection(&This->cs);

    if (FAILED(hr))
        return hr;

    if (!*pCapabilities)
        return E_FAIL;
    if (*pCapabilities != originalcaps)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI MediaSeeking_IsFormatSupported(IMediaSeeking *iface, const GUID *pFormat)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%s)\n", This, iface, debugstr_guid(pFormat));

    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, pFormat))
    {
        WARN("Unhandled time format %s\n", debugstr_guid(pFormat));
        return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_QueryPreferredFormat(IMediaSeeking *iface, GUID *pFormat)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    if (!pFormat)
        return E_POINTER;

    FIXME("(%p/%p)->(%p): semi-stub !!!\n", This, iface, pFormat);
    memcpy(pFormat, &TIME_FORMAT_MEDIA_TIME, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetTimeFormat(IMediaSeeking *iface, GUID *pFormat)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%p)\n", This, iface, pFormat);
    memcpy(pFormat, &This->timeformatseek, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_IsUsingTimeFormat(IMediaSeeking *iface, const GUID *pFormat)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pFormat);
    if (!pFormat)
        return E_POINTER;

    if (memcmp(pFormat, &This->timeformatseek, sizeof(GUID)))
        return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetTimeFormat(IMediaSeeking *iface, const GUID *pFormat)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%s)\n", This, iface, debugstr_guid(pFormat));

    if (This->state != State_Stopped)
        return VFW_E_WRONG_STATE;

    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, pFormat))
    {
        FIXME("Unhandled time format %s\n", debugstr_guid(pFormat));
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI FoundDuration(IFilterGraphImpl *This, IMediaSeeking *seek, DWORD_PTR pduration)
{
    HRESULT hr;
    LONGLONG duration = 0, *pdur = (LONGLONG*)pduration;

    hr = IMediaSeeking_GetDuration(seek, &duration);
    if (FAILED(hr))
        return hr;

    if (*pdur < duration)
        *pdur = duration;
    return hr;
}

static HRESULT WINAPI MediaSeeking_GetDuration(IMediaSeeking *iface, LONGLONG *pDuration)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDuration);

    if (!pDuration)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    *pDuration = 0;
    hr = all_renderers_seek(This, FoundDuration, (DWORD_PTR)pDuration);
    LeaveCriticalSection(&This->cs);

    TRACE("--->%08x\n", hr);
    return hr;
}

static HRESULT WINAPI MediaSeeking_GetStopPosition(IMediaSeeking *iface, LONGLONG *pStop)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->(%p)\n", This, iface, pStop);

    if (!pStop)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    if (This->stop_position < 0)
        /* Stop position not set, use duration instead */
        hr = IMediaSeeking_GetDuration(iface, pStop);
    else
        *pStop = This->stop_position;
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaSeeking_GetCurrentPosition(IMediaSeeking *iface, LONGLONG *pCurrent)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);
    LONGLONG time = 0;

    if (!pCurrent)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    if (This->state == State_Running && This->refClock && This->start_time >= 0)
    {
        IReferenceClock_GetTime(This->refClock, &time);
        if (time)
            time -= This->start_time;
    }
    if (This->pause_time > 0)
        time += This->pause_time;
    *pCurrent = time;
    LeaveCriticalSection(&This->cs);

    TRACE("Time: %u.%03u\n", (DWORD)(*pCurrent / 10000000), (DWORD)((*pCurrent / 10000)%1000));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *pTarget,
        const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->(%p, %s, 0x%s, %s)\n", This, iface, pTarget,
        debugstr_guid(pTargetFormat), wine_dbgstr_longlong(Source), debugstr_guid(pSourceFormat));

    if (!pSourceFormat)
        pSourceFormat = &This->timeformatseek;

    if (!pTargetFormat)
        pTargetFormat = &This->timeformatseek;

    if (IsEqualGUID(pTargetFormat, pSourceFormat))
        *pTarget = Source;
    else
        FIXME("conversion %s->%s not supported\n", debugstr_guid(pSourceFormat), debugstr_guid(pTargetFormat));

    return S_OK;
}

struct pos_args {
    LONGLONG* current, *stop;
    DWORD curflags, stopflags;
};

static HRESULT WINAPI found_setposition(IFilterGraphImpl *This, IMediaSeeking *seek, DWORD_PTR pargs)
{
    struct pos_args *args = (void*)pargs;

    return IMediaSeeking_SetPositions(seek, args->current, args->curflags, args->stop, args->stopflags);
}

static HRESULT WINAPI MediaSeeking_SetPositions(IMediaSeeking *iface, LONGLONG *pCurrent,
        DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);
    HRESULT hr = S_OK;
    FILTER_STATE state;
    struct pos_args args;

    TRACE("(%p/%p)->(%p, %08x, %p, %08x)\n", This, iface, pCurrent, dwCurrentFlags, pStop, dwStopFlags);

    EnterCriticalSection(&This->cs);
    state = This->state;
    TRACE("State: %s\n", state == State_Running ? "Running" : (state == State_Paused ? "Paused" : (state == State_Stopped ? "Stopped" : "UNKNOWN")));

    if ((dwCurrentFlags & 0x7) != AM_SEEKING_AbsolutePositioning &&
        (dwCurrentFlags & 0x7) != AM_SEEKING_NoPositioning)
        FIXME("Adjust method %x not handled yet!\n", dwCurrentFlags & 0x7);

    if ((dwStopFlags & 0x7) == AM_SEEKING_AbsolutePositioning)
        This->stop_position = *pStop;
    else if ((dwStopFlags & 0x7) != AM_SEEKING_NoPositioning)
        FIXME("Stop position not handled yet!\n");

    if (state == State_Running && !(dwCurrentFlags & AM_SEEKING_NoFlush))
        IMediaControl_Pause(&This->IMediaControl_iface);
    args.current = pCurrent;
    args.stop = pStop;
    args.curflags = dwCurrentFlags;
    args.stopflags = dwStopFlags;
    hr = all_renderers_seek(This, found_setposition, (DWORD_PTR)&args);

    if ((dwCurrentFlags & 0x7) != AM_SEEKING_NoPositioning)
        This->pause_time = This->start_time = -1;
    if (state == State_Running && !(dwCurrentFlags & AM_SEEKING_NoFlush))
        IMediaControl_Run(&This->IMediaControl_iface);
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaSeeking_GetPositions(IMediaSeeking *iface, LONGLONG *pCurrent,
        LONGLONG *pStop)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pCurrent, pStop);
    hr = IMediaSeeking_GetCurrentPosition(iface, pCurrent);
    if (SUCCEEDED(hr))
        hr = IMediaSeeking_GetStopPosition(iface, pStop);

    return hr;
}

static HRESULT WINAPI MediaSeeking_GetAvailable(IMediaSeeking *iface, LONGLONG *pEarliest,
        LONGLONG *pLatest)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    FIXME("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pEarliest, pLatest);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetRate(IMediaSeeking *iface, double dRate)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    FIXME("(%p/%p)->(%f): stub !!!\n", This, iface, dRate);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetRate(IMediaSeeking *iface, double *pdRate)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pdRate);

    if (!pdRate)
        return E_POINTER;

    *pdRate = 1.0;

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetPreroll(IMediaSeeking *iface, LONGLONG *pllPreroll)
{
    IFilterGraphImpl *This = impl_from_IMediaSeeking(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pllPreroll);

    return S_OK;
}


static const IMediaSeekingVtbl IMediaSeeking_VTable =
{
    MediaSeeking_QueryInterface,
    MediaSeeking_AddRef,
    MediaSeeking_Release,
    MediaSeeking_GetCapabilities,
    MediaSeeking_CheckCapabilities,
    MediaSeeking_IsFormatSupported,
    MediaSeeking_QueryPreferredFormat,
    MediaSeeking_GetTimeFormat,
    MediaSeeking_IsUsingTimeFormat,
    MediaSeeking_SetTimeFormat,
    MediaSeeking_GetDuration,
    MediaSeeking_GetStopPosition,
    MediaSeeking_GetCurrentPosition,
    MediaSeeking_ConvertTimeFormat,
    MediaSeeking_SetPositions,
    MediaSeeking_GetPositions,
    MediaSeeking_GetAvailable,
    MediaSeeking_SetRate,
    MediaSeeking_GetRate,
    MediaSeeking_GetPreroll
};

static inline IFilterGraphImpl *impl_from_IMediaPosition(IMediaPosition *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IMediaPosition_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI MediaPosition_QueryInterface(IMediaPosition* iface, REFIID riid, void** ppvObj)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI MediaPosition_AddRef(IMediaPosition *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MediaPosition_Release(IMediaPosition *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);
}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaPosition_GetTypeInfoCount(IMediaPosition *iface, UINT* pctinfo)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_GetTypeInfo(IMediaPosition *iface, UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_GetIDsOfNames(IMediaPosition* iface, REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_Invoke(IMediaPosition* iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT ConvertFromREFTIME(IMediaSeeking *seek, REFTIME time_in, LONGLONG *time_out)
{
    GUID time_format;
    HRESULT hr;

    hr = MediaSeeking_GetTimeFormat(seek, &time_format);
    if (FAILED(hr))
        return hr;
    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, &time_format))
    {
        FIXME("Unsupported time format.\n");
        return E_NOTIMPL;
    }

    *time_out = (LONGLONG) (time_in * 10000000); /* convert from 1 second intervals to 100 ns intervals */
    return S_OK;
}

static HRESULT ConvertToREFTIME(IMediaSeeking *seek, LONGLONG time_in, REFTIME *time_out)
{
    GUID time_format;
    HRESULT hr;

    hr = MediaSeeking_GetTimeFormat(seek, &time_format);
    if (FAILED(hr))
        return hr;
    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, &time_format))
    {
        FIXME("Unsupported time format.\n");
        return E_NOTIMPL;
    }

    *time_out = (REFTIME)time_in / 10000000; /* convert from 100 ns intervals to 1 second intervals */
    return S_OK;
}

/*** IMediaPosition methods ***/
static HRESULT WINAPI MediaPosition_get_Duration(IMediaPosition * iface, REFTIME *plength)
{
    LONGLONG duration;
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );
    HRESULT hr = IMediaSeeking_GetDuration(&This->IMediaSeeking_iface, &duration);
    if (FAILED(hr))
        return hr;
    return ConvertToREFTIME(&This->IMediaSeeking_iface, duration, plength);
}

static HRESULT WINAPI MediaPosition_put_CurrentPosition(IMediaPosition * iface, REFTIME llTime)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );
    LONGLONG reftime;
    HRESULT hr;

    hr = ConvertFromREFTIME(&This->IMediaSeeking_iface, llTime, &reftime);
    if (FAILED(hr))
        return hr;
    return IMediaSeeking_SetPositions(&This->IMediaSeeking_iface, &reftime,
            AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

static HRESULT WINAPI MediaPosition_get_CurrentPosition(IMediaPosition * iface, REFTIME *pllTime)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );
    LONGLONG pos;
    HRESULT hr;

    hr = IMediaSeeking_GetCurrentPosition(&This->IMediaSeeking_iface, &pos);
    if (FAILED(hr))
        return hr;
    return ConvertToREFTIME(&This->IMediaSeeking_iface, pos, pllTime);
}

static HRESULT WINAPI MediaPosition_get_StopTime(IMediaPosition * iface, REFTIME *pllTime)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );
    LONGLONG pos;
    HRESULT hr = IMediaSeeking_GetStopPosition(&This->IMediaSeeking_iface, &pos);
    if (FAILED(hr))
        return hr;
    return ConvertToREFTIME(&This->IMediaSeeking_iface, pos, pllTime);
}

static HRESULT WINAPI MediaPosition_put_StopTime(IMediaPosition * iface, REFTIME llTime)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );
    LONGLONG reftime;
    HRESULT hr;

    hr = ConvertFromREFTIME(&This->IMediaSeeking_iface, llTime, &reftime);
    if (FAILED(hr))
        return hr;
    return IMediaSeeking_SetPositions(&This->IMediaSeeking_iface, NULL, AM_SEEKING_NoPositioning,
            &reftime, AM_SEEKING_AbsolutePositioning);
}

static HRESULT WINAPI MediaPosition_get_PrerollTime(IMediaPosition * iface, REFTIME *pllTime)
{
    FIXME("(%p)->(%p) stub!\n", iface, pllTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_PrerollTime(IMediaPosition * iface, REFTIME llTime)
{
    FIXME("(%p)->(%f) stub!\n", iface, llTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_Rate(IMediaPosition * iface, double dRate)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );
    return IMediaSeeking_SetRate(&This->IMediaSeeking_iface, dRate);
}

static HRESULT WINAPI MediaPosition_get_Rate(IMediaPosition * iface, double *pdRate)
{
    IFilterGraphImpl *This = impl_from_IMediaPosition( iface );
    return IMediaSeeking_GetRate(&This->IMediaSeeking_iface, pdRate);
}

static HRESULT WINAPI MediaPosition_CanSeekForward(IMediaPosition * iface, LONG *pCanSeekForward)
{
    FIXME("(%p)->(%p) stub!\n", iface, pCanSeekForward);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_CanSeekBackward(IMediaPosition * iface, LONG *pCanSeekBackward)
{
    FIXME("(%p)->(%p) stub!\n", iface, pCanSeekBackward);
    return E_NOTIMPL;
}


static const IMediaPositionVtbl IMediaPosition_VTable =
{
    MediaPosition_QueryInterface,
    MediaPosition_AddRef,
    MediaPosition_Release,
    MediaPosition_GetTypeInfoCount,
    MediaPosition_GetTypeInfo,
    MediaPosition_GetIDsOfNames,
    MediaPosition_Invoke,
    MediaPosition_get_Duration,
    MediaPosition_put_CurrentPosition,
    MediaPosition_get_CurrentPosition,
    MediaPosition_get_StopTime,
    MediaPosition_put_StopTime,
    MediaPosition_get_PrerollTime,
    MediaPosition_put_PrerollTime,
    MediaPosition_put_Rate,
    MediaPosition_get_Rate,
    MediaPosition_CanSeekForward,
    MediaPosition_CanSeekBackward
};

static inline IFilterGraphImpl *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IObjectWithSite_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ObjectWithSite_QueryInterface(IObjectWithSite* iface, REFIID riid, void** ppvObj)
{
    IFilterGraphImpl *This = impl_from_IObjectWithSite( iface );

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI ObjectWithSite_AddRef(IObjectWithSite *iface)
{
    IFilterGraphImpl *This = impl_from_IObjectWithSite( iface );

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI ObjectWithSite_Release(IObjectWithSite *iface)
{
    IFilterGraphImpl *This = impl_from_IObjectWithSite( iface );

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);
}

/*** IObjectWithSite methods ***/

static HRESULT WINAPI ObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    IFilterGraphImpl *This = impl_from_IObjectWithSite( iface );

    TRACE("(%p/%p)->()\n", This, iface);
    if (This->pSite) IUnknown_Release(This->pSite);
    This->pSite = pUnkSite;
    IUnknown_AddRef(This->pSite);
    return S_OK;
}

static HRESULT WINAPI ObjectWithSite_GetSite(IObjectWithSite *iface, REFIID riid, PVOID *ppvSite)
{
    IFilterGraphImpl *This = impl_from_IObjectWithSite( iface );

    TRACE("(%p/%p)->(%s)\n", This, iface,debugstr_guid(riid));

    *ppvSite = NULL;
    if (!This->pSite)
        return E_FAIL;
    else
        return IUnknown_QueryInterface(This->pSite, riid, ppvSite);
}

static const IObjectWithSiteVtbl IObjectWithSite_VTable =
{
    ObjectWithSite_QueryInterface,
    ObjectWithSite_AddRef,
    ObjectWithSite_Release,
    ObjectWithSite_SetSite,
    ObjectWithSite_GetSite,
};

static HRESULT GetTargetInterface(IFilterGraphImpl* pGraph, REFIID riid, LPVOID* ppvObj)
{
    struct filter *filter;
    HRESULT hr;
    int entry;

    /* Check if the interface type is already registered */
    for (entry = 0; entry < pGraph->nItfCacheEntries; entry++)
        if (riid == pGraph->ItfCacheEntries[entry].riid)
        {
            if (pGraph->ItfCacheEntries[entry].iface)
            {
                /* Return the interface if available */
                *ppvObj = pGraph->ItfCacheEntries[entry].iface;
                return S_OK;
            }
            break;
        }

    if (entry >= MAX_ITF_CACHE_ENTRIES)
    {
        FIXME("Not enough space to store interface in the cache\n");
        return E_OUTOFMEMORY;
    }

    /* Find a filter supporting the requested interface */
    LIST_FOR_EACH_ENTRY(filter, &pGraph->filters, struct filter, entry)
    {
        hr = IBaseFilter_QueryInterface(filter->filter, riid, ppvObj);
        if (hr == S_OK)
        {
            pGraph->ItfCacheEntries[entry].riid = riid;
            pGraph->ItfCacheEntries[entry].filter = filter->filter;
            pGraph->ItfCacheEntries[entry].iface = *ppvObj;
            if (entry >= pGraph->nItfCacheEntries)
                pGraph->nItfCacheEntries++;
            return S_OK;
        }
        if (hr != E_NOINTERFACE)
            return hr;
    }

    return IsEqualGUID(riid, &IID_IBasicAudio) ? E_NOTIMPL : E_NOINTERFACE;
}

static inline IFilterGraphImpl *impl_from_IBasicAudio(IBasicAudio *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IBasicAudio_iface);
}

static HRESULT WINAPI BasicAudio_QueryInterface(IBasicAudio *iface, REFIID riid, void **ppvObj)
{
    IFilterGraphImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI BasicAudio_AddRef(IBasicAudio *iface)
{
    IFilterGraphImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI BasicAudio_Release(IBasicAudio *iface)
{
    IFilterGraphImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI BasicAudio_GetTypeInfoCount(IBasicAudio *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI BasicAudio_GetTypeInfo(IBasicAudio *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#x, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IBasicAudio_tid, typeinfo);
}

static HRESULT WINAPI BasicAudio_GetIDsOfNames(IBasicAudio *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#x, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicAudio_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI BasicAudio_Invoke(IBasicAudio *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %d, iid %s, lcid %#x, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicAudio_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

/*** IBasicAudio methods ***/
static HRESULT WINAPI BasicAudio_put_Volume(IBasicAudio *iface, LONG lVolume)
{
    IFilterGraphImpl *This = impl_from_IBasicAudio(iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, lVolume);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_put_Volume(pBasicAudio, lVolume);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_get_Volume(IBasicAudio *iface, LONG *plVolume)
{
    IFilterGraphImpl *This = impl_from_IBasicAudio(iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, plVolume);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_get_Volume(pBasicAudio, plVolume);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_put_Balance(IBasicAudio *iface, LONG lBalance)
{
    IFilterGraphImpl *This = impl_from_IBasicAudio(iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, lBalance);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_put_Balance(pBasicAudio, lBalance);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_get_Balance(IBasicAudio *iface, LONG *plBalance)
{
    IFilterGraphImpl *This = impl_from_IBasicAudio(iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, plBalance);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_get_Balance(pBasicAudio, plBalance);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static const IBasicAudioVtbl IBasicAudio_VTable =
{
    BasicAudio_QueryInterface,
    BasicAudio_AddRef,
    BasicAudio_Release,
    BasicAudio_GetTypeInfoCount,
    BasicAudio_GetTypeInfo,
    BasicAudio_GetIDsOfNames,
    BasicAudio_Invoke,
    BasicAudio_put_Volume,
    BasicAudio_get_Volume,
    BasicAudio_put_Balance,
    BasicAudio_get_Balance
};

static inline IFilterGraphImpl *impl_from_IBasicVideo2(IBasicVideo2 *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IBasicVideo2_iface);
}

static HRESULT WINAPI BasicVideo_QueryInterface(IBasicVideo2 *iface, REFIID riid, void **ppvObj)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI BasicVideo_AddRef(IBasicVideo2 *iface)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI BasicVideo_Release(IBasicVideo2 *iface)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI BasicVideo_GetTypeInfoCount(IBasicVideo2 *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI BasicVideo_GetTypeInfo(IBasicVideo2 *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#x, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IBasicVideo_tid, typeinfo);
}

static HRESULT WINAPI BasicVideo_GetIDsOfNames(IBasicVideo2 *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#x, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicVideo_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI BasicVideo_Invoke(IBasicVideo2 *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %d, iid %s, lcid %#x, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicVideo_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

/*** IBasicVideo methods ***/
static HRESULT WINAPI BasicVideo_get_AvgTimePerFrame(IBasicVideo2 *iface, REFTIME *pAvgTimePerFrame)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pAvgTimePerFrame);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_AvgTimePerFrame(pBasicVideo, pAvgTimePerFrame);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_BitRate(IBasicVideo2 *iface, LONG *pBitRate)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitRate);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_BitRate(pBasicVideo, pBitRate);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_BitErrorRate(IBasicVideo2 *iface, LONG *pBitErrorRate)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitErrorRate);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_BitErrorRate(pBasicVideo, pBitErrorRate);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_VideoWidth(IBasicVideo2 *iface, LONG *pVideoWidth)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_VideoWidth(pBasicVideo, pVideoWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_VideoHeight(IBasicVideo2 *iface, LONG *pVideoHeight)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_VideoHeight(pBasicVideo, pVideoHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceLeft(IBasicVideo2 *iface, LONG SourceLeft)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceLeft(pBasicVideo, SourceLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceLeft(IBasicVideo2 *iface, LONG *pSourceLeft)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceLeft(pBasicVideo, pSourceLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceWidth(IBasicVideo2 *iface, LONG SourceWidth)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceWidth(pBasicVideo, SourceWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceWidth(IBasicVideo2 *iface, LONG *pSourceWidth)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceWidth(pBasicVideo, pSourceWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceTop(IBasicVideo2 *iface, LONG SourceTop)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceTop(pBasicVideo, SourceTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceTop(IBasicVideo2 *iface, LONG *pSourceTop)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceTop(pBasicVideo, pSourceTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceHeight(IBasicVideo2 *iface, LONG SourceHeight)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, SourceHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceHeight(pBasicVideo, SourceHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceHeight(IBasicVideo2 *iface, LONG *pSourceHeight)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceHeight(pBasicVideo, pSourceHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationLeft(IBasicVideo2 *iface, LONG DestinationLeft)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationLeft(pBasicVideo, DestinationLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationLeft(IBasicVideo2 *iface, LONG *pDestinationLeft)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationLeft(pBasicVideo, pDestinationLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationWidth(IBasicVideo2 *iface, LONG DestinationWidth)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationWidth(pBasicVideo, DestinationWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationWidth(IBasicVideo2 *iface, LONG *pDestinationWidth)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationWidth(pBasicVideo, pDestinationWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationTop(IBasicVideo2 *iface, LONG DestinationTop)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationTop(pBasicVideo, DestinationTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationTop(IBasicVideo2 *iface, LONG *pDestinationTop)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationTop(pBasicVideo, pDestinationTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationHeight(IBasicVideo2 *iface, LONG DestinationHeight)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, DestinationHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationHeight(pBasicVideo, DestinationHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationHeight(IBasicVideo2 *iface,
        LONG *pDestinationHeight)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationHeight(pBasicVideo, pDestinationHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetSourcePosition(IBasicVideo2 *iface, LONG Left, LONG Top,
        LONG Width, LONG Height)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetSourcePosition(pBasicVideo, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetSourcePosition(IBasicVideo2 *iface, LONG *pLeft, LONG *pTop,
        LONG *pWidth, LONG *pHeight)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetSourcePosition(pBasicVideo, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDefaultSourcePosition(IBasicVideo2 *iface)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDefaultSourcePosition(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDestinationPosition(IBasicVideo2 *iface, LONG Left, LONG Top,
        LONG Width, LONG Height)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDestinationPosition(pBasicVideo, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetDestinationPosition(IBasicVideo2 *iface, LONG *pLeft,
        LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetDestinationPosition(pBasicVideo, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDefaultDestinationPosition(IBasicVideo2 *iface)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDefaultDestinationPosition(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetVideoSize(IBasicVideo2 *iface, LONG *pWidth, LONG *pHeight)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetVideoSize(pBasicVideo, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetVideoPaletteEntries(IBasicVideo2 *iface, LONG StartIndex,
        LONG Entries, LONG *pRetrieved, LONG *pPalette)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %p, %p)\n", This, iface, StartIndex, Entries, pRetrieved, pPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetVideoPaletteEntries(pBasicVideo, StartIndex, Entries, pRetrieved, pPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetCurrentImage(IBasicVideo2 *iface, LONG *pBufferSize,
        LONG *pDIBImage)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pBufferSize, pDIBImage);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetCurrentImage(pBasicVideo, pBufferSize, pDIBImage);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_IsUsingDefaultSource(IBasicVideo2 *iface)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_IsUsingDefaultSource(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_IsUsingDefaultDestination(IBasicVideo2 *iface)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_IsUsingDefaultDestination(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo2_GetPreferredAspectRatio(IBasicVideo2 *iface, LONG *plAspectX,
        LONG *plAspectY)
{
    IFilterGraphImpl *This = impl_from_IBasicVideo2(iface);
    IBasicVideo2 *pBasicVideo2;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo2, (LPVOID*)&pBasicVideo2);

    if (hr == S_OK)
        hr = BasicVideo2_GetPreferredAspectRatio(iface, plAspectX, plAspectY);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static const IBasicVideo2Vtbl IBasicVideo_VTable =
{
    BasicVideo_QueryInterface,
    BasicVideo_AddRef,
    BasicVideo_Release,
    BasicVideo_GetTypeInfoCount,
    BasicVideo_GetTypeInfo,
    BasicVideo_GetIDsOfNames,
    BasicVideo_Invoke,
    BasicVideo_get_AvgTimePerFrame,
    BasicVideo_get_BitRate,
    BasicVideo_get_BitErrorRate,
    BasicVideo_get_VideoWidth,
    BasicVideo_get_VideoHeight,
    BasicVideo_put_SourceLeft,
    BasicVideo_get_SourceLeft,
    BasicVideo_put_SourceWidth,
    BasicVideo_get_SourceWidth,
    BasicVideo_put_SourceTop,
    BasicVideo_get_SourceTop,
    BasicVideo_put_SourceHeight,
    BasicVideo_get_SourceHeight,
    BasicVideo_put_DestinationLeft,
    BasicVideo_get_DestinationLeft,
    BasicVideo_put_DestinationWidth,
    BasicVideo_get_DestinationWidth,
    BasicVideo_put_DestinationTop,
    BasicVideo_get_DestinationTop,
    BasicVideo_put_DestinationHeight,
    BasicVideo_get_DestinationHeight,
    BasicVideo_SetSourcePosition,
    BasicVideo_GetSourcePosition,
    BasicVideo_SetDefaultSourcePosition,
    BasicVideo_SetDestinationPosition,
    BasicVideo_GetDestinationPosition,
    BasicVideo_SetDefaultDestinationPosition,
    BasicVideo_GetVideoSize,
    BasicVideo_GetVideoPaletteEntries,
    BasicVideo_GetCurrentImage,
    BasicVideo_IsUsingDefaultSource,
    BasicVideo_IsUsingDefaultDestination,
    BasicVideo2_GetPreferredAspectRatio
};

static inline IFilterGraphImpl *impl_from_IVideoWindow(IVideoWindow *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IVideoWindow_iface);
}

static HRESULT WINAPI VideoWindow_QueryInterface(IVideoWindow *iface, REFIID riid, void **ppvObj)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI VideoWindow_AddRef(IVideoWindow *iface)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI VideoWindow_Release(IVideoWindow *iface)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);
}

HRESULT WINAPI VideoWindow_GetTypeInfoCount(IVideoWindow *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

HRESULT WINAPI VideoWindow_GetTypeInfo(IVideoWindow *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#x, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IVideoWindow_tid, typeinfo);
}

HRESULT WINAPI VideoWindow_GetIDsOfNames(IVideoWindow *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#x, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IVideoWindow_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI VideoWindow_Invoke(IVideoWindow *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %d, iid %s, lcid %#x, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IVideoWindow_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

/*** IVideoWindow methods ***/
static HRESULT WINAPI VideoWindow_put_Caption(IVideoWindow *iface, BSTR strCaption)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strCaption), strCaption);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Caption(pVideoWindow, strCaption);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Caption(IVideoWindow *iface, BSTR *strCaption)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, strCaption);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Caption(pVideoWindow, strCaption);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowStyle(IVideoWindow *iface, LONG WindowStyle)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowStyle);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowStyle(pVideoWindow, WindowStyle);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowStyle(IVideoWindow *iface, LONG *WindowStyle)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyle);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowStyle(pVideoWindow, WindowStyle);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowStyleEx(IVideoWindow *iface, LONG WindowStyleEx)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowStyleEx);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowStyleEx(pVideoWindow, WindowStyleEx);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowStyleEx(IVideoWindow *iface, LONG *WindowStyleEx)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowStyleEx(pVideoWindow, WindowStyleEx);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_AutoShow(IVideoWindow *iface, LONG AutoShow)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, AutoShow);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_AutoShow(pVideoWindow, AutoShow);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_AutoShow(IVideoWindow *iface, LONG *AutoShow)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_AutoShow(pVideoWindow, AutoShow);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowState(IVideoWindow *iface, LONG WindowState)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowState);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowState(pVideoWindow, WindowState);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowState(IVideoWindow *iface, LONG *WindowState)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowState);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowState(pVideoWindow, WindowState);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_BackgroundPalette(IVideoWindow *iface, LONG BackgroundPalette)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, BackgroundPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_BackgroundPalette(pVideoWindow, BackgroundPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_BackgroundPalette(IVideoWindow *iface,
        LONG *pBackgroundPalette)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBackgroundPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_BackgroundPalette(pVideoWindow, pBackgroundPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Visible(IVideoWindow *iface, LONG Visible)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Visible);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Visible(pVideoWindow, Visible);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Visible(IVideoWindow *iface, LONG *pVisible)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Visible(pVideoWindow, pVisible);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Left(IVideoWindow *iface, LONG Left)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Left);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Left(pVideoWindow, Left);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Left(IVideoWindow *iface, LONG *pLeft)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Left(pVideoWindow, pLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Width(IVideoWindow *iface, LONG Width)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Width);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Width(pVideoWindow, Width);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Width(IVideoWindow *iface, LONG *pWidth)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Width(pVideoWindow, pWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Top(IVideoWindow *iface, LONG Top)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Top);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Top(pVideoWindow, Top);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Top(IVideoWindow *iface, LONG *pTop)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Top(pVideoWindow, pTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Height(IVideoWindow *iface, LONG Height)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Height(pVideoWindow, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Height(IVideoWindow *iface, LONG *pHeight)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Height(pVideoWindow, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Owner(IVideoWindow *iface, OAHWND Owner)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Owner(pVideoWindow, Owner);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Owner(IVideoWindow *iface, OAHWND *Owner)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Owner);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Owner(pVideoWindow, Owner);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_MessageDrain(IVideoWindow *iface, OAHWND Drain)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Drain);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_MessageDrain(pVideoWindow, Drain);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_MessageDrain(IVideoWindow *iface, OAHWND *Drain)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Drain);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_MessageDrain(pVideoWindow, Drain);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_BorderColor(IVideoWindow *iface, LONG *Color)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Color);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_BorderColor(pVideoWindow, Color);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_BorderColor(IVideoWindow *iface, LONG Color)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Color);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_BorderColor(pVideoWindow, Color);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_FullScreenMode(IVideoWindow *iface, LONG *FullScreenMode)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, FullScreenMode);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_FullScreenMode(pVideoWindow, FullScreenMode);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_FullScreenMode(IVideoWindow *iface, LONG FullScreenMode)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, FullScreenMode);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_FullScreenMode(pVideoWindow, FullScreenMode);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_SetWindowForeground(IVideoWindow *iface, LONG Focus)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Focus);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_SetWindowForeground(pVideoWindow, Focus);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_NotifyOwnerMessage(IVideoWindow *iface, OAHWND hwnd, LONG uMsg,
        LONG_PTR wParam, LONG_PTR lParam)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%08lx, %d, %08lx, %08lx)\n", This, iface, hwnd, uMsg, wParam, lParam);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_NotifyOwnerMessage(pVideoWindow, hwnd, uMsg, wParam, lParam);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_SetWindowPosition(IVideoWindow *iface, LONG Left, LONG Top,
        LONG Width, LONG Height)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_SetWindowPosition(pVideoWindow, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetWindowPosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop,
        LONG *pWidth, LONG *pHeight)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetWindowPosition(pVideoWindow, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetMinIdealImageSize(IVideoWindow *iface, LONG *pWidth,
        LONG *pHeight)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetMinIdealImageSize(pVideoWindow, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetMaxIdealImageSize(IVideoWindow *iface, LONG *pWidth,
        LONG *pHeight)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetMaxIdealImageSize(pVideoWindow, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetRestorePosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop,
        LONG *pWidth, LONG *pHeight)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetRestorePosition(pVideoWindow, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_HideCursor(IVideoWindow *iface, LONG HideCursor)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, HideCursor);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_HideCursor(pVideoWindow, HideCursor);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_IsCursorHidden(IVideoWindow *iface, LONG *CursorHidden)
{
    IFilterGraphImpl *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, CursorHidden);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_IsCursorHidden(pVideoWindow, CursorHidden);

    LeaveCriticalSection(&This->cs);

    return hr;
}


static const IVideoWindowVtbl IVideoWindow_VTable =
{
    VideoWindow_QueryInterface,
    VideoWindow_AddRef,
    VideoWindow_Release,
    VideoWindow_GetTypeInfoCount,
    VideoWindow_GetTypeInfo,
    VideoWindow_GetIDsOfNames,
    VideoWindow_Invoke,
    VideoWindow_put_Caption,
    VideoWindow_get_Caption,
    VideoWindow_put_WindowStyle,
    VideoWindow_get_WindowStyle,
    VideoWindow_put_WindowStyleEx,
    VideoWindow_get_WindowStyleEx,
    VideoWindow_put_AutoShow,
    VideoWindow_get_AutoShow,
    VideoWindow_put_WindowState,
    VideoWindow_get_WindowState,
    VideoWindow_put_BackgroundPalette,
    VideoWindow_get_BackgroundPalette,
    VideoWindow_put_Visible,
    VideoWindow_get_Visible,
    VideoWindow_put_Left,
    VideoWindow_get_Left,
    VideoWindow_put_Width,
    VideoWindow_get_Width,
    VideoWindow_put_Top,
    VideoWindow_get_Top,
    VideoWindow_put_Height,
    VideoWindow_get_Height,
    VideoWindow_put_Owner,
    VideoWindow_get_Owner,
    VideoWindow_put_MessageDrain,
    VideoWindow_get_MessageDrain,
    VideoWindow_get_BorderColor,
    VideoWindow_put_BorderColor,
    VideoWindow_get_FullScreenMode,
    VideoWindow_put_FullScreenMode,
    VideoWindow_SetWindowForeground,
    VideoWindow_NotifyOwnerMessage,
    VideoWindow_SetWindowPosition,
    VideoWindow_GetWindowPosition,
    VideoWindow_GetMinIdealImageSize,
    VideoWindow_GetMaxIdealImageSize,
    VideoWindow_GetRestorePosition,
    VideoWindow_HideCursor,
    VideoWindow_IsCursorHidden
};

static inline IFilterGraphImpl *impl_from_IMediaEventEx(IMediaEventEx *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IMediaEventEx_iface);
}

static HRESULT WINAPI MediaEvent_QueryInterface(IMediaEventEx *iface, REFIID riid, void **ppvObj)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppvObj);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI MediaEvent_AddRef(IMediaEventEx *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MediaEvent_Release(IMediaEventEx *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return IUnknown_Release(This->outer_unk);
}

/*** IDispatch methods ***/
static HRESULT WINAPI MediaEvent_GetTypeInfoCount(IMediaEventEx *iface, UINT *pctinfo)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%p): stub !!!\n", This, iface, pctinfo);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetTypeInfo(IMediaEventEx *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%d, %d, %p): stub !!!\n", This, iface, iTInfo, lcid, ppTInfo);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetIDsOfNames(IMediaEventEx *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%s, %p, %d, %d, %p): stub !!!\n", This, iface, debugstr_guid(riid), rgszNames,
            cNames, lcid, rgDispId);

    return S_OK;
}

static HRESULT WINAPI MediaEvent_Invoke(IMediaEventEx *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExepInfo,
        UINT *puArgErr)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%d, %s, %d, %04x, %p, %p, %p, %p): stub !!!\n", This, iface, dispIdMember,
            debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    return S_OK;
}

/*** IMediaEvent methods ***/
static HRESULT WINAPI MediaEvent_GetEventHandle(IMediaEventEx *iface, OAEVENT *hEvent)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, hEvent);

    *hEvent = (OAEVENT)This->evqueue.msg_event;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetEvent(IMediaEventEx *iface, LONG *lEventCode, LONG_PTR *lParam1,
        LONG_PTR *lParam2, LONG msTimeout)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);
    Event evt;

    TRACE("(%p/%p)->(%p, %p, %p, %d)\n", This, iface, lEventCode, lParam1, lParam2, msTimeout);

    if (EventsQueue_GetEvent(&This->evqueue, &evt, msTimeout))
    {
	*lEventCode = evt.lEventCode;
	*lParam1 = evt.lParam1;
	*lParam2 = evt.lParam2;
	return S_OK;
    }

    *lEventCode = 0;
    return E_ABORT;
}

static HRESULT WINAPI MediaEvent_WaitForCompletion(IMediaEventEx *iface, LONG msTimeout,
        LONG *pEvCode)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, msTimeout, pEvCode);

    if (This->state != State_Running)
        return VFW_E_WRONG_STATE;

    if (WaitForSingleObject(This->hEventCompletion, msTimeout) == WAIT_OBJECT_0)
    {
	*pEvCode = This->CompletionStatus;
	return S_OK;
    }

    *pEvCode = 0;
    return E_ABORT;
}

static HRESULT WINAPI MediaEvent_CancelDefaultHandling(IMediaEventEx *iface, LONG lEvCode)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = FALSE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = FALSE;
    else if (lEvCode == EC_CLOCK_CHANGED)
        This->HandleEcClockChanged = FALSE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_RestoreDefaultHandling(IMediaEventEx *iface, LONG lEvCode)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = TRUE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = TRUE;
    else if (lEvCode == EC_CLOCK_CHANGED)
        This->HandleEcClockChanged = TRUE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_FreeEventParams(IMediaEventEx *iface, LONG lEvCode,
        LONG_PTR lParam1, LONG_PTR lParam2)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%d, %08lx, %08lx): stub !!!\n", This, iface, lEvCode, lParam1, lParam2);

    return S_OK;
}

/*** IMediaEventEx methods ***/
static HRESULT WINAPI MediaEvent_SetNotifyWindow(IMediaEventEx *iface, OAHWND hwnd, LONG lMsg,
        LONG_PTR lInstanceData)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%08lx, %d, %08lx)\n", This, iface, hwnd, lMsg, lInstanceData);

    This->notif.hWnd = (HWND)hwnd;
    This->notif.msg = lMsg;
    This->notif.instance = lInstanceData;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_SetNotifyFlags(IMediaEventEx *iface, LONG lNoNotifyFlags)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lNoNotifyFlags);

    if ((lNoNotifyFlags != 0) && (lNoNotifyFlags != 1))
	return E_INVALIDARG;

    This->notif.disabled = lNoNotifyFlags;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetNotifyFlags(IMediaEventEx *iface, LONG *lplNoNotifyFlags)
{
    IFilterGraphImpl *This = impl_from_IMediaEventEx(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, lplNoNotifyFlags);

    if (!lplNoNotifyFlags)
	return E_POINTER;

    *lplNoNotifyFlags = This->notif.disabled;

    return S_OK;
}


static const IMediaEventExVtbl IMediaEventEx_VTable =
{
    MediaEvent_QueryInterface,
    MediaEvent_AddRef,
    MediaEvent_Release,
    MediaEvent_GetTypeInfoCount,
    MediaEvent_GetTypeInfo,
    MediaEvent_GetIDsOfNames,
    MediaEvent_Invoke,
    MediaEvent_GetEventHandle,
    MediaEvent_GetEvent,
    MediaEvent_WaitForCompletion,
    MediaEvent_CancelDefaultHandling,
    MediaEvent_RestoreDefaultHandling,
    MediaEvent_FreeEventParams,
    MediaEvent_SetNotifyWindow,
    MediaEvent_SetNotifyFlags,
    MediaEvent_GetNotifyFlags
};


static inline IFilterGraphImpl *impl_from_IMediaFilter(IMediaFilter *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IMediaFilter_iface);
}

static HRESULT WINAPI MediaFilter_QueryInterface(IMediaFilter *iface, REFIID riid, void **ppv)
{
    IFilterGraphImpl *This = impl_from_IMediaFilter(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI MediaFilter_AddRef(IMediaFilter *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaFilter(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MediaFilter_Release(IMediaFilter *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaFilter(iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI MediaFilter_GetClassID(IMediaFilter *iface, CLSID * pClassID)
{
    FIXME("(%p): stub\n", pClassID);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_Stop(IMediaFilter *iface)
{
    IFilterGraphImpl *graph = impl_from_IMediaFilter(iface);

    TRACE("graph %p.\n", graph);

    EnterCriticalSection(&graph->cs);

    if (graph->state == State_Stopped)
    {
        LeaveCriticalSection(&graph->cs);
        return S_OK;
    }

    if (graph->state == State_Running)
        SendFilterMessage(graph, SendPause, 0);
    SendFilterMessage(graph, SendStop, 0);
    graph->state = State_Stopped;

    LeaveCriticalSection(&graph->cs);
    return S_OK;
}

static HRESULT WINAPI MediaFilter_Pause(IMediaFilter *iface)
{
    IFilterGraphImpl *graph = impl_from_IMediaFilter(iface);

    TRACE("graph %p.\n", graph);

    EnterCriticalSection(&graph->cs);

    if (graph->state == State_Paused)
    {
        LeaveCriticalSection(&graph->cs);
        return S_OK;
    }

    if (graph->defaultclock && !graph->refClock)
        IFilterGraph2_SetDefaultSyncSource(&graph->IFilterGraph2_iface);

    if (graph->state == State_Running && graph->refClock && graph->start_time >= 0)
        IReferenceClock_GetTime(graph->refClock, &graph->pause_time);
    else
        graph->pause_time = -1;

    SendFilterMessage(graph, SendPause, 0);
    graph->state = State_Paused;

    LeaveCriticalSection(&graph->cs);
    return S_FALSE;
}

static HRESULT WINAPI MediaFilter_Run(IMediaFilter *iface, REFERENCE_TIME start)
{
    IFilterGraphImpl *graph = impl_from_IMediaFilter(iface);

    TRACE("graph %p, start %s.\n", graph, wine_dbgstr_longlong(start));

    EnterCriticalSection(&graph->cs);

    if (graph->state == State_Running)
    {
        LeaveCriticalSection(&graph->cs);
        return S_OK;
    }
    graph->EcCompleteCount = 0;

    if (graph->defaultclock && !graph->refClock)
        IFilterGraph2_SetDefaultSyncSource(&graph->IFilterGraph2_iface);

    if (!start && graph->refClock)
    {
        REFERENCE_TIME now;
        IReferenceClock_GetTime(graph->refClock, &now);
        if (graph->state == State_Stopped)
            graph->start_time = now + 500000;
        else if (graph->pause_time >= 0)
            graph->start_time += now - graph->pause_time;
        else
            graph->start_time = now;
    }
    else
        graph->start_time = start;

    SendFilterMessage(graph, SendRun, (DWORD_PTR)&graph->start_time);
    graph->state = State_Running;

    LeaveCriticalSection(&graph->cs);
    return S_FALSE;
}

static HRESULT WINAPI MediaFilter_GetState(IMediaFilter *iface, DWORD timeout, FILTER_STATE *state)
{
    IFilterGraphImpl *graph = impl_from_IMediaFilter(iface);
    DWORD end;

    TRACE("graph %p, timeout %u, state %p.\n", graph, timeout, state);

    if (!state)
        return E_POINTER;

    EnterCriticalSection(&graph->cs);

    *state = graph->state;
    if (timeout > 0)
        end = GetTickCount() + timeout;
    else if (timeout == INFINITE)
        end = INFINITE;
    else
        end = 0;
    if (end)
        SendFilterMessage(graph, SendGetState, end);

    LeaveCriticalSection(&graph->cs);
    return S_OK;
}

static HRESULT WINAPI MediaFilter_SetSyncSource(IMediaFilter *iface, IReferenceClock *pClock)
{
    IFilterGraphImpl *This = impl_from_IMediaFilter(iface);
    struct filter *filter;
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClock);

    EnterCriticalSection(&This->cs);
    {
        LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
        {
            hr = IBaseFilter_SetSyncSource(filter->filter, pClock);
            if (FAILED(hr))
                break;
        }

        if (FAILED(hr))
        {
            LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
                IBaseFilter_SetSyncSource(filter->filter, This->refClock);
        }
        else
        {
            if (This->refClock)
                IReferenceClock_Release(This->refClock);
            This->refClock = pClock;
            if (This->refClock)
                IReferenceClock_AddRef(This->refClock);
            This->defaultclock = FALSE;

            if (This->HandleEcClockChanged)
            {
                IMediaEventSink *pEventSink;
                HRESULT eshr;

                eshr = IMediaFilter_QueryInterface(iface, &IID_IMediaEventSink, (void **)&pEventSink);
                if (SUCCEEDED(eshr))
                {
                    IMediaEventSink_Notify(pEventSink, EC_CLOCK_CHANGED, 0, 0);
                    IMediaEventSink_Release(pEventSink);
                }
            }
        }
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaFilter_GetSyncSource(IMediaFilter *iface, IReferenceClock **ppClock)
{
    IFilterGraphImpl *This = impl_from_IMediaFilter(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppClock);

    if (!ppClock)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    {
        *ppClock = This->refClock;
        if (*ppClock)
            IReferenceClock_AddRef(*ppClock);
    }
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static const IMediaFilterVtbl IMediaFilter_VTable =
{
    MediaFilter_QueryInterface,
    MediaFilter_AddRef,
    MediaFilter_Release,
    MediaFilter_GetClassID,
    MediaFilter_Stop,
    MediaFilter_Pause,
    MediaFilter_Run,
    MediaFilter_GetState,
    MediaFilter_SetSyncSource,
    MediaFilter_GetSyncSource
};

static inline IFilterGraphImpl *impl_from_IMediaEventSink(IMediaEventSink *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IMediaEventSink_iface);
}

static HRESULT WINAPI MediaEventSink_QueryInterface(IMediaEventSink *iface, REFIID riid, void **ppv)
{
    IFilterGraphImpl *This = impl_from_IMediaEventSink(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI MediaEventSink_AddRef(IMediaEventSink *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaEventSink(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MediaEventSink_Release(IMediaEventSink *iface)
{
    IFilterGraphImpl *This = impl_from_IMediaEventSink(iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI MediaEventSink_Notify(IMediaEventSink *iface, LONG EventCode,
        LONG_PTR EventParam1, LONG_PTR EventParam2)
{
    IFilterGraphImpl *This = impl_from_IMediaEventSink(iface);
    Event evt;

    TRACE("(%p/%p)->(%d, %ld, %ld)\n", This, iface, EventCode, EventParam1, EventParam2);

    /* We need thread safety here, let's use the events queue's one */
    EnterCriticalSection(&This->evqueue.msg_crst);

    if ((EventCode == EC_COMPLETE) && This->HandleEcComplete)
    {
        TRACE("Process EC_COMPLETE notification\n");
        if (++This->EcCompleteCount == This->nRenderers)
        {
            evt.lEventCode = EC_COMPLETE;
            evt.lParam1 = S_OK;
            evt.lParam2 = 0;
            TRACE("Send EC_COMPLETE to app\n");
            EventsQueue_PutEvent(&This->evqueue, &evt);
            if (!This->notif.disabled && This->notif.hWnd)
	    {
                TRACE("Send Window message\n");
                PostMessageW(This->notif.hWnd, This->notif.msg, 0, This->notif.instance);
            }
            This->CompletionStatus = EC_COMPLETE;
            SetEvent(This->hEventCompletion);
        }
    }
    else if ((EventCode == EC_REPAINT) && This->HandleEcRepaint)
    {
        /* FIXME: Not handled yet */
    }
    else
    {
        evt.lEventCode = EventCode;
        evt.lParam1 = EventParam1;
        evt.lParam2 = EventParam2;
        EventsQueue_PutEvent(&This->evqueue, &evt);
        if (!This->notif.disabled && This->notif.hWnd)
            PostMessageW(This->notif.hWnd, This->notif.msg, 0, This->notif.instance);
    }

    LeaveCriticalSection(&This->evqueue.msg_crst);
    return S_OK;
}

static const IMediaEventSinkVtbl IMediaEventSink_VTable =
{
    MediaEventSink_QueryInterface,
    MediaEventSink_AddRef,
    MediaEventSink_Release,
    MediaEventSink_Notify
};

static inline IFilterGraphImpl *impl_from_IGraphConfig(IGraphConfig *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IGraphConfig_iface);
}

static HRESULT WINAPI GraphConfig_QueryInterface(IGraphConfig *iface, REFIID riid, void **ppv)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI GraphConfig_AddRef(IGraphConfig *iface)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI GraphConfig_Release(IGraphConfig *iface)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI GraphConfig_Reconnect(IGraphConfig *iface, IPin *pOutputPin, IPin *pInputPin,
        const AM_MEDIA_TYPE *pmtFirstConnection, IBaseFilter *pUsingFilter, HANDLE hAbortEvent,
        DWORD dwFlags)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p, %p, %p, %p, %p, %x): stub!\n", This, pOutputPin, pInputPin, pmtFirstConnection, pUsingFilter, hAbortEvent, dwFlags);
    strmbase_dump_media_type(pmtFirstConnection);
    
    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_Reconfigure(IGraphConfig *iface, IGraphConfigCallback *pCallback,
        void *pvContext, DWORD dwFlags, HANDLE hAbortEvent)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);
    HRESULT hr;

    WARN("(%p)->(%p, %p, %x, %p): partial stub!\n", This, pCallback, pvContext, dwFlags, hAbortEvent);

    if (hAbortEvent)
        FIXME("The parameter hAbortEvent is not handled!\n");

    EnterCriticalSection(&This->cs);

    hr = IGraphConfigCallback_Reconfigure(pCallback, pvContext, dwFlags);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI GraphConfig_AddFilterToCache(IGraphConfig *iface, IBaseFilter *pFilter)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p): stub!\n", This, pFilter);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_EnumCacheFilter(IGraphConfig *iface, IEnumFilters **pEnum)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p): stub!\n", This, pEnum);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_RemoveFilterFromCache(IGraphConfig *iface, IBaseFilter *pFilter)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p): stub!\n", This, pFilter);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_GetStartTime(IGraphConfig *iface, REFERENCE_TIME *prtStart)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p): stub!\n", This, prtStart);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_PushThroughData(IGraphConfig *iface, IPin *pOutputPin,
        IPinConnection *pConnection, HANDLE hEventAbort)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p, %p, %p): stub!\n", This, pOutputPin, pConnection, hEventAbort);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_SetFilterFlags(IGraphConfig *iface, IBaseFilter *pFilter,
        DWORD dwFlags)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p, %x): stub!\n", This, pFilter, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_GetFilterFlags(IGraphConfig *iface, IBaseFilter *pFilter,
        DWORD *dwFlags)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p, %p): stub!\n", This, pFilter, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_RemoveFilterEx(IGraphConfig *iface, IBaseFilter *pFilter,
        DWORD dwFlags)
{
    IFilterGraphImpl *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p, %x): stub!\n", This, pFilter, dwFlags);

    return E_NOTIMPL;
}

static const IGraphConfigVtbl IGraphConfig_VTable =
{
    GraphConfig_QueryInterface,
    GraphConfig_AddRef,
    GraphConfig_Release,
    GraphConfig_Reconnect,
    GraphConfig_Reconfigure,
    GraphConfig_AddFilterToCache,
    GraphConfig_EnumCacheFilter,
    GraphConfig_RemoveFilterFromCache,
    GraphConfig_GetStartTime,
    GraphConfig_PushThroughData,
    GraphConfig_SetFilterFlags,
    GraphConfig_GetFilterFlags,
    GraphConfig_RemoveFilterEx
};

static inline IFilterGraphImpl *impl_from_IGraphVersion(IGraphVersion *iface)
{
    return CONTAINING_RECORD(iface, IFilterGraphImpl, IGraphVersion_iface);
}

static HRESULT WINAPI GraphVersion_QueryInterface(IGraphVersion *iface, REFIID riid, void **ppv)
{
    IFilterGraphImpl *This = impl_from_IGraphVersion(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI GraphVersion_AddRef(IGraphVersion *iface)
{
    IFilterGraphImpl *This = impl_from_IGraphVersion(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI GraphVersion_Release(IGraphVersion *iface)
{
    IFilterGraphImpl *This = impl_from_IGraphVersion(iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI GraphVersion_QueryVersion(IGraphVersion *iface, LONG *pVersion)
{
    IFilterGraphImpl *This = impl_from_IGraphVersion(iface);

    if(!pVersion)
        return E_POINTER;

    TRACE("(%p)->(%p): current version %i\n", This, pVersion, This->version);

    *pVersion = This->version;
    return S_OK;
}

static const IGraphVersionVtbl IGraphVersion_VTable =
{
    GraphVersion_QueryInterface,
    GraphVersion_AddRef,
    GraphVersion_Release,
    GraphVersion_QueryVersion,
};

static const IUnknownVtbl IInner_VTable =
{
    FilterGraphInner_QueryInterface,
    FilterGraphInner_AddRef,
    FilterGraphInner_Release
};

static HRESULT filter_graph_common_create(IUnknown *outer, void **out, BOOL threaded)
{
    IFilterGraphImpl *fimpl;
    HRESULT hr;

    *out = NULL;

    fimpl = CoTaskMemAlloc(sizeof(*fimpl));
    fimpl->defaultclock = TRUE;
    fimpl->IUnknown_inner.lpVtbl = &IInner_VTable;
    fimpl->IFilterGraph2_iface.lpVtbl = &IFilterGraph2_VTable;
    fimpl->IMediaControl_iface.lpVtbl = &IMediaControl_VTable;
    fimpl->IMediaSeeking_iface.lpVtbl = &IMediaSeeking_VTable;
    fimpl->IBasicAudio_iface.lpVtbl = &IBasicAudio_VTable;
    fimpl->IBasicVideo2_iface.lpVtbl = &IBasicVideo_VTable;
    fimpl->IVideoWindow_iface.lpVtbl = &IVideoWindow_VTable;
    fimpl->IMediaEventEx_iface.lpVtbl = &IMediaEventEx_VTable;
    fimpl->IMediaFilter_iface.lpVtbl = &IMediaFilter_VTable;
    fimpl->IMediaEventSink_iface.lpVtbl = &IMediaEventSink_VTable;
    fimpl->IGraphConfig_iface.lpVtbl = &IGraphConfig_VTable;
    fimpl->IMediaPosition_iface.lpVtbl = &IMediaPosition_VTable;
    fimpl->IObjectWithSite_iface.lpVtbl = &IObjectWithSite_VTable;
    fimpl->IGraphVersion_iface.lpVtbl = &IGraphVersion_VTable;
    fimpl->ref = 1;
    list_init(&fimpl->filters);
    fimpl->nameIndex = 1;
    fimpl->refClock = NULL;
    fimpl->hEventCompletion = CreateEventW(0, TRUE, FALSE, 0);
    fimpl->HandleEcComplete = TRUE;
    fimpl->HandleEcRepaint = TRUE;
    fimpl->HandleEcClockChanged = TRUE;
    fimpl->notif.hWnd = 0;
    fimpl->notif.disabled = FALSE;
    fimpl->nRenderers = 0;
    fimpl->EcCompleteCount = 0;
    fimpl->refClockProvider = NULL;
    fimpl->state = State_Stopped;
    fimpl->pSite = NULL;
    EventsQueue_Init(&fimpl->evqueue);
    InitializeCriticalSection(&fimpl->cs);
    fimpl->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IFilterGraphImpl.cs");
    fimpl->nItfCacheEntries = 0;
    memcpy(&fimpl->timeformatseek, &TIME_FORMAT_MEDIA_TIME, sizeof(GUID));
    fimpl->start_time = fimpl->pause_time = 0;
    fimpl->stop_position = -1;
    fimpl->punkFilterMapper2 = NULL;
    fimpl->recursioncount = 0;
    fimpl->version = 0;

    if (threaded)
    {
        fimpl->message_thread = CreateThread(NULL, 0, message_thread_run, fimpl, 0, &fimpl->message_thread_id);
        fimpl->message_thread_ret = CreateEventW(NULL, FALSE, FALSE, NULL);
    }
    else
        fimpl->message_thread = NULL;

    fimpl->outer_unk = outer ? outer : &fimpl->IUnknown_inner;

    /* create Filtermapper aggregated. */
    hr = CoCreateInstance(&CLSID_FilterMapper2, fimpl->outer_unk, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&fimpl->punkFilterMapper2);

    if (FAILED(hr)) {
        ERR("Unable to create filter mapper (%x)\n", hr);
        if (fimpl->punkFilterMapper2) IUnknown_Release(fimpl->punkFilterMapper2);
        CloseHandle(fimpl->hEventCompletion);
        EventsQueue_Destroy(&fimpl->evqueue);
        fimpl->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&fimpl->cs);
        CoTaskMemFree(fimpl);
        return hr;
    }

    *out = &fimpl->IUnknown_inner;
    return S_OK;
}

HRESULT filter_graph_create(IUnknown *outer, void **out)
{
    TRACE("outer %p, out %p.\n", outer, out);

    return filter_graph_common_create(outer, out, TRUE);
}

HRESULT filter_graph_no_thread_create(IUnknown *outer, void **out)
{
    TRACE("outer %p, out %p.\n", outer, out);

    return filter_graph_common_create(outer, out, FALSE);
}
