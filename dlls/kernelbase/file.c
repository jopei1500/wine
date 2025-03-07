/*
 * File handling functions
 *
 * Copyright 1993 John Burton
 * Copyright 1996, 2004 Alexandre Julliard
 * Copyright 2008 Jeff Zaroyko
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
#include <stdio.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "wincon.h"
#include "fileapi.h"
#include "ddk/ntddk.h"
#include "ddk/ntddser.h"

#include "kernelbase.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

const WCHAR windows_dir[] = {'C',':','\\','w','i','n','d','o','w','s',0};
const WCHAR system_dir[] = {'C',':','\\','w','i','n','d','o','w','s','\\','s','y','s','t','e','m','3','2',0};

static const WCHAR krnl386W[] = {'k','r','n','l','3','8','6','.','e','x','e','1','6',0};

static BOOL oem_file_apis;


static void WINAPI read_write_apc( void *apc_user, PIO_STATUS_BLOCK io, ULONG reserved )
{
    LPOVERLAPPED_COMPLETION_ROUTINE func = apc_user;
    func( RtlNtStatusToDosError( io->u.Status ), io->Information, (LPOVERLAPPED)io );
}


/***********************************************************************
 * Operations on file names
 ***********************************************************************/


/***********************************************************************
 *	contains_path
 *
 * Check if the file name contains a path; helper for SearchPathW.
 * A relative path is not considered a path unless it starts with ./ or ../
 */
static inline BOOL contains_path( const WCHAR *name )
{
    if (RtlDetermineDosPathNameType_U( name ) != RELATIVE_PATH) return TRUE;
    if (name[0] != '.') return FALSE;
    if (name[1] == '/' || name[1] == '\\') return TRUE;
    return (name[1] == '.' && (name[2] == '/' || name[2] == '\\'));
}


/***********************************************************************
 *	append_ext
 */
static WCHAR *append_ext( const WCHAR *name, const WCHAR *ext )
{
    const WCHAR *p;
    WCHAR *ret;
    DWORD len;

    if (!ext) return NULL;
    p = wcsrchr( name, '.' );
    if (p && !wcschr( p, '/' ) && !wcschr( p, '\\' )) return NULL;

    len = lstrlenW( name ) + lstrlenW( ext );
    if ((ret = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
    {
        lstrcpyW( ret, name );
        lstrcatW( ret, ext );
    }
    return ret;
}


/***********************************************************************
 *	find_actctx_dllpath
 *
 * Find the path (if any) of the dll from the activation context.
 * Returned path doesn't include a name.
 */
static NTSTATUS find_actctx_dllpath( const WCHAR *name, WCHAR **path )
{
    static const WCHAR winsxsW[] = {'\\','w','i','n','s','x','s','\\'};
    static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0};

    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info;
    ACTCTX_SECTION_KEYED_DATA data;
    UNICODE_STRING nameW;
    NTSTATUS status;
    SIZE_T needed, size = 1024;
    WCHAR *p;

    RtlInitUnicodeString( &nameW, name );
    data.cbSize = sizeof(data);
    status = RtlFindActivationContextSectionString( FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                                    &nameW, &data );
    if (status != STATUS_SUCCESS) return status;

    for (;;)
    {
        if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, size )))
        {
            status = STATUS_NO_MEMORY;
            goto done;
        }
        status = RtlQueryInformationActivationContext( 0, data.hActCtx, &data.ulAssemblyRosterIndex,
                                                       AssemblyDetailedInformationInActivationContext,
                                                       info, size, &needed );
        if (status == STATUS_SUCCESS) break;
        if (status != STATUS_BUFFER_TOO_SMALL) goto done;
        RtlFreeHeap( GetProcessHeap(), 0, info );
        size = needed;
        /* restart with larger buffer */
    }

    if (!info->lpAssemblyManifestPath || !info->lpAssemblyDirectoryName)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }

    if ((p = wcsrchr( info->lpAssemblyManifestPath, '\\' )))
    {
        DWORD dirlen = info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);

        p++;
        if (wcsnicmp( p, info->lpAssemblyDirectoryName, dirlen ) || wcsicmp( p + dirlen, dotManifestW ))
        {
            /* manifest name does not match directory name, so it's not a global
             * windows/winsxs manifest; use the manifest directory name instead */
            dirlen = p - info->lpAssemblyManifestPath;
            needed = (dirlen + 1) * sizeof(WCHAR);
            if (!(*path = p = HeapAlloc( GetProcessHeap(), 0, needed )))
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            memcpy( p, info->lpAssemblyManifestPath, dirlen * sizeof(WCHAR) );
            *(p + dirlen) = 0;
            goto done;
        }
    }

    needed = (lstrlenW( windows_dir ) * sizeof(WCHAR) +
              sizeof(winsxsW) + info->ulAssemblyDirectoryNameLength + 2*sizeof(WCHAR));

    if (!(*path = p = RtlAllocateHeap( GetProcessHeap(), 0, needed )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }
    lstrcpyW( p, windows_dir );
    p += lstrlenW(p);
    memcpy( p, winsxsW, sizeof(winsxsW) );
    p += ARRAY_SIZE( winsxsW );
    memcpy( p, info->lpAssemblyDirectoryName, info->ulAssemblyDirectoryNameLength );
    p += info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);
    *p++ = '\\';
    *p = 0;
done:
    RtlFreeHeap( GetProcessHeap(), 0, info );
    RtlReleaseActivationContext( data.hActCtx );
    return status;
}


/***********************************************************************
 *           copy_filename_WtoA
 *
 * copy a file name back to OEM/Ansi, but only if the buffer is large enough
 */
static DWORD copy_filename_WtoA( LPCWSTR nameW, LPSTR buffer, DWORD len )
{
    UNICODE_STRING strW;
    DWORD ret;

    RtlInitUnicodeString( &strW, nameW );

    ret = oem_file_apis ? RtlUnicodeStringToOemSize( &strW ) : RtlUnicodeStringToAnsiSize( &strW );
    if (buffer && ret <= len)
    {
        ANSI_STRING str;

        str.Buffer = buffer;
        str.MaximumLength = min( len, UNICODE_STRING_MAX_CHARS );
        if (oem_file_apis)
            RtlUnicodeStringToOemString( &str, &strW, FALSE );
        else
            RtlUnicodeStringToAnsiString( &str, &strW, FALSE );
        ret = str.Length;  /* length without terminating 0 */
    }
    return ret;
}


/***********************************************************************
 *           file_name_AtoW
 *
 * Convert a file name to Unicode, taking into account the OEM/Ansi API mode.
 *
 * If alloc is FALSE uses the TEB static buffer, so it can only be used when
 * there is no possibility for the function to do that twice, taking into
 * account any called function.
 */
WCHAR *file_name_AtoW( LPCSTR name, BOOL alloc )
{
    ANSI_STRING str;
    UNICODE_STRING strW, *pstrW;
    NTSTATUS status;

    RtlInitAnsiString( &str, name );
    pstrW = alloc ? &strW : &NtCurrentTeb()->StaticUnicodeString;
    if (oem_file_apis)
        status = RtlOemStringToUnicodeString( pstrW, &str, alloc );
    else
        status = RtlAnsiStringToUnicodeString( pstrW, &str, alloc );
    if (status == STATUS_SUCCESS) return pstrW->Buffer;

    if (status == STATUS_BUFFER_OVERFLOW)
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return NULL;
}


/***********************************************************************
 *           file_name_WtoA
 *
 * Convert a file name back to OEM/Ansi. Returns number of bytes copied.
 */
DWORD file_name_WtoA( LPCWSTR src, INT srclen, LPSTR dest, INT destlen )
{
    DWORD ret;

    if (srclen < 0) srclen = lstrlenW( src ) + 1;
    if (oem_file_apis)
        RtlUnicodeToOemN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    else
        RtlUnicodeToMultiByteN( dest, destlen, &ret, src, srclen * sizeof(WCHAR) );
    return ret;
}


