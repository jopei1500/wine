/*
 * Unit tests for service functions
 *
 * Copyright (c) 2007 Paul Vriens
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winsvc.h"
#include "winnls.h"
#include "lmcons.h"
#include "aclapi.h"

#include "wine/test.h"

static const CHAR spooler[] = "Spooler"; /* Should be available on all platforms */
static CHAR selfname[MAX_PATH];

static BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);

static BOOL (WINAPI *pChangeServiceConfig2A)(SC_HANDLE,DWORD,LPVOID);
static BOOL (WINAPI *pChangeServiceConfig2W)(SC_HANDLE,DWORD,LPVOID);
static BOOL (WINAPI *pEnumServicesStatusExA)(SC_HANDLE, SC_ENUM_TYPE, DWORD,
                                             DWORD, LPBYTE, DWORD, LPDWORD,
                                             LPDWORD, LPDWORD, LPCSTR);
static BOOL (WINAPI *pEnumServicesStatusExW)(SC_HANDLE, SC_ENUM_TYPE, DWORD,
                                             DWORD, LPBYTE, DWORD, LPDWORD,
                                             LPDWORD, LPDWORD, LPCWSTR);
static DWORD (WINAPI *pGetSecurityInfo)(HANDLE, SE_OBJECT_TYPE, SECURITY_INFORMATION,
                                        PSID*, PSID*, PACL*, PACL*, PSECURITY_DESCRIPTOR*);
static BOOL (WINAPI *pQueryServiceConfig2A)(SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD);
static BOOL (WINAPI *pQueryServiceConfig2W)(SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD);
static BOOL (WINAPI *pQueryServiceStatusEx)(SC_HANDLE, SC_STATUS_TYPE, LPBYTE,
                                            DWORD, LPDWORD);
static BOOL (WINAPI *pQueryServiceObjectSecurity)(SC_HANDLE, SECURITY_INFORMATION,
                                                  PSECURITY_DESCRIPTOR, DWORD, LPDWORD);
static DWORD (WINAPI *pNotifyServiceStatusChangeW)(SC_HANDLE,DWORD,SERVICE_NOTIFYW*);

static void init_function_pointers(void)
{
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");

    pChangeServiceConfig2A = (void*)GetProcAddress(hadvapi32, "ChangeServiceConfig2A");
    pChangeServiceConfig2W = (void*)GetProcAddress(hadvapi32, "ChangeServiceConfig2W");
    pEnumServicesStatusExA= (void*)GetProcAddress(hadvapi32, "EnumServicesStatusExA");
    pEnumServicesStatusExW= (void*)GetProcAddress(hadvapi32, "EnumServicesStatusExW");
    pGetSecurityInfo = (void *)GetProcAddress(hadvapi32, "GetSecurityInfo");
    pQueryServiceConfig2A= (void*)GetProcAddress(hadvapi32, "QueryServiceConfig2A");
    pQueryServiceConfig2W= (void*)GetProcAddress(hadvapi32, "QueryServiceConfig2W");
    pQueryServiceStatusEx= (void*)GetProcAddress(hadvapi32, "QueryServiceStatusEx");
    pQueryServiceObjectSecurity = (void*)GetProcAddress(hadvapi32, "QueryServiceObjectSecurity");
    pNotifyServiceStatusChangeW = (void*)GetProcAddress(hadvapi32, "NotifyServiceStatusChangeW");

    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");
}

static void test_open_scm(void)
{
    SC_HANDLE scm_handle;

    /* No access rights */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, 0);
    ok(scm_handle != NULL, "Expected success, got error %u\n", GetLastError());
    CloseServiceHandle(scm_handle);

    /* Unknown database name */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, "DoesNotExist", SC_MANAGER_CONNECT);
    ok(!scm_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle); /* Just in case */

    /* MSDN says only ServiceActive is allowed, or NULL */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, SERVICES_FAILED_DATABASEA, SC_MANAGER_CONNECT);
    ok(!scm_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_DATABASE_DOES_NOT_EXIST, "Expected ERROR_DATABASE_DOES_NOT_EXIST, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle); /* Just in case */

    /* Remote unknown host */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA("DOESNOTEXIST", SERVICES_ACTIVE_DATABASEA, SC_MANAGER_CONNECT);
    todo_wine
    {
    ok(!scm_handle, "Expected failure\n");
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE || GetLastError() == RPC_S_INVALID_NET_ADDR /* w2k8 */,
       "Expected RPC_S_SERVER_UNAVAILABLE or RPC_S_INVALID_NET_ADDR, got %d\n", GetLastError());
    }
    CloseServiceHandle(scm_handle); /* Just in case */

    /* Proper call with an empty hostname */
    scm_handle = OpenSCManagerA("", SERVICES_ACTIVE_DATABASEA, SC_MANAGER_CONNECT);
    ok(scm_handle != NULL, "Expected success, got error %u\n", GetLastError());
    CloseServiceHandle(scm_handle);

    /* Again a correct one */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    ok(GetLastError() == ERROR_SUCCESS || broken(GetLastError() == ERROR_IO_PENDING) /* win2k */,
       "Expected ERROR_SUCCESS, got %u\n", GetLastError());
    ok(scm_handle != NULL, "Expected success, got error %u\n", GetLastError());
    CloseServiceHandle(scm_handle);
}

