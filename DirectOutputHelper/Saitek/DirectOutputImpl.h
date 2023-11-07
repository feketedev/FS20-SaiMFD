#pragma once

// -----------------------------------------------------------------
// Basic access to DirectOutput service.
// Almost original file from Saitek example app, simplified for X52.
// -----------------------------------------------------------------


#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "DirectOutput.h"



namespace Saitek {

	///
	/// DirectOutput
	///
	/// Simplifies the LoadLibrary interface to DirectOutput
	///
	class DirectOutput
	{
	public:
		DirectOutput();
		~DirectOutput();

		// Initialize the library
		// Parameters
		//    wszPluginName : null-terminated wchar_t name of the plugin. Used for debugging purposes. Can be NULL
		// Returns
		//    S_OK : succeeded
		HRESULT Initialize(const wchar_t* wszPluginName);

		// Cleanup the library
		// Parameters (None)
		// Returns
		//    S_OK : succeeded
		HRESULT Deinitialize();



		// Register a callback. Callback will be called whenever a device is added or removed, or when DirectOutput_Enumerate is called
		// Parameters
		//     pfnCb : Pointer to the callback function to be called when a device is added or removed
		//     pCtxt : Caller supplied context pointer, passed to the callback function
		// Returns
		//     S_OK : succeeded
		HRESULT RegisterDeviceCallback(Pfn_DirectOutput_DeviceChange pfnCb, void* pCtxt);

		// Enumerate all devices currently attached.
		// Parameters
		//     pCtxt : Caller supplied context pointer, passed to the callback function
		// Returns
		//     S_OK : succeeded
		HRESULT Enumerate(Pfn_DirectOutput_EnumerateCallback pfnCb, void* pCtxt) const;



		// Register a callback. Called when the page changes. Callee will only recieve notifications about pages they added
		// Parameters
		//     hDevice : opaque device handle
		//     pfnCb : caller supplied callback function, called when the active page is changed to/from one of the caller's pages
		//     pCtxt : caller supplied context pointer, passed to the callback function
		// Returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		HRESULT RegisterPageCallback(void* hDevice, Pfn_DirectOutput_PageChange pfnCb, void* pCtxt);

		// Register a callback. Called when the soft buttons are changed and the callee's page is active
		// Parameters
		//     hDevice : opaque device handle
		//     pfnCb : caller supplied callback function, called when the soft buttons are changed and one of the caller's pages is active
		//     pCtxt : caller supplied context pointer, passed to the callback function
		// Returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		HRESULT RegisterSoftButtonCallback(void* hDevice, Pfn_DirectOutput_SoftButtonChange pfnCb, void* pCtxt);



		// Get the device type GUID. See DeviceType_* constants
		// Parameters
		//     hDevice : opaque device handle
		//     pGuid : pointer to GUID to recieve device type identifier. See DeviceType_* constants
		// Returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		//     E_INVALIDARG : pGuid is NULL
		HRESULT GetDeviceType(void* hDevice, LPGUID pGuid) const;

		// Get the device instance GUID used by IDirectInput::CreateDevice
		// Parameters
		//     hDevice : opaque device handle
		//     pGuid : pointer to GUID to recieve device's DirectInput Instance Guid.
		// Returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		//     E_INVALIDARG : pGuid is NULL
		//     E_NOTIMPL : hDevice does not support DirectInput.
		HRESULT GetDeviceInstance(void* hDevice, LPGUID pGuid) const;

		// Get the device unique serial number
		// Parameters
		//     hDevice : opaque device handle
		//     pszSerialNumber : the serial number string
		//     dwSize : the number of the serial number string
		// Returns
		//     S_OK : succeeded
		//     E_FAIL : error
		HRESULT GetSerialNumber(void* hDevice, wchar_t* pszSerialNumber, DWORD dwSize) const;



		// Set the profile used on the device.
		// Parameters
		//     hDevice : opaque device handle
		//     cchProfile : count of wchar_t's in wszProfile
		//     wszProfile : full path of the profile to activate. passing NULL will clear the profile
		// Returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		//     E_NOTIMPL : hDevice does not support SST profiles
		HRESULT SetProfile(void* hDevice, DWORD cchProfile, const wchar_t* wszProfile);



		// Adds a page to the device
		// Parameters
		//     hDevice : opaque device handle
		//     dwPage : caller assigned page id to add
		//     wszDebugName : null-terminated wchar_t string. Only used for debugging, can be NULL
		//     dwFlags : flags ( 0 | FLAG_SET_AS_ACTIVE )
		// Returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		HRESULT AddPage(void* hDevice, DWORD dwPage, const wchar_t* wszDebugName, DWORD dwFlags);

		// Removes a page from the device
		// Parameters
		//     hDevice : opaque device handle
		//     dwPage : caller assigned page id to remove
		// Returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		//     E_INVALIDARG : dwPage is not a valid page id
		HRESULT RemovePage(void* hDevice, DWORD dwPage);

		// Set the state of a LED on the device
		// Parameters
		//     hDevice : opaque device handle
		//     dwPage : page to display the led on
		//     dwIndex : index of the led
		//     dwValue : value of the led (0 is off)
		// returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		//     E_NOTIMPL : hDevice does not have any leds
		//     E_INVALIDARG : dwPage or dwIndex is not a valid id
		//     E_PAGENOTACTIVE : dwPage is not the active page
		HRESULT SetLed(void* hDevice, DWORD dwPage, DWORD dwIndex, DWORD dwValue);

		// Set the string on the device
		// Parameters
		//     hDevice : opaque device handle
		//     dwPage : page to display the string on
		//     dwIndex : index of the string
		//     cchValue : the count of wchar_t's in wszValue
		//     wszValue : the string to display
		// returns
		//     S_OK : succeeded
		//     E_HANDLE : hDevice is not a valid device handle
		//     E_NOTIMPL : hDevice does not have any strings
		//     E_INVALIDARG : dwPage or dwIndex is not a valid id
		//     E_PAGENOTACTIVE : dwPage is not the active page
		HRESULT SetString(void* hDevice, DWORD dwPage, DWORD dwIndex, DWORD cchValue, const wchar_t* wszValue);


	private:
		HMODULE										m_module;

		Pfn_DirectOutput_Initialize					m_initialize;
		Pfn_DirectOutput_Deinitialize				m_deinitialize;
		Pfn_DirectOutput_RegisterDeviceCallback		m_registerdevicecallback;
		Pfn_DirectOutput_Enumerate					m_enumerate;
		Pfn_DirectOutput_RegisterPageCallback		m_registerpagecallback;
		Pfn_DirectOutput_RegisterSoftButtonCallback	m_registersoftbuttoncallback;
		Pfn_DirectOutput_GetDeviceType				m_getdevicetype;
		Pfn_DirectOutput_GetDeviceInstance			m_getdeviceinstance;
		Pfn_DirectOutput_GetSerialNumber			m_getserialnumber;
		Pfn_DirectOutput_SetProfile					m_setprofile;
		Pfn_DirectOutput_AddPage					m_addpage;
		Pfn_DirectOutput_RemovePage					m_removepage;
		Pfn_DirectOutput_SetLed						m_setled;
		Pfn_DirectOutput_SetString					m_setstring;

		bool GetDirectOutputFilename(LPTSTR filename, DWORD length);
	};

}	// namespace Saitek