/******************************************************************************
 *	AreFileApisANSI   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH AreFileApisANSI(void)
{
    return !oem_file_apis;
}


/***********************************************************************
 *	CreateDirectoryA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryA( LPCSTR path, LPSECURITY_ATTRIBUTES sa )
{
    WCHAR *pathW;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return FALSE;
    return CreateDirectoryW( pathW, sa );
}


/***********************************************************************
 *	CreateDirectoryW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryW( LPCWSTR path, LPSECURITY_ATTRIBUTES sa )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nt_name;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;

    TRACE( "%s\n", debugstr_w(path) );

    if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = sa ? sa->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateFile( &handle, GENERIC_READ | SYNCHRONIZE, &attr, &io, NULL,
                           FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_CREATE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    if (status == STATUS_SUCCESS) NtClose( handle );

    RtlFreeUnicodeString( &nt_name );
    return set_ntstatus( status );
}


/***********************************************************************
 *	CreateDirectoryEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CreateDirectoryExW( LPCWSTR template, LPCWSTR path,
                                                  LPSECURITY_ATTRIBUTES sa )
{
    return CreateDirectoryW( path, sa );
}


/*************************************************************************
 *	CreateFile2   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFile2( LPCWSTR name, DWORD access, DWORD sharing, DWORD creation,
                                             CREATEFILE2_EXTENDED_PARAMETERS *params )
{
    LPSECURITY_ATTRIBUTES sa = params ? params->lpSecurityAttributes : NULL;
    DWORD attributes = params ? params->dwFileAttributes : 0;
    HANDLE template = params ? params->hTemplateFile : NULL;

    FIXME( "(%s %x %x %x %p), partial stub\n", debugstr_w(name), access, sharing, creation, params );

    return CreateFileW( name, access, sharing, sa, creation, attributes, template );
}


/*************************************************************************
 *	CreateFileA   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileA( LPCSTR name, DWORD access, DWORD sharing,
                                             LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                             DWORD attributes, HANDLE template)
{
    WCHAR *nameW;

    if ((GetVersion() & 0x80000000) && IsBadStringPtrA( name, -1 )) return INVALID_HANDLE_VALUE;
    if (!(nameW = file_name_AtoW( name, FALSE ))) return INVALID_HANDLE_VALUE;
    return CreateFileW( nameW, access, sharing, sa, creation, attributes, template );
}


/*************************************************************************
 *	CreateFileW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateFileW( LPCWSTR filename, DWORD access, DWORD sharing,
                                             LPSECURITY_ATTRIBUTES sa, DWORD creation,
                                             DWORD attributes, HANDLE template )
{
    NTSTATUS status;
    UINT options;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    IO_STATUS_BLOCK io;
    HANDLE ret;
    DWORD dosdev;
    const WCHAR *vxd_name = NULL;
    static const WCHAR bkslashes_with_dotW[] = {'\\','\\','.','\\',0};
    static const WCHAR coninW[] = {'C','O','N','I','N','$',0};
    static const WCHAR conoutW[] = {'C','O','N','O','U','T','$',0};
    SECURITY_QUALITY_OF_SERVICE qos;

    static const UINT nt_disposition[5] =
    {
        FILE_CREATE,        /* CREATE_NEW */
        FILE_OVERWRITE_IF,  /* CREATE_ALWAYS */
        FILE_OPEN,          /* OPEN_EXISTING */
        FILE_OPEN_IF,       /* OPEN_ALWAYS */
        FILE_OVERWRITE      /* TRUNCATE_EXISTING */
    };


    /* sanity checks */

    if (!filename || !filename[0])
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    TRACE( "%s %s%s%s%s%s%s%s creation %d attributes 0x%x\n", debugstr_w(filename),
           (access & GENERIC_READ) ? "GENERIC_READ " : "",
           (access & GENERIC_WRITE) ? "GENERIC_WRITE " : "",
           (access & GENERIC_EXECUTE) ? "GENERIC_EXECUTE " : "",
           !access ? "QUERY_ACCESS " : "",
           (sharing & FILE_SHARE_READ) ? "FILE_SHARE_READ " : "",
           (sharing & FILE_SHARE_WRITE) ? "FILE_SHARE_WRITE " : "",
           (sharing & FILE_SHARE_DELETE) ? "FILE_SHARE_DELETE " : "",
           creation, attributes);

    /* Open a console for CONIN$ or CONOUT$ */

    if (!wcsicmp(filename, coninW) || !wcsicmp(filename, conoutW))
    {
        ret = OpenConsoleW( filename, access, sa && sa->bInheritHandle, creation ? OPEN_EXISTING : 0 );
        if (ret == INVALID_HANDLE_VALUE) SetLastError( ERROR_INVALID_PARAMETER );
        return ret;
    }

    if (!wcsncmp( filename, bkslashes_with_dotW, 4 ))
    {
        static const WCHAR pipeW[] = {'P','I','P','E','\\',0};
        static const WCHAR mailslotW[] = {'M','A','I','L','S','L','O','T','\\',0};

        if ((iswalpha(filename[4]) && filename[5] == ':' && filename[6] == '\0') ||
            !wcsnicmp( filename + 4, pipeW, 5 ) ||
            !wcsnicmp( filename + 4, mailslotW, 9 ))
        {
            dosdev = 0;
        }
        else if ((dosdev = RtlIsDosDeviceName_U( filename + 4 )))
        {
            dosdev += MAKELONG( 0, 4*sizeof(WCHAR) );  /* adjust position to start of filename */
        }
        else if (GetVersion() & 0x80000000)
        {
            vxd_name = filename + 4;
            if (!creation) creation = OPEN_EXISTING;
        }
    }
    else dosdev = RtlIsDosDeviceName_U( filename );

    if (dosdev)
    {
        static const WCHAR conW[] = {'C','O','N'};

        if (LOWORD(dosdev) == sizeof(conW) &&
            !wcsnicmp( filename + HIWORD(dosdev)/sizeof(WCHAR), conW, ARRAY_SIZE( conW )))
        {
            switch (access & (GENERIC_READ|GENERIC_WRITE))
            {
            case GENERIC_READ:
                return OpenConsoleW( coninW, access, sa && sa->bInheritHandle, OPEN_EXISTING );
            case GENERIC_WRITE:
                return OpenConsoleW( conoutW, access, sa && sa->bInheritHandle, OPEN_EXISTING );
            default:
                SetLastError( ERROR_FILE_NOT_FOUND );
                return INVALID_HANDLE_VALUE;
            }
        }
    }

    if (creation < CREATE_NEW || creation > TRUNCATE_EXISTING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    if (!RtlDosPathNameToNtPathName_U( filename, &nameW, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }

    /* now call NtCreateFile */

    options = 0;
    if (attributes & FILE_FLAG_BACKUP_SEMANTICS)
        options |= FILE_OPEN_FOR_BACKUP_INTENT;
    else
        options |= FILE_NON_DIRECTORY_FILE;
    if (attributes & FILE_FLAG_DELETE_ON_CLOSE)
    {
        options |= FILE_DELETE_ON_CLOSE;
        access |= DELETE;
    }
    if (attributes & FILE_FLAG_NO_BUFFERING)
        options |= FILE_NO_INTERMEDIATE_BUFFERING;
    if (!(attributes & FILE_FLAG_OVERLAPPED))
        options |= FILE_SYNCHRONOUS_IO_NONALERT;
    if (attributes & FILE_FLAG_RANDOM_ACCESS)
        options |= FILE_RANDOM_ACCESS;
    if (attributes & FILE_FLAG_WRITE_THROUGH)
        options |= FILE_WRITE_THROUGH;
    attributes &= FILE_ATTRIBUTE_VALID_FLAGS;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = sa ? sa->lpSecurityDescriptor : NULL;
    if (attributes & SECURITY_SQOS_PRESENT)
    {
        qos.Length = sizeof(qos);
        qos.ImpersonationLevel = (attributes >> 16) & 0x3;
        qos.ContextTrackingMode = attributes & SECURITY_CONTEXT_TRACKING ? SECURITY_DYNAMIC_TRACKING : SECURITY_STATIC_TRACKING;
        qos.EffectiveOnly = (attributes & SECURITY_EFFECTIVE_ONLY) != 0;
        attr.SecurityQualityOfService = &qos;
    }
    else
        attr.SecurityQualityOfService = NULL;

    if (sa && sa->bInheritHandle) attr.Attributes |= OBJ_INHERIT;

    status = NtCreateFile( &ret, access | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attr, &io,
                           NULL, attributes, sharing, nt_disposition[creation - CREATE_NEW],
                           options, NULL, 0 );
    if (status)
    {
        if (vxd_name && vxd_name[0])
        {
            static HANDLE (*vxd_open)(LPCWSTR,DWORD,SECURITY_ATTRIBUTES*);
            if (!vxd_open) vxd_open = (void *)GetProcAddress( GetModuleHandleW(krnl386W),
                                                              "__wine_vxd_open" );
            if (vxd_open && (ret = vxd_open( vxd_name, access, sa ))) goto done;
        }

        WARN("Unable to create file %s (status %x)\n", debugstr_w(filename), status);
        ret = INVALID_HANDLE_VALUE;

        /* In the case file creation was rejected due to CREATE_NEW flag
         * was specified and file with that name already exists, correct
         * last error is ERROR_FILE_EXISTS and not ERROR_ALREADY_EXISTS.
         * Note: RtlNtStatusToDosError is not the subject to blame here.
         */
        if (status == STATUS_OBJECT_NAME_COLLISION)
            SetLastError( ERROR_FILE_EXISTS );
        else
            SetLastError( RtlNtStatusToDosError(status) );
    }
    else
    {
        if ((creation == CREATE_ALWAYS && io.Information == FILE_OVERWRITTEN) ||
            (creation == OPEN_ALWAYS && io.Information == FILE_OPENED))
            SetLastError( ERROR_ALREADY_EXISTS );
        else
            SetLastError( 0 );
    }
    RtlFreeUnicodeString( &nameW );

 done:
    if (!ret) ret = INVALID_HANDLE_VALUE;
    TRACE("returning %p\n", ret);
    return ret;
}


/***********************************************************************
 *	DeleteFileA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileA( LPCSTR path )
{
    WCHAR *pathW;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return FALSE;
    return DeleteFileW( pathW );
}


/***********************************************************************
 *	DeleteFileW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeleteFileW( LPCWSTR path )
{
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE hFile;
    IO_STATUS_BLOCK io;

    TRACE( "%s\n", debugstr_w(path) );

    if (!RtlDosPathNameToNtPathName_U( path, &nameW, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nameW;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateFile(&hFile, SYNCHRONIZE | DELETE, &attr, &io, NULL, 0,
			  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			  FILE_OPEN, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE, NULL, 0);
    if (status == STATUS_SUCCESS) status = NtClose(hFile);

    RtlFreeUnicodeString( &nameW );
    return set_ntstatus( status );
}


/****************************************************************************
 *	FindCloseChangeNotification   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindCloseChangeNotification( HANDLE handle )
{
    return CloseHandle( handle );
}


/****************************************************************************
 *	FindFirstChangeNotificationA   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstChangeNotificationA( LPCSTR path, BOOL subtree, DWORD filter )
{
    WCHAR *pathW;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return INVALID_HANDLE_VALUE;
    return FindFirstChangeNotificationW( pathW, subtree, filter );
}


/*
 * NtNotifyChangeDirectoryFile may write back to the IO_STATUS_BLOCK
 * asynchronously.  We don't care about the contents, but it can't
 * be placed on the stack since it will go out of scope when we return.
 */
static IO_STATUS_BLOCK dummy_iosb;

/****************************************************************************
 *	FindFirstChangeNotificationW   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH FindFirstChangeNotificationW( LPCWSTR path, BOOL subtree, DWORD filter )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE handle = INVALID_HANDLE_VALUE;

    TRACE( "%s %d %x\n", debugstr_w(path), subtree, filter );

    if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return handle;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, FILE_LIST_DIRECTORY | SYNCHRONIZE, &attr, &dummy_iosb,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (!set_ntstatus( status )) return INVALID_HANDLE_VALUE;

    status = NtNotifyChangeDirectoryFile( handle, NULL, NULL, NULL, &dummy_iosb, NULL, 0, filter, subtree );
    if (status != STATUS_PENDING)
    {
        NtClose( handle );
        SetLastError( RtlNtStatusToDosError(status) );
        return INVALID_HANDLE_VALUE;
    }
    return handle;
}


/****************************************************************************
 *	FindNextChangeNotification   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindNextChangeNotification( HANDLE handle )
{
    NTSTATUS status = NtNotifyChangeDirectoryFile( handle, NULL, NULL, NULL, &dummy_iosb,
                                                   NULL, 0, FILE_NOTIFY_CHANGE_SIZE, 0 );
    if (status == STATUS_PENDING) return TRUE;
    return set_ntstatus( status );
}


/******************************************************************************
 *	GetCompressedFileSizeA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetCompressedFileSizeA( LPCSTR name, LPDWORD size_high )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return INVALID_FILE_SIZE;
    return GetCompressedFileSizeW( nameW, size_high );
}


/******************************************************************************
 *	GetCompressedFileSizeW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetCompressedFileSizeW( LPCWSTR name, LPDWORD size_high )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;
    DWORD ret;

    TRACE("%s %p\n", debugstr_w(name), size_high);

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_FILE_SIZE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );
    if (!set_ntstatus( status )) return INVALID_FILE_SIZE;

    /* we don't support compressed files, simply return the file size */
    ret = GetFileSize( handle, size_high );
    NtClose( handle );
    return ret;
}


/***********************************************************************
 *           GetCurrentDirectoryA    (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetCurrentDirectoryA( UINT buflen, LPSTR buf )
{
    WCHAR bufferW[MAX_PATH];
    DWORD ret;

    if (buflen && buf && ((ULONG_PTR)buf >> 16) == 0)
    {
        /* Win9x catches access violations here, returning zero.
         * This behaviour resulted in some people not noticing
         * that they got the argument order wrong. So let's be
         * nice and fail gracefully if buf is invalid and looks
         * more like a buflen. */
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    ret = RtlGetCurrentDirectory_U( sizeof(bufferW), bufferW );
    if (!ret) return 0;
    if (ret > sizeof(bufferW))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return copy_filename_WtoA( bufferW, buf, buflen );
}


/***********************************************************************
 *           GetCurrentDirectoryW    (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetCurrentDirectoryW( UINT buflen, LPWSTR buf )
{
    return RtlGetCurrentDirectory_U( buflen * sizeof(WCHAR), buf ) / sizeof(WCHAR);
}


/**************************************************************************
 *	GetFileAttributesA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesA( LPCSTR name )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return INVALID_FILE_ATTRIBUTES;
    return GetFileAttributesW( nameW );
}


/**************************************************************************
 *	GetFileAttributesW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileAttributesW( LPCWSTR name )
{
    FILE_BASIC_INFORMATION info;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    TRACE( "%s\n", debugstr_w(name) );

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_FILE_ATTRIBUTES;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtQueryAttributesFile( &attr, &info );
    RtlFreeUnicodeString( &nt_name );

    if (status == STATUS_SUCCESS) return info.FileAttributes;

    /* NtQueryAttributesFile fails on devices, but GetFileAttributesW succeeds */
    if (RtlIsDosDeviceName_U( name )) return FILE_ATTRIBUTE_ARCHIVE;

    SetLastError( RtlNtStatusToDosError(status) );
    return INVALID_FILE_ATTRIBUTES;
}