static void test_open_svc(void)
{
    SC_HANDLE scm_handle, svc_handle;
    CHAR displayname[4096];
    DWORD displaysize;

    /* All NULL (invalid access rights) */
    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(NULL, NULL, 0);
    ok(!svc_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* TODO: Add some tests with invalid handles. These produce errors on Windows but crash on Wine */

    /* NULL service */
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(scm_handle, NULL, GENERIC_READ);
    ok(!svc_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle);

    /* Nonexistent service */
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(scm_handle, "deadbeef", GENERIC_READ);
    ok(!svc_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST, "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    CloseServiceHandle(scm_handle);

    /* Proper SCM handle but different access rights */
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(scm_handle, spooler, GENERIC_WRITE);
    if (!svc_handle && (GetLastError() == ERROR_ACCESS_DENIED))
        skip("Not enough rights to get a handle to the service\n");
    else
    {
        ok(svc_handle != NULL, "Expected success, got error %u\n", GetLastError());
        CloseServiceHandle(svc_handle);
    }

    /* Test to show we can't open a service with the displayname */

    /* Retrieve the needed size for the buffer */
    displaysize = 0;
    GetServiceDisplayNameA(scm_handle, spooler, NULL, &displaysize);
    /* Get the displayname */
    GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    /* Try to open the service with this displayname, unless the displayname equals
     * the servicename as that would defeat the purpose of this test.
     */
    if (!lstrcmpiA(spooler, displayname))
    {
        skip("displayname equals servicename\n");
        CloseServiceHandle(scm_handle);
        return;
    }

    SetLastError(0xdeadbeef);
    svc_handle = OpenServiceA(scm_handle, displayname, GENERIC_READ);
    ok(!svc_handle, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST, "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    /* Just in case */
    CloseServiceHandle(svc_handle);

    CloseServiceHandle(scm_handle);
}

static void test_create_delete_svc(void)
{
    SC_HANDLE scm_handle, svc_handle1, svc_handle2;
    CHAR username[UNLEN + 1], domain[MAX_PATH];
    DWORD user_size = UNLEN + 1;
    CHAR account[UNLEN + 3];
    static const CHAR servicename         [] = "Winetest";
    static const CHAR pathname            [] = "we_dont_care.exe";
    static const CHAR empty               [] = "";
    static const CHAR password            [] = "secret";
    BOOL spooler_exists = FALSE;
    BOOL ret;
    CHAR display[4096];
    DWORD display_size = sizeof(display);

    /* Get the username and turn it into an account to be used in some tests */
    GetUserNameA(username, &user_size);
    /* Get the domainname to cater for that situation */
    if (GetEnvironmentVariableA("USERDOMAIN", domain, MAX_PATH))
        sprintf(account, "%s\\%s", domain, username);
    else
        sprintf(account, ".\\%s", username);

    /* All NULL */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    /* Only a valid handle to the Service Control Manager */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, W2K3, XP, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Now with a servicename */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, W2K3, XP, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Or just a binary name */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, NULL, NULL, 0, 0, 0, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, W2K3, XP, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Both servicename and binary name (We only have connect rights) */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, 0, 0, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* They can even be empty at this stage of parameter checking */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, empty, NULL, 0, 0, 0, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, 0, 0, 0, empty, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* Open the Service Control Manager with minimal rights for creation
     * (Verified with 'SC_MANAGER_ALL_ACCESS &~ SC_MANAGER_CREATE_SERVICE')
     */
    CloseServiceHandle(scm_handle);
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to get a handle to the manager\n");
        return;
    }

    /* TODO: It looks like account (ServiceStartName) and (maybe) password are checked at this place */

    /* Empty strings for servicename and binary name are checked */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, empty, NULL, 0, 0, 0, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, 0, 0, 0, empty, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, empty, NULL, 0, 0, 0, 0, empty, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Expected ERROR_INVALID_NAME, got %d\n", GetLastError());

    /* Valid call (as we will see later) except for the empty binary name (to proof it's indeed
     * an ERROR_INVALID_PARAMETER)
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_DISABLED, 0, empty, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Windows checks if the 'service type', 'access type' and the combination of them are valid, so let's test that */

    /* Illegal (service-type, which is used as a mask can't have a mix. Except the one with
     * SERVICE_INTERACTIVE_PROCESS which will be tested below in a valid call)
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL, SERVICE_WIN32_OWN_PROCESS | SERVICE_WIN32_SHARE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal (SERVICE_INTERACTIVE_PROCESS is only allowed with SERVICE_WIN32_OWN_PROCESS or SERVICE_WIN32_SHARE_PROCESS) */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL, SERVICE_FILE_SYSTEM_DRIVER | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal (this combination is only allowed when the LocalSystem account (ServiceStartName) is used)
     * Not having a correct account would have resulted in an ERROR_INVALID_SERVICE_ACCOUNT.
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL, SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, account, password);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INVALID_SERVICE_ACCOUNT,
       "Expected ERROR_INVALID_PARAMETER or ERROR_INVALID_SERVICE_ACCOUNT, got %d\n", GetLastError());

    /* Illegal (start-type is not a mask and should only be one of the possibilities)
     * Remark : 'OR'-ing them could result in a valid possibility (but doesn't make sense as
     * it's most likely not the wanted start-type)
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL, SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_AUTO_START | SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Illegal (SERVICE_BOOT_START and SERVICE_SYSTEM_START are only allowed for driver services) */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, SERVICE_WIN32_OWN_PROCESS,
                                 SERVICE_BOOT_START, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Test if ServiceType can be a combined one for drivers */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER,
                                 SERVICE_BOOT_START, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle1, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* The service already exists (check first, just in case) */
    svc_handle1 = OpenServiceA(scm_handle, spooler, GENERIC_READ);
    if (svc_handle1)
    {
        spooler_exists = TRUE;
        CloseServiceHandle(svc_handle1);
        SetLastError(0xdeadbeef);
        svc_handle1 = CreateServiceA(scm_handle, spooler, NULL, 0, SERVICE_WIN32_OWN_PROCESS,
                                     SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
        ok(!svc_handle1, "Expected failure\n");
        ok(GetLastError() == ERROR_SERVICE_EXISTS, "Expected ERROR_SERVICE_EXISTS, got %d\n", GetLastError());
    }
    else
        skip("Spooler service doesn't exist\n");

    /* To find an existing displayname we check the 'Spooler' service. Although the registry
     * doesn't show DisplayName on NT4, this call will return a displayname which is equal
     * to the servicename and can't be used as well for a new displayname.
     */
    if (spooler_exists)
    {
        ret = GetServiceDisplayNameA(scm_handle, spooler, display, &display_size);

        if (!ret)
            skip("Could not retrieve a displayname for the Spooler service\n");
        else
        {
            svc_handle1 = CreateServiceA(scm_handle, servicename, display, 0, SERVICE_WIN32_OWN_PROCESS,
                                         SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
            ok(!svc_handle1, "Expected failure for display name '%s'\n", display);
            ok(GetLastError() == ERROR_DUPLICATE_SERVICE_NAME,
               "Expected ERROR_DUPLICATE_SERVICE_NAME, got %d\n", GetLastError());
        }
    }
    else
        skip("Could not retrieve a displayname (Spooler service doesn't exist)\n");

    /* Windows doesn't care about the access rights for creation (which makes
     * sense as there is no service yet) as long as there are sufficient
     * rights to the manager.
     */
    SetLastError(0xdeadbeef);
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, 0, SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(svc_handle1 != NULL, "Could not create the service : %d\n", GetLastError());

    /* DeleteService however must have proper rights */
    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle1);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED,
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* Open the service with minimal rights for deletion.
     * (Verified with 'SERVICE_ALL_ACCESS &~ DELETE')
     */
    CloseServiceHandle(svc_handle1);
    svc_handle1 = OpenServiceA(scm_handle, servicename, DELETE);

    /* Now that we have the proper rights, we should be able to delete */
    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle1);
    ok(ret, "Expected success, got error %u\n", GetLastError());

    /* Service is marked for delete, but handle is still open. Try to open service again. */
    svc_handle2 = OpenServiceA(scm_handle, servicename, GENERIC_READ);
    ok(svc_handle2 != NULL, "got %p, error %u\n", svc_handle2, GetLastError());
    CloseServiceHandle(svc_handle2);

    CloseServiceHandle(svc_handle1);
    CloseServiceHandle(scm_handle);

    /* Wait a while. One of the following tests also does a CreateService for the
     * same servicename and this would result in an ERROR_SERVICE_MARKED_FOR_DELETE
     * error if we do this too quickly. Vista seems more picky than the others.
     */
    Sleep(1000);

    /* And a final NULL check */
    SetLastError(0xdeadbeef);
    ret = DeleteService(NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
        "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
}

static void test_get_displayname(void)
{
    SC_HANDLE scm_handle, svc_handle;
    BOOL ret;
    CHAR displayname[4096];
    WCHAR displaynameW[2048];
    DWORD displaysize, tempsize, tempsizeW;
    static const CHAR deadbeef[] = "Deadbeef";
    static const WCHAR spoolerW[] = {'S','p','o','o','l','e','r',0};
    static const WCHAR deadbeefW[] = {'D','e','a','d','b','e','e','f',0};
    static const WCHAR abcW[] = {'A','B','C',0};
    static const CHAR servicename[] = "Winetest";
    static const CHAR pathname[] = "we_dont_care.exe";

    /* Having NULL for the size of the buffer will crash on W2K3 */

    SetLastError(0xdeadbeef);
    ret = GetServiceDisplayNameA(NULL, NULL, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    SetLastError(0xdeadbeef);
    ret = GetServiceDisplayNameA(scm_handle, NULL, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    displaysize = sizeof(displayname);
    ret = GetServiceDisplayNameA(scm_handle, NULL, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Test for nonexistent service */
    SetLastError(0xdeadbeef);
    displaysize = -1;
    ret = GetServiceDisplayNameA(scm_handle, deadbeef, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetServiceDisplayNameA(scm_handle, deadbeef, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(displaysize == 1, "Service size expected 1, got %d\n", displaysize);

    displaysize = 15;
    strcpy(displayname, "ABC");
    ret = GetServiceDisplayNameA(scm_handle, deadbeef, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(displaysize == 15, "Service size expected 15, got %d\n", displaysize);
    ok(displayname[0] == 0, "Service name not empty\n");

    displaysize = 15;
    lstrcpyW( displaynameW, abcW );
    ret = GetServiceDisplayNameW(scm_handle, deadbeefW, displaynameW, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    ok(displaysize == 15, "Service size expected 15, got %d\n", displaysize);
    ok(displaynameW[0] == 0, "Service name not empty\n");

    displaysize = 0;
    strcpy(displayname, "ABC");
    ret = GetServiceDisplayNameA(scm_handle, deadbeef, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(displaysize == 1, "Service size expected 1, got %d\n", displaysize);
    ok(displayname[0] == 'A', "Service name changed\n");

    displaysize = 0;
    lstrcpyW( displaynameW, abcW );
    ret = GetServiceDisplayNameW(scm_handle, deadbeefW, displaynameW, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == 2, "Service size expected 2, got %d\n", displaysize);
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    ok(displaynameW[0] == 'A', "Service name changed\n");

    displaysize = 1;
    strcpy(displayname, "ABC");
    ret = GetServiceDisplayNameA(scm_handle, deadbeef, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(displaysize == 1, "Service size expected 1, got %d\n", displaysize);
    ok(displayname[0] == 0, "Service name not empty\n");

    displaysize = 1;
    lstrcpyW( displaynameW, abcW );
    ret = GetServiceDisplayNameW(scm_handle, deadbeefW, displaynameW, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == 2, "Service size expected 2, got %d\n", displaysize);
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    ok(displaynameW[0] == 'A', "Service name changed\n");

    displaysize = 2;
    strcpy(displayname, "ABC");
    ret = GetServiceDisplayNameA(scm_handle, deadbeef, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(displaysize == 2, "Service size expected 2, got %d\n", displaysize);
    ok(displayname[0] == 0, "Service name not empty\n");

    displaysize = 2;
    lstrcpyW( displaynameW, abcW );
    ret = GetServiceDisplayNameW(scm_handle, deadbeefW, displaynameW, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == 2, "Service size expected 2, got %d\n", displaysize);
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    ok(displaynameW[0] == 0, "Service name not empty\n");

    /* Check if 'Spooler' exists */
    svc_handle = OpenServiceA(scm_handle, spooler, GENERIC_READ);
    if (!svc_handle)
    {
        skip("Spooler service doesn't exist\n");
        CloseServiceHandle(scm_handle);
        return;
    }
    CloseServiceHandle(svc_handle);

    /* Retrieve the needed size for the buffer */
    SetLastError(0xdeadbeef);
    displaysize = -1;
    ret = GetServiceDisplayNameA(scm_handle, spooler, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    tempsize = displaysize;

    displaysize = 0;
    ret = GetServiceDisplayNameA(scm_handle, spooler, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(displaysize == tempsize, "Buffer size mismatch (%d vs %d)\n", tempsize, displaysize);

    /* Buffer is too small */
    SetLastError(0xdeadbeef);
    displaysize = (tempsize / 2);
    ret = GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == tempsize, "Expected the needed buffersize\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* First try with a buffer that should be big enough to hold
     * the ANSI string (and terminating character). This succeeds on Windows
     *  although when asked (see above 2 tests) it will return twice the needed size.
     */
    SetLastError(0xdeadbeef);
    displaysize = (tempsize / 2) + 1;
    ret = GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(displaysize == ((tempsize / 2) + 1), "Expected no change for the needed buffer size\n");

    /* Now with the original returned size */
    SetLastError(0xdeadbeef);
    displaysize = tempsize;
    ret = GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(displaysize == tempsize, "Expected no change for the needed buffer size\n");

    /* And with a bigger than needed buffer */
    SetLastError(0xdeadbeef);
    displaysize = tempsize * 2;
    ret = GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    /* Test that shows that if the buffersize is enough, it's not changed */
    ok(displaysize == tempsize * 2, "Expected no change for the needed buffer size\n");
    ok(strlen(displayname) == tempsize/2,
       "Expected the buffer to be twice the length of the string\n") ;

    /* Do the buffer(size) tests also for GetServiceDisplayNameW */
    SetLastError(0xdeadbeef);
    displaysize = -1;
    ret = GetServiceDisplayNameW(scm_handle, spoolerW, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Buffer is too small */
    SetLastError(0xdeadbeef);
    tempsizeW = displaysize;
    displaysize = tempsizeW / 2;
    ret = GetServiceDisplayNameW(scm_handle, spoolerW, displaynameW, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == tempsizeW, "Expected the needed buffersize\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Now with the original returned size */
    SetLastError(0xdeadbeef);
    displaysize = tempsizeW;
    ret = GetServiceDisplayNameW(scm_handle, spoolerW, displaynameW, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == tempsizeW, "Expected the needed buffersize\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* And with a bigger than needed buffer */
    SetLastError(0xdeadbeef);
    displaysize = tempsizeW + 1; /* This caters for the null terminating character */
    ret = GetServiceDisplayNameW(scm_handle, spoolerW, displaynameW, &displaysize);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(displaysize == tempsizeW, "Expected the needed buffersize\n");
    ok(lstrlenW(displaynameW) == displaysize,
       "Expected the buffer to be the length of the string\n") ;
    ok(tempsize / 2 == tempsizeW,
       "Expected the needed buffersize (in bytes) to be the same for the A and W call\n");

    CloseServiceHandle(scm_handle);

    /* Test for a service without a displayname (which is valid). This should return
     * the servicename itself.
     */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to get a handle to the manager\n");
        return;
    }

    SetLastError(0xdeadbeef);
    svc_handle = CreateServiceA(scm_handle, servicename, NULL, DELETE,
                                SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(svc_handle != NULL, "Could not create the service : %d\n", GetLastError());
    if (!svc_handle)
    {
        CloseServiceHandle(scm_handle);
        return;
    }

    /* Retrieve the needed size for the buffer */
    SetLastError(0xdeadbeef);
    displaysize = -1;
    ret = GetServiceDisplayNameA(scm_handle, servicename, NULL, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == strlen(servicename) * 2,
       "Expected the displaysize to be twice the size of the servicename\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Buffer is too small */
    SetLastError(0xdeadbeef);
    tempsize = displaysize;
    displaysize = (tempsize / 2);
    ret = GetServiceDisplayNameA(scm_handle, servicename, displayname, &displaysize);
    ok(!ret, "Expected failure\n");
    ok(displaysize == tempsize, "Expected the needed buffersize\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Get the displayname */
    SetLastError(0xdeadbeef);
    ret = GetServiceDisplayNameA(scm_handle, servicename, displayname, &displaysize);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(!lstrcmpiA(displayname, servicename),
       "Expected displayname to be %s, got %s\n", servicename, displayname);

    /* Delete the service */
    ret = DeleteService(svc_handle);
    ok(ret, "Expected success (err=%d)\n", GetLastError());

    CloseServiceHandle(svc_handle);
    CloseServiceHandle(scm_handle);

    /* Wait a while. Just in case one of the following tests does a CreateService again */
    Sleep(1000);
}

static void test_get_servicekeyname(void)
{
    SC_HANDLE scm_handle, svc_handle;
    CHAR servicename[4096];
    CHAR displayname[4096];
    WCHAR servicenameW[4096];
    WCHAR displaynameW[4096];
    DWORD servicesize, displaysize, tempsize;
    BOOL ret;
    static const CHAR deadbeef[] = "Deadbeef";
    static const WCHAR deadbeefW[] = {'D','e','a','d','b','e','e','f',0};
    static const WCHAR abcW[] = {'A','B','C',0};

    /* Having NULL for the size of the buffer will crash on W2K3 */

    SetLastError(0xdeadbeef);
    ret = GetServiceKeyNameA(NULL, NULL, NULL, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    servicesize = 200;
    SetLastError(0xdeadbeef);
    ret = GetServiceKeyNameA(scm_handle, NULL, NULL, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    todo_wine ok(servicesize == 1, "Service size expected 1, got %d\n", servicesize);

    /* Valid handle and buffer but no displayname */
    servicesize = 200;
    SetLastError(0xdeadbeef);
    ret = GetServiceKeyNameA(scm_handle, NULL, servicename, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS   /* W2K, XP, W2K3, Vista */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Expected ERROR_INVALID_ADDRESS or ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    todo_wine ok(servicesize == 200, "Service size expected 1, got %d\n", servicesize);

    /* Test for nonexistent displayname */
    SetLastError(0xdeadbeef);
    ret = GetServiceKeyNameA(scm_handle, deadbeef, NULL, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(servicesize == 1, "Service size expected 1, got %d\n", servicesize);

    servicesize = 15;
    strcpy(servicename, "ABC");
    ret = GetServiceKeyNameA(scm_handle, deadbeef, servicename, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(servicesize == 15, "Service size expected 15, got %d\n", servicesize);
    ok(servicename[0] == 0, "Service name not empty\n");

    servicesize = 15;
    lstrcpyW( servicenameW, abcW );
    ret = GetServiceKeyNameW(scm_handle, deadbeefW, servicenameW, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    ok(servicesize == 15, "Service size expected 15, got %d\n", servicesize);
    ok(servicenameW[0] == 0, "Service name not empty\n");

    servicesize = 0;
    strcpy(servicename, "ABC");
    ret = GetServiceKeyNameA(scm_handle, deadbeef, servicename, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(servicesize == 1, "Service size expected 1, got %d\n", servicesize);
    ok(servicename[0] == 'A', "Service name changed\n");

    servicesize = 0;
    lstrcpyW( servicenameW, abcW );
    ret = GetServiceKeyNameW(scm_handle, deadbeefW, servicenameW, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(servicesize == 2, "Service size expected 2, got %d\n", servicesize);
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    ok(servicenameW[0] == 'A', "Service name changed\n");

    servicesize = 1;
    strcpy(servicename, "ABC");
    ret = GetServiceKeyNameA(scm_handle, deadbeef, servicename, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(servicesize == 1, "Service size expected 1, got %d\n", servicesize);
    ok(servicename[0] == 0, "Service name not empty\n");

    servicesize = 1;
    lstrcpyW( servicenameW, abcW );
    ret = GetServiceKeyNameW(scm_handle, deadbeefW, servicenameW, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(servicesize == 2, "Service size expected 2, got %d\n", servicesize);
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    ok(servicenameW[0] == 'A', "Service name changed\n");

    servicesize = 2;
    strcpy(servicename, "ABC");
    ret = GetServiceKeyNameA(scm_handle, deadbeef, servicename, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    todo_wine ok(servicesize == 2, "Service size expected 2, got %d\n", servicesize);
    ok(servicename[0] == 0, "Service name not empty\n");

    servicesize = 2;
    lstrcpyW( servicenameW, abcW );
    ret = GetServiceKeyNameW(scm_handle, deadbeefW, servicenameW, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(servicesize == 2, "Service size expected 2, got %d\n", servicesize);
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());
    ok(servicenameW[0] == 0, "Service name not empty\n");

    /* Check if 'Spooler' exists */
    svc_handle = OpenServiceA(scm_handle, spooler, GENERIC_READ);
    if (!svc_handle)
    {
        skip("Spooler service doesn't exist\n");
        CloseServiceHandle(scm_handle);
        return;
    }
    CloseServiceHandle(svc_handle);

    /* Get the displayname for the 'Spooler' service */
    GetServiceDisplayNameA(scm_handle, spooler, NULL, &displaysize);
    GetServiceDisplayNameA(scm_handle, spooler, displayname, &displaysize);

    /* Retrieve the needed size for the buffer */
    SetLastError(0xdeadbeef);
    servicesize = 0;
    ret = GetServiceKeyNameA(scm_handle, displayname, NULL, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    /* Valid call with the correct buffersize */
    SetLastError(0xdeadbeef);
    tempsize = servicesize;
    servicesize *= 2;
    ret = GetServiceKeyNameA(scm_handle, displayname, servicename, &servicesize);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    if (ret)
    {
        ok(strlen(servicename) == tempsize/2,
           "Expected the buffer to be twice the length of the string\n") ;
        ok(!lstrcmpiA(servicename, spooler), "Expected %s, got %s\n", spooler, servicename);
        ok(servicesize == (tempsize * 2),
           "Expected servicesize not to change if buffer not insufficient\n") ;
    }

    MultiByteToWideChar(CP_ACP, 0, displayname, -1, displaynameW, sizeof(displaynameW)/2);
    SetLastError(0xdeadbeef);
    servicesize *= 2;
    ret = GetServiceKeyNameW(scm_handle, displaynameW, servicenameW, &servicesize);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    if (ret)
    {
        ok(strlen(servicename) == tempsize/2,
           "Expected the buffer to be twice the length of the string\n") ;
        ok(servicesize == lstrlenW(servicenameW),
           "Expected servicesize not to change if buffer not insufficient\n") ;
    }

    SetLastError(0xdeadbeef);
    servicesize = 3;
    ret = GetServiceKeyNameW(scm_handle, displaynameW, servicenameW, &servicesize);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(servicenameW[0] == 0, "Buffer not empty\n");

    CloseServiceHandle(scm_handle);
}

static void test_query_svc(void)
{
    SC_HANDLE scm_handle, svc_handle;
    BOOL ret;
    SERVICE_STATUS status;
    SERVICE_STATUS_PROCESS *statusproc;
    DWORD bufsize, needed;

    /* All NULL or wrong  */
    SetLastError(0xdeadbeef);
    ret = QueryServiceStatus(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    /* Check if 'Spooler' exists.
     * Open with not enough rights to query the status.
     */
    svc_handle = OpenServiceA(scm_handle, spooler, STANDARD_RIGHTS_READ);
    if (!svc_handle)
    {
        skip("Spooler service doesn't exist\n");
        CloseServiceHandle(scm_handle);
        return;
    }

    SetLastError(0xdeadbeef);
    ret = QueryServiceStatus(svc_handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = QueryServiceStatus(svc_handle, &status);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED,
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* Open the service with just enough rights.
     * (Verified with 'SERVICE_ALL_ACCESS &~ SERVICE_QUERY_STATUS')
     */
    CloseServiceHandle(svc_handle);
    svc_handle = OpenServiceA(scm_handle, spooler, SERVICE_QUERY_STATUS);

    SetLastError(0xdeadbeef);
    ret = QueryServiceStatus(svc_handle, &status);
    ok(ret, "Expected success, got error %u\n", GetLastError());

    CloseServiceHandle(svc_handle);

    /* More or less the same tests for QueryServiceStatusEx */
    if (!pQueryServiceStatusEx)
    {
        win_skip( "QueryServiceStatusEx not available\n" );
        CloseServiceHandle(scm_handle);
        return;
    }

    /* Open service with not enough rights to query the status */
    svc_handle = OpenServiceA(scm_handle, spooler, STANDARD_RIGHTS_READ);

    /* All NULL or wrong, this proves that info level is checked first */
    SetLastError(0xdeadbeef);
    ret = pQueryServiceStatusEx(NULL, 1, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL,
       "Expected ERROR_INVALID_LEVEL, got %d\n", GetLastError());

    /* Passing a NULL parameter for the needed buffer size
     * will crash on anything but NT4.
     */

    /* Only info level is correct. It looks like the buffer/size is checked second */
    SetLastError(0xdeadbeef);
    ret = pQueryServiceStatusEx(NULL, SC_STATUS_PROCESS_INFO, NULL, 0, &needed);
    /* NT4 checks the handle first */
    if (GetLastError() != ERROR_INVALID_HANDLE)
    {
        ok(!ret, "Expected failure\n");
        ok(needed == sizeof(SERVICE_STATUS_PROCESS),
           "Needed buffersize is wrong : %d\n", needed);
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
           "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    }

    /* Pass a correct buffer and buffersize but a NULL handle */
    statusproc = HeapAlloc(GetProcessHeap(), 0, sizeof(SERVICE_STATUS_PROCESS));
    bufsize = needed;
    SetLastError(0xdeadbeef);
    ret = pQueryServiceStatusEx(NULL, SC_STATUS_PROCESS_INFO, (BYTE*)statusproc, bufsize, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, statusproc);

    /* Correct handle and info level */
    SetLastError(0xdeadbeef);
    ret = pQueryServiceStatusEx(svc_handle, SC_STATUS_PROCESS_INFO, NULL, 0, &needed);
    /* NT4 doesn't return the needed size */
    if (GetLastError() != ERROR_INVALID_PARAMETER)
    {
        ok(!ret, "Expected failure\n");
        ok(needed == sizeof(SERVICE_STATUS_PROCESS),
           "Needed buffersize is wrong : %d\n", needed);
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
           "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    }

    /* All parameters are OK but we don't have enough rights */
    statusproc = HeapAlloc(GetProcessHeap(), 0, sizeof(SERVICE_STATUS_PROCESS));
    bufsize = sizeof(SERVICE_STATUS_PROCESS);
    SetLastError(0xdeadbeef);
    ret = pQueryServiceStatusEx(svc_handle, SC_STATUS_PROCESS_INFO, (BYTE*)statusproc, bufsize, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED,
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, statusproc);

    /* Open the service with just enough rights. */
    CloseServiceHandle(svc_handle);
    svc_handle = OpenServiceA(scm_handle, spooler, SERVICE_QUERY_STATUS);

    /* Everything should be fine now. */
    statusproc = HeapAlloc(GetProcessHeap(), 0, sizeof(SERVICE_STATUS_PROCESS));
    bufsize = sizeof(SERVICE_STATUS_PROCESS);
    SetLastError(0xdeadbeef);
    ret = pQueryServiceStatusEx(svc_handle, SC_STATUS_PROCESS_INFO, (BYTE*)statusproc, bufsize, &needed);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    if (statusproc->dwCurrentState == SERVICE_RUNNING)
        ok(statusproc->dwProcessId != 0,
           "Expect a process id for this running service\n");
    else
        ok(statusproc->dwProcessId == 0,
           "Expect no process id for this stopped service\n");

    /* same call with null needed pointer */
    SetLastError(0xdeadbeef);
    ret = pQueryServiceStatusEx(svc_handle, SC_STATUS_PROCESS_INFO, (BYTE*)statusproc, bufsize, NULL);
    ok(!ret, "Expected failure\n");
    ok(broken(GetLastError() == ERROR_INVALID_PARAMETER) /* NT4 */ ||
       GetLastError() == ERROR_INVALID_ADDRESS, "got %d\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, statusproc);

    CloseServiceHandle(svc_handle);
    CloseServiceHandle(scm_handle);
}

static void test_enum_svc(void)
{
    SC_HANDLE scm_handle;
    BOOL ret;
    DWORD bufsize, needed, returned, resume;
    DWORD neededW, returnedW;
    DWORD tempneeded, tempreturned, missing;
    DWORD servicecountactive, servicecountinactive;
    ENUM_SERVICE_STATUSA *services;
    ENUM_SERVICE_STATUSW *servicesW;
    ENUM_SERVICE_STATUS_PROCESSA *exservices;
    UINT i;

    /* All NULL or wrong  */
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(NULL, 1, 0, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(NULL, 1, 0, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* Open the service control manager with not enough rights at first */
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    /* Valid handle but rest is still NULL or wrong  */
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, 1, 0, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, 1, 0, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    /* Don't specify the two required pointers */
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, 0, 0, NULL, 0, NULL, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(returned == 0xdeadbeef, "Expected no change to the number of services variable\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, 0, 0, NULL, 0, NULL, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(returned == 0xdeadbeef, "Expected no change to the number of services variable\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    /* Don't specify the two required pointers */
    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, 0, 0, NULL, 0, &needed, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0xdeadbeef || broken(needed != 0xdeadbeef), /* nt4 */
       "Expected no change to the needed buffer variable\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, 0, 0, NULL, 0, &needed, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0xdeadbeef || broken(needed != 0xdeadbeef), /* nt4 */
       "Expected no change to the needed buffer variable\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    /* No valid servicetype and servicestate */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, 0, 0, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0, "Expected number of services to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, 0, 0, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0 || broken(returned != 0), /* nt4 */
       "Expected number of services to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No valid servicestate */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, SERVICE_WIN32, 0, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0, "Expected number of services to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, SERVICE_WIN32, 0, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0 || broken(returned != 0), /* nt4 */
       "Expected number of services to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No valid servicetype */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, 0, SERVICE_STATE_ALL, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0, "Expected number of services to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, 0, SERVICE_STATE_ALL, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0 || broken(returned != 0), /* nt4 */
       "Expected number of services to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* All parameters are correct but our access rights are wrong */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0, "Expected number of services to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_ACCESS_DENIED,
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0 || broken(returned != 0), /* nt4 */
       "Expected number of services to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_ACCESS_DENIED,
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* Open the service control manager with the needed rights */
    CloseServiceHandle(scm_handle);
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    /* All parameters are correct. Request the needed buffer size */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed != 0xdeadbeef && needed > 0, "Expected the needed buffer size for this one service\n");
    ok(returned == 0, "Expected no service returned, got %d\n", returned);
    ok(GetLastError() == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", GetLastError());

    /* Test to show we get the same needed buffer size for the W-call */
    neededW = 0xdeadbeef;
    returnedW = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &neededW, &returnedW, NULL);
    ok(!ret, "Expected failure\n");
    ok(neededW != 0xdeadbeef && neededW > 0, "Expected the needed buffer size for this one service\n");
    ok(neededW == needed, "Expected needed buffersize to be the same for A- and W-calls\n");
    ok(returnedW == 0, "Expected no service returned, got %d\n", returnedW);
    ok(GetLastError() == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", GetLastError());

    /* Store the needed bytes */
    tempneeded = needed;

    /* Allocate the correct needed bytes */
    services = HeapAlloc(GetProcessHeap(), 0, needed);
    bufsize = needed;
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL,
                              services, bufsize, &needed, &returned, NULL);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(needed == 0, "Expected needed buffer to be 0 as we are done\n");
    ok(returned != 0xdeadbeef && returned > 0, "Expected some returned services\n");
    HeapFree(GetProcessHeap(), 0, services);

    /* Store the number of returned services */
    tempreturned = returned;

    servicesW = HeapAlloc(GetProcessHeap(), 0, neededW);
    bufsize = neededW;
    neededW = 0xdeadbeef;
    returnedW = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusW(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL,
                              servicesW, bufsize, &neededW, &returnedW, NULL);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(neededW == 0, "Expected needed buffer to be 0 as we are done\n");
    ok(returnedW != 0xdeadbeef && returnedW > 0, "Expected some returned services\n");
    HeapFree(GetProcessHeap(), 0, servicesW);

    /* Allocate less than the needed bytes and don't specify a resume handle */
    services = HeapAlloc(GetProcessHeap(), 0, tempneeded);
    bufsize = (tempreturned - 1) * sizeof(ENUM_SERVICE_STATUSA);
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL,
                              services, bufsize, &needed, &returned, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed != 0xdeadbeef && needed > 0, "Expected the needed buffer size for this one service\n");
    ok(returned < tempreturned, "Expected fewer services to be returned\n");
    ok(GetLastError() == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", GetLastError());

    /* Allocate less than the needed bytes, this time with a correct resume handle */
    bufsize = (tempreturned - 1) * sizeof(ENUM_SERVICE_STATUSA);
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    resume = 0;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL,
                              services, bufsize, &needed, &returned, &resume);
    ok(!ret, "Expected failure\n");
    ok(needed != 0xdeadbeef && needed > 0, "Expected the needed buffer size for this one service\n");
    ok(returned < tempreturned, "Expected fewer services to be returned\n");
    todo_wine ok(resume, "Expected a resume handle\n");
    ok(GetLastError() == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", GetLastError());

    /* Fetch the missing services but pass a bigger buffer size */
    missing = tempreturned - returned;
    bufsize = tempneeded;
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL,
                              services, bufsize, &needed, &returned, &resume);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(needed == 0, "Expected needed buffer to be 0 as we are done\n");
    ok(returned == missing, "Expected %u services to be returned\n", missing);
    ok(resume == 0, "Expected the resume handle to be 0\n");
    HeapFree(GetProcessHeap(), 0, services);

    /* See if things add up */

    /* Vista only shows the drivers with a state of SERVICE_RUNNING as active
     * and doesn't count the others as inactive. This means that Vista could
     * show a total that is greater than the sum of active and inactive
     * drivers.
     * The number of active and inactive drivers is greatly influenced by the
     * time when tests are run, immediately after boot or later for example.
     *
     * Both reasons make calculations for drivers not so useful
     */

    /* Get the number of active win32 services */
    EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_ACTIVE, NULL, 0,
                        &needed, &returned, NULL);
    services = HeapAlloc(GetProcessHeap(), 0, needed);
    EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_ACTIVE, services,
                        needed, &needed, &returned, NULL);
    HeapFree(GetProcessHeap(), 0, services);

    servicecountactive = returned;

    /* Get the number of inactive win32 services */
    EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_INACTIVE, NULL, 0,
                        &needed, &returned, NULL);
    services = HeapAlloc(GetProcessHeap(), 0, needed);
    EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_INACTIVE, services,
                        needed, &needed, &returned, NULL);
    HeapFree(GetProcessHeap(), 0, services);

    servicecountinactive = returned;

    /* Get the number of win32 services */
    EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0,
                        &needed, &returned, NULL);
    services = HeapAlloc(GetProcessHeap(), 0, needed);
    EnumServicesStatusA(scm_handle, SERVICE_WIN32, SERVICE_STATE_ALL, services,
                        needed, &needed, &returned, NULL);
    HeapFree(GetProcessHeap(), 0, services);

    /* Check if total is the same as active and inactive win32 services */
    ok(returned == (servicecountactive + servicecountinactive),
       "Something wrong in the calculation\n");

    /* Get all drivers and services
     *
     * Fetch the status of the last call as failing could make the following tests crash
     * on Wine (we don't return anything yet).
     */
    EnumServicesStatusA(scm_handle, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL,
                        NULL, 0, &needed, &returned, NULL);
    services = HeapAlloc(GetProcessHeap(), 0, needed);
    ret = EnumServicesStatusA(scm_handle, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL,
                              services, needed, &needed, &returned, NULL);

    /* Loop through all those returned drivers and services */
    for (i = 0; ret && i < returned; i++)
    {
        SERVICE_STATUS status = services[i].ServiceStatus;

        /* lpServiceName and lpDisplayName should always be filled */
        ok(services[i].lpServiceName[0], "Expected a service name\n");
        ok(services[i].lpDisplayName && services[i].lpDisplayName[0], "Expected a display name\n");

        /* Decrement the counters to see if the functions calls return the same
         * numbers as the contents of these structures.
         */
        if (status.dwServiceType & (SERVICE_WIN32_OWN_PROCESS | SERVICE_WIN32_SHARE_PROCESS))
        {
            if (status.dwCurrentState == SERVICE_STOPPED)
                servicecountinactive--;
            else
                servicecountactive--;
        }
    }
    HeapFree(GetProcessHeap(), 0, services);

    ok(servicecountactive == 0, "Active services mismatch %u\n", servicecountactive);
    ok(servicecountinactive == 0, "Inactive services mismatch %u\n", servicecountinactive);

    CloseServiceHandle(scm_handle);

    /* More or less the same for EnumServicesStatusExA */
    if (!pEnumServicesStatusExA)
    {
        win_skip( "EnumServicesStatusExA not available\n" );
        return;
    }

    /* All NULL or wrong */
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(NULL, 1, 0, 0, NULL, 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL,
       "Expected ERROR_INVALID_LEVEL, got %d\n", GetLastError());

    /* All NULL or wrong, just the info level is correct */
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(NULL, 0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* Open the service control manager with not enough rights at first */
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    /* Valid handle and info level but rest is still NULL or wrong */
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    /* Don't specify the two required pointers */
    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, 0, 0, NULL, 0, &needed, NULL, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0xdeadbeef || broken(needed != 0xdeadbeef), /* nt4 */
       "Expected no change to the needed buffer variable\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    /* Don't specify the two required pointers */
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, 0, 0, NULL, 0, NULL, &returned, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(returned == 0xdeadbeef, "Expected no change to the number of services variable\n");
    ok(GetLastError() == ERROR_INVALID_ADDRESS ||
       GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
       "Unexpected last error %d\n", GetLastError());

    /* No valid servicetype and servicestate */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, 0, 0, NULL, 0, &needed, &returned, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(returned == 0, "Expected number of service to be set to 0, got %d\n", returned);
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No valid servicestate */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, 0, NULL, 0,
                                 &needed, &returned, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(returned == 0, "Expected number of service to be set to 0, got %d\n", returned);
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No valid servicetype */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, 0, SERVICE_STATE_ALL, NULL, 0,
                                 &needed, &returned, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(returned == 0, "Expected number of service to be set to 0, got %d\n", returned);
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* No valid servicetype and servicestate and unknown service group */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, 0, 0, NULL, 0, &needed,
                                 &returned, NULL, "deadbeef_group");
    ok(!ret, "Expected failure\n");
    ok(returned == 0, "Expected number of service to be set to 0, got %d\n", returned);
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* All parameters are correct but our access rights are wrong */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 NULL, 0, &needed, &returned, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0, "Expected number of service to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_ACCESS_DENIED,
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* All parameters are correct, access rights are wrong but the
     * group name won't be checked yet.
     */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 NULL, 0, &needed, &returned, NULL, "deadbeef_group");
    ok(!ret, "Expected failure\n");
    ok(needed == 0 || broken(needed != 0), /* nt4 */
       "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(returned == 0, "Expected number of service to be set to 0, got %d\n", returned);
    ok(GetLastError() == ERROR_ACCESS_DENIED,
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* Open the service control manager with the needed rights */
    CloseServiceHandle(scm_handle);
    scm_handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    /* All parameters are correct and the group will be checked */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 NULL, 0, &needed, &returned, NULL, "deadbeef_group");
    ok(!ret, "Expected failure\n");
    ok(returned == 0, "Expected number of service to be set to 0, got %d\n", returned);
    ok(needed == 0, "Expected needed buffer size to be set to 0, got %d\n", needed);
    ok(GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST,
       "Expected ERROR_SERVICE_DOES_NOT_EXIST, got %d\n", GetLastError());

    /* TODO: Create a test that makes sure we enumerate all services that don't
     * belong to a group. (specifying "").
     */

    /* All parameters are correct. Request the needed buffer size */
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 NULL, 0, &needed, &returned, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(returned == 0, "Expected no service returned, got %d\n", returned);
    ok(needed != 0xdeadbeef && needed > 0, "Expected the needed buffer size\n");
    ok(GetLastError() == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", GetLastError());

    /* Test to show we get the same needed buffer size for the W-call */
    neededW = 0xdeadbeef;
    ret = pEnumServicesStatusExW(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 NULL, 0, &neededW, &returnedW, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(neededW == needed, "Expected needed buffersize to be the same for A- and W-calls\n");

    /* Store the needed bytes */
    tempneeded = needed;

    /* Allocate the correct needed bytes */
    exservices = HeapAlloc(GetProcessHeap(), 0, needed);
    bufsize = needed;
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 (BYTE*)exservices, bufsize, &needed, &returned, NULL, NULL);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(needed == 0, "Expected needed buffer to be 0 as we are done\n");
    ok(returned == tempreturned, "Expected the same number of service from this function\n");
    HeapFree(GetProcessHeap(), 0, exservices);

    /* Store the number of returned services */
    tempreturned = returned;

    /* Allocate less than the needed bytes and don't specify a resume handle */
    exservices = HeapAlloc(GetProcessHeap(), 0, tempneeded);
    bufsize = (tempreturned - 1) * sizeof(ENUM_SERVICE_STATUSA);
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 (BYTE*)exservices, bufsize, &needed, &returned, NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed != 0xdeadbeef && needed > 0, "Expected the needed buffer size\n");
    ok(returned < tempreturned, "Expected fewer services to be returned\n");
    ok(GetLastError() == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", GetLastError());

    /* Allocate less than the needed bytes, this time with a correct resume handle */
    bufsize = (tempreturned - 1) * sizeof(ENUM_SERVICE_STATUSA);
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    resume = 0;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 (BYTE*)exservices, bufsize, &needed, &returned, &resume, NULL);
    ok(!ret, "Expected failure\n");
    ok(needed != 0xdeadbeef && needed > 0, "Expected the needed buffer size\n");
    ok(returned < tempreturned, "Expected fewer services to be returned\n");
    todo_wine ok(resume, "Expected a resume handle\n");
    ok(GetLastError() == ERROR_MORE_DATA,
       "Expected ERROR_MORE_DATA, got %d\n", GetLastError());

    /* Fetch that last service but pass a bigger buffer size */
    missing = tempreturned - returned;
    bufsize = tempneeded;
    needed = 0xdeadbeef;
    returned = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                                 (BYTE*)exservices, bufsize, &needed, &returned, &resume, NULL);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    ok(needed == 0, "Expected needed buffer to be 0 as we are done\n");
    ok(returned == missing, "Expected %u services to be returned\n", missing);
    ok(resume == 0, "Expected the resume handle to be 0\n");
    HeapFree(GetProcessHeap(), 0, exservices);

    /* See if things add up */

    /* Get the number of active win32 services */
    pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_ACTIVE,
                           NULL, 0, &needed, &returned, NULL, NULL);
    exservices = HeapAlloc(GetProcessHeap(), 0, needed);
    pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_ACTIVE,
                           (BYTE*)exservices, needed, &needed, &returned, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, exservices);

    servicecountactive = returned;

    /* Get the number of inactive win32 services */
    pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_INACTIVE,
                           NULL, 0, &needed, &returned, NULL, NULL);
    exservices = HeapAlloc(GetProcessHeap(), 0, needed);
    pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_INACTIVE,
                           (BYTE*)exservices, needed, &needed, &returned, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, exservices);

    servicecountinactive = returned;

    /* Get the number of win32 services */
    pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                           NULL, 0, &needed, &returned, NULL, NULL);
    exservices = HeapAlloc(GetProcessHeap(), 0, needed);
    pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32, SERVICE_STATE_ALL,
                           (BYTE*)exservices, needed, &needed, &returned, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, exservices);

    /* Check if total is the same as active and inactive win32 services */
    ok(returned == (servicecountactive + servicecountinactive),
       "Something wrong in the calculation\n");

    /* Get all drivers and services */
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32 | SERVICE_DRIVER,
                                 SERVICE_STATE_ALL, NULL, 0, &needed, &returned, NULL, NULL);
    ok(!ret, "Expected failure\n");
    exservices = HeapAlloc(GetProcessHeap(), 0, needed);
    ret = pEnumServicesStatusExA(scm_handle, 0, SERVICE_WIN32 | SERVICE_DRIVER,
                                 SERVICE_STATE_ALL, (BYTE*)exservices, needed, &needed, &returned, NULL, NULL);
    ok(ret, "Expected success %u\n", GetLastError());

    /* Loop through all those returned drivers and services */
    for (i = 0; i < returned; i++)
    {
        SERVICE_STATUS_PROCESS status = exservices[i].ServiceStatusProcess;

        /* lpServiceName and lpDisplayName should always be filled */
        ok(exservices[i].lpServiceName[0], "Expected a service name\n");
        ok(exservices[i].lpDisplayName && exservices[i].lpDisplayName[0], "Expected a display name\n");

        /* Decrement the counters to see if the functions calls return the
         * same numbers as the contents of these structures.
         * Check some process id specifics.
         */
        if (status.dwServiceType & (SERVICE_FILE_SYSTEM_DRIVER | SERVICE_KERNEL_DRIVER))
        {
            /* We shouldn't have a process id for drivers */
            ok(status.dwProcessId == 0,
               "This driver shouldn't have an associated process id\n");
        }

        if (status.dwServiceType & (SERVICE_WIN32_OWN_PROCESS | SERVICE_WIN32_SHARE_PROCESS))
        {
            if (status.dwCurrentState != SERVICE_STOPPED)
            {
                /* We expect a process id for every running service */
                ok(status.dwProcessId > 0, "Expected a process id for this running service (%s)\n",
                   exservices[i].lpServiceName);

                servicecountactive--;
            }
            else
            {
                /* We shouldn't have a process id for inactive services */
                ok(status.dwProcessId == 0, "Service %s state %u shouldn't have an associated process id\n",
                   exservices[i].lpServiceName, status.dwCurrentState);

                servicecountinactive--;
            }
        }
    }
    HeapFree(GetProcessHeap(), 0, exservices);

    ok(servicecountactive == 0, "Active services mismatch %u\n", servicecountactive);
    ok(servicecountinactive == 0, "Inactive services mismatch %u\n", servicecountinactive);

    CloseServiceHandle(scm_handle);
}

static void test_close(void)
{
    SC_HANDLE handle;
    BOOL ret;

    /* NULL handle */
    SetLastError(0xdeadbeef);
    ret = CloseServiceHandle(NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* TODO: Add some tests with invalid handles. These produce errors on Windows but crash on Wine */

    /* Proper call */
    handle = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    SetLastError(0xdeadbeef);
    ret = CloseServiceHandle(handle);
    ok(ret, "Expected success got error %u\n", GetLastError());
}

static void test_wow64(void)
{
    SC_HANDLE manager, service;
    BOOL wow64, ret;
    HANDLE file;

    if (!pIsWow64Process || !pIsWow64Process(GetCurrentProcess(), &wow64) || !wow64)
    {
        skip("Not running under WoW64.\n");
        return;
    }

    if (!(manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE)))
    {
        skip("Not enough permissions to create a service.\n");
        return;
    }

    file = CreateFileA("C:\\windows\\syswow64\\winetestsvc.exe", GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, 0, NULL);
    CloseHandle(file);

    service = CreateServiceA(manager, "winetestsvc", "winetestsvc",
            SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE,
            SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
            "C:\\windows\\system32\\winetestsvc.exe service serve", NULL, NULL, NULL, NULL, NULL);
    ok(!!service, "Failed to create service, error %u.\n", GetLastError());
    ret = StartServiceA(service, 0, NULL);
    ok(!ret, "Expected failure.\n");
    todo_wine ok(GetLastError() == ERROR_BAD_EXE_FORMAT, "Got error %u.\n", GetLastError());

    ret = DeleteService(service);
    ok(ret, "Failed to delete service, error %u.\n", GetLastError());
    CloseServiceHandle(service);

    service = CreateServiceA(manager, "winetestsvc2", "winetestsvc2", SERVICE_START | DELETE,
            SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
            "C:\\windows\\system32\\winetestsvc.exe", NULL, NULL, NULL, NULL, NULL);
    ok(!!service, "Failed to create service, error %u.\n", GetLastError());
    ret = StartServiceA(service, 0, NULL);
    ok(!ret, "Expected failure.\n");
    todo_wine ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got error %u.\n", GetLastError());

    ret = DeleteService(service);
    ok(ret, "Failed to delete service, error %u.\n", GetLastError());
    CloseServiceHandle(service);

    ret = DeleteFileA("C:\\windows\\syswow64\\winetestsvc.exe");
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());

    file = CreateFileA("C:\\windows\\sysnative\\winetestsvc.exe", GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, 0, NULL);
    CloseHandle(file);

    service = CreateServiceA(manager, "winetestsvc3", "winetestsvc3", SERVICE_START | DELETE,
            SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
            "C:\\windows\\system32\\winetestsvc.exe service serve", NULL, NULL, NULL, NULL, NULL);
    ok(!!service, "Failed to create service, error %u.\n", GetLastError());
    ret = StartServiceA(service, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got error %u.\n", GetLastError());

    ret = DeleteService(service);
    ok(ret, "Failed to delete service, error %u.\n", GetLastError());
    CloseServiceHandle(service);

    service = CreateServiceA(manager, "winetestsvc4", "winetestsvc4", SERVICE_START | DELETE,
            SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
            "C:\\windows\\system32\\winetestsvc.exe", NULL, NULL, NULL, NULL, NULL);
    ok(!!service, "Failed to create service, error %u.\n", GetLastError());
    ret = StartServiceA(service, 0, NULL);
    ok(!ret, "Expected failure.\n");
    todo_wine ok(GetLastError() == ERROR_BAD_EXE_FORMAT, "Got error %u.\n", GetLastError());

    ret = DeleteService(service);
    ok(ret, "Failed to delete service, error %u.\n", GetLastError());
    CloseServiceHandle(service);

    ret = DeleteFileA("C:\\windows\\sysnative\\winetestsvc.exe");
    ok(ret, "Failed to delete file, error %u.\n", GetLastError());

    CloseServiceHandle(manager);
}

static void test_sequence(void)
{
    SC_HANDLE scm_handle, svc_handle;
    BOOL ret, is_nt4;
    QUERY_SERVICE_CONFIGA *config;
    DWORD given, needed;
    static const CHAR servicename [] = "Winetest";
    static const CHAR displayname [] = "Winetest dummy service";
    static const CHAR displayname2[] = "Winetest dummy service (2)";
    static const CHAR pathname    [] = "we_dont_care.exe";
    static const CHAR dependencies[] = "Master1\0Master2\0+MasterGroup1\0";
    static const CHAR password    [] = "";
    static const CHAR empty       [] = "";
    static const CHAR localsystem [] = "LocalSystem";

    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to get a handle to the manager\n");
        return;
    }
    else
        ok(scm_handle != NULL, "Could not get a handle to the manager: %d\n", GetLastError());

    if (!scm_handle) return;
    svc_handle = OpenServiceA(scm_handle, NULL, GENERIC_READ);
    is_nt4=(svc_handle == NULL && GetLastError() == ERROR_INVALID_PARAMETER);
    CloseServiceHandle(svc_handle);

    /* Create a dummy service */
    SetLastError(0xdeadbeef);
    svc_handle = CreateServiceA(scm_handle, servicename, displayname, GENERIC_ALL,
        SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS, SERVICE_DISABLED, SERVICE_ERROR_IGNORE,
        pathname, NULL, NULL, dependencies, NULL, password);

    if (!svc_handle && (GetLastError() == ERROR_SERVICE_EXISTS))
    {
        /* We try and open the service and do the rest of the tests. Some could
         * fail if the tests were changed between these runs.
         */
        trace("Deletion probably didn't work last time\n");
        SetLastError(0xdeadbeef);
        svc_handle = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
        if (!svc_handle && (GetLastError() == ERROR_ACCESS_DENIED))
        {
            skip("Not enough rights to open the service\n");
            CloseServiceHandle(scm_handle);        
            return;
        }
        ok(svc_handle != NULL, "Could not open the service : %d\n", GetLastError());
    }
    else if (!svc_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to create the service\n");
        CloseServiceHandle(scm_handle);        
        return;
    }
    else
    {
        ok(svc_handle != NULL, "Could not create the service : %d\n", GetLastError());
        if ((svc_handle != NULL) && (pGetSecurityInfo != NULL))
        {
            PSID sidOwner, sidGroup;
            PACL dacl, sacl;
            PSECURITY_DESCRIPTOR pSD;
            DWORD error, n1, n2;
            HRESULT retval;
            BOOL bret;

            /* Test using GetSecurityInfo to obtain security information */
            retval = pGetSecurityInfo(svc_handle, SE_SERVICE, DACL_SECURITY_INFORMATION, &sidOwner,
                                      &sidGroup, &dacl, &sacl, &pSD);
            LocalFree(pSD);
            ok(retval == ERROR_SUCCESS, "Expected GetSecurityInfo to succeed: result %d\n", retval);
            retval = pGetSecurityInfo(svc_handle, SE_SERVICE, DACL_SECURITY_INFORMATION, NULL,
                                      NULL, NULL, NULL, &pSD);
            LocalFree(pSD);
            ok(retval == ERROR_SUCCESS, "Expected GetSecurityInfo to succeed: result %d\n", retval);
            if (!is_nt4)
            {
                retval = pGetSecurityInfo(svc_handle, SE_SERVICE, DACL_SECURITY_INFORMATION, NULL,
                                          NULL, &dacl, NULL, &pSD);
                ok(retval == ERROR_SUCCESS, "Expected GetSecurityInfo to succeed: result %d\n", retval);
                LocalFree(pSD);
                SetLastError(0xdeadbeef);
                retval = pGetSecurityInfo(svc_handle, SE_SERVICE, DACL_SECURITY_INFORMATION, NULL,
                                          NULL, NULL, NULL, NULL);
                error = GetLastError();
                ok(retval == ERROR_INVALID_PARAMETER, "Expected GetSecurityInfo to fail: result %d\n", retval);
                ok(error == 0xdeadbeef, "Unexpected last error %d\n", error);
            }
            else
                win_skip("A NULL security descriptor in GetSecurityInfo results in an exception on NT4.\n");

            /* Test using QueryServiceObjectSecurity to obtain security information */
            SetLastError(0xdeadbeef);
            bret = pQueryServiceObjectSecurity(svc_handle, DACL_SECURITY_INFORMATION, NULL, 0, &n1);
            error = GetLastError();
            ok(!bret, "Expected QueryServiceObjectSecurity to fail: result %d\n", bret);
            ok(error == ERROR_INSUFFICIENT_BUFFER ||
               broken(error == ERROR_INVALID_ADDRESS) || broken(error == ERROR_INVALID_PARAMETER),
               "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", error);
            if (error != ERROR_INSUFFICIENT_BUFFER) n1 = 1024;
            pSD = LocalAlloc(0, n1);
            bret = pQueryServiceObjectSecurity(svc_handle, DACL_SECURITY_INFORMATION, pSD, n1, &n2);
            ok(bret, "Expected QueryServiceObjectSecurity to succeed: result %d\n", bret);
            LocalFree(pSD);
        }
    }

    if (!svc_handle) {
        CloseServiceHandle(scm_handle);
        return;
    }

    /* TODO:
     * Before we do a QueryServiceConfig we should check the registry. This will make sure
     * that the correct keys are used.
     */

    /* Request the size for the buffer */
    SetLastError(0xdeadbeef);
    ret = QueryServiceConfigA(svc_handle, NULL, 0, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    config = HeapAlloc(GetProcessHeap(), 0, needed);
    given = needed;
    SetLastError(0xdeadbeef);
    ret = QueryServiceConfigA(svc_handle, config, given, &needed);
    ok(ret, "Expected success, got error %u\n", GetLastError());

    ok(config->lpBinaryPathName && config->lpLoadOrderGroup && config->lpDependencies && config->lpServiceStartName &&
        config->lpDisplayName, "Expected all string struct members to be non-NULL\n");
    ok(config->dwServiceType == (SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS),
        "Expected SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS, got %d\n", config->dwServiceType);
    ok(config->dwStartType == SERVICE_DISABLED, "Expected SERVICE_DISABLED, got %d\n", config->dwStartType);
    ok(config->dwErrorControl == SERVICE_ERROR_IGNORE, "Expected SERVICE_ERROR_IGNORE, got %d\n", config->dwErrorControl);
    ok(!strcmp(config->lpBinaryPathName, pathname), "Expected '%s', got '%s'\n", pathname, config->lpBinaryPathName);
    ok(!strcmp(config->lpLoadOrderGroup, empty), "Expected an empty string, got '%s'\n", config->lpLoadOrderGroup);
    ok(config->dwTagId == 0, "Expected 0, got %d\n", config->dwTagId);
    /* TODO: Show the double 0 terminated string */
    todo_wine
    {
    ok(!memcmp(config->lpDependencies, dependencies, sizeof(dependencies)), "Wrong string\n");
    }
    ok(!strcmp(config->lpServiceStartName, localsystem), "Expected 'LocalSystem', got '%s'\n", config->lpServiceStartName);
    ok(!strcmp(config->lpDisplayName, displayname), "Expected '%s', got '%s'\n", displayname, config->lpDisplayName);

    ret = ChangeServiceConfigA(svc_handle, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_ERROR_NORMAL, NULL, "TestGroup2",
                               NULL, NULL, NULL, NULL, displayname2);
    ok(ret, "ChangeServiceConfig failed (err=%d)\n", GetLastError());

    QueryServiceConfigA(svc_handle, NULL, 0, &needed);
    config = HeapReAlloc(GetProcessHeap(), 0, config, needed);
    ok(QueryServiceConfigA(svc_handle, config, needed, &needed), "QueryServiceConfig failed\n");
    ok(config->lpBinaryPathName && config->lpLoadOrderGroup && config->lpDependencies && config->lpServiceStartName &&
        config->lpDisplayName, "Expected all string struct members to be non-NULL\n");
    ok(config->dwServiceType == (SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS),
        "Expected SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS, got %d\n", config->dwServiceType);
    ok(config->dwStartType == SERVICE_DISABLED, "Expected SERVICE_DISABLED, got %d\n", config->dwStartType);
    ok(config->dwErrorControl == SERVICE_ERROR_NORMAL, "Expected SERVICE_ERROR_NORMAL, got %d\n", config->dwErrorControl);
    ok(!strcmp(config->lpBinaryPathName, pathname), "Expected '%s', got '%s'\n", pathname, config->lpBinaryPathName);
    ok(!strcmp(config->lpLoadOrderGroup, "TestGroup2"), "Expected 'TestGroup2', got '%s'\n", config->lpLoadOrderGroup);
    ok(config->dwTagId == 0, "Expected 0, got %d\n", config->dwTagId);
    ok(!strcmp(config->lpServiceStartName, localsystem), "Expected 'LocalSystem', got '%s'\n", config->lpServiceStartName);
    ok(!strcmp(config->lpDisplayName, displayname2), "Expected '%s', got '%s'\n", displayname2, config->lpDisplayName);

    SetLastError(0xdeadbeef);
    ret = DeleteService(svc_handle);
    ok(ret, "Expected success, got error %u\n", GetLastError());
    CloseServiceHandle(svc_handle);

    /* Wait a while. The following test does a CreateService again */
    Sleep(1000);

    CloseServiceHandle(scm_handle);
    HeapFree(GetProcessHeap(), 0, config);
}

static void test_queryconfig2(void)
{
    SC_HANDLE scm_handle, svc_handle;
    BOOL ret;
    DWORD expected, needed;
    BYTE buffer[MAX_PATH];
    LPSERVICE_DESCRIPTIONA pConfig = (LPSERVICE_DESCRIPTIONA)buffer;
    LPSERVICE_DESCRIPTIONW pConfigW = (LPSERVICE_DESCRIPTIONW)buffer;
    SERVICE_PRESHUTDOWN_INFO preshutdown_info;
    static const CHAR servicename [] = "Winetest";
    static const CHAR displayname [] = "Winetest dummy service";
    static const CHAR pathname    [] = "we_dont_care.exe";
    static const CHAR dependencies[] = "Master1\0Master2\0+MasterGroup1\0";
    static const CHAR password    [] = "";
    static const CHAR description [] = "Description";
    static const CHAR description_empty[] = "";
    static const WCHAR descriptionW [] = {'D','e','s','c','r','i','p','t','i','o','n','W',0};
    static const WCHAR descriptionW_empty[] = {0};

    if(!pQueryServiceConfig2A)
    {
        win_skip("function QueryServiceConfig2A not present\n");
        return;
    }

    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    if (!scm_handle)
    {
	if(GetLastError() == ERROR_ACCESS_DENIED)
            skip("Not enough rights to get a handle to the manager\n");
        else
            ok(FALSE, "Could not get a handle to the manager: %d\n", GetLastError());
        return;
    }

    /* Create a dummy service */
    SetLastError(0xdeadbeef);
    svc_handle = CreateServiceA(scm_handle, servicename, displayname, GENERIC_ALL,
        SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS, SERVICE_DISABLED, SERVICE_ERROR_IGNORE,
        pathname, NULL, NULL, dependencies, NULL, password);

    if (!svc_handle)
    {
        if(GetLastError() == ERROR_SERVICE_EXISTS)
        {
            /* We try and open the service and do the rest of the tests. Some could
             * fail if the tests were changed between these runs.
             */
            trace("Deletion probably didn't work last time\n");
            SetLastError(0xdeadbeef);
            svc_handle = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
            if (!svc_handle)
            {
                if(GetLastError() == ERROR_ACCESS_DENIED)
                    skip("Not enough rights to open the service\n");
                else
                    ok(FALSE, "Could not open the service : %d\n", GetLastError());
                CloseServiceHandle(scm_handle);
                return;
            }
        }
        if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            skip("Not enough rights to create the service\n");
            CloseServiceHandle(scm_handle);
            return;
        }
        ok(svc_handle != NULL, "Could not create the service : %d\n", GetLastError());
	if (!svc_handle)
        {
            CloseServiceHandle(scm_handle);
            return;
        }
    }
    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle,0xfff0,buffer,sizeof(SERVICE_DESCRIPTIONA),&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INVALID_LEVEL == GetLastError(), "expected error ERROR_INVALID_LEVEL, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle,0xfff0,buffer,sizeof(SERVICE_DESCRIPTIONA),NULL);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INVALID_LEVEL == GetLastError(), "expected error ERROR_INVALID_LEVEL, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,sizeof(SERVICE_DESCRIPTIONA),NULL);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INVALID_ADDRESS == GetLastError(), "expected error ERROR_INVALID_ADDRESS, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,NULL,sizeof(SERVICE_DESCRIPTIONA),&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok((ERROR_INVALID_ADDRESS == GetLastError()) || (ERROR_INSUFFICIENT_BUFFER == GetLastError()),
       "expected error ERROR_INVALID_ADDRESS or ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,NULL,sizeof(SERVICE_DESCRIPTIONA),NULL);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INVALID_ADDRESS == GetLastError(), "expected error ERROR_INVALID_ADDRESS, got %d\n", GetLastError());

    needed = 0;
    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,sizeof(SERVICE_DESCRIPTIONA)-1,&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == sizeof(SERVICE_DESCRIPTIONA), "got %d\n", needed);

    needed = 0;
    pConfig->lpDescription = (LPSTR)0xdeadbeef;
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,sizeof(SERVICE_DESCRIPTIONA),&needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(needed == sizeof(SERVICE_DESCRIPTIONA), "got %d\n", needed);
    ok(!pConfig->lpDescription, "expected lpDescription to be NULL, got %p\n", pConfig->lpDescription);

    SetLastError(0xdeadbeef);
    needed = 0;
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,NULL,0,&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == sizeof(SERVICE_DESCRIPTIONA), "got %d\n", needed);

    if(!pChangeServiceConfig2A)
    {
        win_skip("function ChangeServiceConfig2A not present\n");
        goto cleanup;
    }

    pConfig->lpDescription = (LPSTR) description;
    ret = pChangeServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer);
    ok(ret, "ChangeServiceConfig2A failed\n");
    if (!ret) {
        goto cleanup;
    }

    SetLastError(0xdeadbeef);
    needed = 0;
    expected = sizeof(SERVICE_DESCRIPTIONA) + sizeof(description) * sizeof(WCHAR); /* !! */
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,sizeof(SERVICE_DESCRIPTIONA),&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == expected, "expected needed to be %d, got %d\n", expected, needed);

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,needed-1,&needed);
    ok(!ret, "expected QueryServiceConfig2A to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer,needed,&needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(pConfig->lpDescription && !strcmp(description,pConfig->lpDescription),
        "expected lpDescription to be %s, got %s\n",description ,pConfig->lpDescription);

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer, needed + 1,&needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(pConfig->lpDescription && !strcmp(description,pConfig->lpDescription),
        "expected lpDescription to be %s, got %s\n",description ,pConfig->lpDescription);

    if(!pQueryServiceConfig2W)
    {
        win_skip("function QueryServiceConfig2W not present\n");
        goto cleanup;
    }
    SetLastError(0xdeadbeef);
    needed = 0;
    expected = sizeof(SERVICE_DESCRIPTIONW) + sizeof(WCHAR) * sizeof(description);
    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION,NULL,0,&needed);
    ok(!ret, "expected QueryServiceConfig2W to fail\n");
    ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(), "expected error ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == expected, "expected needed to be %d, got %d\n", expected, needed);

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION,buffer, needed,&needed);
    ok(ret, "expected QueryServiceConfig2W to succeed\n");

    pConfig->lpDescription = (LPSTR)description;
    ret = pChangeServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION, &buffer);
    ok(ret, "expected ChangeServiceConfig2A to succeed\n");

    pConfig->lpDescription = NULL;
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION, buffer, sizeof(buffer), &needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(pConfig->lpDescription && !strcmp(description, pConfig->lpDescription),
        "expected lpDescription to be %s, got %s\n", description, pConfig->lpDescription);

    pConfig->lpDescription = NULL;
    ret = pChangeServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION, &buffer);
    ok(ret, "expected ChangeServiceConfig2A to succeed\n");

    pConfig->lpDescription = NULL;
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION, buffer, sizeof(buffer), &needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(pConfig->lpDescription && !strcmp(description, pConfig->lpDescription),
        "expected lpDescription to be %s, got %s\n", description, pConfig->lpDescription);

    pConfig->lpDescription = (LPSTR)description_empty;
    ret = pChangeServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION, &buffer);
    ok(ret, "expected ChangeServiceConfig2A to succeed\n");

    pConfig->lpDescription = (void*)0xdeadbeef;
    ret = pQueryServiceConfig2A(svc_handle, SERVICE_CONFIG_DESCRIPTION, buffer, sizeof(buffer), &needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(!pConfig->lpDescription,
        "expected lpDescription to be null, got %s\n", pConfig->lpDescription);

    pConfigW->lpDescription = (LPWSTR)descriptionW;
    ret = pChangeServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION, &buffer);
    ok(ret, "expected ChangeServiceConfig2W to succeed\n");

    pConfigW->lpDescription = NULL;
    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION, buffer, sizeof(buffer), &needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(pConfigW->lpDescription && !lstrcmpW(descriptionW, pConfigW->lpDescription),
        "expected lpDescription to be %s, got %s\n", wine_dbgstr_w(descriptionW), wine_dbgstr_w(pConfigW->lpDescription));

    pConfigW->lpDescription = NULL;
    ret = pChangeServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION, &buffer);
    ok(ret, "expected ChangeServiceConfig2W to succeed\n");

    pConfigW->lpDescription = NULL;
    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION, buffer, sizeof(buffer), &needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(pConfigW->lpDescription && !lstrcmpW(descriptionW, pConfigW->lpDescription),
        "expected lpDescription to be %s, got %s\n", wine_dbgstr_w(descriptionW), wine_dbgstr_w(pConfigW->lpDescription));

    pConfigW->lpDescription = (LPWSTR)descriptionW_empty;
    ret = pChangeServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION, &buffer);
    ok(ret, "expected ChangeServiceConfig2W to succeed\n");

    pConfigW->lpDescription = (void*)0xdeadbeef;
    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_DESCRIPTION, buffer, sizeof(buffer), &needed);
    ok(ret, "expected QueryServiceConfig2A to succeed\n");
    ok(!pConfigW->lpDescription,
        "expected lpDescription to be null, got %s\n", wine_dbgstr_w(pConfigW->lpDescription));

    SetLastError(0xdeadbeef);
    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_PRESHUTDOWN_INFO,
            (LPBYTE)&preshutdown_info, sizeof(preshutdown_info), &needed);
    if(!ret && GetLastError()==ERROR_INVALID_LEVEL)
    {
        /* Win2k3 and older */
        win_skip("SERVICE_CONFIG_PRESHUTDOWN_INFO not supported\n");
        goto cleanup;
    }
    ok(ret, "expected QueryServiceConfig2W to succeed (%d)\n", GetLastError());
    ok(needed == sizeof(preshutdown_info), "needed = %d\n", needed);
    ok(preshutdown_info.dwPreshutdownTimeout == 180000, "Default PreshutdownTimeout = %d\n",
            preshutdown_info.dwPreshutdownTimeout);

    SetLastError(0xdeadbeef);
    preshutdown_info.dwPreshutdownTimeout = -1;
    ret = pChangeServiceConfig2A(svc_handle, SERVICE_CONFIG_PRESHUTDOWN_INFO,
            (LPVOID)&preshutdown_info);
    ok(ret, "expected ChangeServiceConfig2A to succeed (%d)\n", GetLastError());

    ret = pQueryServiceConfig2W(svc_handle, SERVICE_CONFIG_PRESHUTDOWN_INFO,
            (LPBYTE)&preshutdown_info, sizeof(preshutdown_info), &needed);
    ok(ret, "expected QueryServiceConfig2W to succeed (%d)\n", GetLastError());
    ok(needed == sizeof(preshutdown_info), "needed = %d\n", needed);
    ok(preshutdown_info.dwPreshutdownTimeout == -1, "New PreshutdownTimeout = %d\n",
            preshutdown_info.dwPreshutdownTimeout);

cleanup:
    DeleteService(svc_handle);

    CloseServiceHandle(svc_handle);

    /* Wait a while. The following test does a CreateService again */
    Sleep(1000);

    CloseServiceHandle(scm_handle);
}

static DWORD try_start_stop(SC_HANDLE svc_handle, const char* name, DWORD is_nt4)
{
    BOOL ret;
    DWORD le1, le2;
    SERVICE_STATUS status;

    ret = StartServiceA(svc_handle, 0, NULL);
    le1 = GetLastError();
    ok(!ret, "%s: StartServiceA() should have failed\n", name);

    if (pQueryServiceStatusEx)
    {
        DWORD needed;
        SERVICE_STATUS_PROCESS statusproc;

        ret = pQueryServiceStatusEx(svc_handle, SC_STATUS_PROCESS_INFO, (BYTE*)&statusproc, sizeof(statusproc), &needed);
        ok(ret, "%s: QueryServiceStatusEx() failed le=%u\n", name, GetLastError());
        ok(statusproc.dwCurrentState == SERVICE_STOPPED, "%s: should be stopped state=%x\n", name, statusproc.dwCurrentState);
        ok(statusproc.dwProcessId == 0, "%s: ProcessId should be 0 instead of %x\n", name, statusproc.dwProcessId);
    }

    ret = StartServiceA(svc_handle, 0, NULL);
    le2 = GetLastError();
    ok(!ret, "%s: StartServiceA() should have failed\n", name);
    ok(le2 == le1, "%s: the second try should yield the same error: %u != %u\n", name, le1, le2);

    status.dwCurrentState = 0xdeadbeef;
    ret = ControlService(svc_handle, SERVICE_CONTROL_STOP, &status);
    le2 = GetLastError();
    ok(!ret, "%s: ControlService() should have failed\n", name);
    ok(le2 == ERROR_SERVICE_NOT_ACTIVE, "%s: %d != ERROR_SERVICE_NOT_ACTIVE\n", name, le2);
    ok(status.dwCurrentState == SERVICE_STOPPED ||
       broken(is_nt4), /* NT4 returns a random value */
       "%s: should be stopped state=%x\n", name, status.dwCurrentState);

    return le1;
}

#define PHASE_STOPPED 1
#define PHASE_RUNNING 2

struct notify_data {
    SERVICE_NOTIFYW notify;
    SC_HANDLE svc;
    BOOL was_called;
    DWORD phase;
};

static void CALLBACK notify_cb(void *user)
{
    struct notify_data *data = user;
    switch (data->phase)
    {
    case PHASE_STOPPED:
        ok(data->notify.dwNotificationStatus == ERROR_SUCCESS,
                "Got wrong notification status: %u\n", data->notify.dwNotificationStatus);
        ok(data->notify.ServiceStatus.dwCurrentState == SERVICE_STOPPED,
                "Got wrong service state: 0x%x\n", data->notify.ServiceStatus.dwCurrentState);
        ok(data->notify.dwNotificationTriggered == SERVICE_NOTIFY_STOPPED,
                "Got wrong notification triggered: 0x%x\n", data->notify.dwNotificationTriggered);
        break;

    case PHASE_RUNNING:
        ok(data->notify.dwNotificationStatus == ERROR_SUCCESS,
                "Got wrong notification status: %u\n", data->notify.dwNotificationStatus);
        ok(data->notify.ServiceStatus.dwCurrentState == SERVICE_RUNNING,
                "Got wrong service state: 0x%x\n", data->notify.ServiceStatus.dwCurrentState);
        ok(data->notify.dwNotificationTriggered == SERVICE_NOTIFY_RUNNING,
                "Got wrong notification triggered: 0x%x\n", data->notify.dwNotificationTriggered);
        break;
    }

    data->was_called = TRUE;
}

static void test_servicenotify(SC_HANDLE scm_handle, const char *servicename)
{
    DWORD dr, dr2;
    struct notify_data data;
    struct notify_data data2;
    BOOL br;
    SERVICE_STATUS status;
    HANDLE svc, svc2;

    if(!pNotifyServiceStatusChangeW){
        win_skip("No NotifyServiceStatusChangeW\n");
        return;
    }

    svc = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
    svc2 = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
    ok(svc != NULL && svc2 != NULL, "Failed to open service\n");
    if(!svc || !svc2)
        return;

    /* receive stopped notification, then start service */
    memset(&data.notify, 0, sizeof(data.notify));
    data.notify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    data.notify.pfnNotifyCallback = &notify_cb;
    data.notify.pContext = &data;
    data.svc = svc;
    data.phase = PHASE_STOPPED;
    data.was_called = FALSE;

    dr = pNotifyServiceStatusChangeW(svc, SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING, &data.notify);
    ok(dr == ERROR_SUCCESS, "NotifyServiceStatusChangeW failed: %u\n", dr);

    dr = SleepEx(100, TRUE);
    ok(dr == WAIT_IO_COMPLETION, "Got wrong SleepEx result: %u\n", dr);
    ok(data.was_called == TRUE, "APC wasn't called\n");

    br = StartServiceA(svc, 0, NULL);
    ok(br, "StartService failed: %u\n", GetLastError());

    /* receive running notification */
    data.phase = PHASE_RUNNING;
    data.was_called = FALSE;

    dr = pNotifyServiceStatusChangeW(svc, SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING, &data.notify);
    ok(dr == ERROR_SUCCESS, "NotifyServiceStatusChangeW failed: %u\n", dr);

    dr = SleepEx(100, TRUE);
    ok(dr == WAIT_IO_COMPLETION, "Got wrong SleepEx result: %u\n", dr);
    ok(data.was_called == TRUE, "APC wasn't called\n");

    /* cannot register two notifications on the same handle */
    data.phase = PHASE_STOPPED;
    data.was_called = FALSE;

    dr = pNotifyServiceStatusChangeW(svc, SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING, &data.notify);
    ok(dr == ERROR_SUCCESS, "NotifyServiceStatusChangeW failed: %u\n", dr);

    memset(&data2.notify, 0, sizeof(data2.notify));
    data2.notify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    data2.notify.pfnNotifyCallback = &notify_cb;
    data2.notify.pContext = &data2;

    dr = pNotifyServiceStatusChangeW(svc, SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING, &data2.notify);
    ok(dr == ERROR_SUCCESS || /* win8+ */
            dr == ERROR_ALREADY_REGISTERED, "NotifyServiceStatusChangeW gave wrong result: %u\n", dr);

    /* should receive no notification because status has not changed.
     * on win8+, SleepEx quits early but the callback is still not invoked. */
    dr2 = SleepEx(100, TRUE);
    ok((dr == ERROR_SUCCESS && dr2 == WAIT_IO_COMPLETION) || /* win8+ */
            (dr == ERROR_ALREADY_REGISTERED && dr2 == 0), "Got wrong SleepEx result: %u\n", dr);
    ok(data.was_called == FALSE, "APC should not have been called\n");

    memset(&data2.notify, 0, sizeof(data2.notify));
    data2.notify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    data2.notify.pfnNotifyCallback = &notify_cb;
    data2.notify.pContext = &data;
    data2.svc = svc2;
    data2.phase = PHASE_STOPPED;
    data2.was_called = FALSE;

    /* it's possible to have multiple notifications using different service handles */
    dr = pNotifyServiceStatusChangeW(svc2, SERVICE_NOTIFY_STOPPED, &data2.notify);
    ok(dr == ERROR_SUCCESS, "NotifyServiceStatusChangeW gave wrong result: %u\n", dr);

    /* stop service and receive notifiction */
    br = ControlService(svc, SERVICE_CONTROL_STOP, &status);
    ok(br, "ControlService failed: %u\n", GetLastError());

    dr = SleepEx(100, TRUE);
    dr2 = SleepEx(100, TRUE);
    ok(dr == WAIT_IO_COMPLETION || dr2 == WAIT_IO_COMPLETION, "Got wrong SleepEx result: %u\n", dr);
    ok(data.was_called == TRUE, "APC wasn't called\n");
    ok(data2.was_called == TRUE, "APC wasn't called\n");

    /* test cancelation: create notify on svc that will block until service
     * start; close svc; start service on svc2; verify that notification does
     * not happen */

    data.phase = PHASE_RUNNING;
    data.was_called = FALSE;
    dr = pNotifyServiceStatusChangeW(svc, SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_RUNNING, &data.notify);
    ok(dr == ERROR_SUCCESS, "NotifyServiceStatusChangeW failed: %u\n", dr);

    CloseServiceHandle(svc);

    br = StartServiceA(svc2, 0, NULL);
    ok(br, "StartService failed: %u\n", GetLastError());

    dr = SleepEx(100, TRUE);
    ok(dr == 0, "Got wrong SleepEx result: %u\n", dr);
    ok(data.was_called == FALSE, "APC should not have been called\n");

    br = ControlService(svc2, SERVICE_CONTROL_STOP, &status);
    ok(br, "ControlService failed: %u\n", GetLastError());

    CloseServiceHandle(svc2);
}

static void test_start_stop(void)
{
    BOOL ret;
    SC_HANDLE scm_handle, svc_handle;
    DWORD le, is_nt4;
    static const char servicename[] = "Winetest";
    char cmd[MAX_PATH+20];
    const char* displayname;

    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);
    if (!scm_handle)
    {
	if(GetLastError() == ERROR_ACCESS_DENIED)
            skip("Not enough rights to get a handle to the manager\n");
        else
            ok(FALSE, "Could not get a handle to the manager: %d\n", GetLastError());
        return;
    }

    /* Detect NT4 */
    svc_handle = OpenServiceA(scm_handle, NULL, GENERIC_READ);
    is_nt4=(svc_handle == NULL && GetLastError() == ERROR_INVALID_PARAMETER);

    /* Do some cleanup in case a previous run crashed */
    svc_handle = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
    if (svc_handle)
    {
        DeleteService(svc_handle);
        CloseServiceHandle(svc_handle);
    }

    /* Create a dummy disabled service */
    sprintf(cmd, "\"%s\" service exit", selfname);
    displayname = "Winetest Disabled Service";
    svc_handle = CreateServiceA(scm_handle, servicename, displayname,
        GENERIC_ALL, SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DISABLED, SERVICE_ERROR_IGNORE, cmd, NULL,
        NULL, NULL, NULL, NULL);
    if (!svc_handle)
    {
        if(GetLastError() == ERROR_ACCESS_DENIED)
            skip("Not enough rights to create the service\n");
        else
            ok(FALSE, "Could not create the service: %d\n", GetLastError());
        goto cleanup;
    }
    le = try_start_stop(svc_handle, displayname, is_nt4);
    ok(le == ERROR_SERVICE_DISABLED, "%d != ERROR_SERVICE_DISABLED\n", le);

    /* Then one with a bad path */
    displayname = "Winetest Bad Path";
    ret = ChangeServiceConfigA(svc_handle, SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, "c:\\no_such_file.exe", NULL, NULL, NULL, NULL, NULL, displayname);
    ok(ret, "ChangeServiceConfig() failed le=%u\n", GetLastError());
    try_start_stop(svc_handle, displayname, is_nt4);

    if (is_nt4)
    {
        /* NT4 does not detect when a service fails to start and uses an
         * insanely long timeout: 120s. So skip the rest of the tests.
         */
        win_skip("Skip some service start/stop tests on NT4\n");
        goto cleanup;
    }

    /* Again with a process that exits right away */
    displayname = "Winetest Exit Service";
    ret = ChangeServiceConfigA(svc_handle, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, cmd, NULL, NULL, NULL, NULL, NULL, displayname);
    ok(ret, "ChangeServiceConfig() failed le=%u\n", GetLastError());
    le = try_start_stop(svc_handle, displayname, is_nt4);
    ok(le == ERROR_SERVICE_REQUEST_TIMEOUT, "%d != ERROR_SERVICE_REQUEST_TIMEOUT\n", le);

    /* create a real service and test notifications */
    sprintf(cmd, "%s service serve", selfname);
    displayname = "Winetest Service";
    ret = ChangeServiceConfigA(svc_handle, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, cmd, NULL, NULL, NULL, NULL, NULL, displayname);
    ok(ret, "ChangeServiceConfig() failed le=%u\n", GetLastError());
    test_servicenotify(scm_handle, servicename);

cleanup:
    if (svc_handle)
    {
        DeleteService(svc_handle);
        CloseServiceHandle(svc_handle);
    }

    /* Wait a while. The following test does a CreateService again */
    Sleep(1000);

    CloseServiceHandle(scm_handle);
}

static void test_refcount(void)
{
    SC_HANDLE scm_handle, svc_handle1, svc_handle2, svc_handle3, svc_handle4, svc_handle5;
    static const CHAR servicename         [] = "Winetest";
    static const CHAR pathname            [] = "we_dont_care.exe";
    BOOL ret;

    /* Get a handle to the Service Control Manager */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);
    if (!scm_handle && (GetLastError() == ERROR_ACCESS_DENIED))
    {
        skip("Not enough rights to get a handle to the manager\n");
        return;
    }

    /* Create a service */
    svc_handle1 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL,
                                 SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(svc_handle1 != NULL, "Expected success, got error %u\n", GetLastError());

    /* Get a handle to this new service */
    svc_handle2 = OpenServiceA(scm_handle, servicename, GENERIC_READ);
    ok(svc_handle2 != NULL, "Expected success, got error %u\n", GetLastError());

    /* Get another handle to this new service */
    svc_handle3 = OpenServiceA(scm_handle, servicename, GENERIC_READ);
    ok(svc_handle3 != NULL, "Expected success, got error %u\n", GetLastError());

    /* Check if we can close the handle to the Service Control Manager */
    ret = CloseServiceHandle(scm_handle);
    ok(ret, "Expected success (err=%d)\n", GetLastError());

    /* Get a new handle to the Service Control Manager */
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);
    ok(scm_handle != NULL, "Expected success, got error %u\n", GetLastError());

    /* Get a handle to this new service */
    svc_handle4 = OpenServiceA(scm_handle, servicename, GENERIC_ALL);
    ok(svc_handle4 != NULL, "Expected success, got error %u\n", GetLastError());

    /* Delete the service */
    ret = DeleteService(svc_handle4);
    ok(ret, "Expected success (err=%d)\n", GetLastError());

    /* We cannot create the same service again as it's still marked as 'being deleted'.
     * The reason is that we still have 4 open handles to this service even though we
     * closed the handle to the Service Control Manager in between.
     */
    SetLastError(0xdeadbeef);
    svc_handle5 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL,
                                 SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(!svc_handle5, "Expected failure\n");
    ok(GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE,
       "Expected ERROR_SERVICE_MARKED_FOR_DELETE, got %d\n", GetLastError());

    /* Close all the handles to the service and try again */
    ret = CloseServiceHandle(svc_handle4);
    ok(ret, "Expected success (err=%d)\n", GetLastError());
    ret = CloseServiceHandle(svc_handle3);
    ok(ret, "Expected success (err=%d)\n", GetLastError());
    ret = CloseServiceHandle(svc_handle2);
    ok(ret, "Expected success (err=%d)\n", GetLastError());
    ret = CloseServiceHandle(svc_handle1);
    ok(ret, "Expected success (err=%d)\n", GetLastError());

    /* Wait a while. Doing a CreateService too soon will result again
     * in an ERROR_SERVICE_MARKED_FOR_DELETE error.
     */
    Sleep(1000);

    /* We succeed now as all handles are closed (tested this also with a long SLeep() */
    svc_handle5 = CreateServiceA(scm_handle, servicename, NULL, GENERIC_ALL,
                                 SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                 SERVICE_DISABLED, 0, pathname, NULL, NULL, NULL, NULL, NULL);
    ok(svc_handle5 != NULL, "Expected success, got error %u\n", GetLastError());

    /* Delete the service */
    ret = DeleteService(svc_handle5);
    ok(ret, "Expected success (err=%d)\n", GetLastError());

    /* Wait a while. Just in case one of the following tests does a CreateService again */
    Sleep(1000);

    CloseServiceHandle(svc_handle5);
    CloseServiceHandle(scm_handle);
}

static DWORD WINAPI ctrl_handler(DWORD ctl, DWORD type, void *data, void *user)
{
    HANDLE evt = user;

    switch(ctl){
    case SERVICE_CONTROL_STOP:
        SetEvent(evt);
        break;
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

static void WINAPI service_main(DWORD argc, char **argv)
{
    SERVICE_STATUS_HANDLE st_handle;
    SERVICE_STATUS st;
    HANDLE evt = CreateEventW(0, FALSE, FALSE, 0);

    st_handle = RegisterServiceCtrlHandlerExA("", &ctrl_handler, evt);

    st.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    st.dwServiceSpecificExitCode = 0;
    st.dwCurrentState = SERVICE_RUNNING;
    st.dwWin32ExitCode = NO_ERROR;
    st.dwWaitHint = 0;
    st.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    st.dwCheckPoint = 0;

    SetServiceStatus(st_handle, &st);

    WaitForSingleObject(evt, 5000);

    st.dwCurrentState = SERVICE_STOPPED;

    SetServiceStatus(st_handle, &st);
}

static void run_service(void)
{
    char empty[] = {0};
    SERVICE_TABLE_ENTRYA table[] = {
        {empty, &service_main },
        {0, 0}
    };

    StartServiceCtrlDispatcherA(table);
}

START_TEST(service)
{
    SC_HANDLE scm_handle;
    int myARGC;
    char** myARGV;

    myARGC = winetest_get_mainargs(&myARGV);
    GetFullPathNameA(myARGV[0], sizeof(selfname), selfname, NULL);
    if (myARGC >= 3)
    {
        if (strcmp(myARGV[2], "serve") == 0)
            run_service();
        return;
    }

    /* Bail out if we are on win98 */
    SetLastError(0xdeadbeef);
    scm_handle = OpenSCManagerA(NULL, NULL, GENERIC_ALL);

    if (!scm_handle && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("OpenSCManagerA is not implemented, we are most likely on win9x\n");
        return;
    }
    CloseServiceHandle(scm_handle);

    init_function_pointers();

    /* First some parameter checking */
    test_open_scm();
    test_open_svc();
    test_create_delete_svc();
    test_get_displayname();
    test_get_servicekeyname();
    test_query_svc();
    test_enum_svc();
    test_close();
    test_wow64();
    /* Test the creation, querying and deletion of a service */
    test_sequence();
    test_queryconfig2();
    test_start_stop();
    /* The main reason for this test is to check if any refcounting is used
     * and what the rules are
     */
    test_refcount();
}
