#include<windows.h>
#include<ShlObj_core.h>
#include<string>
#include<iostream>
#include "Headers.h"
typedef struct
{
    WORD idReserved; // must be 0
    WORD idType; // 1 = ICON, 2 = CURSOR
    WORD idCount; // number of images (and ICONDIRs)

    // ICONDIR [1...n]
    // ICONIMAGE [1...n]

} ICONHEADER;

//
// An array of ICONDIRs immediately follow the ICONHEADER
//
typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes; // for cursors, this field = wXHotSpot
    WORD wBitCount; // for cursors, this field = wYHotSpot
    DWORD dwBytesInRes;
    DWORD dwImageOffset; // file-offset to the start of ICONIMAGE

} ICONDIR;

//
// After the ICONDIRs follow the ICONIMAGE structures -
// consisting of a BITMAPINFOHEADER, (optional) RGBQUAD array, then
// the color and mask bitmap bits (all packed together
//
typedef struct
{
    BITMAPINFOHEADER biHeader; // header for color bitmap (no mask header)
    //RGBQUAD rgbColors[1...n];
    //BYTE bXOR[1]; // DIB bits for color bitmap
    //BYTE bAND[1]; // DIB bits for mask bitmap

} ICONIMAGE;

//
// Write the ICO header to disk
//
static UINT WriteIconHeader(HANDLE hFile, int nImages)
{
    ICONHEADER iconheader;
    DWORD nWritten;

    // Setup the icon header
    iconheader.idReserved = 0; // Must be 0
    iconheader.idType = 1; // Type 1 = ICON (type 2 = CURSOR)
    iconheader.idCount = nImages; // number of ICONDIRs

    // Write the header to disk
    WriteFile(hFile, &iconheader, sizeof(iconheader), &nWritten, 0);

    // following ICONHEADER is a series of ICONDIR structures (idCount of them, in fact)
    return nWritten;
}

//
// Return the number of BYTES the bitmap will take ON DISK
//
static UINT NumBitmapBytes(BITMAP* pBitmap)
{
    int nWidthBytes = pBitmap->bmWidthBytes;

    // bitmap scanlines MUST be a multiple of 4 bytes when stored
    // inside a bitmap resource, so round up if necessary
    if (nWidthBytes & 3)
        nWidthBytes = (nWidthBytes + 4) & ~3;

    return nWidthBytes * pBitmap->bmHeight;
}

//
// Return number of bytes written
//
static UINT WriteIconImageHeader(HANDLE hFile, BITMAP* pbmpColor, BITMAP* pbmpMask)
{
    BITMAPINFOHEADER biHeader;
    DWORD nWritten;
    UINT nImageBytes;

    // calculate how much space the COLOR and MASK bitmaps take
    nImageBytes = NumBitmapBytes(pbmpColor) + NumBitmapBytes(pbmpMask);

    // write the ICONIMAGE to disk (first the BITMAPINFOHEADER)
    ZeroMemory(&biHeader, sizeof(biHeader));

    // Fill in only those fields that are necessary
    biHeader.biSize = sizeof(biHeader);
    biHeader.biWidth = pbmpColor->bmWidth;
    biHeader.biHeight = pbmpColor->bmHeight * 2; // height of color+mono
    biHeader.biPlanes = pbmpColor->bmPlanes;
    biHeader.biBitCount = pbmpColor->bmBitsPixel;
    biHeader.biSizeImage = nImageBytes;

    // write the BITMAPINFOHEADER
    WriteFile(hFile, &biHeader, sizeof(biHeader), &nWritten, 0);

    // write the RGBQUAD color table (for 16 and 256 colour icons)
    if (pbmpColor->bmBitsPixel == 2 || pbmpColor->bmBitsPixel == 8)
    {

    }

    return nWritten;
}

//
// Wrapper around GetIconInfo and GetObject(BITMAP)
//
static BOOL GetIconBitmapInfo(HICON hIcon, ICONINFO* pIconInfo, BITMAP* pbmpColor, BITMAP* pbmpMask)
{
    if (!GetIconInfo(hIcon, pIconInfo))
        return FALSE;

    if (!GetObject(pIconInfo->hbmColor, sizeof(BITMAP), pbmpColor))
        return FALSE;

    if (!GetObject(pIconInfo->hbmMask, sizeof(BITMAP), pbmpMask))
        return FALSE;

    return TRUE;
}