/**************************************************************************
 *	GetFileAttributesExA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileAttributesExA( LPCSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return FALSE;
    return GetFileAttributesExW( nameW, level, ptr );
}


/**************************************************************************
 *	GetFileAttributesExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileAttributesExW( LPCWSTR name, GET_FILEEX_INFO_LEVELS level, void *ptr )
{
    FILE_NETWORK_OPEN_INFORMATION info;
    WIN32_FILE_ATTRIBUTE_DATA *data = ptr;
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    TRACE("%s %d %p\n", debugstr_w(name), level, ptr);

    if (level != GetFileExInfoStandard)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtQueryFullAttributesFile( &attr, &info );
    RtlFreeUnicodeString( &nt_name );
    if (!set_ntstatus( status )) return FALSE;

    data->dwFileAttributes                = info.FileAttributes;
    data->ftCreationTime.dwLowDateTime    = info.CreationTime.u.LowPart;
    data->ftCreationTime.dwHighDateTime   = info.CreationTime.u.HighPart;
    data->ftLastAccessTime.dwLowDateTime  = info.LastAccessTime.u.LowPart;
    data->ftLastAccessTime.dwHighDateTime = info.LastAccessTime.u.HighPart;
    data->ftLastWriteTime.dwLowDateTime   = info.LastWriteTime.u.LowPart;
    data->ftLastWriteTime.dwHighDateTime  = info.LastWriteTime.u.HighPart;
    data->nFileSizeLow                    = info.EndOfFile.u.LowPart;
    data->nFileSizeHigh                   = info.EndOfFile.u.HighPart;
    return TRUE;
}


/***********************************************************************
 *	GetFullPathNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFullPathNameA( LPCSTR name, DWORD len, LPSTR buffer, LPSTR *lastpart )
{
    WCHAR *nameW;
    WCHAR bufferW[MAX_PATH], *lastpartW = NULL;
    DWORD ret;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return 0;

    ret = GetFullPathNameW( nameW, MAX_PATH, bufferW, &lastpartW );

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    ret = copy_filename_WtoA( bufferW, buffer, len );
    if (ret < len && lastpart)
    {
        if (lastpartW)
            *lastpart = buffer + file_name_WtoA( bufferW, lastpartW - bufferW, NULL, 0 );
        else
            *lastpart = NULL;
    }
    return ret;
}


/***********************************************************************
 *	GetFullPathNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFullPathNameW( LPCWSTR name, DWORD len, LPWSTR buffer, LPWSTR *lastpart )
{
    return RtlGetFullPathName_U( name, len * sizeof(WCHAR), buffer, lastpart ) / sizeof(WCHAR);
}


/***********************************************************************
 *	GetLongPathNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetLongPathNameA( LPCSTR shortpath, LPSTR longpath, DWORD longlen )
{
    WCHAR *shortpathW;
    WCHAR longpathW[MAX_PATH];
    DWORD ret;

    TRACE( "%s\n", debugstr_a( shortpath ));

    if (!(shortpathW = file_name_AtoW( shortpath, FALSE ))) return 0;

    ret = GetLongPathNameW( shortpathW, longpathW, MAX_PATH );

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return copy_filename_WtoA( longpathW, longpath, longlen );
}


/***********************************************************************
 *	GetLongPathNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetLongPathNameW( LPCWSTR shortpath, LPWSTR longpath, DWORD longlen )
{
    static const WCHAR wildcardsW[] = {'*','?',0};
    WCHAR tmplongpath[1024];
    DWORD sp = 0, lp = 0, tmplen;
    WIN32_FIND_DATAW wfd;
    UNICODE_STRING nameW;
    LPCWSTR p;
    HANDLE handle;

    TRACE("%s,%p,%u\n", debugstr_w(shortpath), longpath, longlen);

    if (!shortpath)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!shortpath[0])
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return 0;
    }

    if (shortpath[0] == '\\' && shortpath[1] == '\\')
    {
        FIXME( "UNC pathname %s\n", debugstr_w(shortpath) );
        tmplen = lstrlenW( shortpath );
        if (tmplen < longlen)
        {
            if (longpath != shortpath) lstrcpyW( longpath, shortpath );
            return tmplen;
        }
        return tmplen + 1;
    }

    /* check for drive letter */
    if (shortpath[0] != '/' && shortpath[1] == ':' )
    {
        tmplongpath[0] = shortpath[0];
        tmplongpath[1] = ':';
        lp = sp = 2;
    }

    if (wcspbrk( shortpath + sp, wildcardsW ))
    {
        SetLastError( ERROR_INVALID_NAME );
        return 0;
    }

    while (shortpath[sp])
    {
        /* check for path delimiters and reproduce them */
        if (shortpath[sp] == '\\' || shortpath[sp] == '/')
        {
            tmplongpath[lp++] = shortpath[sp++];
            tmplongpath[lp] = 0; /* terminate string */
            continue;
        }

        for (p = shortpath + sp; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (shortpath + sp);
        lstrcpynW( tmplongpath + lp, shortpath + sp, tmplen + 1 );

        if (tmplongpath[lp] == '.')
        {
            if (tmplen == 1 || (tmplen == 2 && tmplongpath[lp + 1] == '.'))
            {
                lp += tmplen;
                sp += tmplen;
                continue;
            }
        }

        /* Check if the file exists */
        handle = FindFirstFileW( tmplongpath, &wfd );
        if (handle == INVALID_HANDLE_VALUE)
        {
            TRACE( "not found %s\n", debugstr_w( tmplongpath ));
            SetLastError ( ERROR_FILE_NOT_FOUND );
            return 0;
        }
        FindClose( handle );

        /* Use the existing file name if it's a short name */
        RtlInitUnicodeString( &nameW, tmplongpath + lp );
        if (RtlIsNameLegalDOS8Dot3( &nameW, NULL, NULL )) lstrcpyW( tmplongpath + lp, wfd.cFileName );
        lp += lstrlenW( tmplongpath + lp );
        sp += tmplen;
    }
    tmplen = lstrlenW( shortpath ) - 1;
    if ((shortpath[tmplen] == '/' || shortpath[tmplen] == '\\') &&
        (tmplongpath[lp - 1] != '/' && tmplongpath[lp - 1] != '\\'))
        tmplongpath[lp++] = shortpath[tmplen];
    tmplongpath[lp] = 0;

    tmplen = lstrlenW( tmplongpath ) + 1;
    if (tmplen <= longlen)
    {
        lstrcpyW( longpath, tmplongpath );
        TRACE("returning %s\n", debugstr_w( longpath ));
        tmplen--; /* length without 0 */
    }
    return tmplen;
}


/***********************************************************************
 *	GetShortPathNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetShortPathNameW( LPCWSTR longpath, LPWSTR shortpath, DWORD shortlen )
{
    static const WCHAR wildcardsW[] = {'*','?',0};
    WIN32_FIND_DATAW wfd;
    WCHAR *tmpshortpath;
    HANDLE handle;
    LPCWSTR p;
    DWORD sp = 0, lp = 0, tmplen, buf_len;

    TRACE( "%s,%p,%u\n", debugstr_w(longpath), shortpath, shortlen );

    if (!longpath)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!longpath[0])
    {
        SetLastError( ERROR_BAD_PATHNAME );
        return 0;
    }

    /* code below only removes characters from string, never adds, so this is
     * the largest buffer that tmpshortpath will need to have */
    buf_len = lstrlenW(longpath) + 1;
    tmpshortpath = HeapAlloc( GetProcessHeap(), 0, buf_len * sizeof(WCHAR) );
    if (!tmpshortpath)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    if (longpath[0] == '\\' && longpath[1] == '\\' && longpath[2] == '?' && longpath[3] == '\\')
    {
        memcpy( tmpshortpath, longpath, 4 * sizeof(WCHAR) );
        sp = lp = 4;
    }

    if (wcspbrk( longpath + lp, wildcardsW ))
    {
        HeapFree( GetProcessHeap(), 0, tmpshortpath );
        SetLastError( ERROR_INVALID_NAME );
        return 0;
    }

    /* check for drive letter */
    if (longpath[lp] != '/' && longpath[lp + 1] == ':' )
    {
        tmpshortpath[sp] = longpath[lp];
        tmpshortpath[sp + 1] = ':';
        sp += 2;
        lp += 2;
    }

    while (longpath[lp])
    {
        /* check for path delimiters and reproduce them */
        if (longpath[lp] == '\\' || longpath[lp] == '/')
        {
            tmpshortpath[sp++] = longpath[lp++];
            tmpshortpath[sp] = 0; /* terminate string */
            continue;
        }

        p = longpath + lp;
        for (; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (longpath + lp);
        lstrcpynW( tmpshortpath + sp, longpath + lp, tmplen + 1 );

        if (tmpshortpath[sp] == '.')
        {
            if (tmplen == 1 || (tmplen == 2 && tmpshortpath[sp + 1] == '.'))
            {
                sp += tmplen;
                lp += tmplen;
                continue;
            }
        }

        /* Check if the file exists and use the existing short file name */
        handle = FindFirstFileW( tmpshortpath, &wfd );
        if (handle == INVALID_HANDLE_VALUE) goto notfound;
        FindClose( handle );

        /* In rare cases (like "a.abcd") short path may be longer than original path.
         * Make sure we have enough space in temp buffer. */
        if (wfd.cAlternateFileName[0] && tmplen < lstrlenW(wfd.cAlternateFileName))
        {
            WCHAR *new_buf;
            buf_len += lstrlenW( wfd.cAlternateFileName ) - tmplen;
            new_buf = HeapReAlloc( GetProcessHeap(), 0, tmpshortpath, buf_len * sizeof(WCHAR) );
            if(!new_buf)
            {
                HeapFree( GetProcessHeap(), 0, tmpshortpath );
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }
            tmpshortpath = new_buf;
        }

        lstrcpyW( tmpshortpath + sp, wfd.cAlternateFileName[0] ? wfd.cAlternateFileName : wfd.cFileName );
        sp += lstrlenW( tmpshortpath + sp );
        lp += tmplen;
    }
    tmpshortpath[sp] = 0;

    tmplen = lstrlenW( tmpshortpath ) + 1;
    if (tmplen <= shortlen)
    {
        lstrcpyW( shortpath, tmpshortpath );
        TRACE( "returning %s\n", debugstr_w( shortpath ));
        tmplen--; /* length without 0 */
    }

    HeapFree( GetProcessHeap(), 0, tmpshortpath );
    return tmplen;

 notfound:
    HeapFree( GetProcessHeap(), 0, tmpshortpath );
    TRACE( "not found\n" );
    SetLastError( ERROR_FILE_NOT_FOUND );
    return 0;
}


