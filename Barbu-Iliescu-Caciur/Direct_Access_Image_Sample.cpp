//===========================================================================
//===========================================================================
//===========================================================================
//==   Direct_Access_Image_Sample.cpp  ==  Author: Costin-Anton BOIANGIU   ==
//===========================================================================
//===========================================================================
//===========================================================================

//===========================================================================
//===========================================================================
#include "stdafx.h"
#include "Direct_Access_Image.h"
#include "stdlib.h"
#include "conio.h"
#include "string.h"
#include <direct.h>
//===========================================================================
//===========================================================================

//===========================================================================
//===========================================================================

#pragma region Matrix Allocation and Deallocation

BYTE** AllocateByteMatrix(int height, int width)
{
	BYTE** matrix = new BYTE*[height];
	
	for(int i = 0; i < height; i++)
		matrix[i] = new BYTE[width];
	
	return matrix;
}

void DeallocateByteMatrix(BYTE** matrix, int height)
{
	for(int i = 0; i < height; i++)
	{
		delete matrix[i];
	}

	delete[] matrix;
}

#pragma endregion

#pragma region Auxiliary Methods

double ComputeLocalStats( BYTE ** imageSrc, BYTE ** localMeans, BYTE ** standardDeviationMatrix, int winWidth, int winHeight, int height, int width) {

    double maxDeviation=0, sum, squareSum;
	int currentPixel;
    int winHalfWidth = winWidth / 2;
    int winHalfHeight = winHeight / 2;
    int xFirstTh = winHalfWidth;
    int yLastTh = height - winHalfHeight - 1;
    int yFirstTh = winHalfHeight;
    double winArea = winWidth * winHeight;

	for (int j = yFirstTh ; j <= yLastTh; j++) 
    {
        //First windows from current Row
        sum = squareSum = 0;
        for (int wy = 0 ; wy < winHeight; wy++)
            for (int wx = 0 ; wx < winWidth; wx++) {
                currentPixel = imageSrc[j - winHalfHeight + wy][wx];
                sum += currentPixel;
                squareSum += pow((double)currentPixel, 2);
            }
       
		localMeans[j][xFirstTh] = sum / winArea;
        standardDeviationMatrix[j][xFirstTh] = sqrt((squareSum - pow(sum,2) / winArea) / winArea);
		if (standardDeviationMatrix[j][xFirstTh] > maxDeviation)
            maxDeviation = standardDeviationMatrix[j][xFirstTh];

        //Move window to right until we hit the border
        for (int i = 1 ; i <= width - winWidth; ++i) {

            //Delete influence of the first column of the old window in the new windows
            for (int wy = 0; wy < winHeight; ++wy) {
                currentPixel = imageSrc[j - winHalfHeight + wy][i - 1];
                sum -= currentPixel;
                squareSum -= pow((double)currentPixel,2);
                currentPixel = imageSrc[j - winHalfHeight + wy][i + winWidth - 1];
                sum += currentPixel;
                squareSum += pow((double)currentPixel,2);
            }

            localMeans[j][i + winHalfWidth] = (double)sum / winArea;;
            standardDeviationMatrix[j][i + winHalfWidth] =(double) sqrt ((squareSum - pow(sum,2)/winArea)/winArea);

			if (standardDeviationMatrix[j][i + winHalfWidth] > maxDeviation)
                maxDeviation = standardDeviationMatrix[j][i + winHalfWidth];
        }
    }

    return maxDeviation;
}

void MinMaxElementValue(BYTE ** imageSrc, double* min, double* max, int height, int width) {
	int i, j;
	*max = imageSrc[0][0];
	*min = imageSrc[0][0];

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			if (imageSrc[i][j] > *max) {
				*max = imageSrc[i][j];
			}

			if (imageSrc[i][j] < *min) {
				*min = imageSrc[i][j];
			}
		}
	}
}

