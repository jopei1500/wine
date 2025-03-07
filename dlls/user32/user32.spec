@ stdcall ActivateKeyboardLayout(long long)
@ stdcall AddClipboardFormatListener(long)
@ stdcall AdjustWindowRect(ptr long long)
@ stdcall AdjustWindowRectEx(ptr long long long)
@ stdcall AdjustWindowRectExForDpi(ptr long long long long)
@ stdcall AlignRects(ptr long long long)
# @ stub AllowForegroundActivation
@ stdcall AllowSetForegroundWindow (long)
@ stdcall AnimateWindow(long long long)
@ stdcall AnyPopup()
@ stdcall AppendMenuA(long long long ptr)
@ stdcall AppendMenuW(long long long ptr)
@ stdcall AreDpiAwarenessContextsEqual(long long)
@ stdcall ArrangeIconicWindows(long)
@ stdcall AttachThreadInput(long long long)
@ stdcall BeginDeferWindowPos(long)
@ stdcall BeginPaint(long ptr)
@ stdcall BlockInput(long)
@ stdcall BringWindowToTop(long)
@ stdcall BroadcastSystemMessage(long ptr long long long) BroadcastSystemMessageA
@ stdcall BroadcastSystemMessageA(long ptr long long long)
@ stdcall BroadcastSystemMessageExA(long ptr long long long ptr)
@ stdcall BroadcastSystemMessageExW(long ptr long long long ptr)
@ stdcall BroadcastSystemMessageW(long ptr long long long)
# @ stub BuildReasonArray
@ stdcall CalcChildScroll(long long)
@ stdcall CalcMenuBar(long long long long ptr) CalcMenuBar
@ stdcall CallMsgFilter(ptr long) CallMsgFilterA
@ stdcall CallMsgFilterA(ptr long)
@ stdcall CallMsgFilterW(ptr long)
@ stdcall CallNextHookEx(long long long long)
@ stdcall CallWindowProcA(ptr long long long long)
@ stdcall CallWindowProcW(ptr long long long long)
@ stdcall CascadeChildWindows(long long)
@ stdcall CascadeWindows(long long ptr long ptr)
@ stdcall ChangeClipboardChain(long long)
@ stdcall ChangeDisplaySettingsA(ptr long)
@ stdcall ChangeDisplaySettingsExA(str ptr long long ptr)
@ stdcall ChangeDisplaySettingsExW(wstr ptr long long ptr)
@ stdcall ChangeDisplaySettingsW(ptr long)
@ stdcall ChangeMenuA(long long ptr long long)
@ stdcall ChangeMenuW(long long ptr long long)
@ stdcall ChangeWindowMessageFilter(long long)
@ stdcall ChangeWindowMessageFilterEx(long long long ptr)
@ stdcall CharLowerA(str)
@ stdcall CharLowerBuffA(str long)
@ stdcall CharLowerBuffW(wstr long)
@ stdcall CharLowerW(wstr)
@ stdcall CharNextA(str)
@ stdcall CharNextExA(long str long)
@ stdcall CharNextExW(long wstr long)
@ stdcall CharNextW(wstr)
@ stdcall CharPrevA(str str)
@ stdcall CharPrevExA(long str str long)
@ stdcall CharPrevExW(long wstr wstr long)
@ stdcall CharPrevW(wstr wstr)
@ stdcall CharToOemA(str ptr)
@ stdcall CharToOemBuffA(str ptr long)
@ stdcall CharToOemBuffW(wstr ptr long)
@ stdcall CharToOemW(wstr ptr)
@ stdcall CharUpperA(str)
@ stdcall CharUpperBuffA(str long)
@ stdcall CharUpperBuffW(wstr long)
@ stdcall CharUpperW(wstr)
@ stdcall CheckDlgButton(long long long)
@ stdcall CheckMenuItem(long long long)
@ stdcall CheckMenuRadioItem(long long long long long)
@ stdcall CheckRadioButton(long long long long)
@ stdcall ChildWindowFromPoint(long int64)
@ stdcall ChildWindowFromPointEx(long int64 long)
@ stub CliImmSetHotKey
@ stub ClientThreadConnect
@ stub ClientThreadSetup
@ stdcall ClientToScreen(long ptr)
@ stdcall ClipCursor(ptr)
@ stdcall CloseClipboard()
@ stdcall CloseDesktop(long)
@ stdcall CloseTouchInputHandle(long)
@ stdcall CloseWindow(long)
@ stdcall CloseWindowStation(long)
@ stdcall CopyAcceleratorTableA(long ptr long)
@ stdcall CopyAcceleratorTableW(long ptr long)
@ stdcall CopyIcon(long)
@ stdcall CopyImage(long long long long long)
@ stdcall CopyRect(ptr ptr)
@ stdcall CountClipboardFormats()
@ stdcall CreateAcceleratorTableA(ptr long)
@ stdcall CreateAcceleratorTableW(ptr long)
@ stdcall CreateCaret(long long long long)
@ stdcall CreateCursor(long long long long long ptr ptr)
@ stdcall CreateDesktopA(str str ptr long long ptr)
@ stdcall CreateDesktopW(wstr wstr ptr long long ptr)
@ stdcall CreateDialogIndirectParamA(long ptr long ptr long)
@ stdcall CreateDialogIndirectParamAorW(long ptr long ptr long long)
@ stdcall CreateDialogIndirectParamW(long ptr long ptr long)
@ stdcall CreateDialogParamA(long str long ptr long)
@ stdcall CreateDialogParamW(long wstr long ptr long)
@ stdcall CreateIcon(long long long long long ptr ptr)
@ stdcall CreateIconFromResource (ptr long long long)
@ stdcall CreateIconFromResourceEx(ptr long long long long long long)
@ stdcall CreateIconIndirect(ptr)
@ stdcall CreateMDIWindowA(str str long long long long long long long long)
@ stdcall CreateMDIWindowW(wstr wstr long long long long long long long long)
@ stdcall CreateMenu()
@ stdcall CreatePopupMenu()
# @ stub CreateSystemThreads
@ stdcall CreateWindowExA(long str str long long long long long long long long ptr)
@ stdcall CreateWindowExW(long wstr wstr long long long long long long long long ptr)
@ stdcall CreateWindowStationA(str long long ptr)
@ stdcall CreateWindowStationW(wstr long long ptr)
# @ stub CsrBroadcastSystemMessageExW
# @ stub CtxInitUser32
@ stdcall DdeAbandonTransaction(long long long)
@ stdcall DdeAccessData(long ptr)
@ stdcall DdeAddData(long ptr long long)
@ stdcall DdeClientTransaction(ptr long long long long long long ptr)
@ stdcall DdeCmpStringHandles(long long)
@ stdcall DdeConnect(long long long ptr)
@ stdcall DdeConnectList(long long long long ptr)
@ stdcall DdeCreateDataHandle(long ptr long long long long long)
@ stdcall DdeCreateStringHandleA(long str long)
@ stdcall DdeCreateStringHandleW(long wstr long)
@ stdcall DdeDisconnect(long)
@ stdcall DdeDisconnectList(long)
@ stdcall DdeEnableCallback(long long long)
@ stdcall DdeFreeDataHandle(long)
@ stdcall DdeFreeStringHandle(long long)
@ stdcall DdeGetData(long ptr long long)
@ stdcall DdeGetLastError(long)
@ stub DdeGetQualityOfService
@ stdcall DdeImpersonateClient(long)
@ stdcall DdeInitializeA(ptr ptr long long)
@ stdcall DdeInitializeW(ptr ptr long long)
@ stdcall DdeKeepStringHandle(long long)
@ stdcall DdeNameService(long long long long)
@ stdcall DdePostAdvise(long long long)
@ stdcall DdeQueryConvInfo(long long ptr)
@ stdcall DdeQueryNextServer(long long)
@ stdcall DdeQueryStringA(long long ptr long long)
@ stdcall DdeQueryStringW(long long ptr long long)
@ stdcall DdeReconnect(long)
@ stdcall DdeSetQualityOfService(long ptr ptr)
@ stdcall DdeSetUserHandle (long long long)
@ stdcall DdeUnaccessData(long)
@ stdcall DdeUninitialize(long)
@ stdcall DefDlgProcA(long long long long)
@ stdcall DefDlgProcW(long long long long)
@ stdcall DefFrameProcA(long long long long long)
@ stdcall DefFrameProcW(long long long long long)
@ stdcall DefMDIChildProcA(long long long long)
@ stdcall DefMDIChildProcW(long long long long)
@ stdcall DefRawInputProc(ptr long long)
@ stdcall DefWindowProcA(long long long long)
@ stdcall DefWindowProcW(long long long long)
@ stdcall DeferWindowPos(long long long long long long long long)
@ stdcall DeleteMenu(long long long)
@ stdcall DeregisterShellHookWindow (long)
@ stdcall DestroyAcceleratorTable(long)
@ stdcall DestroyCaret()
@ stdcall DestroyCursor(long)
@ stdcall DestroyIcon(long)
@ stdcall DestroyMenu(long)
# @ stub DestroyReasons
@ stdcall DestroyWindow(long)
# @ stub DeviceEventWorker
@ stdcall DialogBoxIndirectParamA(long ptr long ptr long)
@ stdcall DialogBoxIndirectParamAorW(long ptr long ptr long long)
@ stdcall DialogBoxIndirectParamW(long ptr long ptr long)
@ stdcall DialogBoxParamA(long str long ptr long)
@ stdcall DialogBoxParamW(long wstr long ptr long)
@ stdcall DisableProcessWindowsGhosting()
@ stdcall DispatchMessageA(ptr)
@ stdcall DispatchMessageW(ptr)
@ stdcall DisplayConfigGetDeviceInfo(ptr)
# @ stub DisplayExitWindowsWarnings
@ stdcall DlgDirListA(long str long long long)
@ stdcall DlgDirListComboBoxA(long ptr long long long)
@ stdcall DlgDirListComboBoxW(long ptr long long long)
@ stdcall DlgDirListW(long wstr long long long)
@ stdcall DlgDirSelectComboBoxExA(long ptr long long)
@ stdcall DlgDirSelectComboBoxExW(long ptr long long)
@ stdcall DlgDirSelectExA(long ptr long long)
@ stdcall DlgDirSelectExW(long ptr long long)
@ stdcall DragDetect(long int64)
@ stub DragObject
@ stdcall DrawAnimatedRects(long long ptr ptr)
@ stdcall DrawCaption(long long ptr long)
@ stdcall DrawCaptionTempA(long long ptr long long str long)
@ stdcall DrawCaptionTempW(long long ptr long long wstr long)
@ stdcall DrawEdge(long ptr long long)
@ stdcall DrawFocusRect(long ptr)
@ stub DrawFrame
@ stdcall DrawFrameControl(long ptr long long)
@ stdcall DrawIcon(long long long long)
@ stdcall DrawIconEx(long long long long long long long long long)
@ stdcall DrawMenuBar(long)
@ stdcall DrawMenuBarTemp(long long ptr long long)
@ stdcall DrawStateA(long long ptr long long long long long long long)
@ stdcall DrawStateW(long long ptr long long long long long long long)
@ stdcall DrawTextA(long str long ptr long)
@ stdcall DrawTextExA(long str long ptr long ptr)
@ stdcall DrawTextExW(long wstr long ptr long ptr)
@ stdcall DrawTextW(long wstr long ptr long)
@ stdcall EditWndProc(long long long long) EditWndProcA
@ stdcall EmptyClipboard()
@ stdcall EnableMenuItem(long long long)
@ stdcall EnableMouseInPointer(long)
@ stdcall EnableScrollBar(long long long)
@ stdcall EnableWindow(long long)
@ stdcall EndDeferWindowPos(long)
@ stdcall EndDialog(long long)
@ stdcall EndMenu()
@ stdcall EndPaint(long ptr)
@ stub EndTask
# @ stub EnterReaderModeHelper
@ stdcall EnumChildWindows(long ptr long)
@ stdcall EnumClipboardFormats(long)
@ stdcall EnumDesktopWindows(long ptr ptr)
@ stdcall EnumDesktopsA(ptr ptr long)
@ stdcall EnumDesktopsW(ptr ptr long)
@ stub EnumDisplayDeviceModesA
@ stub EnumDisplayDeviceModesW
@ stdcall EnumDisplayDevicesA(ptr long ptr long)
@ stdcall EnumDisplayDevicesW(ptr long ptr long)
@ stdcall EnumDisplayMonitors(long ptr ptr long)
@ stdcall EnumDisplaySettingsA(str long ptr)
@ stdcall EnumDisplaySettingsExA(str long ptr long)
@ stdcall EnumDisplaySettingsExW(wstr long ptr long)
@ stdcall EnumDisplaySettingsW(wstr long ptr )
@ stdcall EnumPropsA(long ptr)
@ stdcall EnumPropsExA(long ptr long)
@ stdcall EnumPropsExW(long ptr long)
@ stdcall EnumPropsW(long ptr)
@ stdcall EnumThreadWindows(long ptr long)
@ stdcall EnumWindowStationsA(ptr long)
@ stdcall EnumWindowStationsW(ptr long)
@ stdcall EnumWindows(ptr long)
@ stdcall EqualRect(ptr ptr)
@ stdcall ExcludeUpdateRgn(long long)
@ stdcall ExitWindowsEx(long long)
@ stdcall FillRect(long ptr long)
@ stdcall FindWindowA(str str)
@ stdcall FindWindowExA(long long str str)
@ stdcall FindWindowExW(long long wstr wstr)
@ stdcall FindWindowW(wstr wstr)
@ stdcall FlashWindow(long long)
@ stdcall FlashWindowEx(ptr)
@ stdcall FrameRect(long ptr long)
@ stdcall FreeDDElParam(long long)
@ stdcall GetActiveWindow()
@ stdcall GetAltTabInfo(long long ptr ptr long) GetAltTabInfoA
@ stdcall GetAltTabInfoA(long long ptr ptr long)
@ stdcall GetAltTabInfoW(long long ptr ptr long)
@ stdcall GetAncestor(long long)
@ stdcall GetAppCompatFlags(long)
@ stdcall GetAppCompatFlags2(long)
@ stdcall GetAsyncKeyState(long)
@ stdcall GetAutoRotationState(ptr)
@ stdcall GetAwarenessFromDpiAwarenessContext(long)
@ stdcall GetCapture()
@ stdcall GetCaretBlinkTime()
@ stdcall GetCaretPos(ptr)
@ stdcall GetClassInfoA(long str ptr)
@ stdcall GetClassInfoExA(long str ptr)
@ stdcall GetClassInfoExW(long wstr ptr)
@ stdcall GetClassInfoW(long wstr ptr)
@ stdcall GetClassLongA(long long)
@ stdcall GetClassLongW(long long)
@ stdcall -arch=win64 GetClassLongPtrA(long long)
@ stdcall -arch=win64 GetClassLongPtrW(long long)
@ stdcall GetClassNameA(long ptr long)
@ stdcall GetClassNameW(long ptr long)
@ stdcall GetClassWord(long long)
@ stdcall GetClientRect(long long)
@ stdcall GetClipCursor(ptr)
@ stdcall GetClipboardData(long)
@ stdcall GetClipboardFormatNameA(long ptr long)
@ stdcall GetClipboardFormatNameW(long ptr long)
@ stdcall GetClipboardOwner()
@ stdcall GetClipboardSequenceNumber ()
@ stdcall GetClipboardViewer()
@ stdcall GetComboBoxInfo(long ptr)
@ stdcall GetCurrentInputMessageSource(ptr)
@ stdcall GetCursor()
@ stdcall GetCursorFrameInfo(long long long ptr ptr)
@ stdcall GetCursorInfo(ptr)
@ stdcall GetCursorPos(ptr)
@ stdcall GetDC(long)
@ stdcall GetDCEx(long long long)
@ stdcall GetDesktopWindow()
@ stdcall GetDialogBaseUnits()
@ stdcall GetDisplayAutoRotationPreferences(ptr)
@ stdcall GetDisplayConfigBufferSizes(long ptr ptr)
@ stdcall GetDlgCtrlID(long)
@ stdcall GetDlgItem(long long)
@ stdcall GetDlgItemInt(long long ptr long)
@ stdcall GetDlgItemTextA(long long ptr long)
@ stdcall GetDlgItemTextW(long long ptr long)
@ stdcall GetDoubleClickTime()
@ stdcall GetDpiForMonitorInternal(long long ptr ptr)
@ stdcall GetDpiForSystem()
@ stdcall GetDpiForWindow(long)
@ stdcall GetFocus()
@ stdcall GetForegroundWindow()
@ stdcall GetGestureConfig(long long long ptr ptr long)
@ stdcall GetGestureInfo(long ptr)
@ stdcall GetGUIThreadInfo(long ptr)
@ stdcall GetGuiResources(long long)
@ stdcall GetIconInfo(long ptr)
@ stdcall GetIconInfoExA(long ptr)
@ stdcall GetIconInfoExW(long ptr)
@ stub GetInputDesktop
@ stdcall GetInputState()
@ stdcall GetInternalWindowPos(long ptr ptr)
@ stdcall GetKBCodePage()
@ stdcall GetKeyNameTextA(long ptr long)
@ stdcall GetKeyNameTextW(long ptr long)
@ stdcall GetKeyState(long)
@ stdcall GetKeyboardLayout(long)
@ stdcall GetKeyboardLayoutList(long ptr)
@ stdcall GetKeyboardLayoutNameA(ptr)
@ stdcall GetKeyboardLayoutNameW(ptr)
@ stdcall GetKeyboardState(ptr)
@ stdcall GetKeyboardType(long)
@ stdcall GetLastActivePopup(long)
@ stdcall GetLastInputInfo(ptr)
@ stdcall GetLayeredWindowAttributes(long ptr ptr ptr)
@ stdcall GetListBoxInfo(long)
@ stdcall GetMenu(long)
@ stdcall GetMenuBarInfo(long long long ptr)
@ stdcall GetMenuCheckMarkDimensions()
@ stdcall GetMenuContextHelpId(long)
@ stdcall GetMenuDefaultItem(long long long)
@ stub GetMenuIndex
@ stdcall GetMenuInfo(long ptr)
@ stdcall GetMenuItemCount(long)
@ stdcall GetMenuItemID(long long)
@ stdcall GetMenuItemInfoA(long long long ptr)
@ stdcall GetMenuItemInfoW(long long long ptr)
@ stdcall GetMenuItemRect(long long long ptr)
@ stdcall GetMenuState(long long long)
@ stdcall GetMenuStringA(long long ptr long long)
@ stdcall GetMenuStringW(long long ptr long long)
@ stdcall GetMessageA(ptr long long long)
@ stdcall GetMessageExtraInfo()
@ stdcall GetMessagePos()
@ stdcall GetMessageTime()
@ stdcall GetMessageW(ptr long long long)
@ stdcall GetMonitorInfoA(long ptr)
@ stdcall GetMonitorInfoW(long ptr)
@ stdcall GetMouseMovePointsEx(long ptr ptr long long)
@ stdcall GetNextDlgGroupItem(long long long)
@ stdcall GetNextDlgTabItem(long long long)
# @ stub GetNextQueueWindow
@ stdcall GetOpenClipboardWindow()
@ stdcall GetParent(long)
@ stdcall GetPhysicalCursorPos(ptr)
@ stdcall GetPointerDevices(ptr ptr)
@ stdcall GetPointerType(long ptr)
@ stdcall GetPriorityClipboardFormat(ptr long)
@ stdcall GetProcessDefaultLayout(ptr)
@ stdcall GetProcessDpiAwarenessInternal(long ptr)
@ stdcall GetProcessWindowStation()
@ stdcall GetProgmanWindow ()
@ stdcall GetPropA(long str)
@ stdcall GetPropW(long wstr)
@ stdcall GetQueueStatus(long)
@ stdcall GetRawInputBuffer(ptr ptr long)
@ stdcall GetRawInputData(ptr long ptr ptr long)
@ stdcall GetRawInputDeviceInfoA(ptr long ptr ptr)
@ stdcall GetRawInputDeviceInfoW(ptr long ptr ptr)
@ stdcall GetRawInputDeviceList(ptr ptr long)
# @ stub GetReasonTitleFromReasonCode
@ stdcall GetRegisteredRawInputDevices(ptr ptr long)
@ stdcall GetScrollBarInfo(long long ptr)
@ stdcall GetScrollInfo(long long ptr)
@ stdcall GetScrollPos(long long)
@ stdcall GetScrollRange(long long ptr ptr)
@ stdcall GetShellWindow()
@ stdcall GetSubMenu(long long)
@ stdcall GetSysColor(long)
@ stdcall GetSysColorBrush(long)
@ stdcall GetSystemMenu(long long)
@ stdcall GetSystemMetrics(long)
@ stdcall GetSystemMetricsForDpi(long long)
@ stdcall GetTabbedTextExtentA(long str long long ptr)
@ stdcall GetTabbedTextExtentW(long wstr long long ptr)
@ stdcall GetTaskmanWindow ()
@ stdcall GetThreadDesktop(long)
@ stdcall GetThreadDpiAwarenessContext()
@ stdcall GetTitleBarInfo(long ptr)
@ stdcall GetTopWindow(long)
@ stdcall GetTouchInputInfo(long long ptr long)
@ stdcall GetUpdateRect(long ptr long)
@ stdcall GetUpdateRgn(long long long)
@ stdcall GetUpdatedClipboardFormats(ptr long ptr)
@ stdcall GetUserObjectInformationA (long long ptr long ptr)
@ stdcall GetUserObjectInformationW (long long ptr long ptr)
@ stdcall GetUserObjectSecurity (long ptr ptr long ptr)
# @ stub GetWinStationInfo
@ stdcall GetWindow(long long)
@ stdcall GetWindowContextHelpId(long)
@ stdcall GetWindowDC(long)
@ stdcall GetWindowDisplayAffinity(long ptr)
@ stdcall GetWindowDpiAwarenessContext(long)
@ stdcall GetWindowInfo(long ptr)
@ stdcall GetWindowLongA(long long)
@ stdcall -arch=win64 GetWindowLongPtrA(long long)
@ stdcall -arch=win64 GetWindowLongPtrW(long long)
@ stdcall GetWindowLongW(long long)
@ stdcall GetWindowModuleFileName(long ptr long) GetWindowModuleFileNameA
@ stdcall GetWindowModuleFileNameA(long ptr long)
@ stdcall GetWindowModuleFileNameW(long ptr long)
@ stdcall GetWindowPlacement(long ptr)
@ stdcall GetWindowRect(long ptr)
@ stdcall GetWindowRgn(long long)
@ stdcall GetWindowRgnBox(long ptr)
@ stdcall GetWindowTextA(long ptr long)
@ stdcall GetWindowTextLengthA(long)
@ stdcall GetWindowTextLengthW(long)
@ stdcall GetWindowTextW(long ptr long)
@ stdcall GetWindowThreadProcessId(long ptr)
@ stdcall GetWindowWord(long long)
@ stdcall GrayStringA(long long ptr long long long long long long)
@ stdcall GrayStringW(long long ptr long long long long long long)
# @ stub HasSystemSleepStarted
@ stdcall HideCaret(long)
@ stdcall HiliteMenuItem(long long long long)
# @ stub IMPGetIMEA
# @ stub IMPGetIMEW
# @ stub IMPQueryIMEA
# @ stub IMPQueryIMEW
# @ stub IMPSetIMEA
# @ stub IMPSetIMEW
@ stdcall ImpersonateDdeClientWindow(long long)
@ stdcall InSendMessage()
@ stdcall InSendMessageEx(ptr)
@ stdcall InflateRect(ptr long long)
# @ stub InitSharedTable
# @ stub InitTask
# @ stub InitializeLpkHooks
# @ stub InitializeWin32EntryTable
@ stdcall InsertMenuA(long long long long ptr)
@ stdcall InsertMenuItemA(long long long ptr)
@ stdcall InsertMenuItemW(long long long ptr)
@ stdcall InsertMenuW(long long long long ptr)
@ stdcall InternalGetWindowIcon(long long)
@ stdcall InternalGetWindowText(long long long)
@ stdcall IntersectRect(ptr ptr ptr)
@ stdcall InvalidateRect(long ptr long)
@ stdcall InvalidateRgn(long long long)
@ stdcall InvertRect(long ptr)
@ stdcall IsCharAlphaA(long)
@ stdcall IsCharAlphaNumericA(long)
@ stdcall IsCharAlphaNumericW(long)
@ stdcall IsCharAlphaW(long)
@ stdcall IsCharLowerA(long)
@ stdcall IsCharLowerW(long)
@ stdcall IsCharUpperA(long)
@ stdcall IsCharUpperW(long)
@ stdcall IsChild(long long)
@ stdcall IsClipboardFormatAvailable(long)
@ stdcall IsDialogMessage(long ptr) IsDialogMessageA
@ stdcall IsDialogMessageA(long ptr)
@ stdcall IsDialogMessageW(long ptr)
@ stdcall IsDlgButtonChecked(long long)
@ stdcall IsGUIThread(long)
@ stdcall IsHungAppWindow(long)
# @ stub IsHungThread
@ stdcall IsIconic(long)
@ stdcall IsMenu(long)
@ stdcall IsProcessDPIAware()
@ stdcall IsRectEmpty(ptr)
# @ stub IsServerSideWindow
@ stdcall IsTouchWindow(long ptr)
@ stdcall IsValidDpiAwarenessContext(long)
@ stdcall IsWinEventHookInstalled(long)
@ stdcall IsWindow(long)
@ stdcall IsWindowEnabled(long)
# @ stub IsWindowInDestroy
@ stdcall IsWindowRedirectedForPrint(long)
@ stdcall IsWindowUnicode(long)
@ stdcall IsWindowVisible(long)
@ stdcall IsZoomed(long)
@ stdcall KillSystemTimer(long long)
@ stdcall KillTimer(long long)
@ stdcall LoadAcceleratorsA(long str)
@ stdcall LoadAcceleratorsW(long wstr)
@ stdcall LoadBitmapA(long str)
@ stdcall LoadBitmapW(long wstr)
@ stdcall LoadCursorA(long str)
@ stdcall LoadCursorFromFileA(str)
@ stdcall LoadCursorFromFileW(wstr)
@ stdcall LoadCursorW(long wstr)
@ stdcall LoadIconA(long str)
@ stdcall LoadIconW(long wstr)
@ stdcall LoadImageA(long str long long long long)
@ stdcall LoadImageW(long wstr long long long long)
@ stdcall LoadKeyboardLayoutA(str long)
# @ stub LoadKeyboardLayoutEx
@ stdcall LoadKeyboardLayoutW(wstr long)
@ stdcall LoadLocalFonts()
@ stdcall LoadMenuA(long str)
@ stdcall LoadMenuIndirectA(ptr)
@ stdcall LoadMenuIndirectW(ptr)
@ stdcall LoadMenuW(long wstr)
@ stub LoadRemoteFonts
@ stdcall LoadStringA(long long ptr long)
@ stdcall LoadStringW(long long ptr long)
@ stdcall LockSetForegroundWindow (long)
@ stub LockWindowStation
@ stdcall LockWindowUpdate(long)
@ stdcall LockWorkStation()
@ stdcall LogicalToPhysicalPoint(long ptr)
@ stdcall LogicalToPhysicalPointForPerMonitorDPI(long ptr)
@ stdcall LookupIconIdFromDirectory(ptr long)
@ stdcall LookupIconIdFromDirectoryEx(ptr long long long long)
@ stub MBToWCSEx
# @ stub MB_GetString
@ stdcall MapDialogRect(long ptr)
@ stdcall MapVirtualKeyA(long long)
@ stdcall MapVirtualKeyExA(long long long)
@ stdcall MapVirtualKeyExW(long long long)
@ stdcall MapVirtualKeyW(long long)
@ stdcall MapWindowPoints(long long ptr long)
@ stdcall MenuItemFromPoint(long long int64)
@ stub MenuWindowProcA
@ stub MenuWindowProcW
@ stdcall MessageBeep(long)
@ stdcall MessageBoxA(long str str long)
@ stdcall MessageBoxExA(long str str long long)
@ stdcall MessageBoxExW(long wstr wstr long long)
@ stdcall MessageBoxIndirectA(ptr)
@ stdcall MessageBoxIndirectW(ptr)
@ stdcall MessageBoxTimeoutA(long str str long long long)
@ stdcall MessageBoxTimeoutW(long wstr wstr long long long)
@ stdcall MessageBoxW(long wstr wstr long)
# @ stub ModifyAccess
@ stdcall ModifyMenuA(long long long long ptr)
@ stdcall ModifyMenuW(long long long long ptr)
@ stdcall MonitorFromPoint(int64 long)
@ stdcall MonitorFromRect(ptr long)
@ stdcall MonitorFromWindow(long long)
@ stdcall MoveWindow(long long long long long long)
@ stdcall MsgWaitForMultipleObjects(long ptr long long long)
@ stdcall MsgWaitForMultipleObjectsEx(long ptr long long long)
@ stdcall NotifyWinEvent(long long long long)
@ stdcall OemKeyScan(long)
@ stdcall OemToCharA(str ptr)
@ stdcall OemToCharBuffA(ptr ptr long)
@ stdcall OemToCharBuffW(ptr ptr long)
@ stdcall OemToCharW(str ptr)
@ stdcall OffsetRect(ptr long long)
@ stdcall OpenClipboard(long)
@ stdcall OpenDesktopA(str long long long)
@ stdcall OpenDesktopW(wstr long long long)
@ stdcall OpenIcon(long)
@ stdcall OpenInputDesktop(long long long)
@ stdcall OpenWindowStationA(str long long)
@ stdcall OpenWindowStationW(wstr long long)
@ stdcall PackDDElParam(long long long)
@ stdcall PaintDesktop(long)
# @ stub PaintMenuBar
@ stdcall PeekMessageA(ptr long long long long)
@ stdcall PeekMessageW(ptr long long long long)
@ stdcall PhysicalToLogicalPoint(long ptr)
@ stdcall PhysicalToLogicalPointForPerMonitorDPI(long ptr)
@ stub PlaySoundEvent
@ stdcall PostMessageA(long long long long)
@ stdcall PostMessageW(long long long long)
@ stdcall PostQuitMessage(long)
@ stdcall PostThreadMessageA(long long long long)
@ stdcall PostThreadMessageW(long long long long)
@ stdcall PrintWindow(long long long)
@ stdcall PrivateExtractIconExA(str long ptr ptr long)
@ stdcall PrivateExtractIconExW(wstr long ptr ptr long)
@ stdcall PrivateExtractIconsA (str long long long ptr ptr long long)
@ stdcall PrivateExtractIconsW (wstr long long long ptr ptr long long)
# @ stub PrivateSetDbgTag
# @ stub PrivateSetRipFlags
@ stdcall PtInRect(ptr int64)
@ stdcall QueryDisplayConfig(long ptr ptr ptr ptr ptr)
@ stub QuerySendMessage
# @ stub QueryUserCounters
@ stdcall RealChildWindowFromPoint(long int64)
@ stdcall RealGetWindowClass(long ptr long) RealGetWindowClassA
@ stdcall RealGetWindowClassA(long ptr long)
@ stdcall RealGetWindowClassW(long ptr long)
# @ stub ReasonCodeNeedsBugID
# @ stub ReasonCodeNeedsComment
# @ stub RecordShutdownReason
@ stdcall RedrawWindow(long ptr long long)
@ stdcall RegisterClassA(ptr)
@ stdcall RegisterClassExA(ptr)
@ stdcall RegisterClassExW(ptr)
@ stdcall RegisterClassW(ptr)
@ stdcall RegisterClipboardFormatA(str)
@ stdcall RegisterClipboardFormatW(wstr)
@ stdcall RegisterDeviceNotificationA(long ptr long)
@ stdcall RegisterDeviceNotificationW(long ptr long)
@ stdcall RegisterHotKey(long long long long)
@ stdcall RegisterLogonProcess(long long)
# @ stub RegisterMessagePumpHook
@ stub RegisterNetworkCapabilities
@ stdcall RegisterPointerDeviceNotifications(long long)
@ stdcall RegisterPowerSettingNotification(long ptr long)
@ stdcall RegisterRawInputDevices(ptr long long)
@ stdcall RegisterServicesProcess(long)
@ stdcall RegisterShellHookWindow (long)
@ stdcall RegisterSystemThread(long long)
@ stdcall RegisterTasklist (long)
@ stdcall RegisterTouchHitTestingWindow(long long)
@ stdcall RegisterTouchWindow(long long)
# @ stub RegisterUserApiHook
@ stdcall RegisterWindowMessageA(str)
@ stdcall RegisterWindowMessageW(wstr)
@ stdcall ReleaseCapture()
@ stdcall ReleaseDC(long long)
@ stdcall RemoveClipboardFormatListener(long)
@ stdcall RemoveMenu(long long long)
@ stdcall RemovePropA(long str)
@ stdcall RemovePropW(long wstr)
@ stdcall ReplyMessage(long)
@ stub ResetDisplay
# @ stub ResolveDesktopForWOW
@ stdcall ReuseDDElParam(long long long long long)
@ stdcall ScreenToClient(long ptr)
@ stdcall ScrollChildren(long long long long)
@ stdcall ScrollDC(long long long ptr ptr long ptr)
@ stdcall ScrollWindow(long long long ptr ptr)
@ stdcall ScrollWindowEx(long long long ptr ptr long ptr long)
@ stdcall SendDlgItemMessageA(long long long long long)
@ stdcall SendDlgItemMessageW(long long long long long)
@ stdcall SendIMEMessageExA(long long)
@ stdcall SendIMEMessageExW(long long)
@ stdcall SendInput(long ptr long)
@ stdcall SendMessageA(long long long long)
@ stdcall SendMessageCallbackA(long long long long ptr long)
@ stdcall SendMessageCallbackW(long long long long ptr long)
@ stdcall SendMessageTimeoutA(long long long long long long ptr)
@ stdcall SendMessageTimeoutW(long long long long long long ptr)
@ stdcall SendMessageW(long long long long)
@ stdcall SendNotifyMessageA(long long long long)
@ stdcall SendNotifyMessageW(long long long long)
@ stub ServerSetFunctionPointers
@ stdcall SetActiveWindow(long)
@ stdcall SetCapture(long)
@ stdcall SetCaretBlinkTime(long)
@ stdcall SetCaretPos(long long)
@ stdcall SetClassLongA(long long long)
@ stdcall -arch=win64 SetClassLongPtrA(long long long)
@ stdcall -arch=win64 SetClassLongPtrW(long long long)
@ stdcall SetClassLongW(long long long)
@ stdcall SetClassWord(long long long)
@ stdcall SetClipboardData(long long)
@ stdcall SetClipboardViewer(long)
@ stdcall SetCoalescableTimer(long long long ptr long)
# @ stub SetConsoleReserveKeys
@ stdcall SetCursor(long)
@ stub SetCursorContents
@ stdcall SetCursorPos(long long)
@ stdcall SetDebugErrorLevel(long)
@ stdcall SetDeskWallPaper(str)
# @ stub SetDeskWallpaper
# @ stub SetDesktopBitmap
@ stdcall SetDlgItemInt(long long long long)
@ stdcall SetDlgItemTextA(long long str)
@ stdcall SetDlgItemTextW(long long wstr)
@ stdcall SetDoubleClickTime(long)
@ stdcall SetFocus(long)
@ stdcall SetForegroundWindow(long)
@ stdcall SetGestureConfig(ptr long long ptr long)
@ stdcall SetInternalWindowPos(long long ptr ptr)
@ stdcall SetKeyboardState(ptr)
@ stdcall SetLastErrorEx(long long)
@ stdcall SetLayeredWindowAttributes(ptr long long long)
@ stdcall SetLogonNotifyWindow(long long)
@ stdcall SetMenu(long long)
@ stdcall SetMenuContextHelpId(long long)
@ stdcall SetMenuDefaultItem(long long long)
@ stdcall SetMenuInfo(long ptr)
@ stdcall SetMenuItemBitmaps(long long long long long)
@ stdcall SetMenuItemInfoA(long long long ptr)
@ stdcall SetMenuItemInfoW(long long long ptr)
@ stdcall SetMessageExtraInfo(long)
@ stdcall SetMessageQueue(long)
@ stdcall SetParent(long long)
@ stdcall SetPhysicalCursorPos(long long)
@ stdcall SetProcessDPIAware()
@ stdcall SetProcessDefaultLayout(long)
@ stdcall SetProcessDpiAwarenessContext(long)
@ stdcall SetProcessDpiAwarenessInternal(long)
@ stdcall SetProcessWindowStation(long)
@ stdcall SetProgmanWindow (long)
@ stdcall SetPropA(long str long)
@ stdcall SetPropW(long wstr long)
@ stdcall SetRect(ptr long long long long)
@ stdcall SetRectEmpty(ptr)
@ stdcall SetScrollInfo(long long ptr long)
@ stdcall SetScrollPos(long long long long)
@ stdcall SetScrollRange(long long long long long)
@ stdcall SetShellWindow(long)
@ stdcall SetShellWindowEx (long long)
@ stdcall SetSysColors(long ptr ptr)
@ stdcall SetSysColorsTemp(ptr ptr long)
@ stdcall SetSystemCursor(long long)
@ stdcall SetSystemMenu(long long)
@ stdcall SetSystemTimer(long long long ptr)
@ stdcall SetTaskmanWindow (long)
@ stdcall SetThreadDesktop(long)
@ stdcall SetThreadDpiAwarenessContext(ptr)
@ stdcall SetTimer(long long long ptr)
@ stdcall SetUserObjectInformationA(long long ptr long)
@ stdcall SetUserObjectInformationW(long long ptr long)
@ stdcall SetUserObjectSecurity(long ptr ptr)
@ stdcall SetWinEventHook(long long long ptr long long long)
@ stdcall SetWindowCompositionAttribute(ptr ptr)
@ stdcall SetWindowContextHelpId(long long)
@ stdcall SetWindowDisplayAffinity(long long)
@ stub SetWindowFullScreenState
@ stdcall SetWindowLongA(long long long)
@ stdcall -arch=win64 SetWindowLongPtrA(long long long)
@ stdcall -arch=win64 SetWindowLongPtrW(long long long)
@ stdcall SetWindowLongW(long long long)
@ stdcall SetWindowPlacement(long ptr)
@ stdcall SetWindowPos(long long long long long long long)
@ stdcall SetWindowRgn(long long long)
@ stdcall SetWindowStationUser(long long)
@ stdcall SetWindowTextA(long str)
@ stdcall SetWindowTextW(long wstr)
@ stdcall SetWindowWord(long long long)
@ stdcall SetWindowsHookA(long ptr)
@ stdcall SetWindowsHookExA(long long long long)
@ stdcall SetWindowsHookExW(long long long long)
@ stdcall SetWindowsHookW(long ptr)
@ stdcall ShowCaret(long)
@ stdcall ShowCursor(long)
@ stdcall ShowOwnedPopups(long long)
@ stdcall ShowScrollBar(long long long)
@ stub ShowStartGlass
@ stdcall ShowWindow(long long)
@ stdcall ShowWindowAsync(long long)
@ stdcall ShutdownBlockReasonCreate(long wstr)
@ stdcall ShutdownBlockReasonDestroy(long)
# @ stub SoftModalMessageBox
@ stdcall SubtractRect(ptr ptr ptr)
@ stdcall SwapMouseButton(long)
@ stdcall SwitchDesktop(long)
@ stdcall SwitchToThisWindow(long long)
# @ stub SysErrorBox
@ stdcall SystemParametersInfoA(long long ptr long)
@ stdcall SystemParametersInfoForDpi(long long ptr long long)
@ stdcall SystemParametersInfoW(long long ptr long)
@ stdcall TabbedTextOutA(long long long str long long ptr long)
@ stdcall TabbedTextOutW(long long long wstr long long ptr long)
@ stdcall TileChildWindows(long long)
@ stdcall TileWindows(long long ptr long ptr)
@ stdcall ToAscii(long long ptr ptr long)
@ stdcall ToAsciiEx(long long ptr ptr long long)
@ stdcall ToUnicode(long long ptr ptr long long)
@ stdcall ToUnicodeEx(long long ptr ptr long long long)
@ stdcall TrackMouseEvent(ptr)
@ stdcall TrackPopupMenu(long long long long long long ptr)
@ stdcall TrackPopupMenuEx(long long long long long ptr)
@ stdcall TranslateAccelerator(long long ptr) TranslateAcceleratorA
@ stdcall TranslateAcceleratorA(long long ptr)
@ stdcall TranslateAcceleratorW(long long ptr)
@ stdcall TranslateMDISysAccel(long ptr)
@ stdcall TranslateMessage(ptr)
# @ stub TranslateMessageEx
@ stdcall UnhookWinEvent(long)
@ stdcall UnhookWindowsHook(long ptr)
@ stdcall UnhookWindowsHookEx(long)
@ stdcall UnionRect(ptr ptr ptr)
@ stdcall UnloadKeyboardLayout(long)
@ stub UnlockWindowStation
@ stdcall UnpackDDElParam(long long ptr ptr)
@ stdcall UnregisterClassA(str long)
@ stdcall UnregisterClassW(wstr long)
@ stdcall UnregisterDeviceNotification(long)
@ stdcall UnregisterHotKey(long long)
# @ stub UnregisterMessagePumpHook
@ stdcall UnregisterPowerSettingNotification(ptr)
@ stdcall UnregisterTouchWindow(long)
# @ stub UnregisterUserApiHook
@ stdcall UpdateLayeredWindow(long long ptr ptr long ptr long ptr long)
@ stdcall UpdateLayeredWindowIndirect(long ptr)
@ stub UpdatePerUserSystemParameters
@ stdcall UpdateWindow(long)
@ stdcall User32InitializeImmEntryTable(ptr)
@ stdcall UserClientDllInitialize(long long ptr) DllMain
@ stdcall UserHandleGrantAccess(ptr ptr long)
# @ stub UserIsSystemResumeAutomatic
# @ stub UserLpkPSMTextOut
# @ stub UserLpkTabbedTextOut
@ stdcall UserRealizePalette(long)
@ stdcall UserRegisterWowHandlers(ptr ptr)
# @ stub UserSetDeviceHoldState
@ stdcall UserSignalProc(long long long long)
# @ stub VRipOutput
# @ stub VTagOutput
@ stdcall ValidateRect(long ptr)
@ stdcall ValidateRgn(long long)
@ stdcall VkKeyScanA(long)
@ stdcall VkKeyScanExA(long long)
@ stdcall VkKeyScanExW(long long)
@ stdcall VkKeyScanW(long)
@ stub WCSToMBEx
@ stdcall WINNLSEnableIME(long long)
@ stdcall WINNLSGetEnableStatus(long)
@ stdcall WINNLSGetIMEHotkey(long)
@ stub WNDPROC_CALLBACK
@ stdcall WaitForInputIdle(long long)
@ stdcall WaitMessage()
# @ stub Win32PoolAllocationStats
@ stdcall WinHelpA(long str long long)
@ stdcall WinHelpW(long wstr long long)
# @ stub WinOldAppHackoMatic
@ stdcall WindowFromDC(long)
@ stdcall WindowFromPoint(int64)
# @ stub YieldTask
# @ stub _SetProcessDefaultLayout
@ stdcall keybd_event(long long long long)
@ stdcall mouse_event(long long long long long)
@ varargs wsprintfA(str str)
@ varargs wsprintfW(wstr wstr)
@ stdcall wvsprintfA(ptr str ptr)
@ stdcall wvsprintfW(ptr wstr ptr)

################################################################
# Wine internal extensions
#
# All functions must be prefixed with '__wine_' (for internal functions)
# or 'wine_' (for user-visible functions) to avoid namespace conflicts.
#
@ cdecl __wine_send_input(long ptr)
@ cdecl __wine_set_pixel_format(long long)