/***********************************************************************
 *	GetSystemDirectoryA   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemDirectoryA( LPSTR path, UINT count )
{
    return copy_filename_WtoA( system_dir, path, count );
}


/***********************************************************************
 *	GetSystemDirectoryW   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemDirectoryW( LPWSTR path, UINT count )
{
    UINT len = lstrlenW( system_dir ) + 1;
    if (path && count >= len)
    {
        lstrcpyW( path, system_dir );
        len--;
    }
    return len;
}


/***********************************************************************
 *	GetSystemWindowsDirectoryA   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemWindowsDirectoryA( LPSTR path, UINT count )
{
    return GetWindowsDirectoryA( path, count );
}


/***********************************************************************
 *	GetSystemWindowsDirectoryW   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetSystemWindowsDirectoryW( LPWSTR path, UINT count )
{
    return GetWindowsDirectoryW( path, count );
}


/***********************************************************************
 *	GetTempFileNameA   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetTempFileNameA( LPCSTR path, LPCSTR prefix, UINT unique, LPSTR buffer )
{
    WCHAR *pathW, *prefixW = NULL;
    WCHAR bufferW[MAX_PATH];
    UINT ret;

    if (!(pathW = file_name_AtoW( path, FALSE ))) return 0;
    if (prefix && !(prefixW = file_name_AtoW( prefix, TRUE ))) return 0;

    ret = GetTempFileNameW( pathW, prefixW, unique, bufferW );
    if (ret) file_name_WtoA( bufferW, -1, buffer, MAX_PATH );

    HeapFree( GetProcessHeap(), 0, prefixW );
    return ret;
}


/***********************************************************************
 *	GetTempFileNameW   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetTempFileNameW( LPCWSTR path, LPCWSTR prefix, UINT unique, LPWSTR buffer )
{
    static const WCHAR formatW[] = {'%','x','.','t','m','p',0};
    int i;
    LPWSTR p;
    DWORD attr;

    if (!path || !buffer)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* ensure that the provided directory exists */
    attr = GetFileAttributesW( path );
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        TRACE( "path not found %s\n", debugstr_w( path ));
        SetLastError( ERROR_DIRECTORY );
        return 0;
    }

    lstrcpyW( buffer, path );
    p = buffer + lstrlenW(buffer);

    /* add a \, if there isn't one  */
    if ((p == buffer) || (p[-1] != '\\')) *p++ = '\\';

    if (prefix) for (i = 3; (i > 0) && (*prefix); i--) *p++ = *prefix++;

    unique &= 0xffff;
    if (unique) swprintf( p, MAX_PATH - (p - buffer), formatW, unique );
    else
    {
        /* get a "random" unique number and try to create the file */
        HANDLE handle;
        UINT num = NtGetTickCount() & 0xffff;
        static UINT last;

        /* avoid using the same name twice in a short interval */
        if (last - num < 10) num = last + 1;
        if (!num) num = 1;
        unique = num;
        do
        {
            swprintf( p, MAX_PATH - (p - buffer), formatW, unique );
            handle = CreateFileW( buffer, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
            if (handle != INVALID_HANDLE_VALUE)
            {  /* We created it */
                CloseHandle( handle );
                last = unique;
                break;
            }
            if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_SHARING_VIOLATION)
                break;  /* No need to go on */
            if (!(++unique & 0xffff)) unique = 1;
        } while (unique != num);
    }
    TRACE( "returning %s\n", debugstr_w( buffer ));
    return unique;
}


/***********************************************************************
 *	GetTempPathA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTempPathA( DWORD count, LPSTR path )
{
    WCHAR pathW[MAX_PATH];
    UINT ret;

    if (!(ret = GetTempPathW( MAX_PATH, pathW ))) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return copy_filename_WtoA( pathW, path, count );
}


/***********************************************************************
 *	GetTempPathW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTempPathW( DWORD count, LPWSTR path )
{
    static const WCHAR tmp[]  = { 'T','M','P',0 };
    static const WCHAR temp[] = { 'T','E','M','P',0 };
    static const WCHAR userprofile[] = { 'U','S','E','R','P','R','O','F','I','L','E',0 };
    WCHAR tmp_path[MAX_PATH];
    UINT ret;

    if (!(ret = GetEnvironmentVariableW( tmp, tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( temp, tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( userprofile, tmp_path, MAX_PATH )) &&
        !(ret = GetWindowsDirectoryW( tmp_path, MAX_PATH )))
        return 0;

    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    ret = GetFullPathNameW( tmp_path, MAX_PATH, tmp_path, NULL );
    if (!ret) return 0;

    if (ret > MAX_PATH - 2)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    if (tmp_path[ret-1] != '\\')
    {
        tmp_path[ret++] = '\\';
        tmp_path[ret]   = '\0';
    }

    ret++; /* add space for terminating 0 */
    if (count >= ret)
    {
        lstrcpynW( path, tmp_path, count );
        /* the remaining buffer must be zeroed up to 32766 bytes in XP or 32767
         * bytes after it, we will assume the > XP behavior for now */
        memset( path + ret, 0, (min(count, 32767) - ret) * sizeof(WCHAR) );
        ret--; /* return length without 0 */
    }
    else if (count)
    {
        /* the buffer must be cleared if contents will not fit */
        memset( path, 0, count * sizeof(WCHAR) );
    }

    TRACE( "returning %u, %s\n", ret, debugstr_w( path ));
    return ret;
}


/***********************************************************************
 *	GetWindowsDirectoryA   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetWindowsDirectoryA( LPSTR path, UINT count )
{
    return copy_filename_WtoA( windows_dir, path, count );
}


/***********************************************************************
 *	GetWindowsDirectoryW   (kernelbase.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH GetWindowsDirectoryW( LPWSTR path, UINT count )
{
    UINT len = lstrlenW( windows_dir ) + 1;
    if (path && count >= len)
    {
        lstrcpyW( path, windows_dir );
        len--;
    }
    return len;
}


/***********************************************************************
 *	NeedCurrentDirectoryForExePathA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH NeedCurrentDirectoryForExePathA( LPCSTR name )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return TRUE;
    return NeedCurrentDirectoryForExePathW( nameW );
}


/***********************************************************************
 *	NeedCurrentDirectoryForExePathW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH NeedCurrentDirectoryForExePathW( LPCWSTR name )
{
    static const WCHAR env_name[] = {'N','o','D','e','f','a','u','l','t',
                                     'C','u','r','r','e','n','t',
                                     'D','i','r','e','c','t','o','r','y',
                                     'I','n','E','x','e','P','a','t','h',0};
    WCHAR env_val;

    if (wcschr( name, '\\' )) return TRUE;
    /* check the existence of the variable, not value */
    return !GetEnvironmentVariableW( env_name, &env_val, 1 );
}


/***********************************************************************
 *	SearchPathA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SearchPathA( LPCSTR path, LPCSTR name, LPCSTR ext,
                                            DWORD buflen, LPSTR buffer, LPSTR *lastpart )
{
    WCHAR *pathW = NULL, *nameW, *extW = NULL;
    WCHAR bufferW[MAX_PATH];
    DWORD ret;

    if (!name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!(nameW = file_name_AtoW( name, FALSE ))) return 0;
    if (path && !(pathW = file_name_AtoW( path, TRUE ))) return 0;
    if (ext && !(extW = file_name_AtoW( ext, TRUE )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, pathW );
        return 0;
    }

    ret = SearchPathW( pathW, nameW, extW, MAX_PATH, bufferW, NULL );

    RtlFreeHeap( GetProcessHeap(), 0, pathW );
    RtlFreeHeap( GetProcessHeap(), 0, extW );

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    ret = copy_filename_WtoA( bufferW, buffer, buflen );
    if (buflen > ret && lastpart) *lastpart = strrchr(buffer, '\\') + 1;
    return ret;
}


/***********************************************************************
 *	SearchPathW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SearchPathW( LPCWSTR path, LPCWSTR name, LPCWSTR ext, DWORD buflen,
                                            LPWSTR buffer, LPWSTR *lastpart )
{
    DWORD ret = 0;
    WCHAR *name_ext;

    if (!name || !name[0])
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* If the name contains an explicit path, ignore the path */

    if (contains_path( name ))
    {
        /* try first without extension */
        if (RtlDoesFileExists_U( name )) return GetFullPathNameW( name, buflen, buffer, lastpart );

        if ((name_ext = append_ext( name, ext )))
        {
            if (RtlDoesFileExists_U( name_ext ))
                ret = GetFullPathNameW( name_ext, buflen, buffer, lastpart );
            RtlFreeHeap( GetProcessHeap(), 0, name_ext );
        }
    }
    else if (path && path[0])  /* search in the specified path */
    {
        ret = RtlDosSearchPath_U( path, name, ext, buflen * sizeof(WCHAR),
                                  buffer, lastpart ) / sizeof(WCHAR);
    }
    else  /* search in active context and default path */
    {
        WCHAR *dll_path = NULL, *name_ext = append_ext( name, ext );

        if (name_ext) name = name_ext;

        /* When file is found with activation context no attempt is made
          to check if it's really exist, path is returned only basing on context info. */
        if (find_actctx_dllpath( name, &dll_path ) == STATUS_SUCCESS)
        {
            ret = lstrlenW( dll_path ) + lstrlenW( name ) + 1;

            /* count null termination char too */
            if (ret <= buflen)
            {
                lstrcpyW( buffer, dll_path );
                lstrcatW( buffer, name );
                if (lastpart) *lastpart = buffer + lstrlenW( dll_path );
                ret--;
            }
            else if (lastpart) *lastpart = NULL;
            RtlFreeHeap( GetProcessHeap(), 0, dll_path );
        }
        else if (!RtlGetSearchPath( &dll_path ))
        {
            ret = RtlDosSearchPath_U( dll_path, name, NULL, buflen * sizeof(WCHAR),
                                      buffer, lastpart ) / sizeof(WCHAR);
            RtlReleasePath( dll_path );
        }
        RtlFreeHeap( GetProcessHeap(), 0, name_ext );
    }

    if (!ret) SetLastError( ERROR_FILE_NOT_FOUND );
    else TRACE( "found %s\n", debugstr_w(buffer) );
    return ret;
}


/***********************************************************************
 *	SetCurrentDirectoryA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCurrentDirectoryA( LPCSTR dir )
{
    WCHAR *dirW;
    UNICODE_STRING strW;

    if (!(dirW = file_name_AtoW( dir, FALSE ))) return FALSE;
    RtlInitUnicodeString( &strW, dirW );
    return set_ntstatus( RtlSetCurrentDirectory_U( &strW ));
}


/***********************************************************************
 *	SetCurrentDirectoryW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCurrentDirectoryW( LPCWSTR dir )
{
    UNICODE_STRING dirW;

    RtlInitUnicodeString( &dirW, dir );
    return set_ntstatus( RtlSetCurrentDirectory_U( &dirW ));
}


/**************************************************************************
 *	SetFileApisToANSI   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH SetFileApisToANSI(void)
{
    oem_file_apis = FALSE;
}


/**************************************************************************
 *	SetFileApisToOEM   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH SetFileApisToOEM(void)
{
    oem_file_apis = TRUE;
}


/**************************************************************************
 *	SetFileAttributesA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileAttributesA( LPCSTR name, DWORD attributes )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return FALSE;
    return SetFileAttributesW( nameW, attributes );
}


/**************************************************************************
 *	SetFileAttributesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileAttributesW( LPCWSTR name, DWORD attributes )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE handle;

    TRACE( "%s %x\n", debugstr_w(name), attributes );

    if (!RtlDosPathNameToNtPathName_U( name, &nt_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtOpenFile( &handle, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
    RtlFreeUnicodeString( &nt_name );

    if (status == STATUS_SUCCESS)
    {
        FILE_BASIC_INFORMATION info;

        memset( &info, 0, sizeof(info) );
        info.FileAttributes = attributes | FILE_ATTRIBUTE_NORMAL;  /* make sure it's not zero */
        status = NtSetInformationFile( handle, &io, &info, sizeof(info), FileBasicInformation );
        NtClose( handle );
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	Wow64DisableWow64FsRedirection   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Wow64DisableWow64FsRedirection( PVOID *old_value )
{
    return set_ntstatus( RtlWow64EnableFsRedirectionEx( TRUE, (ULONG *)old_value ));
}


/***********************************************************************
 *	Wow64RevertWow64FsRedirection   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Wow64RevertWow64FsRedirection( PVOID old_value )
{
    return set_ntstatus( RtlWow64EnableFsRedirection( !old_value ));
}


/***********************************************************************
 * Operations on file handles
 ***********************************************************************/