int ComputeConfidence(BYTE pixel,BYTE localThreshold)
{
	if(localThreshold == 0 || localThreshold == 255)
		return 255;
	else
		return pixel > localThreshold ? ((double)(255 * (pixel - localThreshold)) / abs(255 - localThreshold)) : ((double)(255 * abs(localThreshold - pixel)) / abs(0 - localThreshold));
}

#pragma endregion

//Sauvola Method

void SauvolaAlgorithm (BYTE ** imageSrc,int height, int width, double k, double dR, KImage *outputImage, KImage *outputConfidenceImage) {

    double m, s, max_s;
    double th = 0;
    double min_I, max_I;
	int winHeight = (2.0 * height - 1) / 3;
	int winWidth =  width - 1 < winHeight ? width - 1 : winHeight;
	if (winWidth > 100)
       winWidth = winHeight = 40;
    int winHalfWidth = winWidth / 2;
    int winHalfHeight = winHeight / 2;
    int xFirstTh= winHalfWidth;
    int xLastTh = width-winHalfWidth - 1;
    int yLastTh = height-winHalfHeight - 1;
    int yFirstTh = winHalfHeight;
	int localConfidencePixel;
	bool pixelType;
	int p, q;

    // Create local statistics and store them in a double matrices
	BYTE ** localMeans = AllocateByteMatrix(height,width);
	BYTE ** standardDeviationMatrix = AllocateByteMatrix(height,width);

	//Threshold Surface matrix
	BYTE ** thresholdMatrix = AllocateByteMatrix(height,width);

    max_s = ComputeLocalStats(imageSrc, localMeans, standardDeviationMatrix, winWidth, winHeight, height, width);

    MinMaxElementValue(imageSrc, &min_I, &max_I, height, width);

	#pragma region Create the threshold surface, including border processing

    for (int j = yFirstTh ; j <= yLastTh; j++) 
	{
        // NORMAL, NON-BORDER AREA IN THE MIDDLE OF THE WINDOW:
        for (int i = 0 ; i <= width - winWidth; i++) {
            m  = localMeans[j][i + winHalfWidth];
            s  = standardDeviationMatrix[j][i + winHalfWidth];
            
			// Calculate the threshold with Sauvola formula
            th = m * (1 + k * (s / dR - 1));

            thresholdMatrix[j][i + winHalfWidth] = th;

            if (i == 0) {
                // save threshol left border
                for (p = 0; p < xFirstTh; ++p)
                    thresholdMatrix[j][p] = th;

                // save threshold left up border
                if (j == yFirstTh)
				{
                    for (p = 0; p < yFirstTh; ++p)
						for (q = 0; q <= xFirstTh; ++q)
						 thresholdMatrix[p][q] = th;
				}

                // save threshold left low border
                if (j == yLastTh)
                    for (p = yLastTh + 1; p < height; ++p)
						for (int q = 0; q <= xFirstTh; ++q)
							thresholdMatrix[p][q] = th;
            }

            // save threshold upper border
            if (j == yFirstTh)
                for (p = 0; p < yFirstTh; ++p)
                    thresholdMatrix[p][i + winHalfWidth] = th;

            // save threshold lower border
            if (j == yLastTh)
                for (p = yLastTh + 1; p < height; ++p)
                    thresholdMatrix[p][i+ winHalfWidth] = th;
        }

        // save threshold for right border
        for (p = xLastTh; p < width; ++p)
            thresholdMatrix[j][p] = th;

        // save threshold value for right up corner
        if (j == yFirstTh)
		{
            for (p = 0; p < yFirstTh; ++p)
				for (q = xLastTh; q < width; ++q)
					thresholdMatrix[p][q] = th;
		}

        // save threshold value for right low corner
        if (j == yLastTh)
		{
            for (p = yLastTh+1; p < height; ++p)
				for (q = xLastTh; q < width; ++q)
					thresholdMatrix[p][q] = th;
		}
	}

	#pragma endregion Finalize Threshold surface and border processing

    for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
		{
			pixelType = (imageSrc[y][x] >= thresholdMatrix[y][x]) ? true : false;
			localConfidencePixel = ComputeConfidence(imageSrc[y][x],thresholdMatrix[y][x]);
			outputImage->Put1BPPPixel(x,y,pixelType);
			outputConfidenceImage->Put8BPPPixel(x, y, localConfidencePixel);
		}

	//Free Memory
	DeallocateByteMatrix(localMeans,height);
	DeallocateByteMatrix(standardDeviationMatrix,height);
	DeallocateByteMatrix(thresholdMatrix,height);
}


