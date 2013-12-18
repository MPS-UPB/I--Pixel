#include "stdafx.h"
#include "Direct_Access_Image.h"
#include <direct.h>

int _tmain(int argc, _TCHAR* argv[])
{
    //Verify command-line usage correctness
    if (argc != 4)
    {
        _tprintf(_T("Use: %s <Input_Image_File_Name (24BPP True-Color)> <Output_Image_File_Name> <Output_Confidence_Image_File_Name>\n"), argv[0]);
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
    //Convert to grayscale
    KImage *pImageGrayscale = pImage->ConvertToGrayscale();
    //Delete the original, now useless image
    delete pImage;

    //Verify conversion success
    if (pImageGrayscale == NULL || !pImageGrayscale->IsValid() || pImageGrayscale->GetBPP() != 8)
    {
        _tprintf(_T("Conversion to grayscale was not successfull!"));
        return -3;
    }
    
    //Request direct access to image pixels in raw format
    BYTE **pDataMatrixGrayscale = NULL;
    if (pImageGrayscale->BeginDirectAccess() && (pDataMatrixGrayscale = pImageGrayscale->GetDataMatrix()) != NULL)
    {
        int intWidth = pImageGrayscale->GetWidth();
        int intHeight = pImageGrayscale->GetHeight();

        KImage *pImageBinary = new KImage(intWidth, intHeight, 1);
		KImage *pImageConfidence = new KImage(intWidth, intHeight, 8);

		if (pImageBinary->BeginDirectAccess() && pImageConfidence->BeginDirectAccess())
        {
			int histData[256];
			for (int i = 0; i < 256; i++)
				histData[i] = 0;

			for (int y = intHeight - 1; y >= 0; y--)
			{
                for (int x = intWidth - 1; x >= 0; x--)
                {
                    BYTE &PixelAtXY = pDataMatrixGrayscale[y][x];
					int position = 0xFF & PixelAtXY;
					histData[position]++;
				}
			}

			int total = intHeight * intWidth;

			float sum = 0;
			for (int i = 0; i < 256; i++)
				sum += i * histData[i];

			float sumB = 0;
			int wB = 0;
			int wF = 0;

			float varMax = 0;
			float threshold = 0;

			for (int i = 0; i < 256; i++)
			{
				wB += histData[i];
				if (wB == 0) continue;

				wF = total - wB;
				if (wF == 0) break;

				sumB += (float)(i * histData[i]);

				float mB = sumB / wB;
				float mF = (sum - sumB) / wF;
				float varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);

				if (varBetween > varMax)
				{
					varMax = varBetween;
					threshold = i;
				}
			}

			for (int y = intHeight - 1; y >= 0; y--)
			{
                for (int x = intWidth - 1; x >= 0; x--)
                {
                    BYTE &PixelAtXY = pDataMatrixGrayscale[y][x];

					if (PixelAtXY < threshold)
					{
                        //...if closer to black, set to black
                        pImageBinary->Put1BPPPixel(x, y, false);
						if (threshold == 0) 
						{
							pImageConfidence->Put8BPPPixel(x, y, 0x0);
						}
						else
						{
							int conf = (int)(255 / threshold * PixelAtXY);
							pImageConfidence->Put8BPPPixel(x, y, conf);
						}
					}
                    else
					{
                        //...if closer to white, set to white
                        pImageBinary->Put1BPPPixel(x, y, true);
						if (threshold == 255) 
						{
							pImageConfidence->Put8BPPPixel(x, y, 0x0);
						}
						else
						{
							int conf = (int)(255 / (255 - threshold) * (PixelAtXY - threshold));
							pImageConfidence->Put8BPPPixel(x, y, conf);
						}
					}
				}
			}

            //Close direct access
            pImageBinary->EndDirectAccess();
			pImageConfidence->EndDirectAccess();

			mkdir("./dir_out");
            //Save binarized image
			_stprintf_s(strNewFileName, sizeof(strNewFileName) / sizeof(TCHAR), _T("./dir_out/%s"), argv[2]);
            pImageBinary->SaveAs(strNewFileName, SAVE_TIFF_CCITTFAX4);

			//Save confidence image
			_stprintf_s(strNewFileName, sizeof(strNewFileName) / sizeof(TCHAR), _T("./dir_out/%s"), argv[3]);
            pImageConfidence->SaveAs(strNewFileName, SAVE_TIFF_LZW);

            delete pImageBinary;
			delete pImageConfidence;
        }
        else
        {
            _tprintf(_T("Unable to obtain direct access in binary image or confidence image!"));
            return -3;
        }

        //Close direct access
        pImageGrayscale->EndDirectAccess();
    }
    else
    {
        _tprintf(_T("Unable to obtain direct access in grayscale image!"));
        return -3;
    }

    //Don't forget to delete the grayscale image
    delete pImageGrayscale;

    //Return with success
    return 0;
}
