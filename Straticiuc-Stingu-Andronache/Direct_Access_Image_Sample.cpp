//===========================================================================
//===========================================================================
//==   Direct_Access_Image_Sample.cpp  ==  Author: Costin-Anton BOIANGIU   ==
//===========================================================================
//===========================================================================

//===========================================================================
#include "stdafx.h"
#include "Direct_Access_Image.h"
#include "ImgBinarization.h"
#include <string>
//===========================================================================

//===========================================================================
int _tmain(int argc, _TCHAR* argv[])
{
    //Verify command-line usage correctness
    if ( argc < 3 || argc > 4 )
    {
		const wchar_t txt[50] = _T("<input_img_filename> <output_img_filename>");
        _tprintf(_T("Usage:\n %s %s\n Or\n %s %s <output_confidence_filename>\n"), argv[0], txt, argv[0], txt);
        return -1;
    }

    //Buffer for the new file names
    TCHAR strNewFileName[0x100];

    //Load and verify that input image is a True-Color one
    KImage *pImage = new KImage(argv[1]);
    if (pImage == NULL || !pImage->IsValid() || pImage->GetBPP() != 24)
    {
        _tprintf(_T("File %s does is not a valid True-Color image!"), argv[0]);
        return -2;
    }
	
    //Apply a Gaussian Blur with small radius to remove obvious noise
    pImage->GaussianBlur(0.5);
    _stprintf_s(strNewFileName, sizeof(strNewFileName) / sizeof(TCHAR), _T("%s_blurred.TIF"), argv[0]);
    pImage->SaveAs(strNewFileName, SAVE_TIFF_LZW);

    //Convert to grayscale
    KImage *pImageGrayscale = pImage->ConvertToGrayscale();
    //Don't forget to delete the original, now useless image
    delete pImage;

    //Verify conversion success...
    if (pImageGrayscale == NULL || !pImageGrayscale->IsValid() || pImageGrayscale->GetBPP() != 8)
    {
        _tprintf(_T("Conversion to grayscale was not successfull!"));
        return -3;
    }
    //... and save grayscale image
    _stprintf_s(strNewFileName, sizeof(strNewFileName) / sizeof(TCHAR), _T("%s_grayscale.TIF"), argv[0]);
    pImageGrayscale->SaveAs(strNewFileName, SAVE_TIFF_LZW);
    
    //Request direct access to image pixels in raw format
    if (pImageGrayscale->BeginDirectAccess() && pImageGrayscale->GetDataMatrix() != NULL)
    {
		ImgBinarization binarization(pImageGrayscale);
		shared_ptr<KImage> img_binared    = binarization.GetBinaredImage();
		shared_ptr<KImage> img_confidence = binarization.GetConfidence();

		if( !img_binared || !img_confidence )
		{
			_tprintf(_T("Unable to obtain direct access in binary image!"));
            return -3;			
		}
		//Save binarized image
        _stprintf_s(strNewFileName, sizeof(strNewFileName) / sizeof(TCHAR), _T("%s.TIF\0"), argv[2]);
        img_binared->SaveAs(strNewFileName, SAVE_TIFF_CCITTFAX4);

		//Save confidence
		if( argc == 3)
			_stprintf_s(strNewFileName, sizeof(strNewFileName) / sizeof(TCHAR), _T("%s_confidence.TIF\0"), argv[2]);
		else
			_stprintf_s(strNewFileName, sizeof(strNewFileName) / sizeof(TCHAR), _T("%s.TIF\0"), argv[3]);
		img_confidence->SaveAs(strNewFileName, SAVE_TIFF_CCITTFAX4);

        //Close direct access
        pImageGrayscale->EndDirectAccess();
    }
    else
    {
        _tprintf(_T("Unable to obtain direct access in grayscale image!"));
        return -4;
    }

    //Don't forget to delete the grayscale image
    delete pImageGrayscale;

    //Return with success
    return 0;
}
//===========================================================================