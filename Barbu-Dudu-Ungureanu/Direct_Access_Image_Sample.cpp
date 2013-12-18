#include "stdafx.h"
#include "Direct_Access_Image.h"

int _tmain(int argc, _TCHAR* argv[])
{
    //Verify command-line usage correctness
    if (argc != 4)
    {
        _tprintf(_T("Use: %s <Input_Image_File_Name (24BPP True-Color)> <Output_Image_File_Name> <Output_Confidence_Image_File_Name>\n"), argv[0]);
        return -1;
    }

    //Buffer for the new file names
    TCHAR strNeweightForegroundileName[0x100];

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
			// Calculate histogram
			int histogram[256];
			for (int i = 0; i < 256; i++)
				histogram[i] = 0;

			for (int y = intHeight - 1; y >= 0; y--)
			{
                for (int x = intWidth - 1; x >= 0; x--)
                {
                    BYTE &PixelAtXY = pDataMatrixGrayscale[y][x];
					int position = 0xFF & PixelAtXY;
					histogram[position]++;
				}
			}

			int total = intHeight * intWidth;							// Total number of pixels

			float sum = 0;
			for (int i = 0; i < 256; i++)
				sum += i * histogram[i];

			float sumB = 0;
			int weightBackground = 0;
			int weightForeground = 0;

			float varMax = 0;
			float threshold = 0;

			for (int i = 0; i < 256; i++)
			{
				weightBackground += histogram[i];						// Weight Background
				if (weightBackground == 0) continue;

				weightForeground = total - weightBackground;			// Weight Foreground
				if (weightForeground == 0) break;

				sumB += (float)(i * histogram[i]);

				float meanBackground = sumB / weightBackground;			// Mean Background
				float meanForeground = (sum - sumB) / weightForeground;	// Mean Foreground
				
				// Calculate Between Class Variance
				float varianceBetween = 
					(float)weightBackground 
					* (float)weightForeground 
					* (meanBackground - meanForeground) 
					* (meanBackground - meanForeground);

				// Check if new maximum found
				if (varianceBetween > varMax)
				{
					varMax = varianceBetween;
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
                        //	If closer to black, set to black
                        pImageBinary->Put1BPPPixel(x, y, false);
						if (threshold == 0) 
						{
							pImageConfidence->Put8BPPPixel(x, y, 0x0);
						}
						else
						{
							int confidence = (int)(255 / threshold * PixelAtXY);
							pImageConfidence->Put8BPPPixel(x, y, confidence);
						}
					}
                    else
					{
                        // If closer to white, set to white
                        pImageBinary->Put1BPPPixel(x, y, true);
						if (threshold == 255) 
						{
							pImageConfidence->Put8BPPPixel(x, y, 0x0);
						}
						else
						{
							int confidence = (int)(255 / (255 - threshold) * (PixelAtXY - threshold));
							pImageConfidence->Put8BPPPixel(x, y, confidence);
						}
					}
				}
			}

            //Close direct access
            pImageBinary->EndDirectAccess();
			pImageConfidence->EndDirectAccess();

            //Save binarized image
			_stprintf_s(strNeweightForegroundileName, sizeof(strNeweightForegroundileName) / sizeof(TCHAR), _T("%s"), argv[2]);
            pImageBinary->SaveAs(strNeweightForegroundileName, SAVE_TIFF_CCITTFAX4);

			//Save confidence image
			_stprintf_s(strNeweightForegroundileName, sizeof(strNeweightForegroundileName) / sizeof(TCHAR), _T("%s"), argv[3]);
            pImageConfidence->SaveAs(strNeweightForegroundileName, SAVE_TIFF_LZW);

			// Delete images
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
