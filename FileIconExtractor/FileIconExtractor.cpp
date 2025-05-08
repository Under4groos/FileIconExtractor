
#include "Headers.h"
#include <filesystem>

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






HICON GetFileIcon(const std::wstring& name, IconSize size, bool linkOverlay) {
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

	// Get the file information
	SHGetFileInfo(name.c_str(), FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), flags);

	// Clone the returned icon
	HICON icon = (HICON)CopyIcon(shfi.hIcon);
	DestroyIcon(shfi.hIcon); // Cleanup

	return icon;
}

extern "C" {
	__declspec(dllexport) bool ExportIcon(const std::wstring& name, std::string exportPath, IconSize size, bool linkOverlay) {
		HICON icon = GetFileIcon(name, size, linkOverlay);
		if (icon) {

			SaveIcon(icon, exportPath);
			DestroyIcon(icon);
			return TRUE;
		}
		else {
			return FALSE;

		}
	}
}

int main() {
	 
	std::wstring input;
	std::getline(std::wcin, input);

	std::wstring input;
	std::getline(std::wcin, input);

	if (std::filesystem::exists(input)) {
		ExportIcon(input, "sadasd.ico", IconSize::Small, false);


	}
	else {
		std::cout << "File not found!" << std::endl;
	}

	return 0;
}
