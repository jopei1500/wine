@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr)
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr)
@ stub AccessCheckByTypeAndAuditAlarmW
@ stub AccessCheckByTypeResultList
@ stub AccessCheckByTypeResultListAndAuditAlarmByHandleW
@ stub AccessCheckByTypeResultListAndAuditAlarmW
@ stdcall AcquireSRWLockExclusive(ptr) ntdll.RtlAcquireSRWLockExclusive
@ stdcall AcquireSRWLockShared(ptr) ntdll.RtlAcquireSRWLockShared
# @ stub AcquireStateLock
@ stdcall ActivateActCtx(ptr ptr)
@ stdcall AddAccessAllowedAce(ptr long long ptr)
@ stdcall AddAccessAllowedAceEx(ptr long long long ptr)
@ stdcall AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall AddAccessDeniedAce(ptr long long ptr)
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall AddAce(ptr long long ptr long)
@ stdcall AddAuditAccessAce(ptr long long ptr long long)
@ stdcall AddAuditAccessAceEx(ptr long long long ptr long long)
@ stdcall AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long)
@ stdcall AddDllDirectory(wstr)
@ stdcall AddMandatoryAce(ptr long long long ptr)
@ stdcall AddRefActCtx(ptr)
# @ stub AddResourceAttributeAce
# @ stub AddSIDToBoundaryDescriptor
# @ stub AddScopedPolicyIDAce
@ stdcall AddVectoredContinueHandler(long ptr) ntdll.RtlAddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) ntdll.RtlAddVectoredExceptionHandler
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr)
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr)
@ stdcall AllocConsole() kernel32.AllocConsole
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr)
@ stdcall AllocateLocallyUniqueId(ptr)
@ stdcall AllocateUserPhysicalPages(long ptr ptr)
@ stdcall AllocateUserPhysicalPagesNuma(long ptr ptr long)
# @ stub AppContainerDeriveSidFromMoniker
# @ stub AppContainerFreeMemory
# @ stub AppContainerLookupDisplayNameMrtReference
# @ stub AppContainerLookupMoniker
# @ stub AppContainerRegisterSid
# @ stub AppContainerUnregisterSid
# @ stub AppPolicyGetClrCompat
# @ stub AppPolicyGetCreateFileAccess
# @ stub AppPolicyGetLifecycleManagement
# @ stub AppPolicyGetMediaFoundationCodecLoading
@ stdcall AppPolicyGetProcessTerminationMethod(ptr ptr)
@ stdcall AppPolicyGetShowDeveloperDiagnostic(ptr ptr)
@ stdcall AppPolicyGetThreadInitializationType(ptr ptr)
@ stdcall AppPolicyGetWindowingModel(ptr ptr)
# @ stub AppXFreeMemory
# @ stub AppXGetApplicationData
# @ stub AppXGetDevelopmentMode
# @ stub AppXGetOSMaxVersionTested
# @ stub AppXGetOSMinVersion
# @ stub AppXGetPackageCapabilities
# @ stub AppXGetPackageSid
# @ stub AppXLookupDisplayName
# @ stub AppXLookupMoniker
# @ stub AppXPostSuccessExtension
# @ stub AppXPreCreationExtension
# @ stub AppXReleaseAppXContext
# @ stub AppXUpdatePackageCapabilities
# @ stub ApplicationUserModelIdFromProductId
@ stdcall AreAllAccessesGranted(long long)
@ stdcall AreAnyAccessesGranted(long long)
@ stdcall AreFileApisANSI()
# @ stub AreThereVisibleLogoffScriptsInternal
# @ stub AreThereVisibleShutdownScriptsInternal
@ stdcall AttachConsole(long)
@ stub BaseCheckAppcompatCache
# @ stub BaseCheckAppcompatCacheEx
@ stub BaseCleanupAppcompatCacheSupport
@ stub BaseDllFreeResourceId
@ stub BaseDllMapResourceIdW
@ stub BaseDumpAppcompatCache
@ stdcall BaseFlushAppcompatCache() kernel32.BaseFlushAppcompatCache
# @ stub BaseFormatObjectAttributes
# @ stub BaseFreeAppCompatDataForProcess
@ stdcall BaseGetNamedObjectDirectory(ptr)
@ stub BaseGetProcessDllPath
@ stub BaseGetProcessExePath
@ stub BaseInitAppcompatCacheSupport
@ stub BaseInvalidateDllSearchPathCache
@ stub BaseInvalidateProcessSearchPathCache
# @ stub BaseIsAppcompatInfrastructureDisabled
# @ stub BaseMarkFileForDelete
# @ stub BaseReadAppCompatDataForProcess
@ stub BaseReleaseProcessDllPath
@ stub BaseReleaseProcessExePath
@ stub BaseUpdateAppcompatCache
# @ stub BasepAdjustObjectAttributesForPrivateNamespace
# @ stub BasepCopyFileCallback
# @ stub BasepCopyFileExW
# @ stub BasepNotifyTrackingService
@ stdcall Beep(long long) kernel32.Beep
@ stub BemCopyReference
@ stub BemCreateContractFrom
@ stub BemCreateReference
@ stub BemFreeContract
@ stub BemFreeReference
# @ stub CLOSE_LOCAL_HANDLE_INTERNAL
@ stdcall CallNamedPipeW(wstr ptr long ptr long ptr long)
@ stdcall CallbackMayRunLong(ptr)
@ stdcall CancelIo(long)
@ stdcall CancelIoEx(long ptr)
@ stdcall CancelSynchronousIo(long)
@ stub CancelThreadpoolIo
@ stdcall CancelWaitableTimer(long)
# @ stub CeipIsOptedIn
@ stdcall ChangeTimerQueueTimer(ptr ptr long long)
@ stdcall CharLowerA(str)
@ stdcall CharLowerBuffA(str long)
@ stdcall CharLowerBuffW(wstr long)
@ stdcall CharLowerW(wstr)
@ stdcall CharNextA(str)
@ stdcall CharNextExA(long str long)
@ stdcall CharNextW(wstr)
@ stdcall CharPrevA(str str)
@ stdcall CharPrevExA(long str str long)
@ stdcall CharPrevW(wstr wstr)
@ stdcall CharUpperA(str)
@ stdcall CharUpperBuffA(str long)
@ stdcall CharUpperBuffW(wstr long)
@ stdcall CharUpperW(wstr)
# @ stub CheckAllowDecryptedRemoteDestinationPolicy
@ stub CheckGroupPolicyEnabled
# @ stub CheckIfStateChangeNotificationExists
@ stdcall CheckRemoteDebuggerPresent(long ptr)
# @ stub CheckTokenCapability
@ stdcall CheckTokenMembership(long ptr ptr)
# @ stub CheckTokenMembershipEx
@ stdcall ChrCmpIA(long long)
@ stdcall ChrCmpIW(long long)
@ stdcall ClearCommBreak(long)
@ stdcall ClearCommError(long ptr ptr)
# @ stub CloseGlobalizationUserSettingsKey
@ stdcall CloseHandle(long)
# @ stub ClosePackageInfo
# @ stub ClosePrivateNamespace
# @ stub CloseState
# @ stub CloseStateAtom
# @ stub CloseStateChangeNotification
# @ stub CloseStateContainer
# @ stub CloseStateLock
@ stdcall CloseThreadpool(ptr) ntdll.TpReleasePool
@ stdcall CloseThreadpoolCleanupGroup(ptr) ntdll.TpReleaseCleanupGroup
@ stdcall CloseThreadpoolCleanupGroupMembers(ptr long ptr) ntdll.TpReleaseCleanupGroupMembers
@ stub CloseThreadpoolIo
@ stdcall CloseThreadpoolTimer(ptr) ntdll.TpReleaseTimer
@ stdcall CloseThreadpoolWait(ptr) ntdll.TpReleaseWait
@ stdcall CloseThreadpoolWork(ptr) ntdll.TpReleaseWork
# @ stub CommitStateAtom
@ stdcall CompareFileTime(ptr ptr)
# @ stub CompareObjectHandles
@ stdcall CompareStringA(long long str long str long) kernel32.CompareStringA
@ stdcall CompareStringEx(wstr long wstr long wstr long ptr ptr long) kernel32.CompareStringEx
@ stdcall CompareStringOrdinal(wstr long wstr long long)
@ stdcall CompareStringW(long long wstr long wstr long) kernel32.CompareStringW
@ stdcall ConnectNamedPipe(long ptr)
@ stdcall ContinueDebugEvent(long long long)
@ stdcall ConvertDefaultLocale(long) kernel32.ConvertDefaultLocale
@ stdcall ConvertFiberToThread()
@ stdcall ConvertThreadToFiber(ptr)
@ stdcall ConvertThreadToFiberEx(ptr long)
@ stdcall ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr)
# @ stub CopyContext
# @ stub CopyFile2
@ stdcall CopyFileExW(wstr wstr ptr ptr ptr long) kernel32.CopyFileExW
@ stdcall CopyFileW(wstr wstr long) kernel32.CopyFileW
# @ stub -arch=x86_64 CopyMemoryNonTemporal
@ stdcall CopySid(long ptr ptr)
# @ stub CouldMultiUserAppsBehaviorBePossibleForPackage
@ stdcall CreateActCtxW(ptr)
# @ stub CreateAppContainerToken
# @ stub CreateBoundaryDescriptorW
@ stdcall CreateConsoleScreenBuffer(long long ptr long ptr)
@ stdcall CreateDirectoryA(str ptr)
@ stdcall CreateDirectoryExW(wstr wstr ptr)
@ stdcall CreateDirectoryW(wstr ptr)
# @ stub CreateEnclave
@ stdcall CreateEventA(ptr long long str)
@ stdcall CreateEventExA(ptr str long long)
@ stdcall CreateEventExW(ptr wstr long long)
@ stdcall CreateEventW(ptr long long wstr)
@ stdcall CreateFiber(long ptr ptr)
@ stdcall CreateFiberEx(long long long ptr ptr)
@ stdcall CreateFile2(wstr long long long ptr)
@ stdcall CreateFileA(str long long ptr long long long)
# @ stub CreateFileMappingFromApp
@ stdcall CreateFileMappingNumaW(long ptr long long long wstr long)
@ stdcall CreateFileMappingW(long ptr long long long wstr)
@ stdcall CreateFileW(wstr long long ptr long long long)
@ stdcall CreateHardLinkA(str str ptr) kernel32.CreateHardLinkA
@ stdcall CreateHardLinkW(wstr wstr ptr) kernel32.CreateHardLinkW
@ stdcall CreateIoCompletionPort(long long long long)
@ stdcall CreateMemoryResourceNotification(long)
@ stdcall CreateMutexA(ptr long str)
@ stdcall CreateMutexExA(ptr str long long)
@ stdcall CreateMutexExW(ptr wstr long long)
@ stdcall CreateMutexW(ptr long wstr)
@ stdcall CreateNamedPipeW(wstr long long long long long long ptr)
@ stdcall CreatePipe(ptr ptr ptr long)
# @ stub CreatePrivateNamespaceW
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr)
@ stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long long ptr)
@ stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr)
@ stdcall CreateProcessA(str str ptr ptr long long ptr str ptr ptr) kernel32.CreateProcessA
@ stdcall CreateProcessAsUserA(long str str ptr ptr long long ptr str ptr ptr) kernel32.CreateProcessAsUserA
@ stdcall CreateProcessAsUserW(long wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32.CreateProcessAsUserW
@ stdcall CreateProcessInternalA(long str str ptr ptr long long ptr str ptr ptr ptr) kernel32.CreateProcessInternalA
@ stdcall CreateProcessInternalW(long wstr wstr ptr ptr long long ptr wstr ptr ptr ptr) kernel32.CreateProcessInternalW
@ stdcall CreateProcessW(wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32.CreateProcessW
@ stdcall CreateRemoteThread(long ptr long ptr long long ptr)
@ stdcall CreateRemoteThreadEx(long ptr long ptr ptr long ptr ptr)
@ stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr)
@ stdcall CreateSemaphoreExW(ptr long long wstr long long)
@ stdcall CreateSemaphoreW(ptr long long wstr)
# @ stub CreateStateAtom
# @ stub CreateStateChangeNotification
# @ stub CreateStateContainer
# @ stub CreateStateLock
# @ stub CreateStateSubcontainer
@ stdcall CreateSymbolicLinkW(wstr wstr long) kernel32.CreateSymbolicLinkW
@ stdcall CreateThread(ptr long ptr long long ptr)
@ stdcall CreateThreadpool(ptr)
@ stdcall CreateThreadpoolCleanupGroup()
@ stdcall CreateThreadpoolIo(ptr ptr ptr ptr)
@ stdcall CreateThreadpoolTimer(ptr ptr ptr)
@ stdcall CreateThreadpoolWait(ptr ptr ptr)
@ stdcall CreateThreadpoolWork(ptr ptr ptr)
@ stdcall CreateTimerQueue()
@ stdcall CreateTimerQueueTimer(ptr long ptr ptr long long long)
@ stdcall CreateWaitableTimerExW(ptr wstr long long)
@ stdcall CreateWaitableTimerW(ptr long wstr)
@ stdcall CreateWellKnownSid(long ptr ptr ptr)
# @ stub CtrlRoutine
# @ stub CveEventWrite
@ stdcall DeactivateActCtx(long long)
@ stdcall DebugActiveProcess(long)
@ stdcall DebugActiveProcessStop(long)
@ stdcall DebugBreak()
@ stdcall DecodePointer(ptr) ntdll.RtlDecodePointer
# @ stub DecodeRemotePointer
@ stdcall DecodeSystemPointer(ptr) ntdll.RtlDecodeSystemPointer
@ stdcall DefineDosDeviceW(long wstr wstr) kernel32.DefineDosDeviceW
@ stdcall DelayLoadFailureHook(str str)
# @ stub DelayLoadFailureHookLookup
@ stdcall DeleteAce(ptr long)
# @ stub DeleteBoundaryDescriptor
@ stdcall DeleteCriticalSection(ptr) ntdll.RtlDeleteCriticalSection
@ stdcall DeleteFiber(ptr)
@ stdcall DeleteFileA(str)
@ stdcall DeleteFileW(wstr)
@ stdcall DeleteProcThreadAttributeList(ptr)
# @ stub DeleteStateAtomValue
# @ stub DeleteStateContainer
# @ stub DeleteStateContainerValue
# @ stub DeleteSynchronizationBarrier
@ stdcall DeleteTimerQueueEx(long long)
@ stdcall DeleteTimerQueueTimer(long long long)
@ stdcall DeleteVolumeMountPointW(wstr) kernel32.DeleteVolumeMountPointW
@ stdcall DestroyPrivateObjectSecurity(ptr)
@ stdcall DeviceIoControl(long long ptr long ptr long ptr ptr)
@ stdcall DisablePredefinedHandleTableInternal(long)
@ stdcall DisableThreadLibraryCalls(long)
@ stdcall DisassociateCurrentThreadFromCallback(ptr) ntdll.TpDisassociateCallback
# @ stub DiscardVirtualMemory
@ stdcall DisconnectNamedPipe(long)
# @ stub DnsHostnameToComputerNameExW
# @ stub DsBindWithSpnExW
# @ stub DsCrackNamesW
# @ stub DsFreeDomainControllerInfoW
# @ stub DsFreeNameResultW
# @ stub DsFreeNgcKey
# @ stub DsFreePasswordCredentials
# @ stub DsGetDomainControllerInfoW
# @ stub DsMakePasswordCredentialsW
# @ stub DsReadNgcKeyW
# @ stub DsUnBindW
# @ stub DsWriteNgcKeyW
@ stdcall DuplicateHandle(long long long ptr long long long)
# @ stub DuplicateStateContainerHandle
@ stdcall DuplicateToken(long long ptr)
@ stdcall DuplicateTokenEx(long long ptr long long ptr)
# @ stub EmptyWorkingSet
@ stdcall EncodePointer(ptr) ntdll.RtlEncodePointer
# @ stub EncodeRemotePointer
@ stdcall EncodeSystemPointer(ptr) ntdll.RtlEncodeSystemPointer
# @ stub EnterCriticalPolicySectionInternal
@ stdcall EnterCriticalSection(ptr) ntdll.RtlEnterCriticalSection
# @ stub EnterSynchronizationBarrier
@ stdcall EnumCalendarInfoExEx(ptr wstr long wstr long long) kernel32.EnumCalendarInfoExEx
@ stdcall EnumCalendarInfoExW(ptr long long long) kernel32.EnumCalendarInfoExW
@ stdcall EnumCalendarInfoW(ptr long long long) kernel32.EnumCalendarInfoW
@ stdcall EnumDateFormatsExEx(ptr wstr long long) kernel32.EnumDateFormatsExEx
@ stdcall EnumDateFormatsExW(ptr long long) kernel32.EnumDateFormatsExW
@ stdcall EnumDateFormatsW(ptr long long) kernel32.EnumDateFormatsW
# @ stub EnumDeviceDrivers
@ stdcall EnumDynamicTimeZoneInformation(long ptr)
@ stdcall EnumLanguageGroupLocalesW(ptr long long ptr)
# @ stub EnumPageFilesA
# @ stub EnumPageFilesW
# @ stub EnumProcessModules
# @ stub EnumProcessModulesEx
# @ stub EnumProcesses
@ stdcall EnumResourceLanguagesExA(long str str ptr long long long)
@ stdcall EnumResourceLanguagesExW(long wstr wstr ptr long long long)
@ stdcall EnumResourceNamesExA(long str ptr long long long)
@ stdcall EnumResourceNamesExW(long wstr ptr long long long)
@ stdcall EnumResourceNamesW(long wstr ptr long)
@ stdcall EnumResourceTypesExA(long ptr long long long)
@ stdcall EnumResourceTypesExW(long ptr long long long)
@ stdcall EnumSystemCodePagesW(ptr long)
@ stdcall EnumSystemFirmwareTables(long ptr long) kernel32.EnumSystemFirmwareTables
@ stdcall EnumSystemGeoID(long long ptr) kernel32.EnumSystemGeoID
@ stdcall EnumSystemLanguageGroupsW(ptr long ptr)
@ stdcall EnumSystemLocalesA(ptr long)
@ stdcall EnumSystemLocalesEx(ptr long long ptr)
@ stdcall EnumSystemLocalesW(ptr long)
@ stdcall EnumTimeFormatsEx(ptr wstr long long) kernel32.EnumTimeFormatsEx
@ stdcall EnumTimeFormatsW(ptr long long) kernel32.EnumTimeFormatsW
@ stdcall EnumUILanguagesW(ptr long long)
# @ stub EnumerateStateAtomValues
# @ stub EnumerateStateContainerItems
@ stub EqualDomainSid
@ stdcall EqualPrefixSid(ptr ptr)
@ stdcall EqualSid(ptr ptr)
@ stdcall EscapeCommFunction(long long)
@ stdcall EventActivityIdControl(long ptr) ntdll.EtwEventActivityIdControl
@ stdcall EventEnabled(int64 ptr) ntdll.EtwEventEnabled
@ stdcall EventProviderEnabled(int64 long int64) ntdll.EtwEventProviderEnabled
@ stdcall EventRegister(ptr ptr ptr ptr) ntdll.EtwEventRegister
@ stdcall EventSetInformation(int64 long ptr long) ntdll.EtwEventSetInformation
@ stdcall EventUnregister(int64) ntdll.EtwEventUnregister
@ stdcall EventWrite(int64 ptr long ptr) ntdll.EtwEventWrite
# @ stub EventWriteEx
@ stdcall EventWriteString(int64 long int64 ptr) ntdll.EtwEventWriteString
@ stdcall EventWriteTransfer(int64 ptr ptr ptr long ptr) ntdll.EtwEventWriteTransfer
@ stdcall ExitProcess(long) ntdll.RtlExitUserProcess
@ stdcall ExitThread(long) ntdll.RtlExitUserThread
@ stdcall ExpandEnvironmentStringsA(str ptr long)
@ stdcall ExpandEnvironmentStringsW(wstr ptr long)
@ stdcall FatalAppExitA(long str)
@ stdcall FatalAppExitW(long wstr)
@ stdcall FileTimeToLocalFileTime(ptr ptr)
@ stdcall FileTimeToSystemTime(ptr ptr)
@ stdcall FillConsoleOutputAttribute(long long long long ptr)
@ stdcall FillConsoleOutputCharacterA(long long long long ptr)
@ stdcall FillConsoleOutputCharacterW(long long long long ptr)
@ stdcall FindActCtxSectionGuid(long ptr long ptr ptr)
@ stdcall FindActCtxSectionStringW(long ptr long wstr ptr)
@ stdcall FindClose(long) kernel32.FindClose
@ stdcall FindCloseChangeNotification(long)
@ stdcall FindFirstChangeNotificationA(str long long)
@ stdcall FindFirstChangeNotificationW(wstr long long)
@ stdcall FindFirstFileA(str ptr) kernel32.FindFirstFileA
@ stdcall FindFirstFileExA(str long ptr long ptr long) kernel32.FindFirstFileExA
@ stdcall FindFirstFileExW(wstr long ptr long ptr long) kernel32.FindFirstFileExW
# @ stub FindFirstFileNameW
@ stdcall FindFirstFileW(wstr ptr) kernel32.FindFirstFileW
@ stdcall FindFirstFreeAce(ptr ptr)
@ stdcall FindFirstStreamW(wstr long ptr long) kernel32.FindFirstStreamW
@ stdcall FindFirstVolumeW(ptr long) kernel32.FindFirstVolumeW
@ stub FindNLSString
@ stdcall FindNLSStringEx(wstr long wstr long wstr long ptr ptr ptr long) kernel32.FindNLSStringEx
@ stdcall FindNextChangeNotification(long)
@ stdcall FindNextFileA(long ptr) kernel32.FindNextFileA
# @ stub FindNextFileNameW
@ stdcall FindNextFileW(long ptr) kernel32.FindNextFileW
@ stdcall FindNextStreamW(long ptr) kernel32.FindNextStreamW
@ stdcall FindNextVolumeW(long ptr long) kernel32.FindNextVolumeW
# @ stub FindPackagesByPackageFamily
@ stdcall FindResourceExW(long wstr wstr long)
@ stdcall FindResourceW(long wstr wstr)
@ stdcall FindStringOrdinal(long wstr long wstr long long)
@ stdcall FindVolumeClose(ptr) kernel32.FindVolumeClose
@ stdcall FlsAlloc(ptr)
@ stdcall FlsFree(long)
@ stdcall FlsGetValue(long)
@ stdcall FlsSetValue(long ptr)
@ stdcall FlushConsoleInputBuffer(long) kernel32.FlushConsoleInputBuffer
@ stdcall FlushFileBuffers(long)
@ stdcall FlushInstructionCache(long long long)
@ stdcall FlushProcessWriteBuffers() kernel32.FlushProcessWriteBuffers
@ stdcall FlushViewOfFile(ptr long)
@ stdcall FoldStringW(long wstr long ptr long) kernel32.FoldStringW
# @ stub ForceSyncFgPolicyInternal
# @ stub FormatApplicationUserModelId
@ stdcall FormatMessageA(long ptr long long ptr long ptr) kernel32.FormatMessageA
@ stdcall FormatMessageW(long ptr long long ptr long ptr) kernel32.FormatMessageW
@ stdcall FreeConsole()
@ stdcall FreeEnvironmentStringsA(ptr) FreeEnvironmentStringsW
@ stdcall FreeEnvironmentStringsW(ptr)
# @ stub FreeGPOListInternalA
# @ stub FreeGPOListInternalW
@ stdcall FreeLibrary(long)
@ stdcall FreeLibraryAndExitThread(long long)
@ stdcall FreeLibraryWhenCallbackReturns(ptr ptr) ntdll.TpCallbackUnloadDllOnCompletion
@ stdcall FreeResource(long)
@ stdcall FreeSid(ptr)
@ stdcall FreeUserPhysicalPages(long ptr ptr)
@ stdcall GenerateConsoleCtrlEvent(long long)
# @ stub GenerateGPNotificationInternal
@ stdcall GetACP() kernel32.GetACP
@ stdcall GetAcceptLanguagesA(ptr ptr)
@ stdcall GetAcceptLanguagesW(ptr ptr)
@ stdcall GetAce(ptr long ptr)
@ stdcall GetAclInformation(ptr ptr long long)
# @ stub GetAdjustObjectAttributesForPrivateNamespaceRoutine
# @ stub GetAlternatePackageRoots
# @ stub GetAppContainerAce
# @ stub GetAppContainerNamedObjectPath
# @ stub GetAppDataFolder
# @ stub GetAppModelVersion
# @ stub GetApplicationRecoveryCallback
@ stdcall GetApplicationRestartSettings(long ptr ptr ptr) kernel32.GetApplicationRestartSettings
# @ stub GetApplicationUserModelId
# @ stub GetApplicationUserModelIdFromToken
# @ stub GetAppliedGPOListInternalA
# @ stub GetAppliedGPOListInternalW
@ stub GetCPFileNameFromRegistry
@ stub GetCPHashNode
@ stdcall GetCPInfo(long ptr) kernel32.GetCPInfo
@ stdcall GetCPInfoExW(long long ptr) kernel32.GetCPInfoExW
# @ stub GetCachedSigningLevel
@ stub GetCalendar
@ stdcall GetCalendarInfoEx(wstr long ptr long ptr long ptr) kernel32.GetCalendarInfoEx
@ stdcall GetCalendarInfoW(long long long ptr long ptr) kernel32.GetCalendarInfoW
@ stdcall GetCommConfig(long ptr ptr)
@ stdcall GetCommMask(long ptr)
@ stdcall GetCommModemStatus(long ptr)
@ stdcall GetCommProperties(long ptr)
@ stdcall GetCommState(long ptr)
@ stdcall GetCommTimeouts(long ptr)
@ stdcall GetCommandLineA()
@ stdcall GetCommandLineW()
@ stdcall GetCompressedFileSizeA(long ptr)
@ stdcall GetCompressedFileSizeW(long ptr)
@ stdcall GetComputerNameExA(long ptr ptr) kernel32.GetComputerNameExA
@ stdcall GetComputerNameExW(long ptr ptr) kernel32.GetComputerNameExW
@ stdcall GetConsoleCP()
@ stdcall GetConsoleCursorInfo(long ptr)
@ stdcall GetConsoleInputExeNameA(long ptr)
@ stdcall GetConsoleInputExeNameW(long ptr)
@ stdcall GetConsoleMode(long ptr)
@ stdcall GetConsoleOutputCP()
@ stdcall GetConsoleScreenBufferInfo(long ptr)
@ stdcall GetConsoleScreenBufferInfoEx(long ptr)
@ stdcall GetConsoleTitleW(ptr long)
@ stdcall GetCurrencyFormatEx(wstr long wstr ptr ptr long) kernel32.GetCurrencyFormatEx
@ stdcall GetCurrencyFormatW(long long wstr ptr ptr long) kernel32.GetCurrencyFormatW
@ stdcall GetCurrentActCtx(ptr)
# @ stub GetCurrentApplicationUserModelId
@ stdcall GetCurrentDirectoryA(long ptr)
@ stdcall GetCurrentDirectoryW(long ptr)
# @ stub GetCurrentPackageApplicationContext
# @ stub GetCurrentPackageApplicationResourcesContext
# @ stub GetCurrentPackageContext
@ stdcall GetCurrentPackageFamilyName(ptr ptr) kernel32.GetCurrentPackageFamilyName
@ stdcall GetCurrentPackageFullName(ptr ptr) kernel32.GetCurrentPackageFullName
@ stdcall GetCurrentPackageId(ptr ptr) kernel32.GetCurrentPackageId
# @ stub GetCurrentPackageInfo
# @ stub GetCurrentPackagePath
# @ stub GetCurrentPackageResourcesContext
# @ stub GetCurrentPackageSecurityContext
@ stdcall -norelay GetCurrentProcess() kernelbase_GetCurrentProcess
@ stdcall -norelay GetCurrentProcessId() kernelbase_GetCurrentProcessId
@ stdcall GetCurrentProcessorNumber() ntdll.NtGetCurrentProcessorNumber
@ stdcall GetCurrentProcessorNumberEx(ptr) ntdll.RtlGetCurrentProcessorNumberEx
# @ stub GetCurrentTargetPlatformContext
@ stdcall -norelay GetCurrentThread() kernelbase_GetCurrentThread
@ stdcall -norelay GetCurrentThreadId() kernelbase_GetCurrentThreadId
@ stdcall GetCurrentThreadStackLimits(ptr ptr)
@ stdcall GetDateFormatA(long long ptr str ptr long) kernel32.GetDateFormatA
@ stdcall GetDateFormatEx(wstr long ptr wstr ptr long wstr) kernel32.GetDateFormatEx
@ stdcall GetDateFormatW(long long ptr wstr ptr long) kernel32.GetDateFormatW
# @ stub GetDeviceDriverBaseNameA
# @ stub GetDeviceDriverBaseNameW
# @ stub GetDeviceDriverFileNameA
# @ stub GetDeviceDriverFileNameW
@ stdcall GetDiskFreeSpaceA(str ptr ptr ptr ptr) kernel32.GetDiskFreeSpaceA
@ stdcall GetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
@ stdcall GetDiskFreeSpaceW(wstr ptr ptr ptr ptr) kernel32.GetDiskFreeSpaceW
@ stdcall GetDriveTypeA(str) kernel32.GetDriveTypeA
@ stdcall GetDriveTypeW(wstr) kernel32.GetDriveTypeW
# @ stub GetDurationFormatEx
@ stdcall GetDynamicTimeZoneInformation(ptr) kernel32.GetDynamicTimeZoneInformation
@ stdcall GetDynamicTimeZoneInformationEffectiveYears(ptr ptr ptr) kernel32.GetDynamicTimeZoneInformationEffectiveYears
# @ stub GetEffectivePackageStatusForUser
# @ stub GetEightBitStringToUnicodeSizeRoutine
# @ stub GetEightBitStringToUnicodeStringRoutine
@ stdcall -ret64 -arch=i386,x86_64 GetEnabledXStateFeatures() kernel32.GetEnabledXStateFeatures
@ stdcall GetEnvironmentStrings() GetEnvironmentStringsA
@ stdcall GetEnvironmentStringsA()
@ stdcall GetEnvironmentStringsW()
@ stdcall GetEnvironmentVariableA(str ptr long)
@ stdcall GetEnvironmentVariableW(wstr ptr long)
@ stub GetEraNameCountedString
@ stdcall GetErrorMode()
@ stdcall GetExitCodeProcess(long ptr)
@ stdcall GetExitCodeThread(long ptr)
@ stub GetFallbackDisplayName
@ stdcall GetFileAttributesA(str)
@ stdcall GetFileAttributesExA(str long ptr)
@ stdcall GetFileAttributesExW(wstr long ptr)
@ stdcall GetFileAttributesW(wstr)
@ stdcall GetFileInformationByHandle(long ptr)
@ stdcall GetFileInformationByHandleEx(long long ptr long)
@ stdcall GetFileMUIInfo(long wstr ptr ptr) kernel32.GetFileMUIInfo
@ stdcall GetFileMUIPath(long wstr wstr ptr ptr ptr ptr) kernel32.GetFileMUIPath
@ stdcall GetFileSecurityW(wstr long ptr long ptr)
@ stdcall GetFileSize(long ptr)
@ stdcall GetFileSizeEx(long ptr)
@ stdcall GetFileTime(long ptr ptr ptr)
@ stdcall GetFileType(long)
@ stdcall GetFileVersionInfoA(str long long ptr)
# @ stub GetFileVersionInfoByHandle
@ stdcall GetFileVersionInfoExA(long str long long ptr)
@ stdcall GetFileVersionInfoExW(long wstr long long ptr)
@ stdcall GetFileVersionInfoSizeA(str ptr)
@ stdcall GetFileVersionInfoSizeExA(long str ptr)
@ stdcall GetFileVersionInfoSizeExW(long wstr ptr)
@ stdcall GetFileVersionInfoSizeW(wstr ptr)
@ stdcall GetFileVersionInfoW(wstr long long ptr)
@ stdcall GetFinalPathNameByHandleA(long ptr long long) kernel32.GetFinalPathNameByHandleA
@ stdcall GetFinalPathNameByHandleW(long ptr long long) kernel32.GetFinalPathNameByHandleW
@ stdcall GetFullPathNameA(str long ptr ptr)
@ stdcall GetFullPathNameW(wstr long ptr ptr)
# @ stub GetGPOListInternalA
# @ stub GetGPOListInternalW
@ stdcall GetGeoInfoW(long long ptr long long) kernel32.GetGeoInfoW
@ stdcall GetHandleInformation(long ptr)
# @ stub GetHivePath
# @ stub GetIntegratedDisplaySize
# @ stub GetIsEdpEnabled
@ stdcall GetKernelObjectSecurity(long long ptr long ptr)
@ stdcall GetLargePageMinimum() kernel32.GetLargePageMinimum
@ stdcall GetLargestConsoleWindowSize(long)
@ stdcall GetLastError() kernelbase_GetLastError
@ stdcall GetLengthSid(ptr)
@ stdcall GetLocalTime(ptr)
@ stdcall GetLocaleInfoA(long long ptr long) kernel32.GetLocaleInfoA
@ stdcall GetLocaleInfoEx(wstr long ptr long) kernel32.GetLocaleInfoEx
@ stub GetLocaleInfoHelper
@ stdcall GetLocaleInfoW(long long ptr long) kernel32.GetLocaleInfoW
@ stdcall GetLogicalDriveStringsW(long ptr) kernel32.GetLogicalDriveStringsW
@ stdcall GetLogicalDrives() kernel32.GetLogicalDrives
@ stdcall GetLogicalProcessorInformation(ptr ptr)
@ stdcall GetLogicalProcessorInformationEx(long ptr ptr)
@ stdcall GetLongPathNameA(str long long)
@ stdcall GetLongPathNameW(wstr long long)
# @ stub GetMappedFileNameA
# @ stub GetMappedFileNameW
# @ stub GetMemoryErrorHandlingCapabilities
# @ stub GetModuleBaseNameA
# @ stub GetModuleBaseNameW
@ stdcall GetModuleFileNameA(long ptr long)
# @ stub GetModuleFileNameExA
# @ stub GetModuleFileNameExW
@ stdcall GetModuleFileNameW(long ptr long)
@ stdcall GetModuleHandleA(str)
@ stdcall GetModuleHandleExA(long ptr ptr)
@ stdcall GetModuleHandleExW(long ptr ptr)
@ stdcall GetModuleHandleW(wstr)
# @ stub GetModuleInformation
@ stub GetNLSVersion
@ stub GetNLSVersionEx
@ stub GetNamedLocaleHashNode
@ stub GetNamedPipeAttribute
@ stub GetNamedPipeClientComputerNameW
@ stdcall GetNamedPipeHandleStateW(long ptr ptr ptr ptr wstr long) kernel32.GetNamedPipeHandleStateW
@ stdcall GetNamedPipeInfo(long ptr ptr ptr ptr)
@ stdcall GetNativeSystemInfo(ptr) kernel32.GetNativeSystemInfo
# @ stub GetNextFgPolicyRefreshInfoInternal
@ stdcall GetNumaHighestNodeNumber(ptr)
@ stdcall GetNumaNodeProcessorMaskEx(long ptr)
@ stdcall GetNumaProximityNodeEx(long ptr)
@ stdcall GetNumberFormatEx(wstr long wstr ptr ptr long) kernel32.GetNumberFormatEx
@ stdcall GetNumberFormatW(long long wstr ptr ptr long) kernel32.GetNumberFormatW
@ stdcall GetNumberOfConsoleInputEvents(long ptr)
@ stdcall GetOEMCP() kernel32.GetOEMCP
# @ stub GetOsManufacturingMode
# @ stub GetOsSafeBootMode
@ stdcall GetOverlappedResult(long ptr ptr long)
# @ stub GetOverlappedResultEx
# @ stub GetPackageApplicationContext
# @ stub GetPackageApplicationIds
# @ stub GetPackageApplicationProperty
# @ stub GetPackageApplicationPropertyString
# @ stub GetPackageApplicationResourcesContext
# @ stub GetPackageContext
# @ stub GetPackageFamilyName
# @ stub GetPackageFamilyNameFromToken
@ stdcall GetPackageFullName(long ptr ptr) kernel32.GetPackageFullName
# @ stub GetPackageFullNameFromToken
# @ stub GetPackageId
# @ stub GetPackageInfo
# @ stub GetPackageInstallTime
# @ stub GetPackageOSMaxVersionTested
# @ stub GetPackagePath
# @ stub GetPackagePathByFullName
# @ stub GetPackagePathOnVolume
# @ stub GetPackageProperty
# @ stub GetPackagePropertyString
# @ stub GetPackageResourcesContext
# @ stub GetPackageResourcesProperty
# @ stub GetPackageSecurityContext
# @ stub GetPackageSecurityProperty
# @ stub GetPackageStatus
# @ stub GetPackageStatusForUser
# @ stub GetPackageTargetPlatformProperty
# @ stub GetPackageVolumeSisPath
# @ stub GetPackagesByPackageFamily
# @ stub GetPerformanceInfo
@ stdcall GetPhysicallyInstalledSystemMemory(ptr) kernel32.GetPhysicallyInstalledSystemMemory
# @ stub GetPreviousFgPolicyRefreshInfoInternal
@ stdcall GetPriorityClass(long)
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr)
@ stdcall GetProcAddress(long str)
# @ stub GetProcAddressForCaller
# @ stub GetProcessDefaultCpuSets
# @ stub GetProcessGroupAffinity
@ stdcall GetProcessHandleCount(long ptr)
@ stdcall -norelay GetProcessHeap() kernelbase_GetProcessHeap
@ stdcall -import GetProcessHeaps(long ptr) RtlGetProcessHeaps
@ stdcall GetProcessId(long)
@ stdcall GetProcessIdOfThread(long)
# @ stub GetProcessImageFileNameA
# @ stub GetProcessImageFileNameW
# @ stub GetProcessInformation
# @ stub GetProcessMemoryInfo
@ stdcall GetProcessMitigationPolicy(long long ptr long)
@ stdcall GetProcessPreferredUILanguages(long ptr ptr ptr) kernel32.GetProcessPreferredUILanguages
@ stdcall GetProcessPriorityBoost(long ptr)
@ stdcall GetProcessShutdownParameters(ptr ptr)
@ stdcall GetProcessTimes(long ptr ptr ptr ptr) kernel32.GetProcessTimes
@ stdcall GetProcessVersion(long) kernel32.GetProcessVersion
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr ptr)
# @ stub GetProcessorSystemCycleTime
@ stdcall GetProductInfo(long long long long ptr)
@ stub GetPtrCalData
@ stub GetPtrCalDataArray
# @ stub GetPublisherCacheFolder
# @ stub GetPublisherRootFolder
@ stdcall GetQueuedCompletionStatus(long ptr ptr ptr long)
@ stdcall GetQueuedCompletionStatusEx(ptr ptr long ptr long long)
# @ stub GetRegistryExtensionFlags
# @ stub GetRoamingLastObservedChangeTime
@ stdcall GetSecurityDescriptorControl(ptr ptr ptr)
@ stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr)
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr)
@ stdcall GetSecurityDescriptorLength(ptr)
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr)
@ stub GetSecurityDescriptorRMControl
@ stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr)
# @ stub GetSerializedAtomBytes
# @ stub GetSharedLocalFolder
@ stdcall GetShortPathNameW(wstr ptr long)
@ stdcall GetSidIdentifierAuthority(ptr)
@ stdcall GetSidLengthRequired(long)
@ stdcall GetSidSubAuthority(ptr long)
@ stdcall GetSidSubAuthorityCount(ptr)
# @ stub GetStagedPackageOrigin
# @ stub GetStagedPackagePathByFullName
@ stdcall GetStartupInfoW(ptr)
# @ stub GetStateContainerDepth
# @ stub GetStateFolder
# @ stub GetStateRootFolder
# @ stub GetStateRootFolderBase
# @ stub GetStateSettingsFolder
# @ stub GetStateVersion
@ stdcall GetStdHandle(long)
# @ stub GetStringScripts
@ stub GetStringTableEntry
@ stdcall GetStringTypeA(long long str long ptr) kernel32.GetStringTypeA
@ stdcall GetStringTypeExW(long long wstr long ptr) kernel32.GetStringTypeExW
@ stdcall GetStringTypeW(long wstr long ptr) kernel32.GetStringTypeW
# @ stub GetSystemAppDataFolder
# @ stub GetSystemAppDataKey
# @ stub GetSystemCpuSetInformation
@ stdcall GetSystemDefaultLCID()
@ stdcall GetSystemDefaultLangID()
@ stdcall GetSystemDefaultLocaleName(ptr long)
@ stdcall GetSystemDefaultUILanguage()
@ stdcall GetSystemDirectoryA(ptr long)
@ stdcall GetSystemDirectoryW(ptr long)
@ stdcall GetSystemFileCacheSize(ptr ptr ptr)
@ stdcall GetSystemFirmwareTable(long long ptr long) kernel32.GetSystemFirmwareTable
@ stdcall GetSystemInfo(ptr) kernel32.GetSystemInfo
# @ stub GetSystemMetadataPath
# @ stub GetSystemMetadataPathForPackage
# @ stub GetSystemMetadataPathForPackageFamily
@ stdcall GetSystemPreferredUILanguages(long ptr ptr ptr) kernel32.GetSystemPreferredUILanguages
# @ stub GetSystemStateRootFolder
@ stdcall GetSystemTime(ptr)
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernel32.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr)
@ stdcall GetSystemTimePreciseAsFileTime(ptr)
@ stdcall GetSystemTimes(ptr ptr ptr) kernel32.GetSystemTimes
@ stdcall GetSystemWindowsDirectoryA(ptr long)
@ stdcall GetSystemWindowsDirectoryW(ptr long)
# @ stub GetSystemWow64Directory2A
# @ stub GetSystemWow64Directory2W
@ stdcall GetSystemWow64DirectoryA(ptr long) kernel32.GetSystemWow64DirectoryA
@ stdcall GetSystemWow64DirectoryW(ptr long) kernel32.GetSystemWow64DirectoryW
# @ stub GetTargetPlatformContext
@ stdcall GetTempFileNameA(str str long ptr)
@ stdcall GetTempFileNameW(wstr wstr long ptr)
@ stdcall GetTempPathA(long ptr)
@ stdcall GetTempPathW(long ptr)
@ stdcall GetThreadContext(long ptr)
# @ stub GetThreadDescription
@ stdcall GetThreadErrorMode()
@ stdcall GetThreadGroupAffinity(long ptr)
@ stdcall GetThreadIOPendingFlag(long ptr)
@ stdcall GetThreadId(ptr)
# @ stub GetThreadIdealProcessorEx
# @ stub GetThreadInformation
@ stdcall GetThreadLocale()
@ stdcall GetThreadPreferredUILanguages(long ptr ptr ptr) kernel32.GetThreadPreferredUILanguages
@ stdcall GetThreadPriority(long)
@ stdcall GetThreadPriorityBoost(long ptr)
# @ stub GetThreadSelectedCpuSets
@ stdcall GetThreadTimes(long ptr ptr ptr ptr)
@ stdcall GetThreadUILanguage()
@ stdcall GetTickCount() kernel32.GetTickCount
@ stdcall -ret64 GetTickCount64() kernel32.GetTickCount64
@ stdcall GetTimeFormatA(long long ptr str ptr long) kernel32.GetTimeFormatA
@ stdcall GetTimeFormatEx(wstr long ptr wstr ptr long) kernel32.GetTimeFormatEx
@ stdcall GetTimeFormatW(long long ptr wstr ptr long) kernel32.GetTimeFormatW
@ stdcall GetTimeZoneInformation(ptr) kernel32.GetTimeZoneInformation
@ stdcall GetTimeZoneInformationForYear(long ptr ptr) kernel32.GetTimeZoneInformationForYear
@ stdcall GetTokenInformation(long long ptr long ptr)
@ stdcall GetTraceEnableFlags(int64) ntdll.EtwGetTraceEnableFlags
@ stdcall GetTraceEnableLevel(int64) ntdll.EtwGetTraceEnableLevel
@ stdcall -ret64 GetTraceLoggerHandle(ptr) ntdll.EtwGetTraceLoggerHandle
@ stub GetUILanguageInfo
# @ stub GetUnicodeStringToEightBitSizeRoutine
# @ stub GetUnicodeStringToEightBitStringRoutine
@ stdcall GetUserDefaultLCID()
@ stdcall GetUserDefaultLangID()
@ stdcall GetUserDefaultLocaleName(ptr long)
@ stdcall GetUserDefaultUILanguage()
@ stdcall GetUserGeoID(long) kernel32.GetUserGeoID
@ stub GetUserInfo
@ stub GetUserInfoWord
# @ stub GetUserOverrideString
# @ stub GetUserOverrideWord
@ stdcall GetUserPreferredUILanguages(long ptr ptr ptr) kernel32.GetUserPreferredUILanguages
@ stdcall GetVersion()
@ stdcall GetVersionExA(ptr)
@ stdcall GetVersionExW(ptr)
@ stdcall GetVolumeInformationA(str ptr long ptr ptr ptr ptr long) kernel32.GetVolumeInformationA
@ stdcall GetVolumeInformationByHandleW(ptr ptr long ptr ptr ptr ptr long) kernel32.GetVolumeInformationByHandleW
@ stdcall GetVolumeInformationW(wstr ptr long ptr ptr ptr ptr long) kernel32.GetVolumeInformationW
@ stdcall GetVolumeNameForVolumeMountPointW(wstr ptr long) kernel32.GetVolumeNameForVolumeMountPointW
@ stdcall GetVolumePathNameW(wstr ptr long) kernel32.GetVolumePathNameW
@ stdcall GetVolumePathNamesForVolumeNameW(wstr ptr long ptr) kernel32.GetVolumePathNamesForVolumeNameW
@ stdcall GetWindowsAccountDomainSid(ptr ptr ptr)
@ stdcall GetWindowsDirectoryA(ptr long)
@ stdcall GetWindowsDirectoryW(ptr long)
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr)
# @ stub GetWsChanges
# @ stub GetWsChangesEx
# @ stub GetXStateFeaturesMask
@ stdcall GlobalAlloc(long long)
@ stdcall GlobalFree(long)
@ stdcall GlobalMemoryStatusEx(ptr) kernel32.GlobalMemoryStatusEx
# @ stub GuardCheckLongJumpTarget
# @ stub HasPolicyForegroundProcessingCompletedInternal
@ stdcall HashData(ptr long ptr long)
@ stdcall HeapAlloc(long long long) ntdll.RtlAllocateHeap
@ stdcall HeapCompact(long long)
@ stdcall HeapCreate(long long long)
@ stdcall HeapDestroy(long)
@ stdcall HeapFree(long long ptr)
@ stdcall HeapLock(long)
@ stdcall HeapQueryInformation(long long ptr long ptr)
@ stdcall HeapReAlloc(long long ptr long) ntdll.RtlReAllocateHeap
@ stdcall HeapSetInformation(ptr long ptr long)
@ stdcall HeapSize(long long ptr) ntdll.RtlSizeHeap
@ stub HeapSummary
@ stdcall HeapUnlock(long)
@ stdcall HeapValidate(long long ptr)
@ stdcall HeapWalk(long ptr)
@ stdcall IdnToAscii(long wstr long ptr long) kernel32.IdnToAscii
@ stdcall IdnToNameprepUnicode(long wstr long ptr long) kernel32.IdnToNameprepUnicode
@ stdcall IdnToUnicode(long wstr long ptr long) kernel32.IdnToUnicode
@ stdcall ImpersonateAnonymousToken(long)
@ stdcall ImpersonateLoggedOnUser(long)
@ stdcall ImpersonateNamedPipeClient(long)
@ stdcall ImpersonateSelf(long)
# @ stub IncrementPackageStatusVersion
@ stdcall InitOnceBeginInitialize(ptr long ptr ptr)
@ stdcall InitOnceComplete(ptr long ptr)
@ stdcall InitOnceExecuteOnce(ptr ptr ptr ptr)
@ stdcall InitOnceInitialize(ptr) ntdll.RtlRunOnceInitialize
@ stdcall InitializeAcl(ptr long long)
@ stdcall InitializeConditionVariable(ptr) ntdll.RtlInitializeConditionVariable
# @ stub InitializeContext
@ stdcall InitializeCriticalSection(ptr) ntdll.RtlInitializeCriticalSection
@ stdcall InitializeCriticalSectionAndSpinCount(ptr long)
@ stdcall InitializeCriticalSectionEx(ptr long long)
# @ stub InitializeEnclave
@ stdcall InitializeProcThreadAttributeList(ptr long long ptr)
# @ stub InitializeProcessForWsWatch
@ stdcall InitializeSListHead(ptr) ntdll.RtlInitializeSListHead
@ stdcall InitializeSRWLock(ptr) ntdll.RtlInitializeSRWLock
@ stdcall InitializeSecurityDescriptor(ptr long)
@ stdcall InitializeSid(ptr ptr long)
# @ stub InitializeSynchronizationBarrier
# @ stub InstallELAMCertificateInfo
@ stdcall -arch=i386 InterlockedCompareExchange(ptr long long)
@ stdcall -arch=i386 -ret64 InterlockedCompareExchange64(ptr int64 int64) ntdll.RtlInterlockedCompareExchange64
@ stdcall -arch=i386 InterlockedDecrement(ptr)
@ stdcall -arch=i386 InterlockedExchange(ptr long)
@ stdcall -arch=i386 InterlockedExchangeAdd(ptr long )
@ stdcall InterlockedFlushSList(ptr) ntdll.RtlInterlockedFlushSList
@ stdcall -arch=i386 InterlockedIncrement(ptr)
@ stdcall InterlockedPopEntrySList(ptr) ntdll.RtlInterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) ntdll.RtlInterlockedPushEntrySList
@ stdcall -fastcall InterlockedPushListSList(ptr ptr ptr long) ntdll.RtlInterlockedPushListSList
@ stdcall InterlockedPushListSListEx(ptr ptr ptr long) ntdll.RtlInterlockedPushListSListEx
@ stub InternalLcidToName
@ stub Internal_EnumCalendarInfo
@ stub Internal_EnumDateFormats
@ stdcall Internal_EnumLanguageGroupLocales(ptr long long ptr long)
@ stdcall Internal_EnumSystemCodePages(ptr long long)
@ stdcall Internal_EnumSystemLanguageGroups(ptr long ptr long)
@ stub Internal_EnumSystemLocales
@ stub Internal_EnumTimeFormats
@ stdcall Internal_EnumUILanguages(ptr long long long)
# @ stub InternetTimeFromSystemTimeA
# @ stub InternetTimeFromSystemTimeW
# @ stub InternetTimeToSystemTimeA
# @ stub InternetTimeToSystemTimeW
# @ stub InvalidateAppModelVersionCache
@ stub InvalidateTzSpecificCache
@ stdcall IsCharAlphaA(long)
@ stdcall IsCharAlphaNumericA(long)
@ stdcall IsCharAlphaNumericW(long)
@ stdcall IsCharAlphaW(long)
@ stdcall IsCharBlankW(long)
@ stdcall IsCharCntrlW(long)
@ stdcall IsCharDigitW(long)
@ stdcall IsCharLowerA(long)
@ stdcall IsCharLowerW(long)
@ stdcall IsCharPunctW(long)
@ stdcall IsCharSpaceA(long)
@ stdcall IsCharSpaceW(long)
@ stdcall IsCharUpperA(long)
@ stdcall IsCharUpperW(long)
@ stdcall IsCharXDigitW(long)
@ stdcall IsDBCSLeadByte(long) kernel32.IsDBCSLeadByte
@ stdcall IsDBCSLeadByteEx(long long) kernel32.IsDBCSLeadByteEx
@ stdcall IsDebuggerPresent()
# @ stub IsDeveloperModeEnabled
# @ stub IsDeveloperModePolicyApplied
# @ stub IsEnclaveTypeSupported
# @ stub IsGlobalizationUserSettingsKeyRedirected
@ stdcall IsInternetESCEnabled()
@ stub IsNLSDefinedString
@ stdcall IsNormalizedString(long wstr long) kernel32.IsNormalizedString
# @ stub IsProcessCritical
@ stdcall IsProcessInJob(long long ptr)
@ stdcall IsProcessorFeaturePresent(long)
# @ stub IsSideloadingEnabled
# @ stub IsSideloadingPolicyApplied
# @ stub IsSyncForegroundPolicyRefresh
@ stdcall IsThreadAFiber()
@ stdcall IsThreadpoolTimerSet(ptr) ntdll.TpIsTimerSet
# @ stub IsTimeZoneRedirectionEnabled
@ stdcall IsTokenRestricted(long)
@ stdcall IsValidAcl(ptr)
@ stdcall IsValidCodePage(long) kernel32.IsValidCodePage
@ stdcall IsValidLanguageGroup(long long)
@ stdcall IsValidLocale(long long) kernel32.IsValidLocale
@ stdcall IsValidLocaleName(wstr) kernel32.IsValidLocaleName
# @ stub IsValidNLSVersion
@ stub IsValidRelativeSecurityDescriptor
@ stdcall IsValidSecurityDescriptor(ptr)
@ stdcall IsValidSid(ptr)
@ stdcall IsWellKnownSid(ptr long)
@ stdcall IsWow64Process(ptr ptr)
# @ stub IsWow64Process2
@ stdcall K32EmptyWorkingSet(long) kernel32.K32EmptyWorkingSet
@ stdcall K32EnumDeviceDrivers(ptr long ptr) kernel32.K32EnumDeviceDrivers
@ stdcall K32EnumPageFilesA(ptr ptr) kernel32.K32EnumPageFilesA
@ stdcall K32EnumPageFilesW(ptr ptr) kernel32.K32EnumPageFilesW
@ stdcall K32EnumProcessModules(long ptr long ptr) kernel32.K32EnumProcessModules
@ stdcall K32EnumProcessModulesEx(long ptr long ptr long) kernel32.K32EnumProcessModulesEx
@ stdcall K32EnumProcesses(ptr long ptr) kernel32.K32EnumProcesses
@ stdcall K32GetDeviceDriverBaseNameA(ptr ptr long) kernel32.K32GetDeviceDriverBaseNameA
@ stdcall K32GetDeviceDriverBaseNameW(ptr ptr long) kernel32.K32GetDeviceDriverBaseNameW
@ stdcall K32GetDeviceDriverFileNameA(ptr ptr long) kernel32.K32GetDeviceDriverFileNameA
@ stdcall K32GetDeviceDriverFileNameW(ptr ptr long) kernel32.K32GetDeviceDriverFileNameW
@ stdcall K32GetMappedFileNameA(long ptr ptr long) kernel32.K32GetMappedFileNameA
@ stdcall K32GetMappedFileNameW(long ptr ptr long) kernel32.K32GetMappedFileNameW
@ stdcall K32GetModuleBaseNameA(long long ptr long) kernel32.K32GetModuleBaseNameA
@ stdcall K32GetModuleBaseNameW(long long ptr long) kernel32.K32GetModuleBaseNameW
@ stdcall K32GetModuleFileNameExA(long long ptr long) kernel32.K32GetModuleFileNameExA
@ stdcall K32GetModuleFileNameExW(long long ptr long) kernel32.K32GetModuleFileNameExW
@ stdcall K32GetModuleInformation(long long ptr long) kernel32.K32GetModuleInformation
@ stdcall K32GetPerformanceInfo(ptr long) kernel32.K32GetPerformanceInfo
@ stdcall K32GetProcessImageFileNameA(long ptr long) kernel32.K32GetProcessImageFileNameA
@ stdcall K32GetProcessImageFileNameW(long ptr long) kernel32.K32GetProcessImageFileNameW
@ stdcall K32GetProcessMemoryInfo(long ptr long) kernel32.K32GetProcessMemoryInfo
@ stdcall K32GetWsChanges(long ptr long) kernel32.K32GetWsChanges
@ stdcall K32GetWsChangesEx(long ptr ptr) kernel32.K32GetWsChangesEx
@ stdcall K32InitializeProcessForWsWatch(long) kernel32.K32InitializeProcessForWsWatch
@ stdcall K32QueryWorkingSet(long ptr long) kernel32.K32QueryWorkingSet
@ stdcall K32QueryWorkingSetEx(long ptr long) kernel32.K32QueryWorkingSetEx
@ stub KernelBaseGetGlobalData
@ stdcall LCIDToLocaleName(long ptr long long)
@ stdcall LCMapStringA(long long str long ptr long) kernel32.LCMapStringA
@ stdcall LCMapStringEx(wstr long wstr long ptr long ptr ptr long) kernel32.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernel32.LCMapStringW
# @ stub LeaveCriticalPolicySectionInternal
@ stdcall LeaveCriticalSection(ptr) ntdll.RtlLeaveCriticalSection
@ stdcall LeaveCriticalSectionWhenCallbackReturns(ptr ptr) ntdll.TpCallbackLeaveCriticalSectionOnCompletion
# @ stub LoadAppInitDlls
# @ stub LoadEnclaveData
@ stdcall LoadLibraryA(str)
@ stdcall LoadLibraryExA( str long long)
@ stdcall LoadLibraryExW(wstr long long)
@ stdcall LoadLibraryW(wstr)
# @ stub LoadPackagedLibrary
@ stdcall LoadResource(long long)
@ stdcall LoadStringA(long long ptr long)
@ stub LoadStringBaseExW
@ stub LoadStringByReference
@ stdcall LoadStringW(long long ptr long)
@ stdcall LocalAlloc(long long)
@ stdcall LocalFileTimeToFileTime(ptr ptr)
@ stdcall LocalFree(long)
@ stdcall LocalLock(long)
@ stdcall LocalReAlloc(long long long)
@ stdcall LocalUnlock(long)
@ stdcall LocaleNameToLCID(wstr long) kernel32.LocaleNameToLCID
# @ stub LocateXStateFeature
@ stdcall LockFile(long long long long long)
@ stdcall LockFileEx(long long long long long ptr)
@ stdcall LockResource(long)
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub MakeAbsoluteSD2
@ stdcall MakeSelfRelativeSD(ptr ptr ptr)
@ stdcall MapGenericMask(ptr ptr)
# @ stub MapPredefinedHandleInternal
@ stdcall MapUserPhysicalPages(ptr long ptr)
@ stdcall MapViewOfFile(long long long long long)
@ stdcall MapViewOfFileEx(long long long long long ptr)
@ stdcall MapViewOfFileExNuma(long long long long long ptr long)
# @ stub MapViewOfFileFromApp
@ stdcall MoveFileExW(wstr wstr long) kernel32.MoveFileExW
# @ stub MoveFileWithProgressTransactedW
@ stdcall MoveFileWithProgressW(wstr wstr ptr ptr long) kernel32.MoveFileWithProgressW
@ stdcall MulDiv(long long long) kernel32.MulDiv
@ stdcall MultiByteToWideChar(long long str long ptr long) kernel32.MultiByteToWideChar
# @ stub NamedPipeEventEnum
# @ stub NamedPipeEventSelect
@ stdcall NeedCurrentDirectoryForExePathA(str)
@ stdcall NeedCurrentDirectoryForExePathW(wstr)
@ stub NlsCheckPolicy
@ stub NlsDispatchAnsiEnumProc
@ stub NlsEventDataDescCreate
@ stub NlsGetACPFromLocale
@ stub NlsGetCacheUpdateCount
@ stub NlsIsUserDefaultLocale
@ stub NlsUpdateLocale
@ stub NlsUpdateSystemLocale
@ stub NlsValidateLocale
@ stub NlsWriteEtwEvent
@ stdcall NormalizeString(long wstr long ptr long) kernel32.NormalizeString
@ stub NotifyMountMgr
@ stub NotifyRedirectedStringChange
@ stdcall ObjectCloseAuditAlarmW(wstr ptr long)
@ stdcall ObjectDeleteAuditAlarmW(wstr ptr long)
@ stdcall ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr)
@ stdcall ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long)
# @ stub OfferVirtualMemory
@ stdcall OpenEventA(long long str)
@ stdcall OpenEventW(long long wstr)
@ stdcall OpenFileById(long ptr long long ptr long)
# @ stub OpenFileMappingFromApp
@ stdcall OpenFileMappingW(long long wstr)
# @ stub OpenGlobalizationUserSettingsKey
@ stdcall OpenMutexW(long long wstr)
# @ stub OpenPackageInfoByFullName
# @ stub OpenPackageInfoByFullNameForUser
# @ stub OpenPrivateNamespaceW
@ stdcall OpenProcess(long long long)
@ stdcall OpenProcessToken(long long ptr)
@ stub OpenRegKey
@ stdcall OpenSemaphoreW(long long wstr)
# @ stub OpenState
# @ stub OpenStateAtom
# @ stub OpenStateExplicit
# @ stub OpenStateExplicitForUserSid
# @ stub OpenStateExplicitForUserSidString
@ stdcall OpenThread(long long long)
@ stdcall OpenThreadToken(long long long ptr)
@ stdcall OpenWaitableTimerW(long long wstr)
@ stdcall OutputDebugStringA(str)
@ stdcall OutputDebugStringW(wstr)
# @ stub OverrideRoamingDataModificationTimesInRange
# @ stub PackageFamilyNameFromFullName
# @ stub PackageFamilyNameFromId
# @ stub PackageFamilyNameFromProductId
# @ stub PackageFullNameFromId
# @ stub PackageFullNameFromProductId
# @ stub PackageIdFromFullName
# @ stub PackageIdFromProductId
# @ stub PackageNameAndPublisherIdFromFamilyName
# @ stub PackageRelativeApplicationIdFromProductId
# @ stub PackageSidFromFamilyName
# @ stub PackageSidFromProductId
# @ stub ParseApplicationUserModelId
@ stdcall ParseURLA(str ptr)
@ stdcall ParseURLW(wstr ptr)
@ stdcall PathAddBackslashA(str)
@ stdcall PathAddBackslashW(wstr)
@ stdcall PathAddExtensionA(str str)
@ stdcall PathAddExtensionW(wstr wstr)
@ stdcall PathAllocCanonicalize(wstr long ptr)
@ stdcall PathAllocCombine(wstr wstr long ptr)
@ stdcall PathAppendA(str str)
@ stdcall PathAppendW(wstr wstr)
@ stdcall PathCanonicalizeA(ptr str)
@ stdcall PathCanonicalizeW(ptr wstr)
@ stdcall PathCchAddBackslash(wstr long)
@ stdcall PathCchAddBackslashEx(wstr long ptr ptr)
@ stdcall PathCchAddExtension(wstr long wstr)
@ stdcall PathCchAppend(wstr long wstr)
@ stdcall PathCchAppendEx(wstr long wstr long)
@ stdcall PathCchCanonicalize(ptr long wstr)
@ stdcall PathCchCanonicalizeEx(ptr long wstr long)
@ stdcall PathCchCombine(ptr long wstr wstr)
@ stdcall PathCchCombineEx(ptr long wstr wstr long)
@ stdcall PathCchFindExtension(wstr long ptr)
@ stdcall PathCchIsRoot(wstr)
@ stdcall PathCchRemoveBackslash(wstr long)
@ stdcall PathCchRemoveBackslashEx(wstr long ptr ptr)
@ stdcall PathCchRemoveExtension(wstr long)
@ stdcall PathCchRemoveFileSpec(wstr long)
@ stdcall PathCchRenameExtension(wstr long wstr)
@ stdcall PathCchSkipRoot(wstr ptr)
@ stdcall PathCchStripPrefix(wstr long)
@ stdcall PathCchStripToRoot(wstr long)
@ stdcall PathCombineA(ptr str str)
@ stdcall PathCombineW(ptr wstr wstr)
@ stdcall PathCommonPrefixA(str str ptr)
@ stdcall PathCommonPrefixW(wstr wstr ptr)
@ stdcall PathCreateFromUrlA(str ptr ptr long)
@ stdcall PathCreateFromUrlAlloc(wstr ptr long)
@ stdcall PathCreateFromUrlW(wstr ptr ptr long)
@ stdcall PathFileExistsA(str)
@ stdcall PathFileExistsW(wstr)
@ stdcall PathFindExtensionA(str)
@ stdcall PathFindExtensionW(wstr)
@ stdcall PathFindFileNameA(str)
@ stdcall PathFindFileNameW(wstr)
@ stdcall PathFindNextComponentA(str)
@ stdcall PathFindNextComponentW(wstr)
@ stdcall PathGetArgsA(str)
@ stdcall PathGetArgsW(wstr)
@ stdcall PathGetCharTypeA(long)
@ stdcall PathGetCharTypeW(long)
@ stdcall PathGetDriveNumberA(str)
@ stdcall PathGetDriveNumberW(wstr)
@ stdcall PathIsFileSpecA(str)
@ stdcall PathIsFileSpecW(wstr)
@ stdcall PathIsLFNFileSpecA(str)
@ stdcall PathIsLFNFileSpecW(wstr)
@ stdcall PathIsPrefixA(str str)
@ stdcall PathIsPrefixW(wstr wstr)
@ stdcall PathIsRelativeA(str)
@ stdcall PathIsRelativeW(wstr)
@ stdcall PathIsRootA(str)
@ stdcall PathIsRootW(wstr)
@ stdcall PathIsSameRootA(str str)
@ stdcall PathIsSameRootW(wstr wstr)
@ stdcall PathIsUNCA(str)
@ stdcall PathIsUNCEx(wstr ptr)
@ stdcall PathIsUNCServerA(str)
@ stdcall PathIsUNCServerShareA(str)
@ stdcall PathIsUNCServerShareW(wstr)
@ stdcall PathIsUNCServerW(wstr)
@ stdcall PathIsUNCW(wstr)
@ stdcall PathIsURLA(str)
@ stdcall PathIsURLW(wstr)
@ stdcall PathIsValidCharA(long long)
@ stdcall PathIsValidCharW(long long)
@ stdcall PathMatchSpecA(str str)
# @ stub PathMatchSpecExA
# @ stub PathMatchSpecExW
@ stdcall PathMatchSpecW(wstr wstr)
@ stdcall PathParseIconLocationA(str)
@ stdcall PathParseIconLocationW(wstr)
@ stdcall PathQuoteSpacesA(str)
@ stdcall PathQuoteSpacesW(wstr)
@ stdcall PathRelativePathToA(ptr str long str long)
@ stdcall PathRelativePathToW(ptr wstr long wstr long)
@ stdcall PathRemoveBackslashA(str)
@ stdcall PathRemoveBackslashW(wstr)
@ stdcall PathRemoveBlanksA(str)
@ stdcall PathRemoveBlanksW(wstr)
@ stdcall PathRemoveExtensionA(str)
@ stdcall PathRemoveExtensionW(wstr)
@ stdcall PathRemoveFileSpecA(str)
@ stdcall PathRemoveFileSpecW(wstr)
@ stdcall PathRenameExtensionA(str str)
@ stdcall PathRenameExtensionW(wstr wstr)
@ stdcall PathSearchAndQualifyA(str ptr long)
@ stdcall PathSearchAndQualifyW(wstr ptr long)
@ stdcall PathSkipRootA(str)
@ stdcall PathSkipRootW(wstr)
@ stdcall PathStripPathA(str)
@ stdcall PathStripPathW(wstr)
@ stdcall PathStripToRootA(str)
@ stdcall PathStripToRootW(wstr)
@ stdcall PathUnExpandEnvStringsA(str ptr long)
@ stdcall PathUnExpandEnvStringsW(wstr ptr long)
@ stdcall PathUnquoteSpacesA(str)
@ stdcall PathUnquoteSpacesW(wstr)
# @ stub PcwAddQueryItem
# @ stub PcwClearCounterSetSecurity
# @ stub PcwCollectData
# @ stub PcwCompleteNotification
# @ stub PcwCreateNotifier
# @ stub PcwCreateQuery
# @ stub PcwDisconnectCounterSet
# @ stub PcwEnumerateInstances
# @ stub PcwIsNotifierAlive
# @ stub PcwQueryCounterSetSecurity
# @ stub PcwReadNotificationData
# @ stub PcwRegisterCounterSet
# @ stub PcwRemoveQueryItem
# @ stub PcwSendNotification
# @ stub PcwSendStatelessNotification
# @ stub PcwSetCounterSetSecurity
# @ stub PcwSetQueryItemUserData
@ stdcall PeekConsoleInputA(ptr ptr long ptr)
@ stdcall PeekConsoleInputW(ptr ptr long ptr)
@ stdcall PeekNamedPipe(long ptr long ptr ptr ptr)
@ stdcall PerfCreateInstance(long ptr wstr long)
# @ stub PerfDecrementULongCounterValue
# @ stub PerfDecrementULongLongCounterValue
@ stdcall PerfDeleteInstance(long ptr)
# @ stub PerfIncrementULongCounterValue
# @ stub PerfIncrementULongLongCounterValue
# @ stub PerfQueryInstance
@ stdcall PerfSetCounterRefValue(long ptr long ptr)
@ stdcall PerfSetCounterSetInfo(long ptr long)
# @ stub PerfSetULongCounterValue
# @ stub PerfSetULongLongCounterValue
@ stdcall PerfStartProvider(ptr ptr ptr)
@ stdcall PerfStartProviderEx(ptr ptr ptr)
@ stdcall PerfStopProvider(long)
# @ stub PoolPerAppKeyStateInternal
@ stdcall PostQueuedCompletionStatus(long long ptr ptr)
# @ stub PrefetchVirtualMemory
@ stub PrivCopyFileExW
@ stdcall PrivilegeCheck(ptr ptr ptr)
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long)
@ stdcall ProcessIdToSessionId(long ptr) kernel32.ProcessIdToSessionId
# @ stub ProductIdFromPackageFamilyName
# @ stub PsmCreateKey
# @ stub PsmCreateKeyWithDynamicId
# @ stub PsmEqualApplication
# @ stub PsmEqualPackage
# @ stub PsmGetApplicationNameFromKey
# @ stub PsmGetKeyFromProcess
# @ stub PsmGetKeyFromToken
# @ stub PsmGetPackageFullNameFromKey
# @ stub PsmIsChildKey
# @ stub PsmIsDynamicKey
# @ stub PsmIsValidKey
# @ stub PssCaptureSnapshot
# @ stub PssDuplicateSnapshot
# @ stub PssFreeSnapshot
# @ stub PssQuerySnapshot
# @ stub PssWalkMarkerCreate
# @ stub PssWalkMarkerFree
# @ stub PssWalkMarkerGetPosition
# @ stub PssWalkMarkerSeekToBeginning
# @ stub PssWalkMarkerSetPosition
# @ stub PssWalkSnapshot
# @ stub PublishStateChangeNotification
@ stdcall PulseEvent(long)
@ stdcall PurgeComm(long long)
@ stdcall QISearch(ptr ptr ptr ptr)
@ stdcall QueryActCtxSettingsW(long ptr wstr wstr ptr long ptr)
@ stdcall QueryActCtxW(long ptr ptr long ptr long ptr)
@ stdcall QueryDepthSList(ptr) ntdll.RtlQueryDepthSList
@ stdcall QueryDosDeviceW(wstr ptr long) kernel32.QueryDosDeviceW
@ stdcall QueryFullProcessImageNameA(ptr long ptr ptr) kernel32.QueryFullProcessImageNameA
@ stdcall QueryFullProcessImageNameW(ptr long ptr ptr) kernel32.QueryFullProcessImageNameW
# @ stub QueryIdleProcessorCycleTime
# @ stub QueryIdleProcessorCycleTimeEx
# @ stub QueryInterruptTime
# @ stub QueryInterruptTimePrecise
@ stdcall QueryMemoryResourceNotification(ptr ptr)
# @ stub QueryOptionalDelayLoadedAPI
@ stdcall QueryPerformanceCounter(ptr) ntdll.RtlQueryPerformanceCounter
@ stdcall QueryPerformanceFrequency(ptr) ntdll.RtlQueryPerformanceFrequency
@ stub QueryProcessAffinityUpdateMode
@ stdcall QueryProcessCycleTime(long ptr) kernel32.QueryProcessCycleTime
# @ stub QueryProtectedPolicy
@ stub QuerySecurityAccessMask
# @ stub QueryStateAtomValueInfo
# @ stub QueryStateContainerCreatedNew
# @ stub QueryStateContainerItemInfo
@ stdcall QueryThreadCycleTime(long ptr) kernel32.QueryThreadCycleTime
@ stub QueryThreadpoolStackInformation
@ stdcall QueryUnbiasedInterruptTime(ptr) ntdll.RtlQueryUnbiasedInterruptTime
# @ stub QueryUnbiasedInterruptTimePrecise
# @ stub QueryVirtualMemoryInformation
# @ stub QueryWorkingSet
# @ stub QueryWorkingSetEx
@ stdcall QueueUserAPC(ptr long long)
@ stdcall QueueUserWorkItem(ptr ptr long)
# @ stub QuirkGetData
# @ stub QuirkGetData2
@ stdcall QuirkIsEnabled(ptr)
# @ stub QuirkIsEnabled2
@ stdcall QuirkIsEnabled3(ptr ptr)
# @ stub QuirkIsEnabledForPackage
# @ stub QuirkIsEnabledForPackage2
# @ stub QuirkIsEnabledForPackage3
# @ stub QuirkIsEnabledForPackage4
# @ stub QuirkIsEnabledForProcess
@ stdcall RaiseException(long long long ptr)
# @ stub RaiseFailFastException
@ stdcall ReOpenFile(ptr long long long)
@ stdcall ReadConsoleA(long ptr long ptr ptr)
@ stdcall ReadConsoleInputA(long ptr long ptr)
@ stub ReadConsoleInputExA
@ stub ReadConsoleInputExW
@ stdcall ReadConsoleInputW(long ptr long ptr) kernel32.ReadConsoleInputW
@ stdcall ReadConsoleOutputA(long ptr long long ptr)
@ stdcall ReadConsoleOutputAttribute(long ptr long long ptr)
@ stdcall ReadConsoleOutputCharacterA(long ptr long long ptr)
@ stdcall ReadConsoleOutputCharacterW(long ptr long long ptr)
@ stdcall ReadConsoleOutputW(long ptr long long ptr)
@ stdcall ReadConsoleW(long ptr long ptr ptr) kernel32.ReadConsoleW
@ stdcall ReadDirectoryChangesW(long ptr long long long ptr ptr ptr)
@ stdcall ReadFile(long ptr long ptr ptr)
@ stdcall ReadFileEx(long ptr long ptr ptr)
@ stdcall ReadFileScatter(long ptr long ptr ptr)
@ stdcall ReadProcessMemory(long ptr ptr long ptr)
# @ stub ReadStateAtomValue
# @ stub ReadStateContainerValue
# @ stub ReclaimVirtualMemory
# @ stub RefreshPolicyExInternal
# @ stub RefreshPolicyInternal
@ stdcall RegCloseKey(long)
@ stdcall RegCopyTreeW(long wstr long)
@ stdcall RegCreateKeyExA(long str long ptr long long ptr ptr ptr)
# @ stub RegCreateKeyExInternalA
# @ stub RegCreateKeyExInternalW
@ stdcall RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr)
@ stdcall RegDeleteKeyExA(long str long long)
# @ stub RegDeleteKeyExInternalA
# @ stub RegDeleteKeyExInternalW
@ stdcall RegDeleteKeyExW(long wstr long long)
@ stdcall RegDeleteKeyValueA(long str str)
@ stdcall RegDeleteKeyValueW(long wstr wstr)
@ stdcall RegDeleteTreeA(long str)
@ stdcall RegDeleteTreeW(long wstr)
@ stdcall RegDeleteValueA(long str)
@ stdcall RegDeleteValueW(long wstr)
# @ stub RegDisablePredefinedCacheEx
@ stdcall RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumValueA(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegEnumValueW(long long ptr ptr ptr ptr ptr ptr)
@ stdcall RegFlushKey(long)
@ stdcall RegGetKeySecurity(long long ptr ptr)
@ stdcall RegGetValueA(long str str long ptr ptr ptr)
@ stdcall RegGetValueW(long wstr wstr long ptr ptr ptr)
# @ stub RegKrnGetAppKeyEventAddressInternal
# @ stub RegKrnGetAppKeyLoaded
# @ stub RegKrnGetClassesEnumTableAddressInternal
# @ stub RegKrnGetHKEY_ClassesRootAddress
# @ stub RegKrnGetTermsrvRegistryExtensionFlags
# @ stub RegKrnResetAppKeyLoaded
# @ stub RegKrnSetDllHasThreadStateGlobal
# @ stub RegKrnSetTermsrvRegistryExtensionFlags
@ stdcall RegLoadAppKeyA(str ptr long long long)
@ stdcall RegLoadAppKeyW(wstr ptr long long long)
@ stdcall RegLoadKeyA(long str str)
@ stdcall RegLoadKeyW(long wstr wstr)
@ stdcall RegLoadMUIStringA(long str str long ptr long str)
@ stdcall RegLoadMUIStringW(long wstr wstr long ptr long wstr)
@ stdcall RegNotifyChangeKeyValue(long long long long long)
@ stdcall RegOpenCurrentUser(long ptr)
@ stdcall RegOpenKeyExA(long str long long ptr)
# @ stub RegOpenKeyExInternalA
# @ stub RegOpenKeyExInternalW
@ stdcall RegOpenKeyExW(long wstr long long ptr)
@ stdcall RegOpenUserClassesRoot(ptr long long ptr)
@ stdcall RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RegQueryValueExA(long str ptr ptr ptr ptr)
@ stdcall RegQueryValueExW(long wstr ptr ptr ptr ptr)
@ stdcall RegRestoreKeyA(long str long)
@ stdcall RegRestoreKeyW(long wstr long)
@ stdcall RegSaveKeyExA(long str ptr long)
@ stdcall RegSaveKeyExW(long wstr ptr long)
@ stdcall RegSetKeySecurity(long long ptr)
@ stdcall RegSetKeyValueA(long str str long ptr long)
@ stdcall RegSetKeyValueW(long wstr wstr long ptr long)
@ stdcall RegSetValueExA(long str long long ptr long)
@ stdcall RegSetValueExW(long wstr long long ptr long)
@ stdcall RegUnLoadKeyA(long str)
@ stdcall RegUnLoadKeyW(long wstr)
# @ stub RegisterBadMemoryNotification
# @ stub RegisterGPNotificationInternal
# @ stub RegisterStateChangeNotification
# @ stub RegisterStateLock
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) ntdll.EtwRegisterTraceGuidsW
@ stdcall RegisterWaitForSingleObjectEx(long ptr ptr long long)
@ stdcall ReleaseActCtx(ptr)
@ stdcall ReleaseMutex(long)
@ stdcall ReleaseMutexWhenCallbackReturns(ptr long) ntdll.TpCallbackReleaseMutexOnCompletion
@ stdcall ReleaseSRWLockExclusive(ptr) ntdll.RtlReleaseSRWLockExclusive
@ stdcall ReleaseSRWLockShared(ptr) ntdll.RtlReleaseSRWLockShared
@ stdcall ReleaseSemaphore(long long ptr)
@ stdcall ReleaseSemaphoreWhenCallbackReturns(ptr long long) ntdll.TpCallbackReleaseSemaphoreOnCompletion
# @ stub ReleaseStateLock
@ stdcall RemapPredefinedHandleInternal(long long)
@ stdcall RemoveDirectoryA(str) kernel32.RemoveDirectoryA
@ stdcall RemoveDirectoryW(wstr) kernel32.RemoveDirectoryW
@ stdcall RemoveDllDirectory(ptr)
# @ stub RemovePackageStatus
# @ stub RemovePackageStatusForUser
@ stdcall RemoveVectoredContinueHandler(ptr) ntdll.RtlRemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) ntdll.RtlRemoveVectoredExceptionHandler
# @ stub ReplaceFileExInternal
@ stdcall ReplaceFileW(wstr wstr wstr long ptr ptr) kernel32.ReplaceFileW
@ stdcall ResetEvent(long)
# @ stub ResetState
@ stdcall ResetWriteWatch(ptr long)
@ stdcall -import ResolveDelayLoadedAPI(ptr ptr ptr ptr ptr long) LdrResolveDelayLoadedAPI
# @ stub ResolveDelayLoadsFromDll
@ stdcall ResolveLocaleName(wstr ptr long)
@ stdcall RestoreLastError(long) ntdll.RtlRestoreLastWin32Error
@ stdcall ResumeThread(long)
@ stdcall RevertToSelf()
# @ stub RsopLoggingEnabledInternal
# @ stub SHCoCreateInstance
@ stdcall SHExpandEnvironmentStringsA(str ptr long) ExpandEnvironmentStringsA
@ stdcall SHExpandEnvironmentStringsW(wstr ptr long) ExpandEnvironmentStringsW
@ stdcall SHLoadIndirectString(wstr ptr long ptr)
# @ stub SHLoadIndirectStringInternal
@ stdcall SHRegCloseUSKey(ptr)
@ stdcall SHRegCreateUSKeyA(str long long ptr long)
@ stdcall SHRegCreateUSKeyW(wstr long long ptr long)
@ stdcall SHRegDeleteEmptyUSKeyA(long str long)
@ stdcall SHRegDeleteEmptyUSKeyW(long wstr long)
@ stdcall SHRegDeleteUSValueA(long str long)
@ stdcall SHRegDeleteUSValueW(long wstr long)
@ stdcall SHRegEnumUSKeyA(long long str ptr long)
@ stdcall SHRegEnumUSKeyW(long long wstr ptr long)
@ stdcall SHRegEnumUSValueA(long long ptr ptr ptr ptr ptr long)
@ stdcall SHRegEnumUSValueW(long long ptr ptr ptr ptr ptr long)
@ stdcall SHRegGetBoolUSValueA(str str long long)
@ stdcall SHRegGetBoolUSValueW(wstr wstr long long)
@ stdcall SHRegGetUSValueA(str str ptr ptr ptr long ptr long)
@ stdcall SHRegGetUSValueW(wstr wstr ptr ptr ptr long ptr long)
@ stdcall SHRegOpenUSKeyA(str long long ptr long)
@ stdcall SHRegOpenUSKeyW(wstr long long ptr long)
@ stdcall SHRegQueryInfoUSKeyA(long ptr ptr ptr ptr long)
@ stdcall SHRegQueryInfoUSKeyW(long ptr ptr ptr ptr long)
@ stdcall SHRegQueryUSValueA(long str ptr ptr ptr long ptr long)
@ stdcall SHRegQueryUSValueW(long wstr ptr ptr ptr long ptr long)
@ stdcall SHRegSetUSValueA(str str long ptr long long)
@ stdcall SHRegSetUSValueW(wstr wstr long ptr long long)
@ stdcall SHRegWriteUSValueA(long str long ptr long long)
@ stdcall SHRegWriteUSValueW(long wstr long ptr long long)
@ stdcall SHTruncateString(str long)
# @ stub SaveAlternatePackageRootPath
# @ stub SaveStateRootFolderPath
@ stdcall ScrollConsoleScreenBufferA(long ptr ptr ptr ptr)
@ stdcall ScrollConsoleScreenBufferW(long ptr ptr ptr ptr)
@ stdcall SearchPathA(str str str long ptr ptr)
@ stdcall SearchPathW(wstr wstr wstr long ptr ptr)
@ stdcall SetAclInformation(ptr ptr long long)
# @ stub SetCachedSigningLevel
@ stdcall SetCalendarInfoW(long long long wstr) kernel32.SetCalendarInfoW
# @ stub SetClientDynamicTimeZoneInformation
# @ stub SetClientTimeZoneInformation
@ stdcall SetCommBreak(long)
@ stdcall SetCommConfig(long ptr long)
@ stdcall SetCommMask(long long)
@ stdcall SetCommState(long ptr)
@ stdcall SetCommTimeouts(long ptr)
@ stdcall SetComputerNameA(str) kernel32.SetComputerNameA
# @ stub SetComputerNameEx2W
@ stdcall SetComputerNameExA(long str) kernel32.SetComputerNameExA
@ stdcall SetComputerNameExW(long wstr) kernel32.SetComputerNameExW
@ stdcall SetComputerNameW(wstr) kernel32.SetComputerNameW
@ stdcall SetConsoleActiveScreenBuffer(long)
@ stdcall SetConsoleCP(long)
@ stdcall SetConsoleCtrlHandler(ptr long) kernel32.SetConsoleCtrlHandler
@ stdcall SetConsoleCursorInfo(long ptr)
@ stdcall SetConsoleCursorPosition(long long)
@ stdcall SetConsoleInputExeNameA(str)
@ stdcall SetConsoleInputExeNameW(wstr)
@ stdcall SetConsoleMode(long long)
@ stdcall SetConsoleOutputCP(long)
@ stdcall SetConsoleScreenBufferInfoEx(long ptr)
@ stdcall SetConsoleScreenBufferSize(long long)
@ stdcall SetConsoleTextAttribute(long long)
@ stdcall SetConsoleTitleW(wstr)
@ stdcall SetConsoleWindowInfo(long long ptr)
@ stdcall SetCriticalSectionSpinCount(ptr long) ntdll.RtlSetCriticalSectionSpinCount
@ stdcall SetCurrentDirectoryA(str)
@ stdcall SetCurrentDirectoryW(wstr)
@ stdcall SetDefaultDllDirectories(long)
# @ stub SetDynamicTimeZoneInformation
@ stdcall SetEndOfFile(long)
@ stub SetEnvironmentStringsW
@ stdcall SetEnvironmentVariableA(str str)
@ stdcall SetEnvironmentVariableW(wstr wstr)
@ stdcall SetErrorMode(long)
@ stdcall SetEvent(long)
@ stdcall SetEventWhenCallbackReturns(ptr long) ntdll.TpCallbackSetEventOnCompletion
@ stdcall SetFileApisToANSI()
@ stdcall SetFileApisToOEM()
@ stdcall SetFileAttributesA(str long)
@ stdcall SetFileAttributesW(wstr long)
@ stdcall SetFileInformationByHandle(long long ptr long)
# @ stub SetFileIoOverlappedRange
@ stdcall SetFilePointer(long long ptr long)
@ stdcall SetFilePointerEx(long int64 ptr long)
@ stdcall SetFileSecurityW(wstr long ptr)
@ stdcall SetFileTime(long ptr ptr ptr)
@ stdcall SetFileValidData(ptr int64)
@ stdcall SetHandleCount(long)
@ stdcall SetHandleInformation(long long long)
# @ stub SetIsDeveloperModeEnabled
# @ stub SetIsSideloadingEnabled
@ stdcall SetKernelObjectSecurity(long long ptr)
@ stub SetLastConsoleEventActive
@ stdcall SetLastError(long) ntdll.RtlSetLastWin32Error
@ stdcall SetLocalTime(ptr)
@ stdcall SetLocaleInfoW(long long wstr) kernel32.SetLocaleInfoW
@ stdcall SetNamedPipeHandleState(long ptr ptr ptr)
@ stdcall SetPriorityClass(long long)
@ stdcall SetPrivateObjectSecurity(long ptr ptr ptr long)
@ stdcall SetPrivateObjectSecurityEx(long ptr ptr long ptr long)
@ stdcall SetProcessAffinityUpdateMode(long long)
# @ stub SetProcessDefaultCpuSets
# @ stub SetProcessGroupAffinity
# @ stub SetProcessInformation
@ stdcall SetProcessMitigationPolicy(long ptr long)
@ stdcall SetProcessPreferredUILanguages(long ptr ptr) kernel32.SetProcessPreferredUILanguages
@ stdcall SetProcessPriorityBoost(long long)
@ stdcall SetProcessShutdownParameters(long long)
# @ stub SetProcessValidCallTargets
@ stdcall SetProcessWorkingSetSizeEx(long long long long)
# @ stub SetProtectedPolicy
# @ stub SetRoamingLastObservedChangeTime
@ stub SetSecurityAccessMask
@ stdcall SetSecurityDescriptorControl(ptr long long)
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long)
@ stdcall SetSecurityDescriptorGroup(ptr ptr long)
@ stdcall SetSecurityDescriptorOwner(ptr ptr long)
@ stub SetSecurityDescriptorRMControl
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long)
# @ stub SetStateVersion
@ stdcall SetStdHandle(long long)
@ stdcall SetStdHandleEx(long long ptr)
@ stdcall SetSystemFileCacheSize(long long long)
@ stdcall SetSystemTime(ptr)
@ stdcall SetSystemTimeAdjustment(long long) kernel32.SetSystemTimeAdjustment
@ stdcall SetThreadContext(long ptr)
# @ stub SetThreadDescription
@ stdcall SetThreadErrorMode(long ptr)
@ stdcall SetThreadGroupAffinity(long ptr ptr)
@ stdcall SetThreadIdealProcessor(long long)
@ stdcall SetThreadIdealProcessorEx(long ptr ptr)
# @ stub SetThreadInformation
@ stdcall SetThreadLocale(long)
@ stdcall SetThreadPreferredUILanguages(long ptr ptr) kernel32.SetThreadPreferredUILanguages
@ stdcall SetThreadPriority(long long)
@ stdcall SetThreadPriorityBoost(long long)
# @ stub SetThreadSelectedCpuSets
@ stdcall SetThreadStackGuarantee(ptr)
@ stdcall SetThreadToken(ptr ptr)
@ stdcall SetThreadUILanguage(long)
@ stub SetThreadpoolStackInformation
@ stdcall SetThreadpoolThreadMaximum(ptr long) ntdll.TpSetPoolMaxThreads
@ stdcall SetThreadpoolThreadMinimum(ptr long) ntdll.TpSetPoolMinThreads
@ stdcall SetThreadpoolTimer(ptr ptr long long) ntdll.TpSetTimer
# @ stub SetThreadpoolTimerEx
@ stdcall SetThreadpoolWait(ptr long ptr) ntdll.TpSetWait
# @ stub SetThreadpoolWaitEx
@ stdcall SetTimeZoneInformation(ptr) kernel32.SetTimeZoneInformation
@ stdcall SetTokenInformation(long long ptr long)
@ stdcall SetUnhandledExceptionFilter(ptr)
@ stdcall SetUserGeoID(long) kernel32.SetUserGeoID
@ stdcall SetWaitableTimer(long ptr long ptr ptr long)
@ stdcall SetWaitableTimerEx(long ptr long ptr ptr ptr long)
# @ stub SetXStateFeaturesMask
@ stdcall SetupComm(long long long)
# @ stub SharedLocalIsEnabled
@ stdcall SignalObjectAndWait(long long long long)
@ stdcall SizeofResource(long long)
@ stdcall Sleep(long)
@ stdcall SleepConditionVariableCS(ptr ptr long)
@ stdcall SleepConditionVariableSRW(ptr ptr long long)
@ stdcall SleepEx(long long)
@ stub SpecialMBToWC
@ stub StartThreadpoolIo
# @ stub StmAlignSize
# @ stub StmAllocateFlat
# @ stub StmCoalesceChunks
# @ stub StmDeinitialize
# @ stub StmInitialize
# @ stub StmReduceSize
# @ stub StmReserve
# @ stub StmWrite
@ stdcall StrCSpnA(str str)
@ stdcall StrCSpnIA(str str)
@ stdcall StrCSpnIW(wstr wstr)
@ stdcall StrCSpnW(wstr wstr)
@ stdcall StrCatBuffA(str str long)
@ stdcall StrCatBuffW(wstr wstr long)
@ stdcall StrCatChainW(ptr long long wstr)
@ stdcall StrChrA(str long)
# @ stub StrChrA_MB
@ stdcall StrChrIA(str long)
@ stdcall StrChrIW(wstr long)
# @ stub StrChrNIW
@ stdcall StrChrNW(wstr long long)
@ stdcall StrChrW(wstr long)
@ stdcall StrCmpCA(str str)
@ stdcall StrCmpCW(wstr wstr)
@ stdcall StrCmpICA(str str)
@ stdcall StrCmpICW(wstr wstr)
@ stdcall StrCmpIW(wstr wstr)
@ stdcall StrCmpLogicalW(wstr wstr)
@ stdcall StrCmpNA(str str long)
@ stdcall StrCmpNCA(str str long)
@ stdcall StrCmpNCW(wstr wstr long)
@ stdcall StrCmpNIA(str str long)
@ stdcall StrCmpNICA(str str long)
@ stdcall StrCmpNICW(wstr wstr long)
@ stdcall StrCmpNIW(wstr wstr long)
@ stdcall StrCmpNW(wstr wstr long)
@ stdcall StrCmpW(wstr wstr)
@ stdcall StrCpyNW(ptr wstr long)
@ stdcall StrCpyNXA(ptr str long)
@ stdcall StrCpyNXW(ptr wstr long)
@ stdcall StrDupA(str)
@ stdcall StrDupW(wstr)
@ stdcall StrIsIntlEqualA(long str str long)
@ stdcall StrIsIntlEqualW(long wstr wstr long)
@ stdcall StrPBrkA(str str)
@ stdcall StrPBrkW(wstr wstr)
@ stdcall StrRChrA(str str long)
@ stdcall StrRChrIA(str str long)
@ stdcall StrRChrIW(wstr wstr long)
@ stdcall StrRChrW(wstr wstr long)
@ stdcall StrRStrIA(str str str)
@ stdcall StrRStrIW(wstr wstr wstr)
@ stdcall StrSpnA(str str)
@ stdcall StrSpnW(wstr wstr)
@ stdcall StrStrA(str str)
@ stdcall StrStrIA(str str)
@ stdcall StrStrIW(wstr wstr)
@ stdcall StrStrNIW(wstr wstr long)
@ stdcall StrStrNW(wstr wstr long)
@ stdcall StrStrW(wstr wstr)
@ stdcall StrToInt64ExA(str long ptr)
@ stdcall StrToInt64ExW(wstr long ptr)
@ stdcall StrToIntA(str)
@ stdcall StrToIntExA(str long ptr)
@ stdcall StrToIntExW(wstr long ptr)
@ stdcall StrToIntW(wstr)
@ stdcall StrTrimA(str str)
@ stdcall StrTrimW(wstr wstr)
@ stdcall SubmitThreadpoolWork(ptr) ntdll.TpPostWork
# @ stub SubscribeEdpEnabledStateChange
# @ stub SubscribeStateChangeNotification
@ stdcall SuspendThread(long)
@ stdcall SwitchToFiber(ptr)
@ stdcall SwitchToThread()
@ stdcall SystemTimeToFileTime(ptr ptr)
@ stdcall SystemTimeToTzSpecificLocalTime(ptr ptr ptr) kernel32.SystemTimeToTzSpecificLocalTime
@ stub SystemTimeToTzSpecificLocalTimeEx
@ stdcall TerminateProcess(long long)
# @ stub TerminateProcessOnMemoryExhaustion
@ stdcall TerminateThread(long long)
@ stdcall TlsAlloc()
@ stdcall TlsFree(long)
@ stdcall TlsGetValue(long)
@ stdcall TlsSetValue(long ptr)
@ stdcall TraceEvent(int64 ptr) ntdll.EtwLogTraceEvent
@ varargs TraceMessage(int64 long ptr long) ntdll.EtwTraceMessage
@ stdcall TraceMessageVa(int64 long ptr long ptr) ntdll.EtwTraceMessageVa
@ stdcall TransactNamedPipe(long ptr long ptr long ptr ptr)
@ stdcall TransmitCommChar(long long)
@ stdcall TryAcquireSRWLockExclusive(ptr) ntdll.RtlTryAcquireSRWLockExclusive
@ stdcall TryAcquireSRWLockShared(ptr) ntdll.RtlTryAcquireSRWLockShared
@ stdcall TryEnterCriticalSection(ptr) ntdll.RtlTryEnterCriticalSection
@ stdcall TrySubmitThreadpoolCallback(ptr ptr ptr)
@ stdcall TzSpecificLocalTimeToSystemTime(ptr ptr ptr) kernel32.TzSpecificLocalTimeToSystemTime
@ stub TzSpecificLocalTimeToSystemTimeEx
@ stdcall UnhandledExceptionFilter(ptr)
@ stdcall UnlockFile(long long long long long)
@ stdcall UnlockFileEx(long long long long ptr)
@ stdcall UnmapViewOfFile(ptr)
# @ stub UnmapViewOfFileEx
# @ stub UnregisterBadMemoryNotification
# @ stub UnregisterGPNotificationInternal
# @ stub UnregisterStateChangeNotification
# @ stub UnregisterStateLock
@ stdcall UnregisterTraceGuids(int64) ntdll.EtwUnregisterTraceGuids
@ stdcall UnregisterWaitEx(long long)
# @ stub UnsubscribeEdpEnabledStateChange
# @ stub UnsubscribeStateChangeNotification
# @ stub UpdatePackageStatus
# @ stub UpdatePackageStatusForUser
@ stdcall UpdateProcThreadAttribute(ptr long long ptr long ptr ptr)
@ stdcall UrlApplySchemeA(str ptr ptr long)
@ stdcall UrlApplySchemeW(wstr ptr ptr long)
@ stdcall UrlCanonicalizeA(str ptr ptr long)
@ stdcall UrlCanonicalizeW(wstr ptr ptr long)
@ stdcall UrlCombineA(str str ptr ptr long)
@ stdcall UrlCombineW(wstr wstr ptr ptr long)
@ stdcall UrlCompareA(str str long)
@ stdcall UrlCompareW(wstr wstr long)
@ stdcall UrlCreateFromPathA(str ptr ptr long)
@ stdcall UrlCreateFromPathW(wstr ptr ptr long)
@ stdcall UrlEscapeA(str ptr ptr long)
@ stdcall UrlEscapeW(wstr ptr ptr long)
@ stdcall UrlFixupW(wstr wstr long)
@ stdcall UrlGetLocationA(str)
@ stdcall UrlGetLocationW(wstr)
@ stdcall UrlGetPartA(str ptr ptr long long)
@ stdcall UrlGetPartW(wstr ptr ptr long long)
@ stdcall UrlHashA(str ptr long)
@ stdcall UrlHashW(wstr ptr long)
@ stdcall UrlIsA(str long)
@ stdcall UrlIsNoHistoryA(str)
@ stdcall UrlIsNoHistoryW(wstr)
@ stdcall UrlIsOpaqueA(str)
@ stdcall UrlIsOpaqueW(wstr)
@ stdcall UrlIsW(wstr long)
@ stdcall UrlUnescapeA(str ptr ptr long)
@ stdcall UrlUnescapeW(wstr ptr ptr long)
@ stdcall VerFindFileA(long str str str ptr ptr ptr ptr)
@ stdcall VerFindFileW(long wstr wstr wstr ptr ptr ptr ptr)
@ stdcall VerLanguageNameA(long str long)
@ stdcall VerLanguageNameW(long wstr long)
@ stdcall VerQueryValueA(ptr str ptr ptr)
@ stdcall VerQueryValueW(ptr wstr ptr ptr)
@ stdcall -ret64 VerSetConditionMask(long long long long) ntdll.VerSetConditionMask
# @ stub VerifyApplicationUserModelId
# @ stub VerifyPackageFamilyName
# @ stub VerifyPackageFullName
# @ stub VerifyPackageId
# @ stub VerifyPackageRelativeApplicationId
# @ stub VerifyScripts
@ stdcall VirtualAlloc(ptr long long long)
@ stdcall VirtualAllocEx(long ptr long long long)
@ stdcall VirtualAllocExNuma(long ptr long long long long)
# @ stub VirtualAllocFromApp
@ stdcall VirtualFree(ptr long long)
@ stdcall VirtualFreeEx(long ptr long long)
@ stdcall VirtualLock(ptr long)
@ stdcall VirtualProtect(ptr long long ptr)
@ stdcall VirtualProtectEx(long ptr long long ptr)
# @ stub VirtualProtectFromApp
@ stdcall VirtualQuery(ptr ptr long)
@ stdcall VirtualQueryEx(long ptr ptr long)
@ stdcall VirtualUnlock(ptr long)
# @ stub WTSGetServiceSessionId
# @ stub WTSIsServerContainer
@ stdcall WaitCommEvent(long ptr ptr)
@ stdcall WaitForDebugEvent(ptr long)
# @ stub WaitForDebugEventEx
# @ stub WaitForMachinePolicyForegroundProcessingInternal
@ stdcall WaitForMultipleObjects(long ptr long long)
@ stdcall WaitForMultipleObjectsEx(long ptr long long long)
@ stdcall WaitForSingleObject(long long)
@ stdcall WaitForSingleObjectEx(long long long)
@ stub WaitForThreadpoolIoCallbacks
@ stdcall WaitForThreadpoolTimerCallbacks(ptr long) ntdll.TpWaitForTimer
@ stdcall WaitForThreadpoolWaitCallbacks(ptr long) ntdll.TpWaitForWait
@ stdcall WaitForThreadpoolWorkCallbacks(ptr long) ntdll.TpWaitForWork
# @ stub WaitForUserPolicyForegroundProcessingInternal
@ stdcall WaitNamedPipeW(wstr long)
@ stdcall WaitOnAddress(ptr ptr long long)
@ stdcall WakeAllConditionVariable(ptr) ntdll.RtlWakeAllConditionVariable
@ stdcall WakeByAddressAll(ptr) ntdll.RtlWakeAddressAll
@ stdcall WakeByAddressSingle(ptr) ntdll.RtlWakeAddressSingle
@ stdcall WakeConditionVariable(ptr) ntdll.RtlWakeConditionVariable
# @ stub WerGetFlags
@ stdcall WerRegisterFile(wstr long long) kernel32.WerRegisterFile
@ stdcall WerRegisterMemoryBlock(ptr long) kernel32.WerRegisterMemoryBlock
@ stdcall WerRegisterRuntimeExceptionModule(wstr ptr) kernel32.WerRegisterRuntimeExceptionModule
@ stdcall WerSetFlags(long) kernel32.WerSetFlags
# @ stub WerUnregisterFile
@ stdcall WerUnregisterMemoryBlock(ptr) kernel32.WerUnregisterMemoryBlock
@ stdcall WerUnregisterRuntimeExceptionModule(wstr ptr) kernel32.WerUnregisterRuntimeExceptionModule
# @ stub WerpNotifyLoadStringResource
# @ stub WerpNotifyUseStringResource
@ stdcall WideCharToMultiByte(long long wstr long ptr long ptr ptr) kernel32.WideCharToMultiByte
@ stdcall Wow64DisableWow64FsRedirection(ptr)
@ stdcall Wow64RevertWow64FsRedirection(ptr)
# @ stub Wow64SetThreadDefaultGuestMachine
# @ stub -arch=i386 Wow64Transition
@ stdcall WriteConsoleA(long ptr long ptr ptr)
@ stdcall WriteConsoleInputA(long ptr long ptr)
@ stdcall WriteConsoleInputW(long ptr long ptr)
@ stdcall WriteConsoleOutputA(long ptr long long ptr)
@ stdcall WriteConsoleOutputAttribute(long ptr long long ptr)
@ stdcall WriteConsoleOutputCharacterA(long ptr long long ptr)
@ stdcall WriteConsoleOutputCharacterW(long ptr long long ptr)
@ stdcall WriteConsoleOutputW(long ptr long long ptr)
@ stdcall WriteConsoleW(long ptr long ptr ptr) kernel32.WriteConsoleW
@ stdcall WriteFile(long ptr long ptr ptr)
@ stdcall WriteFileEx(long ptr long ptr ptr)
@ stdcall WriteFileGather(long ptr long ptr ptr)
@ stdcall WriteProcessMemory(long ptr ptr long ptr)
# @ stub WriteStateAtomValue
# @ stub WriteStateContainerValue
@ stdcall ZombifyActCtx(ptr)
# @ stub _AddMUIStringToCache
# @ stub _GetMUIStringFromCache
# @ stub _OpenMuiStringCache
@ stdcall -arch=x86_64 -private __C_specific_handler(ptr long ptr ptr) ntdll.__C_specific_handler
@ stdcall -arch=arm,x86_64 -private -norelay __chkstk() ntdll.__chkstk
# @ stub __dllonexit3
@ stub __misaligned_access
# @ stub __wgetmainargs
# @ stub _amsg_exit
# @ stub _c_exit
# @ stub _cexit
# @ stub _exit
# @ stub _initterm
# @ stub _initterm_e
# @ stub _invalid_parameter
@ stdcall -arch=x86_64 -private _local_unwind(ptr ptr) ntdll._local_unwind
# @ stub _onexit
# @ stub _purecall
# @ stub _time64
# @ stub atexit
# @ stub exit
# @ stub hgets
# @ stub hwprintf
@ stdcall lstrcmp(str str) lstrcmpA
@ stdcall lstrcmpA(str str)
@ stdcall lstrcmpW(wstr wstr)
@ stdcall lstrcmpi(str str) lstrcmpiA
@ stdcall lstrcmpiA(str str)
@ stdcall lstrcmpiW(wstr wstr)
@ stdcall lstrcpyn(ptr str long) KERNELBASE_lstrcpynA
@ stdcall lstrcpynA(ptr str long) KERNELBASE_lstrcpynA
@ stdcall lstrcpynW(ptr wstr long) KERNELBASE_lstrcpynW
@ stdcall lstrlen(str) KERNELBASE_lstrlenA
@ stdcall lstrlenA(str) KERNELBASE_lstrlenA
@ stdcall lstrlenW(wstr) KERNELBASE_lstrlenW
# @ stub time
# @ stub wprintf
