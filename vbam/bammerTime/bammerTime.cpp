//===========================================================================
//===========================================================================
//===========================================================================
//==   Direct_Access_Image_Sample.cpp  ==  Author: Andrei Gliga   ==
//===========================================================================
//===========================================================================
//===========================================================================

//===========================================================================
//===========================================================================
#include "stdafx.h"
#include "Direct_Access_Image.h"
#include <iostream>
#include <fstream> 
#include <string>
//===========================================================================
//===========================================================================

//===========================================================================
//===========================================================================
using namespace std;
int _tmain(int argc, _TCHAR* argv[])
{
    if (argc < 3)
    {
        _tprintf(_T("Wrong usage\n"), argv[0]);
        return -99;
    }

	KImage *outputs[50];
	KImage *confidences[50];
	KImage *original = new KImage(argv[1]);

	if (original == NULL || !original->IsValid()) {
		cout << "no original\n";
		return -1;
	}

	int j = 0;
	for (int i = 2; i < argc; i+= 2) {
		TCHAR strNewFileName[0x100];
         _tprintf(_T("%s %s\n"), argv[i], argv[i + 1]);
		KImage *pImage = new KImage(argv[i]);

		if (pImage != NULL && pImage->IsValid() && pImage->GetWidth() == original->GetWidth() && pImage->GetHeight() == original->GetHeight()) {
			outputs[j] = pImage;
			TCHAR strNewFileName[0x100];
			pImage = new KImage(argv[i + 1]);
			if (pImage != NULL && pImage->IsValid() && pImage->GetWidth() == original->GetWidth() && pImage->GetHeight() == original->GetHeight()) {
				confidences[j] = pImage;
				j++;
			}
		} else {
			std::cout << "Wrong input image " << argv[i] << endl;
		}
	}

	// Confidence
	KImage *pImageBinary = new KImage(confidences[0]->GetWidth(), confidences[0]->GetHeight(), 1);
	if (pImageBinary->BeginDirectAccess()) {
		for(int i = 0; i < j; i++) {
			BYTE **pDataMatrix = NULL;
			if (confidences[i]->BeginDirectAccess() && (pDataMatrix = confidences[i]->GetDataMatrix()) != NULL) {
				for(int x = 0; x < confidences[0]->GetWidth(); x++) {
					for(int y = 0; y < confidences[0]->GetHeight(); y++) {
						BYTE &PixelAtXY = pDataMatrix[y][x];
						if (PixelAtXY < 0x80)
							//...if black, set to black
							pImageBinary->Put1BPPPixel(x, y, true);
						else {
						// //...if not black check the options ;)
							pImageBinary->Put1BPPPixel(x, y, false);
						}
					}
				}
			}
		}

		KImage *pImageAverage = new KImage(confidences[0]->GetWidth(), confidences[0]->GetHeight(), 1);
		if (pImageAverage->BeginDirectAccess()) {
			for(int x = 0; x < outputs[0]->GetWidth(); x++) 
					for(int y = 0; y < outputs[0]->GetHeight(); y++) 
						pImageAverage->Put1BPPPixel(x, y, true);
			
			BYTE **pDataMatrix = NULL;
				
			for(int x = 0; x < outputs[0]->GetWidth(); x++) {
				for(int y = 0; y < outputs[0]->GetHeight(); y++) {
					int count = 0;
					for(int i = 0; i < j; i++) {
						if (outputs[i]->BeginDirectAccess() && (pDataMatrix = outputs[i]->GetDataMatrix()) != NULL) {
							if (outputs[i]->Get1BPPPixel(x, y) == false || pImageBinary->Get1BPPPixel(x, y))
								count++;
						}
					}
					if (count > 0 && count / j >= j / 2)
						pImageAverage->Put1BPPPixel(x, y, false);
				}
			}
		}

		pImageAverage->EndDirectAccess();
		TCHAR strNewFileName[0x100];
		_stprintf_s(strNewFileName, sizeof(strNewFileName) / sizeof(TCHAR), _T("%s_average.TIF"), argv[0]);
        pImageAverage->SaveAs(strNewFileName, SAVE_TIFF_CCITTFAX4);

		// Average
		pImageBinary->EndDirectAccess();
		TCHAR strNewFileNamae[0x100];
		_stprintf_s(strNewFileNamae, sizeof(strNewFileNamae) / sizeof(TCHAR), _T("%s_confidence.TIF"), argv[0]);
        pImageBinary->SaveAs(strNewFileNamae, SAVE_TIFF_CCITTFAX4);

		//Don't forget to delete the binary image
        delete pImageBinary;
    }
    else
    {
        _tprintf(_T("Unable to obtain direct access in binary image!"));
        return -3;
    }
	
	//Return with success
    return 0;
}
//===========================================================================
//===========================================================================