/***********************************************************************
 *	CancelIo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelIo( HANDLE handle )
{
    IO_STATUS_BLOCK io;

    NtCancelIoFile( handle, &io );
    return set_ntstatus( io.u.Status );
}


/***********************************************************************
 *	CancelIoEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelIoEx( HANDLE handle, LPOVERLAPPED overlapped )
{
    IO_STATUS_BLOCK io;

    NtCancelIoFileEx( handle, (PIO_STATUS_BLOCK)overlapped, &io );
    return set_ntstatus( io.u.Status );
}


/***********************************************************************
 *	CancelSynchronousIo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH CancelSynchronousIo( HANDLE thread )
{
    FIXME( "(%p): stub\n", thread );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *	FlushFileBuffers   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FlushFileBuffers( HANDLE file )
{
    IO_STATUS_BLOCK iosb;

    /* this will fail (as expected) for an output handle */
    if (is_console_handle( file )) return FlushConsoleInputBuffer( file );

    return set_ntstatus( NtFlushBuffersFile( file, &iosb ));
}


/***********************************************************************
 *	GetFileInformationByHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileInformationByHandle( HANDLE file, BY_HANDLE_FILE_INFORMATION *info )
{
    FILE_ALL_INFORMATION all_info;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    status = NtQueryInformationFile( file, &io, &all_info, sizeof(all_info), FileAllInformation );
    if (status == STATUS_BUFFER_OVERFLOW) status = STATUS_SUCCESS;
    if (!set_ntstatus( status )) return FALSE;

    info->dwFileAttributes                = all_info.BasicInformation.FileAttributes;
    info->ftCreationTime.dwHighDateTime   = all_info.BasicInformation.CreationTime.u.HighPart;
    info->ftCreationTime.dwLowDateTime    = all_info.BasicInformation.CreationTime.u.LowPart;
    info->ftLastAccessTime.dwHighDateTime = all_info.BasicInformation.LastAccessTime.u.HighPart;
    info->ftLastAccessTime.dwLowDateTime  = all_info.BasicInformation.LastAccessTime.u.LowPart;
    info->ftLastWriteTime.dwHighDateTime  = all_info.BasicInformation.LastWriteTime.u.HighPart;
    info->ftLastWriteTime.dwLowDateTime   = all_info.BasicInformation.LastWriteTime.u.LowPart;
    info->dwVolumeSerialNumber            = 0;  /* FIXME */
    info->nFileSizeHigh                   = all_info.StandardInformation.EndOfFile.u.HighPart;
    info->nFileSizeLow                    = all_info.StandardInformation.EndOfFile.u.LowPart;
    info->nNumberOfLinks                  = all_info.StandardInformation.NumberOfLinks;
    info->nFileIndexHigh                  = all_info.InternalInformation.IndexNumber.u.HighPart;
    info->nFileIndexLow                   = all_info.InternalInformation.IndexNumber.u.LowPart;
    return TRUE;
}


/***********************************************************************
 *	GetFileInformationByHandleEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileInformationByHandleEx( HANDLE handle, FILE_INFO_BY_HANDLE_CLASS class,
                                                            LPVOID info, DWORD size )
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    switch (class)
    {
    case FileStreamInfo:
    case FileCompressionInfo:
    case FileRemoteProtocolInfo:
    case FileFullDirectoryInfo:
    case FileFullDirectoryRestartInfo:
    case FileStorageInfo:
    case FileAlignmentInfo:
    case FileIdExtdDirectoryInfo:
    case FileIdExtdDirectoryRestartInfo:
        FIXME( "%p, %u, %p, %u\n", handle, class, info, size );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;

    case FileAttributeTagInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileAttributeTagInformation );
        break;

    case FileBasicInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileBasicInformation );
        break;

    case FileStandardInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileStandardInformation );
        break;

    case FileNameInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileNameInformation );
        break;

    case FileIdInfo:
        status = NtQueryInformationFile( handle, &io, info, size, FileIdInformation );
        break;

    case FileIdBothDirectoryRestartInfo:
    case FileIdBothDirectoryInfo:
        status = NtQueryDirectoryFile( handle, NULL, NULL, NULL, &io, info, size,
                                       FileIdBothDirectoryInformation, FALSE, NULL,
                                       (class == FileIdBothDirectoryRestartInfo) );
        break;

    case FileRenameInfo:
    case FileDispositionInfo:
    case FileAllocationInfo:
    case FileIoPriorityHintInfo:
    case FileEndOfFileInfo:
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	GetFileSize   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileSize( HANDLE file, LPDWORD size_high )
{
    LARGE_INTEGER size;

    if (!GetFileSizeEx( file, &size )) return INVALID_FILE_SIZE;
    if (size_high) *size_high = size.u.HighPart;
    if (size.u.LowPart == INVALID_FILE_SIZE) SetLastError( 0 );
    return size.u.LowPart;
}


/***********************************************************************
 *	GetFileSizeEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileSizeEx( HANDLE file, PLARGE_INTEGER size )
{
    FILE_STANDARD_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (is_console_handle( file ))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!set_ntstatus( NtQueryInformationFile( file, &io, &info, sizeof(info), FileStandardInformation )))
        return FALSE;

    *size = info.EndOfFile;
    return TRUE;
}


/***********************************************************************
 *	GetFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetFileTime( HANDLE file, FILETIME *creation,
                                           FILETIME *access, FILETIME *write )
{
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (!set_ntstatus( NtQueryInformationFile( file, &io, &info, sizeof(info), FileBasicInformation )))
        return FALSE;

    if (creation)
    {
        creation->dwHighDateTime = info.CreationTime.u.HighPart;
        creation->dwLowDateTime  = info.CreationTime.u.LowPart;
    }
    if (access)
    {
        access->dwHighDateTime = info.LastAccessTime.u.HighPart;
        access->dwLowDateTime  = info.LastAccessTime.u.LowPart;
    }
    if (write)
    {
        write->dwHighDateTime = info.LastWriteTime.u.HighPart;
        write->dwLowDateTime  = info.LastWriteTime.u.LowPart;
    }
    return TRUE;
}


/***********************************************************************
 *	GetFileType   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetFileType( HANDLE file )
{
    FILE_FS_DEVICE_INFORMATION info;
    IO_STATUS_BLOCK io;

    if (file == (HANDLE)STD_INPUT_HANDLE ||
        file == (HANDLE)STD_OUTPUT_HANDLE ||
        file == (HANDLE)STD_ERROR_HANDLE)
        file = GetStdHandle( (DWORD_PTR)file );

    if (is_console_handle( file )) return FILE_TYPE_CHAR;

    if (!set_ntstatus( NtQueryVolumeInformationFile( file, &io, &info, sizeof(info),
                                                     FileFsDeviceInformation )))
        return FILE_TYPE_UNKNOWN;

    switch (info.DeviceType)
    {
    case FILE_DEVICE_NULL:
    case FILE_DEVICE_SERIAL_PORT:
    case FILE_DEVICE_PARALLEL_PORT:
    case FILE_DEVICE_TAPE:
    case FILE_DEVICE_UNKNOWN:
        return FILE_TYPE_CHAR;
    case FILE_DEVICE_NAMED_PIPE:
        return FILE_TYPE_PIPE;
    default:
        return FILE_TYPE_DISK;
    }
}


/***********************************************************************
 *	GetOverlappedResult   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetOverlappedResult( HANDLE file, LPOVERLAPPED overlapped,
                                                   LPDWORD result, BOOL wait )
{
    NTSTATUS status;

    TRACE( "(%p %p %p %x)\n", file, overlapped, result, wait );

    status = overlapped->Internal;
    if (status == STATUS_PENDING)
    {
        if (!wait)
        {
            SetLastError( ERROR_IO_INCOMPLETE );
            return FALSE;
        }
        if (WaitForSingleObject( overlapped->hEvent ? overlapped->hEvent : file, INFINITE ) == WAIT_FAILED)
            return FALSE;

        status = overlapped->Internal;
        if (status == STATUS_PENDING) status = STATUS_SUCCESS;
    }

    *result = overlapped->InternalHigh;
    return set_ntstatus( status );
}


/**************************************************************************
 *	LockFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LockFile( HANDLE file, DWORD offset_low, DWORD offset_high,
                                        DWORD count_low, DWORD count_high )
{
    LARGE_INTEGER count, offset;

    TRACE( "%p %x%08x %x%08x\n", file, offset_high, offset_low, count_high, count_low );

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = offset_low;
    offset.u.HighPart = offset_high;
    return set_ntstatus( NtLockFile( file, 0, NULL, NULL, NULL, &offset, &count, NULL, TRUE, TRUE ));
}


/**************************************************************************
 *	LockFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LockFileEx( HANDLE file, DWORD flags, DWORD reserved,
                                          DWORD count_low, DWORD count_high, LPOVERLAPPED overlapped )
{
    LARGE_INTEGER count, offset;
    LPVOID cvalue = NULL;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    TRACE( "%p %x%08x %x%08x flags %x\n",
           file, overlapped->u.s.OffsetHigh, overlapped->u.s.Offset, count_high, count_low, flags );

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;

    if (((ULONG_PTR)overlapped->hEvent & 1) == 0) cvalue = overlapped;

    return set_ntstatus( NtLockFile( file, overlapped->hEvent, NULL, cvalue,
                                     NULL, &offset, &count, NULL,
                                     flags & LOCKFILE_FAIL_IMMEDIATELY,
                                     flags & LOCKFILE_EXCLUSIVE_LOCK ));
}


/***********************************************************************
 *	OpenFileById   (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenFileById( HANDLE handle, LPFILE_ID_DESCRIPTOR id, DWORD access,
                                              DWORD share, LPSECURITY_ATTRIBUTES sec_attr, DWORD flags )
{
    UINT options;
    HANDLE result;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING objectName;

    if (!id)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    options = FILE_OPEN_BY_FILE_ID;
    if (flags & FILE_FLAG_BACKUP_SEMANTICS)
        options |= FILE_OPEN_FOR_BACKUP_INTENT;
    else
        options |= FILE_NON_DIRECTORY_FILE;
    if (flags & FILE_FLAG_NO_BUFFERING) options |= FILE_NO_INTERMEDIATE_BUFFERING;
    if (!(flags & FILE_FLAG_OVERLAPPED)) options |= FILE_SYNCHRONOUS_IO_NONALERT;
    if (flags & FILE_FLAG_RANDOM_ACCESS) options |= FILE_RANDOM_ACCESS;
    flags &= FILE_ATTRIBUTE_VALID_FLAGS;

    objectName.Length             = sizeof(ULONGLONG);
    objectName.Buffer             = (WCHAR *)&id->u.FileId;
    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = handle;
    attr.Attributes               = 0;
    attr.ObjectName               = &objectName;
    attr.SecurityDescriptor       = sec_attr ? sec_attr->lpSecurityDescriptor : NULL;
    attr.SecurityQualityOfService = NULL;
    if (sec_attr && sec_attr->bInheritHandle) attr.Attributes |= OBJ_INHERIT;

    if (!set_ntstatus( NtCreateFile( &result, access | SYNCHRONIZE, &attr, &io, NULL, flags,
                                     share, OPEN_EXISTING, options, NULL, 0 )))
        return INVALID_HANDLE_VALUE;
    return result;
}


/***********************************************************************
 *	ReOpenFile   (kernelbase.@)
 */
HANDLE WINAPI /* DECLSPEC_HOTPATCH */ ReOpenFile( HANDLE handle, DWORD access, DWORD sharing, DWORD flags )
{
    FIXME( "(%p, %d, %d, %d): stub\n", handle, access, sharing, flags );
    return INVALID_HANDLE_VALUE;
}


static void WINAPI invoke_completion( void *context, IO_STATUS_BLOCK *io, ULONG res )
{
    LPOVERLAPPED_COMPLETION_ROUTINE completion = context;
    completion( io->u.Status, io->Information, (LPOVERLAPPED)io );
}

