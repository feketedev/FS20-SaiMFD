
#include "DirectOutputImpl.h"


namespace Saitek
{
	DirectOutput::DirectOutput() :
		m_module(NULL),
		m_initialize(0), m_deinitialize(0),
		m_registerdevicecallback(0), m_enumerate(0),
		m_registerpagecallback(0), m_registersoftbuttoncallback(0),
		m_getdevicetype(0), m_getdeviceinstance(0), m_getserialnumber(0),
		m_setprofile(0), m_addpage(0), m_removepage(0),
		m_setled(0), m_setstring(0)
	{
		TCHAR filename[2048] = { 0 };
		if (GetDirectOutputFilename(filename, sizeof(filename) / sizeof(filename[0])))
		{
			m_module = LoadLibrary(filename);
			if (m_module)
			{
				m_initialize = (Pfn_DirectOutput_Initialize)GetProcAddress(m_module, "DirectOutput_Initialize");
				m_deinitialize = (Pfn_DirectOutput_Deinitialize)GetProcAddress(m_module, "DirectOutput_Deinitialize");
				m_registerdevicecallback = (Pfn_DirectOutput_RegisterDeviceCallback)GetProcAddress(m_module, "DirectOutput_RegisterDeviceCallback");
				m_enumerate = (Pfn_DirectOutput_Enumerate)GetProcAddress(m_module, "DirectOutput_Enumerate");
				m_registerpagecallback = (Pfn_DirectOutput_RegisterPageCallback)GetProcAddress(m_module, "DirectOutput_RegisterPageCallback");
				m_registersoftbuttoncallback = (Pfn_DirectOutput_RegisterSoftButtonCallback)GetProcAddress(m_module, "DirectOutput_RegisterSoftButtonCallback");
				m_getdevicetype = (Pfn_DirectOutput_GetDeviceType)GetProcAddress(m_module, "DirectOutput_GetDeviceType");
				m_getdeviceinstance = (Pfn_DirectOutput_GetDeviceInstance)GetProcAddress(m_module, "DirectOutput_GetDeviceInstance");
				m_getserialnumber = (Pfn_DirectOutput_GetSerialNumber)GetProcAddress(m_module, "DirectOutput_GetSerialNumber");
				m_setprofile = (Pfn_DirectOutput_SetProfile)GetProcAddress(m_module, "DirectOutput_SetProfile");
				m_addpage = (Pfn_DirectOutput_AddPage)GetProcAddress(m_module, "DirectOutput_AddPage");
				m_removepage = (Pfn_DirectOutput_RemovePage)GetProcAddress(m_module, "DirectOutput_RemovePage");
				m_setled = (Pfn_DirectOutput_SetLed)GetProcAddress(m_module, "DirectOutput_SetLed");
				m_setstring = (Pfn_DirectOutput_SetString)GetProcAddress(m_module, "DirectOutput_SetString");
			}
		}
	}
	DirectOutput::~DirectOutput()
	{
		if (m_module)
		{
			FreeLibrary(m_module);
		}
	}
	HRESULT DirectOutput::Initialize(const wchar_t* wszPluginName)
	{
		if (m_module && m_initialize)
		{
			return m_initialize(wszPluginName);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::Deinitialize()
	{
		if (m_module && m_deinitialize)
		{
			return m_deinitialize();
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::RegisterDeviceCallback(Pfn_DirectOutput_DeviceChange pfnCb, void* pCtxt)
	{
		if (m_module && m_registerdevicecallback)
		{
			return m_registerdevicecallback(pfnCb, pCtxt);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::Enumerate(Pfn_DirectOutput_EnumerateCallback pfnCb, void* pCtxt) const
	{
		if (m_module && m_enumerate)
		{
			return m_enumerate(pfnCb, pCtxt);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::RegisterPageCallback(void* hDevice, Pfn_DirectOutput_PageChange pfnCb, void* pCtxt)
	{
		if (m_module && m_registerpagecallback)
		{
			return m_registerpagecallback(hDevice, pfnCb, pCtxt);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::RegisterSoftButtonCallback(void* hDevice, Pfn_DirectOutput_SoftButtonChange pfnCb, void* pCtxt)
	{
		if (m_module && m_registersoftbuttoncallback)
		{
			return m_registersoftbuttoncallback(hDevice, pfnCb, pCtxt);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::GetDeviceType(void* hDevice, LPGUID pGuid) const
	{
		if (m_module && m_getdevicetype)
		{
			return m_getdevicetype(hDevice, pGuid);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::GetDeviceInstance(void* hDevice, LPGUID pGuid) const
	{
		if (m_module && m_getdeviceinstance)
		{
			return m_getdeviceinstance(hDevice, pGuid);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::GetSerialNumber(void* hDevice, wchar_t* pszSerialNumber, DWORD dwSize) const
	{
		if (m_module && m_getserialnumber)
		{
			return m_getserialnumber(hDevice, pszSerialNumber, dwSize);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::SetProfile(void* hDevice, DWORD cchProfile, const wchar_t* wszProfile)
	{
		if (m_module && m_setprofile)
		{
			return m_setprofile(hDevice, cchProfile, wszProfile);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::AddPage(void* hDevice, DWORD dwPage, const wchar_t* wszDebugName, DWORD dwFlags)
	{
		if (m_module && m_addpage)
		{
			return m_addpage(hDevice, dwPage, dwFlags);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::RemovePage(void* hDevice, DWORD dwPage)
	{
		if (m_module && m_removepage)
		{
			return m_removepage(hDevice, dwPage);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::SetLed(void* hDevice, DWORD dwPage, DWORD dwIndex, DWORD dwValue)
	{
		if (m_module && m_setled)
		{
			return m_setled(hDevice, dwPage, dwIndex, dwValue);
		}
		return E_NOTIMPL;
	}
	HRESULT DirectOutput::SetString(void* hDevice, DWORD dwPage, DWORD dwIndex, DWORD cchValue, const wchar_t* wszValue)
	{
		if (m_module && m_setstring)
		{
			return m_setstring(hDevice, dwPage, dwIndex, cchValue, wszValue);
		}
		return E_NOTIMPL;
	}

	bool DirectOutput::GetDirectOutputFilename(LPTSTR filename, DWORD length)
	{
		bool retval(false);
		HKEY hk;
		// Read the Full Path to DirectOutput.dll from the registry
		long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Logitech\\DirectOutput", 0, KEY_READ, &hk);
		if (ERROR_SUCCESS == lRet)
		{
			DWORD size(length * sizeof(filename[0]));
			// Note: this DirectOutput entry will point to the correct version on x86 or x64 systems
			lRet = RegQueryValueEx(hk, L"DirectOutput", 0, 0, (LPBYTE)filename, &size);
			if (ERROR_SUCCESS == lRet)
			{
				retval = true;
			}
			RegCloseKey(hk);
		}
		return retval;
	}

}	// namespace Saitek