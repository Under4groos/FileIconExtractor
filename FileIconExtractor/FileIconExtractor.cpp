
#include "Headers.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <locale>
#include <codecvt>
#include <olectl.h>
#pragma comment(lib, "oleaut32.lib")
#include <atlbase.h>
void SaveIconToFile(HICON hico, std::string szFileName, BOOL bAutoDelete = FALSE)
{
	PICTDESC pd = { sizeof(pd), PICTYPE_ICON };
	pd.icon.hicon = hico;

	CComPtr<IPicture> pPict = NULL;
	CComPtr<IStream>  pStrm = NULL;
	LONG cbSize = 0;

	BOOL res = FALSE;

	res = SUCCEEDED(::CreateStreamOnHGlobal(NULL, TRUE, &pStrm));
	res = SUCCEEDED(::OleCreatePictureIndirect(&pd, IID_IPicture, bAutoDelete, (void**)&pPict));
	res = SUCCEEDED(pPict->SaveAsFile(pStrm, TRUE, &cbSize));

	if (res)
	{
		// rewind stream to the beginning
		LARGE_INTEGER li = { 0 };
		pStrm->Seek(li, STREAM_SEEK_SET, NULL);

		// write to file
		HANDLE hFile = ::CreateFileA(szFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (INVALID_HANDLE_VALUE != hFile)
		{
			DWORD dwWritten = 0, dwRead = 0, dwDone = 0;
			BYTE  buf[4096];
			while (dwDone < cbSize)
			{
				if (SUCCEEDED(pStrm->Read(buf, sizeof(buf), &dwRead)))
				{
					::WriteFile(hFile, buf, dwRead, &dwWritten, NULL);
					if (dwWritten != dwRead)
						break;
					dwDone += dwRead;
				}
				else
					break;
			}

			_ASSERTE(dwDone == cbSize);
			::CloseHandle(hFile);
		}
	}
}
HRESULT SaveIcon2(HICON hIcon, const char* path) {
	// Create the IPicture intrface
	PICTDESC desc = { sizeof(PICTDESC) };
	desc.picType = PICTYPE_BITMAP;
	desc.icon.hicon = hIcon;
	IPicture* pPicture = 0;
	HRESULT hr = OleCreatePictureIndirect(&desc, IID_IPicture, FALSE, (void**)&pPicture);
	if (FAILED(hr)) return hr;

	// Create a stream and save the image
	IStream* pStream = 0;
	CreateStreamOnHGlobal(0, TRUE, &pStream);
	LONG cbSize = 0;
	hr = pPicture->SaveAsFile(pStream, TRUE, &cbSize);

	// Write the stream content to the file
	if (!FAILED(hr)) {
		HGLOBAL hBuf = 0;
		GetHGlobalFromStream(pStream, &hBuf);
		void* buffer = GlobalLock(hBuf);
		HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
		if (!hFile) hr = HRESULT_FROM_WIN32(GetLastError());
		else {
			DWORD written = 0;
			WriteFile(hFile, buffer, cbSize, &written, 0);
			CloseHandle(hFile);
		}
		GlobalUnlock(buffer);
	}
	// Cleanup
	pStream->Release();
	pPicture->Release();
	return hr;

}
void SaveIcon(HICON hIcon, const std::string& filename) {
	// Create a BITMAPINFO structure to get the icon's information
	ICONINFO iconInfo;
	GetIconInfo(hIcon, &iconInfo);

	// Create a bitmap from the icon
	HDC hdc = GetDC(NULL);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, iconInfo.xHotspot * 2, iconInfo.yHotspot * 2);


	 



	HDC hdcMem = CreateCompatibleDC(hdc);


	SelectObject(hdcMem, hBitmap);

	// Draw the icon onto the bitmap
	DrawIcon(hdcMem, 0, 0, hIcon);

	// Save the bitmap to a file
	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	BITMAPFILEHEADER bmfHeader;
	BITMAPINFOHEADER bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32; // Use 32 bits for RGBA
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;




	DWORD dwSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
	BYTE* pPixels = new BYTE[dwSize];
	GetDIBits(hdcMem, hBitmap, 0, bmp.bmHeight, pPixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	// Create the file
	HANDLE hFile = CreateFileA(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		// Write the bitmap file header
		bmfHeader.bfType = 0x4D42; // 'BM'
		bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwSize;
		bmfHeader.bfReserved1 = 0;
		bmfHeader.bfReserved2 = 0;
		bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		DWORD dwWritten;
		WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
		WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
		WriteFile(hFile, pPixels, dwSize, &dwWritten, NULL);
		CloseHandle(hFile);
	}

	// Clean up
	delete[] pPixels;
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdc);
}


 



HICON GetFileIcon(const std::wstring& name, int size, bool linkOverlay) {
	SHFILEINFO shfi;
	UINT flags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES;

	if (linkOverlay) {
		flags |= SHGFI_LINKOVERLAY;
	}

	// Check the size specified for return.
	if (size == Small) {
		flags |= SHGFI_SMALLICON; // include the small icon flag
	}
	else {
		flags |= SHGFI_LARGEICON; // include the large icon flag
	}

	// FILE_ATTRIBUTE_TEMPORARY 256
	// FILE_ATTRIBUTE_NORMAL 16
	// Get the file information
	SHGetFileInfo(name.c_str(), FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), flags);

	// Clone the returned icon
	HICON icon = (HICON)CopyIcon(shfi.hIcon);
	DestroyIcon(shfi.hIcon); // Cleanup

	return icon;
}

 
extern "C" {
	__declspec(dllexport) bool ExportIcon(std::string name, std::string exportPath, int size, bool linkOverlay) {

		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

		HICON icon = GetFileIcon(converter.from_bytes(name), size, linkOverlay);

		if (icon) {

			SaveIconToFile(icon, exportPath.c_str());


			DestroyIcon(icon);
			return TRUE;
		}
		else {
			return FALSE;

		}
	}
}

int main(int argc, const char** argv, const char** envp) {

	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " <name> <exportPath>\n";
		return 1;
	}

	std::string inputFile = argv[1];
	std::string exportPath = argv[2];

	if (std::filesystem::exists(inputFile)) {
		//IconSize::Small SHGFI_ICON or SHGFI_SMALLICON 
		// IconSize::Small
		if (ExportIcon(inputFile, exportPath, SHGFI_USEFILEATTRIBUTES, false)) {
			std::cout << "Export: " << exportPath << std::endl;
		}


	}
	return 0;
}