/****************************************************************************
 *	ReadDirectoryChangesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadDirectoryChangesW( HANDLE handle, LPVOID buffer, DWORD len,
                                                     BOOL subtree, DWORD filter, LPDWORD returned,
                                                     LPOVERLAPPED overlapped,
                                                     LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    OVERLAPPED ov, *pov;
    IO_STATUS_BLOCK *ios;
    NTSTATUS status;
    LPVOID cvalue = NULL;

    TRACE( "%p %p %08x %d %08x %p %p %p\n",
           handle, buffer, len, subtree, filter, returned, overlapped, completion );

    if (!overlapped)
    {
        memset( &ov, 0, sizeof ov );
        ov.hEvent = CreateEventW( NULL, 0, 0, NULL );
        pov = &ov;
    }
    else
    {
        pov = overlapped;
        if (completion) cvalue = completion;
        else if (((ULONG_PTR)overlapped->hEvent & 1) == 0) cvalue = overlapped;
    }

    ios = (PIO_STATUS_BLOCK)pov;
    ios->u.Status = STATUS_PENDING;

    status = NtNotifyChangeDirectoryFile( handle, completion && overlapped ? NULL : pov->hEvent,
                                          completion && overlapped ? invoke_completion : NULL,
                                          cvalue, ios, buffer, len, filter, subtree );
    if (status == STATUS_PENDING)
    {
        if (overlapped) return TRUE;
        WaitForSingleObjectEx( ov.hEvent, INFINITE, TRUE );
        if (returned) *returned = ios->Information;
        status = ios->u.Status;
    }
    if (!overlapped) CloseHandle( ov.hEvent );
    return set_ntstatus( status );
}


/***********************************************************************
 *	ReadFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFile( HANDLE file, LPVOID buffer, DWORD count,
                                        LPDWORD result, LPOVERLAPPED overlapped )
{
    LARGE_INTEGER offset;
    PLARGE_INTEGER poffset = NULL;
    IO_STATUS_BLOCK iosb;
    PIO_STATUS_BLOCK io_status = &iosb;
    HANDLE event = 0;
    NTSTATUS status;
    LPVOID cvalue = NULL;

    TRACE( "%p %p %d %p %p\n", file, buffer, count, result, overlapped );

    if (result) *result = 0;

    if (is_console_handle( file ))
    {
        DWORD conread, mode;

        if (!ReadConsoleA( file, buffer, count, &conread, NULL) || !GetConsoleMode( file, &mode ))
            return FALSE;
        /* ctrl-Z (26) means end of file on window (if at beginning of buffer)
         * but Unix uses ctrl-D (4), and ctrl-Z is a bad idea on Unix :-/
         * So map both ctrl-D ctrl-Z to EOF.
         */
        if ((mode & ENABLE_PROCESSED_INPUT) && conread > 0 &&
            (((char *)buffer)[0] == 26 || ((char *)buffer)[0] == 4))
        {
            conread = 0;
        }
        if (result) *result = conread;
        return TRUE;
    }

    if (overlapped)
    {
        offset.u.LowPart = overlapped->u.s.Offset;
        offset.u.HighPart = overlapped->u.s.OffsetHigh;
        poffset = &offset;
        event = overlapped->hEvent;
        io_status = (PIO_STATUS_BLOCK)overlapped;
        if (((ULONG_PTR)event & 1) == 0) cvalue = overlapped;
    }
    else io_status->Information = 0;
    io_status->u.Status = STATUS_PENDING;

    status = NtReadFile( file, event, NULL, cvalue, io_status, buffer, count, poffset, NULL);

    if (status == STATUS_PENDING && !overlapped)
    {
        WaitForSingleObject( file, INFINITE );
        status = io_status->u.Status;
    }

    if (result) *result = overlapped && status ? 0 : io_status->Information;

    if (status == STATUS_END_OF_FILE)
    {
        if (overlapped != NULL)
        {
            SetLastError( RtlNtStatusToDosError(status) );
            return FALSE;
        }
    }
    else if (status && status != STATUS_TIMEOUT)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *	ReadFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFileEx( HANDLE file, LPVOID buffer, DWORD count, LPOVERLAPPED overlapped,
                                          LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    NTSTATUS status;

    TRACE( "(file=%p, buffer=%p, bytes=%u, ovl=%p, ovl_fn=%p)\n",
           file, buffer, count, overlapped, completion );

    if (!overlapped)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->u.Status = STATUS_PENDING;
    io->Information = 0;

    status = NtReadFile( file, NULL, read_write_apc, completion, io, buffer, count, &offset, NULL);
    if (status == STATUS_PENDING) return TRUE;
    return set_ntstatus( status );
}


/***********************************************************************
 *	ReadFileScatter   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ReadFileScatter( HANDLE file, FILE_SEGMENT_ELEMENT *segments, DWORD count,
                                               LPDWORD reserved, LPOVERLAPPED overlapped )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    void *cvalue = NULL;

    TRACE( "(%p %p %u %p)\n", file, segments, count, overlapped );

    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;
    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->u.Status = STATUS_PENDING;
    io->Information = 0;

    return set_ntstatus( NtReadFileScatter( file, overlapped->hEvent, NULL, cvalue, io,
                                            segments, count, &offset, NULL ));
}


/**************************************************************************
 *	SetEndOfFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEndOfFile( HANDLE file )
{
    FILE_POSITION_INFORMATION pos;
    FILE_END_OF_FILE_INFORMATION eof;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    if (!(status = NtQueryInformationFile( file, &io, &pos, sizeof(pos), FilePositionInformation )))
    {
        eof.EndOfFile = pos.CurrentByteOffset;
        status = NtSetInformationFile( file, &io, &eof, sizeof(eof), FileEndOfFileInformation );
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	SetFileInformationByHandle   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileInformationByHandle( HANDLE file, FILE_INFO_BY_HANDLE_CLASS class,
                                                          void *info, DWORD size )
{
    NTSTATUS status;
    IO_STATUS_BLOCK io;

    TRACE( "%p %u %p %u\n", file, class, info, size );

    switch (class)
    {
    case FileNameInfo:
    case FileRenameInfo:
    case FileAllocationInfo:
    case FileEndOfFileInfo:
    case FileStreamInfo:
    case FileIdBothDirectoryInfo:
    case FileIdBothDirectoryRestartInfo:
    case FileFullDirectoryInfo:
    case FileFullDirectoryRestartInfo:
    case FileStorageInfo:
    case FileAlignmentInfo:
    case FileIdInfo:
    case FileIdExtdDirectoryInfo:
    case FileIdExtdDirectoryRestartInfo:
        FIXME( "%p, %u, %p, %u\n", file, class, info, size );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return FALSE;

    case FileBasicInfo:
        status = NtSetInformationFile( file, &io, info, size, FileBasicInformation );
        break;
    case FileDispositionInfo:
        status = NtSetInformationFile( file, &io, info, size, FileDispositionInformation );
        break;
    case FileIoPriorityHintInfo:
        status = NtSetInformationFile( file, &io, info, size, FileIoPriorityHintInformation );
        break;
    case FileStandardInfo:
    case FileCompressionInfo:
    case FileAttributeTagInfo:
    case FileRemoteProtocolInfo:
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	SetFilePointer   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SetFilePointer( HANDLE file, LONG distance, LONG *highword, DWORD method )
{
    LARGE_INTEGER dist, newpos;

    if (highword)
    {
        dist.u.LowPart  = distance;
        dist.u.HighPart = *highword;
    }
    else dist.QuadPart = distance;

    if (!SetFilePointerEx( file, dist, &newpos, method )) return INVALID_SET_FILE_POINTER;

    if (highword) *highword = newpos.u.HighPart;
    if (newpos.u.LowPart == INVALID_SET_FILE_POINTER) SetLastError( 0 );
    return newpos.u.LowPart;
}


/***********************************************************************
 *	SetFilePointerEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFilePointerEx( HANDLE file, LARGE_INTEGER distance,
                                                LARGE_INTEGER *newpos, DWORD method )
{
    LONGLONG pos;
    IO_STATUS_BLOCK io;
    FILE_POSITION_INFORMATION info;
    FILE_END_OF_FILE_INFORMATION eof;

    switch(method)
    {
    case FILE_BEGIN:
        pos = distance.QuadPart;
        break;
    case FILE_CURRENT:
        if (NtQueryInformationFile( file, &io, &info, sizeof(info), FilePositionInformation ))
            goto error;
        pos = info.CurrentByteOffset.QuadPart + distance.QuadPart;
        break;
    case FILE_END:
        if (NtQueryInformationFile( file, &io, &eof, sizeof(eof), FileEndOfFileInformation ))
            goto error;
        pos = eof.EndOfFile.QuadPart + distance.QuadPart;
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (pos < 0)
    {
        SetLastError( ERROR_NEGATIVE_SEEK );
        return FALSE;
    }

    info.CurrentByteOffset.QuadPart = pos;
    if (!NtSetInformationFile( file, &io, &info, sizeof(info), FilePositionInformation ))
    {
        if (newpos) newpos->QuadPart = pos;
        return TRUE;
    }

error:
    return set_ntstatus( io.u.Status );
}


/***********************************************************************
 *	SetFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileTime( HANDLE file, const FILETIME *ctime,
                                           const FILETIME *atime, const FILETIME *mtime )
{
    FILE_BASIC_INFORMATION info;
    IO_STATUS_BLOCK io;

    memset( &info, 0, sizeof(info) );
    if (ctime)
    {
        info.CreationTime.u.HighPart = ctime->dwHighDateTime;
        info.CreationTime.u.LowPart  = ctime->dwLowDateTime;
    }
    if (atime)
    {
        info.LastAccessTime.u.HighPart = atime->dwHighDateTime;
        info.LastAccessTime.u.LowPart  = atime->dwLowDateTime;
    }
    if (mtime)
    {
        info.LastWriteTime.u.HighPart = mtime->dwHighDateTime;
        info.LastWriteTime.u.LowPart  = mtime->dwLowDateTime;
    }

    return set_ntstatus( NtSetInformationFile( file, &io, &info, sizeof(info), FileBasicInformation ));
}


/***********************************************************************
 *	SetFileValidData   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetFileValidData( HANDLE file, LONGLONG length )
{
    FILE_VALID_DATA_LENGTH_INFORMATION info;
    IO_STATUS_BLOCK io;

    info.ValidDataLength.QuadPart = length;
    return set_ntstatus( NtSetInformationFile( file, &io, &info, sizeof(info),
                                               FileValidDataLengthInformation ));
}


/**************************************************************************
 *	UnlockFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnlockFile( HANDLE file, DWORD offset_low, DWORD offset_high,
                                          DWORD count_low, DWORD count_high )
{
    LARGE_INTEGER count, offset;

    count.u.LowPart = count_low;
    count.u.HighPart = count_high;
    offset.u.LowPart = offset_low;
    offset.u.HighPart = offset_high;
    return set_ntstatus( NtUnlockFile( file, NULL, &offset, &count, NULL ));
}


/**************************************************************************
 *	UnlockFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH UnlockFileEx( HANDLE file, DWORD reserved,
                                            DWORD count_low, DWORD count_high, LPOVERLAPPED overlapped )
{
    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (overlapped->hEvent) FIXME("Unimplemented overlapped operation\n");

    return UnlockFile( file, overlapped->u.s.Offset, overlapped->u.s.OffsetHigh, count_low, count_high );
}


/***********************************************************************
 *	WriteFile   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFile( HANDLE file, LPCVOID buffer, DWORD count,
                                         LPDWORD result, LPOVERLAPPED overlapped )
{
    HANDLE event = NULL;
    LARGE_INTEGER offset;
    PLARGE_INTEGER poffset = NULL;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    PIO_STATUS_BLOCK piosb = &iosb;
    LPVOID cvalue = NULL;

    TRACE( "%p %p %d %p %p\n", file, buffer, count, result, overlapped );

    if (is_console_handle( file )) return WriteConsoleA( file, buffer, count, result, NULL);

    if (overlapped)
    {
        offset.u.LowPart = overlapped->u.s.Offset;
        offset.u.HighPart = overlapped->u.s.OffsetHigh;
        poffset = &offset;
        event = overlapped->hEvent;
        piosb = (PIO_STATUS_BLOCK)overlapped;
        if (((ULONG_PTR)event & 1) == 0) cvalue = overlapped;
    }
    else piosb->Information = 0;
    piosb->u.Status = STATUS_PENDING;

    status = NtWriteFile( file, event, NULL, cvalue, piosb, buffer, count, poffset, NULL );

    if (status == STATUS_PENDING && !overlapped)
    {
        WaitForSingleObject( file, INFINITE );
        status = piosb->u.Status;
    }

    if (result) *result = overlapped && status ? 0 : piosb->Information;

    if (status && status != STATUS_TIMEOUT)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *	WriteFileEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFileEx( HANDLE file, LPCVOID buffer,
                                           DWORD count, LPOVERLAPPED overlapped,
                                           LPOVERLAPPED_COMPLETION_ROUTINE completion )
{
    LARGE_INTEGER offset;
    NTSTATUS status;
    PIO_STATUS_BLOCK io;

    TRACE( "%p %p %d %p %p\n", file, buffer, count, overlapped, completion );

    if (!overlapped)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;

    io = (PIO_STATUS_BLOCK)overlapped;
    io->u.Status = STATUS_PENDING;
    io->Information = 0;

    status = NtWriteFile( file, NULL, read_write_apc, completion, io, buffer, count, &offset, NULL );
    if (status == STATUS_PENDING) return TRUE;
    return set_ntstatus( status );
}


/***********************************************************************
 *	WriteFileGather   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WriteFileGather( HANDLE file, FILE_SEGMENT_ELEMENT *segments, DWORD count,
                                               LPDWORD reserved, LPOVERLAPPED overlapped )
{
    PIO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    void *cvalue = NULL;

    TRACE( "%p %p %u %p\n", file, segments, count, overlapped );

    offset.u.LowPart = overlapped->u.s.Offset;
    offset.u.HighPart = overlapped->u.s.OffsetHigh;
    if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
    io = (PIO_STATUS_BLOCK)overlapped;
    io->u.Status = STATUS_PENDING;
    io->Information = 0;

    return set_ntstatus( NtWriteFileGather( file, overlapped->hEvent, NULL, cvalue,
                                            io, segments, count, &offset, NULL ));
}


/***********************************************************************
 * Operations on file times
 ***********************************************************************/