//
// Write one icon directory entry - specify the index of the image
//
static UINT WriteIconDirectoryEntry(HANDLE hFile, int nIdx, HICON hIcon, UINT nImageOffset)
{
    ICONINFO iconInfo;
    ICONDIR iconDir;

    BITMAP bmpColor;
    BITMAP bmpMask;

    DWORD nWritten;
    UINT nColorCount;
    UINT nImageBytes;

    GetIconBitmapInfo(hIcon, &iconInfo, &bmpColor, &bmpMask);

    nImageBytes = NumBitmapBytes(&bmpColor) + NumBitmapBytes(&bmpMask);

    if (bmpColor.bmBitsPixel >= 8)
        nColorCount = 0;
    else
        nColorCount = 1 << (bmpColor.bmBitsPixel * bmpColor.bmPlanes);

    // Create the ICONDIR structure
    iconDir.bWidth = (BYTE)bmpColor.bmWidth;
    iconDir.bHeight = (BYTE)bmpColor.bmHeight;
    iconDir.bColorCount = nColorCount;
    iconDir.bReserved = 0;
    iconDir.wPlanes = bmpColor.bmPlanes;
    iconDir.wBitCount = bmpColor.bmBitsPixel;
    iconDir.dwBytesInRes = sizeof(BITMAPINFOHEADER) + nImageBytes;
    iconDir.dwImageOffset = nImageOffset;

    // Write to disk
    WriteFile(hFile, &iconDir, sizeof(iconDir), &nWritten, 0);

    // Free resources
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);

    return nWritten;
}

static UINT WriteIconData(HANDLE hFile, HBITMAP hBitmap)
{
    BITMAP bmp;
    int i;
    BYTE* pIconData;

    UINT nBitmapBytes;
    DWORD nWritten;

    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    nBitmapBytes = NumBitmapBytes(&bmp);

    pIconData = (BYTE*)malloc(nBitmapBytes);

    GetBitmapBits(hBitmap, nBitmapBytes, pIconData);

    // bitmaps are stored inverted (vertically) when on disk..
    // so write out each line in turn, starting at the bottom + working
    // towards the top of the bitmap. Also, the bitmaps are stored in packed
    // in memory - scanlines are NOT 32bit aligned, just 1-after-the-other
    for (i = bmp.bmHeight - 1; i >= 0; i--)
    {
        // Write the bitmap scanline
        WriteFile(
            hFile,
            pIconData + (i * bmp.bmWidthBytes), // calculate offset to the line
            bmp.bmWidthBytes, // 1 line of BYTES
            &nWritten,
            0);

        // extend to a 32bit boundary (in the file) if necessary
        if (bmp.bmWidthBytes & 3)
        {
            DWORD padding = 0;
            WriteFile(hFile, &padding, 4 - bmp.bmWidthBytes, &nWritten, 0);
        }
    }

    free(pIconData);

    return nBitmapBytes;
}

//
// Create a .ICO file, using the specified array of HICON images
//
BOOL SaveIcon3(LPCSTR szIconFile, HICON hIcon[], int nNumIcons)
{
    HANDLE hFile;
    int i;
    int* pImageOffset;

    if (hIcon == 0 || nNumIcons < 1)
        return FALSE;

    hFile = CreateFileA(szIconFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    WriteIconHeader(hFile, nNumIcons);

    SetFilePointer(hFile, sizeof(ICONDIR) * nNumIcons, 0, FILE_CURRENT);

    pImageOffset = (int*)malloc(nNumIcons * sizeof(int));

    for (i = 0; i < nNumIcons; i++)
    {
        ICONINFO iconInfo;
        BITMAP bmpColor, bmpMask;

        GetIconBitmapInfo(hIcon[i], &iconInfo, &bmpColor, &bmpMask);

        pImageOffset[i] = SetFilePointer(hFile, 0, 0, FILE_CURRENT);

        WriteIconImageHeader(hFile, &bmpColor, &bmpMask);

        WriteIconData(hFile, iconInfo.hbmColor);
        WriteIconData(hFile, iconInfo.hbmMask);

        DeleteObject(iconInfo.hbmColor);
        DeleteObject(iconInfo.hbmMask);
    }
    SetFilePointer(hFile, sizeof(ICONHEADER), 0, FILE_BEGIN);

    for (i = 0; i < nNumIcons; i++)
    {
        WriteIconDirectoryEntry(hFile, i, hIcon[i], pImageOffset[i]);
    }
    free(pImageOffset);
    CloseHandle(hFile);

    return TRUE;
}
 
int main(int argc, const char** argv, const char** envp) {

	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " <name> <exportPath>\n";
		return 1;
	}

	std::string inputFile = argv[1];
	std::string exportPath = argv[2];
	 


	if (std::filesystem::exists(inputFile)) {
 
		/*if (ExportIcon(inputFile, exportPath, SHGFI_ICON or SHGFI_SMALLICON, false)) {
			std::cout << "Export: " << exportPath << std::endl;
		}*/
        std::wstring stemp = std::wstring(inputFile.begin(), inputFile.end());
        LPCWSTR src = stemp.c_str();

  

        HICON icon;

        auto result = SHDefExtractIconW(src, 0, 0, &icon, NULL, 128);
        if (result == S_OK) {
            SaveIcon3(exportPath.c_str(), &icon, 1);
            std::cout << "Icon saved successfully to icon.ico" << std::endl;
        }
	


	}
	return 0;
}