int _tmain(int argc, _TCHAR* argv[])
{
    //Verify command-line usage correctness
    if (argc != 4)
    {
        _tprintf(_T("Use: %s <Input_Image_File_Name (24BPP True-Color)> <Output1BPP Image> <Output8BPP Confidence Image>\n"), argv[0]);
        return -1;
    }

    //Buffer for the new file names (1bpp image and confidence image)
    TCHAR output1BPPImage[0x100],outputConfidenceImage[0x100];

    //Load and verify that input image is a True-Color one
    KImage *pImage = new KImage(argv[1]);
    if (pImage == NULL || !pImage->IsValid() || pImage->GetBPP() != 24)
    {
        _tprintf(_T("File %s does is not a valid True-Color image!"), argv[0]);
        return -2;
    }

	#pragma region Image filters

    //Apply a Gaussian Blur with small radius to remove obvious noise
    pImage->GaussianBlur(0.5);
   
	//Convert to grayscale
    KImage *pImageGrayscale = pImage->ConvertToGrayscale();
    //Don't forget to delete the original, now useless image
    delete pImage;

	#pragma endregion

    if (pImageGrayscale == NULL || !pImageGrayscale->IsValid() || pImageGrayscale->GetBPP() != 8)
    {
        _tprintf(_T("Conversion to grayscale was not successfull!"));
        return -3;
    }
    
    //Request direct access to image pixels in raw format
    BYTE **pDataMatrixGrayscale = NULL;
    
	if (pImageGrayscale->BeginDirectAccess() && (pDataMatrixGrayscale = pImageGrayscale->GetDataMatrix()) != NULL)
    {
        //If direct access is obtained get image attributes and start processing pixels
        int intWidth = pImageGrayscale->GetWidth();
        int intHeight = pImageGrayscale->GetHeight();

        //Create binary image
        KImage *pImageBinary = new KImage(intWidth, intHeight, 1);
		KImage *confidenceImage = new KImage(intWidth,intHeight,8);

        if (pImageBinary->BeginDirectAccess() && confidenceImage->BeginDirectAccess())
        {
			//Applying Sauvola Binarization Algorithm
			SauvolaAlgorithm(pDataMatrixGrayscale, intHeight, intWidth, CONSTANT_BINARIZATION, CONST_DINAM_DEVIATION, pImageBinary, confidenceImage);
  
            //Close direct access
            pImageBinary->EndDirectAccess();
            confidenceImage->EndDirectAccess();
			
			//Create Output directory
			//mkdir("./dir_out");

            //Save binarized image
            _stprintf_s(output1BPPImage, sizeof(output1BPPImage) / sizeof(TCHAR), _T("%s"), argv[2]);
            pImageBinary->SaveAs(output1BPPImage, SAVE_TIFF_CCITTFAX4);

			//Save confidence image
			_stprintf_s(outputConfidenceImage, sizeof(outputConfidenceImage) / sizeof(TCHAR), _T("%s"), argv[3]);
            confidenceImage->SaveAs(outputConfidenceImage, SAVE_TIFF_LZW);

            //Delete the binary image and confidence image
            delete pImageBinary;
			delete confidenceImage;
        }
        else
        {
            _tprintf(_T("Unable to obtain direct access in binary image!"));
            return -3;
        }

        //Close direct access
        pImageGrayscale->EndDirectAccess();
    }
    else
    {
        _tprintf(_T("Unable to obtain direct access in grayscale  image!"));
        return -4;
    }

    //Delete Grayscale image
    delete pImageGrayscale;

    //Return with success
    return 0;
}


//===========================================================================
//===========================================================================