/*********************************************************************
 *	CompareFileTime   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareFileTime( const FILETIME *x, const FILETIME *y )
{
    if (!x || !y) return -1;
    if (x->dwHighDateTime > y->dwHighDateTime) return 1;
    if (x->dwHighDateTime < y->dwHighDateTime) return -1;
    if (x->dwLowDateTime > y->dwLowDateTime) return 1;
    if (x->dwLowDateTime < y->dwLowDateTime) return -1;
    return 0;
}


/*********************************************************************
 *	FileTimeToLocalFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FileTimeToLocalFileTime( const FILETIME *utc, FILETIME *local )
{
    return set_ntstatus( RtlSystemTimeToLocalTime( (const LARGE_INTEGER *)utc, (LARGE_INTEGER *)local ));
}


/*********************************************************************
 *	FileTimeToSystemTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FileTimeToSystemTime( const FILETIME *ft, SYSTEMTIME *systime )
{
    TIME_FIELDS tf;

    RtlTimeToTimeFields( (const LARGE_INTEGER *)ft, &tf );
    systime->wYear = tf.Year;
    systime->wMonth = tf.Month;
    systime->wDay = tf.Day;
    systime->wHour = tf.Hour;
    systime->wMinute = tf.Minute;
    systime->wSecond = tf.Second;
    systime->wMilliseconds = tf.Milliseconds;
    systime->wDayOfWeek = tf.Weekday;
    return TRUE;
}


/*********************************************************************
 *	GetLocalTime   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetLocalTime( SYSTEMTIME *systime )
{
    LARGE_INTEGER ft, ft2;

    NtQuerySystemTime( &ft );
    RtlSystemTimeToLocalTime( &ft, &ft2 );
    FileTimeToSystemTime( (FILETIME *)&ft2, systime );
}


/*********************************************************************
 *	GetSystemTime   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetSystemTime( SYSTEMTIME *systime )
{
    LARGE_INTEGER ft;

    NtQuerySystemTime( &ft );
    FileTimeToSystemTime( (FILETIME *)&ft, systime );
}


/***********************************************************************
 *	GetSystemTimeAsFileTime   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetSystemTimeAsFileTime( FILETIME *time )
{
    NtQuerySystemTime( (LARGE_INTEGER *)time );
}


/***********************************************************************
 *	GetSystemTimePreciseAsFileTime   (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH GetSystemTimePreciseAsFileTime( FILETIME *time )
{
    LARGE_INTEGER t;

    t.QuadPart = RtlGetSystemTimePrecise();
    time->dwLowDateTime = t.u.LowPart;
    time->dwHighDateTime = t.u.HighPart;
}


/*********************************************************************
 *	LocalFileTimeToFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH LocalFileTimeToFileTime( const FILETIME *local, FILETIME *utc )
{
    return set_ntstatus( RtlLocalTimeToSystemTime( (const LARGE_INTEGER *)local, (LARGE_INTEGER *)utc ));
}


/***********************************************************************
 *	SetLocalTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetLocalTime( const SYSTEMTIME *systime )
{
    FILETIME ft;
    LARGE_INTEGER st;

    if (!SystemTimeToFileTime( systime, &ft )) return FALSE;
    RtlLocalTimeToSystemTime( (LARGE_INTEGER *)&ft, &st );
    return set_ntstatus( NtSetSystemTime( &st, NULL ));
}


/***********************************************************************
 *	SetSystemTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetSystemTime( const SYSTEMTIME *systime )
{
    FILETIME ft;

    if (!SystemTimeToFileTime( systime, &ft )) return FALSE;
    return set_ntstatus( NtSetSystemTime( (LARGE_INTEGER *)&ft, NULL ));
}


/*********************************************************************
 *	SystemTimeToFileTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SystemTimeToFileTime( const SYSTEMTIME *systime, FILETIME *ft )
{
    TIME_FIELDS tf;

    tf.Year = systime->wYear;
    tf.Month = systime->wMonth;
    tf.Day = systime->wDay;
    tf.Hour = systime->wHour;
    tf.Minute = systime->wMinute;
    tf.Second = systime->wSecond;
    tf.Milliseconds = systime->wMilliseconds;
    if (RtlTimeFieldsToTime( &tf, (LARGE_INTEGER *)ft )) return TRUE;
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}


/***********************************************************************
 * I/O controls
 ***********************************************************************/


static void dump_dcb( const DCB *dcb )
{
    TRACE( "size=%d rate=%d fParity=%d Parity=%d stopbits=%d %sIXON %sIXOFF CTS=%d RTS=%d DSR=%d DTR=%d %sCRTSCTS\n",
           dcb->ByteSize, dcb->BaudRate, dcb->fParity, dcb->Parity,
           (dcb->StopBits == ONESTOPBIT) ? 1 : (dcb->StopBits == TWOSTOPBITS) ? 2 : 0,
           dcb->fOutX ? "" : "~", dcb->fInX ? "" : "~",
           dcb->fOutxCtsFlow, dcb->fRtsControl, dcb->fOutxDsrFlow, dcb->fDtrControl,
           (dcb->fOutxCtsFlow || dcb->fRtsControl == RTS_CONTROL_HANDSHAKE) ? "" : "~" );
}

/*****************************************************************************
 *	ClearCommBreak   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ClearCommBreak( HANDLE handle )
{
    return EscapeCommFunction( handle, CLRBREAK );
}


/*****************************************************************************
 *	ClearCommError   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ClearCommError( HANDLE handle, DWORD *errors, COMSTAT *stat )
{
    SERIAL_STATUS ss;

    if (!DeviceIoControl( handle, IOCTL_SERIAL_GET_COMMSTATUS, NULL, 0, &ss, sizeof(ss), NULL, NULL ))
        return FALSE;

    TRACE( "status %#x,%#x, in %u, out %u, eof %d, wait %d\n", ss.Errors, ss.HoldReasons,
           ss.AmountInInQueue, ss.AmountInOutQueue, ss.EofReceived, ss.WaitForImmediate );

    if (errors)
    {
        *errors = 0;
        if (ss.Errors & SERIAL_ERROR_BREAK)        *errors |= CE_BREAK;
        if (ss.Errors & SERIAL_ERROR_FRAMING)      *errors |= CE_FRAME;
        if (ss.Errors & SERIAL_ERROR_OVERRUN)      *errors |= CE_OVERRUN;
        if (ss.Errors & SERIAL_ERROR_QUEUEOVERRUN) *errors |= CE_RXOVER;
        if (ss.Errors & SERIAL_ERROR_PARITY)       *errors |= CE_RXPARITY;
    }
    if (stat)
    {
        stat->fCtsHold  = !!(ss.HoldReasons & SERIAL_TX_WAITING_FOR_CTS);
        stat->fDsrHold  = !!(ss.HoldReasons & SERIAL_TX_WAITING_FOR_DSR);
        stat->fRlsdHold = !!(ss.HoldReasons & SERIAL_TX_WAITING_FOR_DCD);
        stat->fXoffHold = !!(ss.HoldReasons & SERIAL_TX_WAITING_FOR_XON);
        stat->fXoffSent = !!(ss.HoldReasons & SERIAL_TX_WAITING_XOFF_SENT);
        stat->fEof      = !!ss.EofReceived;
        stat->fTxim     = !!ss.WaitForImmediate;
        stat->cbInQue   = ss.AmountInInQueue;
        stat->cbOutQue  = ss.AmountInOutQueue;
    }
    return TRUE;
}


/****************************************************************************
 *	DeviceIoControl   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeviceIoControl( HANDLE handle, DWORD code, void *in_buff, DWORD in_count,
                                               void *out_buff, DWORD out_count, DWORD *returned,
                                               OVERLAPPED *overlapped )
{
    IO_STATUS_BLOCK iosb, *piosb = &iosb;
    void *cvalue = NULL;
    HANDLE event = 0;
    NTSTATUS status;

    TRACE( "(%p,%x,%p,%d,%p,%d,%p,%p)\n",
           handle, code, in_buff, in_count, out_buff, out_count, returned, overlapped );

    if (overlapped)
    {
        piosb = (IO_STATUS_BLOCK *)overlapped;
        if (!((ULONG_PTR)overlapped->hEvent & 1)) cvalue = overlapped;
        event = overlapped->hEvent;
        overlapped->Internal = STATUS_PENDING;
        overlapped->InternalHigh = 0;
    }

    if (HIWORD(code) == FILE_DEVICE_FILE_SYSTEM)
        status = NtFsControlFile( handle, event, NULL, cvalue, piosb, code,
                                  in_buff, in_count, out_buff, out_count );
    else
        status = NtDeviceIoControlFile( handle, event, NULL, cvalue, piosb, code,
                                        in_buff, in_count, out_buff, out_count );

    if (returned) *returned = piosb->Information;
    return set_ntstatus( status );
}


/*****************************************************************************
 *	EscapeCommFunction   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EscapeCommFunction( HANDLE handle, DWORD func )
{
    static const DWORD ioctls[] =
    {
        0,
        IOCTL_SERIAL_SET_XOFF,      /* SETXOFF */
        IOCTL_SERIAL_SET_XON,       /* SETXON */
        IOCTL_SERIAL_SET_RTS,       /* SETRTS */
        IOCTL_SERIAL_CLR_RTS,       /* CLRRTS */
        IOCTL_SERIAL_SET_DTR,       /* SETDTR */
        IOCTL_SERIAL_CLR_DTR,       /* CLRDTR */
        IOCTL_SERIAL_RESET_DEVICE,  /* RESETDEV */
        IOCTL_SERIAL_SET_BREAK_ON,  /* SETBREAK */
        IOCTL_SERIAL_SET_BREAK_OFF  /* CLRBREAK */
    };

    if (func >= ARRAY_SIZE(ioctls) || !ioctls[func])
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return DeviceIoControl( handle, ioctls[func], NULL, 0, NULL, 0, NULL, NULL );
}


