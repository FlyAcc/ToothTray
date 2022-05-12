#include "ToothTrayMenu.h"

#include <unordered_map>
#include <windowsx.h>
#include <Windows.h>

void ToothTrayMenu::BuildMenu(std::vector<BluetoothConnector>& connectors) {
    m_handle.reset(CreatePopupMenu());
    m_menuData.clear();

    UINT menuPosition = 0;
    for (std::vector<BluetoothConnector>::iterator ite = connectors.begin(); ite != connectors.end(); ++ite, ++menuPosition) {
        unsigned int currentMenuItemId = IDM_BLUETOOTH_AUDIO_BASE + menuPosition + 1;
        std::pair<std::unordered_map<unsigned int, MenuData>::iterator, bool> pair =
            m_menuData.emplace(std::piecewise_construct, std::forward_as_tuple(currentMenuItemId), std::forward_as_tuple(currentMenuItemId, std::move(*ite)));

        InsertBluetoohConnectorMenuItem(currentMenuItemId, menuPosition, (*(pair.first)).second.menuText.data());
    }

    InsertBluetoohConnectorMenuItem(IDM_EXIT, menuPosition, (WCHAR*)L"Exit");
}

double GetScaleFactor() {
    auto activeWindow = GetActiveWindow();
    HMONITOR monitor = MonitorFromWindow(activeWindow, MONITOR_DEFAULTTONEAREST);

    // Get the logical width and height of the monitor
    MONITORINFOEX monitorInfoEx;
    monitorInfoEx.cbSize = sizeof(monitorInfoEx);
    GetMonitorInfo(monitor, &monitorInfoEx);
    auto cxLogical = monitorInfoEx.rcMonitor.right - monitorInfoEx.rcMonitor.left;

    // Get the physical width and height of the monitor
    DEVMODE devMode;
    devMode.dmSize = sizeof(devMode);
    devMode.dmDriverExtra = 0;
    EnumDisplaySettings(monitorInfoEx.szDevice, ENUM_CURRENT_SETTINGS, &devMode);
    auto cxPhysical = devMode.dmPelsWidth;

    // Calculate the scaling factor
    return ((double)cxPhysical / (double)cxLogical);
}

void ToothTrayMenu::ShowPopupMenu(HWND hwnd, WPARAM mousPosWParam) {
    double scaleFactor = GetScaleFactor();
    int x = (int) GET_X_LPARAM(mousPosWParam) / scaleFactor;
    int y = (int) GET_Y_LPARAM(mousPosWParam) / scaleFactor;

    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-trackpopupmenuex
    // To display a context menu for a notification icon, the current window must be the foreground window before the application calls TrackPopupMenu or TrackPopupMenuEx.
    // Otherwise, the menu will not disappear when the user clicks outside of the menu or the window that created the menu (if it is visible).
    // If the current window is a child window, you must set the (top-level) parent window as the foreground window.
    SetForegroundWindow(hwnd);

    TrackPopupMenuEx(m_handle.get(), TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON, x, y, hwnd, NULL);
}

bool ToothTrayMenu::TryHandleCommand(int commandId) {
    std::unordered_map<unsigned int, MenuData>::iterator pMenuData = m_menuData.find((unsigned int)commandId);
    if (pMenuData == m_menuData.end())
        return false;

    MenuData& menuData = (*pMenuData).second;
    if (menuData.pConnector.IsConnected())
        menuData.pConnector.Disconnect();
    else
        menuData.pConnector.Connect();

    return true;
}

MENUITEMINFOW ToothTrayMenu::InsertBluetoohConnectorMenuItem(UINT id, UINT position, LPWSTR pText) {
    MENUITEMINFOW menuItem{ sizeof(MENUITEMINFOW) };
    menuItem.fMask = MIIM_ID | MIIM_STRING;
    menuItem.fType = MFT_STRING;
    menuItem.fState = MFS_ENABLED;
    menuItem.wID = id;
    menuItem.dwTypeData = pText;
    InsertMenuItemW(m_handle.get(), position, TRUE, &menuItem);

    return menuItem;
}