/***********************************************************************
 *	GetCommConfig   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommConfig( HANDLE handle, COMMCONFIG *config, DWORD *size )
{
    if (!config) return FALSE;

    TRACE( "(%p, %p, %p %u)\n", handle, config, size, *size );

    if (*size < sizeof(COMMCONFIG))
    {
        *size = sizeof(COMMCONFIG);
        return FALSE;
    }
    *size = sizeof(COMMCONFIG);
    config->dwSize = sizeof(COMMCONFIG);
    config->wVersion = 1;
    config->wReserved = 0;
    config->dwProviderSubType = PST_RS232;
    config->dwProviderOffset = 0;
    config->dwProviderSize = 0;
    return GetCommState( handle, &config->dcb );
}


/*****************************************************************************
 *	GetCommMask   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommMask( HANDLE handle, DWORD *mask )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_GET_WAIT_MASK, NULL, 0, mask, sizeof(*mask),
                            NULL, NULL );
}


/***********************************************************************
 *	GetCommModemStatus   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommModemStatus( HANDLE handle, DWORD *status )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_GET_MODEMSTATUS, NULL, 0, status, sizeof(*status),
                            NULL, NULL );
}


/***********************************************************************
 *	GetCommProperties   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommProperties( HANDLE handle, COMMPROP *prop )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_GET_PROPERTIES, NULL, 0, prop, sizeof(*prop), NULL, NULL );
}


/*****************************************************************************
 *	GetCommState   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommState( HANDLE handle, DCB *dcb )
{
    SERIAL_BAUD_RATE sbr;
    SERIAL_LINE_CONTROL slc;
    SERIAL_HANDFLOW shf;
    SERIAL_CHARS sc;

    if (!dcb)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!DeviceIoControl(handle, IOCTL_SERIAL_GET_BAUD_RATE, NULL, 0, &sbr, sizeof(sbr), NULL, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_LINE_CONTROL, NULL, 0, &slc, sizeof(slc), NULL, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_HANDFLOW, NULL, 0, &shf, sizeof(shf), NULL, NULL) ||
        !DeviceIoControl(handle, IOCTL_SERIAL_GET_CHARS, NULL, 0, &sc, sizeof(sc), NULL, NULL))
        return FALSE;

    dcb->DCBlength         = sizeof(*dcb);
    dcb->BaudRate          = sbr.BaudRate;
    /* yes, they seem no never be (re)set on NT */
    dcb->fBinary           = 1;
    dcb->fParity           = 0;
    dcb->fOutxCtsFlow      = !!(shf.ControlHandShake & SERIAL_CTS_HANDSHAKE);
    dcb->fOutxDsrFlow      = !!(shf.ControlHandShake & SERIAL_DSR_HANDSHAKE);
    dcb->fDsrSensitivity   = !!(shf.ControlHandShake & SERIAL_DSR_SENSITIVITY);
    dcb->fTXContinueOnXoff = !!(shf.FlowReplace & SERIAL_XOFF_CONTINUE);
    dcb->fOutX             = !!(shf.FlowReplace & SERIAL_AUTO_TRANSMIT);
    dcb->fInX              = !!(shf.FlowReplace & SERIAL_AUTO_RECEIVE);
    dcb->fErrorChar        = !!(shf.FlowReplace & SERIAL_ERROR_CHAR);
    dcb->fNull             = !!(shf.FlowReplace & SERIAL_NULL_STRIPPING);
    dcb->fAbortOnError     = !!(shf.ControlHandShake & SERIAL_ERROR_ABORT);
    dcb->XonLim            = shf.XonLimit;
    dcb->XoffLim           = shf.XoffLimit;
    dcb->ByteSize          = slc.WordLength;
    dcb->Parity            = slc.Parity;
    dcb->StopBits          = slc.StopBits;
    dcb->XonChar           = sc.XonChar;
    dcb->XoffChar          = sc.XoffChar;
    dcb->ErrorChar         = sc.ErrorChar;
    dcb->EofChar           = sc.EofChar;
    dcb->EvtChar           = sc.EventChar;

    switch (shf.ControlHandShake & (SERIAL_DTR_CONTROL | SERIAL_DTR_HANDSHAKE))
    {
    case SERIAL_DTR_CONTROL:    dcb->fDtrControl = DTR_CONTROL_ENABLE; break;
    case SERIAL_DTR_HANDSHAKE:  dcb->fDtrControl = DTR_CONTROL_HANDSHAKE; break;
    default:                    dcb->fDtrControl = DTR_CONTROL_DISABLE; break;
    }
    switch (shf.FlowReplace & (SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE))
    {
    case SERIAL_RTS_CONTROL:    dcb->fRtsControl = RTS_CONTROL_ENABLE; break;
    case SERIAL_RTS_HANDSHAKE:  dcb->fRtsControl = RTS_CONTROL_HANDSHAKE; break;
    case SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE:
                                dcb->fRtsControl = RTS_CONTROL_TOGGLE; break;
    default:                    dcb->fRtsControl = RTS_CONTROL_DISABLE; break;
    }
    dump_dcb( dcb );
    return TRUE;
}


/*****************************************************************************
 *	GetCommTimeouts   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCommTimeouts( HANDLE handle, COMMTIMEOUTS *timeouts )
{
    if (!timeouts)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return DeviceIoControl( handle, IOCTL_SERIAL_GET_TIMEOUTS, NULL, 0, timeouts, sizeof(*timeouts),
                            NULL, NULL );
}

/********************************************************************
 *	PurgeComm   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PurgeComm(HANDLE handle, DWORD flags)
{
    return DeviceIoControl( handle, IOCTL_SERIAL_PURGE, &flags, sizeof(flags),
                            NULL, 0, NULL, NULL );
}


/*****************************************************************************
 *	SetCommBreak   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommBreak( HANDLE handle )
{
    return EscapeCommFunction( handle, SETBREAK );
}


/***********************************************************************
 *	SetCommConfig   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommConfig( HANDLE handle, COMMCONFIG *config, DWORD size )
{
    TRACE( "(%p, %p, %u)\n", handle, config, size );
    return SetCommState( handle, &config->dcb );
}


/*****************************************************************************
 *	SetCommMask   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommMask( HANDLE handle, DWORD mask )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_SET_WAIT_MASK, &mask, sizeof(mask),
                            NULL, 0, NULL, NULL );
}


/*****************************************************************************
 *	SetCommState   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommState( HANDLE handle, DCB *dcb )
{
    SERIAL_BAUD_RATE sbr;
    SERIAL_LINE_CONTROL slc;
    SERIAL_HANDFLOW shf;
    SERIAL_CHARS sc;

    if (!dcb)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    dump_dcb( dcb );

    sbr.BaudRate   = dcb->BaudRate;
    slc.StopBits   = dcb->StopBits;
    slc.Parity     = dcb->Parity;
    slc.WordLength = dcb->ByteSize;
    shf.ControlHandShake = 0;
    shf.FlowReplace = 0;
    if (dcb->fOutxCtsFlow) shf.ControlHandShake |= SERIAL_CTS_HANDSHAKE;
    if (dcb->fOutxDsrFlow) shf.ControlHandShake |= SERIAL_DSR_HANDSHAKE;
    switch (dcb->fDtrControl)
    {
    case DTR_CONTROL_DISABLE:   break;
    case DTR_CONTROL_ENABLE:    shf.ControlHandShake |= SERIAL_DTR_CONTROL; break;
    case DTR_CONTROL_HANDSHAKE: shf.ControlHandShake |= SERIAL_DTR_HANDSHAKE; break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (dcb->fRtsControl)
    {
    case RTS_CONTROL_DISABLE:   break;
    case RTS_CONTROL_ENABLE:    shf.FlowReplace |= SERIAL_RTS_CONTROL; break;
    case RTS_CONTROL_HANDSHAKE: shf.FlowReplace |= SERIAL_RTS_HANDSHAKE; break;
    case RTS_CONTROL_TOGGLE:    shf.FlowReplace |= SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE; break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (dcb->fDsrSensitivity)   shf.ControlHandShake |= SERIAL_DSR_SENSITIVITY;
    if (dcb->fAbortOnError)     shf.ControlHandShake |= SERIAL_ERROR_ABORT;
    if (dcb->fErrorChar)        shf.FlowReplace |= SERIAL_ERROR_CHAR;
    if (dcb->fNull)             shf.FlowReplace |= SERIAL_NULL_STRIPPING;
    if (dcb->fTXContinueOnXoff) shf.FlowReplace |= SERIAL_XOFF_CONTINUE;
    if (dcb->fOutX)             shf.FlowReplace |= SERIAL_AUTO_TRANSMIT;
    if (dcb->fInX)              shf.FlowReplace |= SERIAL_AUTO_RECEIVE;
    shf.XonLimit  = dcb->XonLim;
    shf.XoffLimit = dcb->XoffLim;
    sc.EofChar    = dcb->EofChar;
    sc.ErrorChar  = dcb->ErrorChar;
    sc.BreakChar  = 0;
    sc.EventChar  = dcb->EvtChar;
    sc.XonChar    = dcb->XonChar;
    sc.XoffChar   = dcb->XoffChar;

    /* note: change DTR/RTS lines after setting the comm attributes,
     * so flow control does not interfere.
     */
    return (DeviceIoControl( handle, IOCTL_SERIAL_SET_BAUD_RATE, &sbr, sizeof(sbr), NULL, 0, NULL, NULL ) &&
            DeviceIoControl( handle, IOCTL_SERIAL_SET_LINE_CONTROL, &slc, sizeof(slc), NULL, 0, NULL, NULL ) &&
            DeviceIoControl( handle, IOCTL_SERIAL_SET_HANDFLOW, &shf, sizeof(shf), NULL, 0, NULL, NULL ) &&
            DeviceIoControl( handle, IOCTL_SERIAL_SET_CHARS, &sc, sizeof(sc), NULL, 0, NULL, NULL ));
}


/*****************************************************************************
 *	SetCommTimeouts   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetCommTimeouts( HANDLE handle, COMMTIMEOUTS *timeouts )
{
    if (!timeouts)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return DeviceIoControl( handle, IOCTL_SERIAL_SET_TIMEOUTS, timeouts, sizeof(*timeouts),
                            NULL, 0, NULL, NULL );
}


/*****************************************************************************
 *      SetupComm   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetupComm( HANDLE handle, DWORD in_size, DWORD out_size )
{
    SERIAL_QUEUE_SIZE sqs;

    sqs.InSize = in_size;
    sqs.OutSize = out_size;
    return DeviceIoControl( handle, IOCTL_SERIAL_SET_QUEUE_SIZE, &sqs, sizeof(sqs), NULL, 0, NULL, NULL );
}


/*****************************************************************************
 *	TransmitCommChar   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TransmitCommChar( HANDLE handle, CHAR ch )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_IMMEDIATE_CHAR, &ch, sizeof(ch), NULL, 0, NULL, NULL );
}


/***********************************************************************
 *	WaitCommEvent   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WaitCommEvent( HANDLE handle, DWORD *events, OVERLAPPED *overlapped )
{
    return DeviceIoControl( handle, IOCTL_SERIAL_WAIT_ON_MASK, NULL, 0, events, sizeof(*events),
                            NULL, overlapped );
}
