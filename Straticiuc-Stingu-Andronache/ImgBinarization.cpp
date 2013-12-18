/*  Straticiuc Vicu 341C5
 *  Andronache Rodica-Elena 341C5
 *	Stingu Andreea-Lavinia 341C5
*/


#include "stdafx.h"
#include <iostream>
#include "ImgBinarization.h"
#include "Direct_Access_Image.h"

#define PART_HEIGHT 10
#define PART_WIDTH  10

ImgBinarization::ImgBinarization(KImage* image) :
	grayscale_(image), //imaginea initiala
	confidence_(nullptr), //imaginea cu confidente
	binared_(nullptr), //imaginea binarizata
	t_l_treshold_(128),
	t_r_treshold_(128),
	b_l_treshold_(128),
	b_r_treshold_(128)
{
	finished_.store(false);
	StartBinarization();	
}

void ImgBinarization::operator()(KImage* image)
{
	if( finished_.load() )
		finished_.store(false);
	StartBinarization();
}

ImgBinarization::~ImgBinarization(void)
{
}

/* se afla cele 4 treshold-uri pentru fiecare din cele 4 matrici
 * rezultate din impartirea matricei mari in 4
 */
void ImgBinarization::ComputeTresholds()
{
	int h_middle = height_ / 2;
	int w_middle = width_ / 2;

	t_l_treshold_ = GetTreshold(Point(0, 0), Point(h_middle, w_middle));
	t_r_treshold_ = GetTreshold(Point(0, w_middle), Point(h_middle, width_));
	b_l_treshold_ = GetTreshold(Point(h_middle, 0), Point(height_, w_middle));
	b_r_treshold_ = GetTreshold(Point(h_middle, w_middle), Point(height_, width_));
}


void ImgBinarization::StartBinarization()
{
	grayscale_->BeginDirectAccess();
	width_  = grayscale_->GetWidth();
	height_ = grayscale_->GetHeight();

	binared_    = shared_ptr<KImage>(new KImage(width_, height_, 1));
	confidence_ = shared_ptr<KImage>(new KImage(width_, height_, 8));

	binared_->BeginDirectAccess();
	confidence_->BeginDirectAccess();

	ComputeTresholds();

	/* 
	* se imparte matricea in portiuni mai mici care sunt apoi procesate
	*/
	const int wBlock = width_ / PART_WIDTH;
	const int hBlock = height_ / PART_HEIGHT;

	for (int i = 0; i < hBlock; i++)
	{
		Point top, bottom;

		top.x = i * PART_HEIGHT;
		bottom.x = (i + 1) * PART_HEIGHT;

		if( i == hBlock - 1 )
			bottom.x = height_;

		for (int j = 0; j < wBlock; j++)
		{
			top.y = j * PART_WIDTH;
			bottom.y = (j + 1) * PART_WIDTH;

			if( j == wBlock - 1 )
				bottom.y = width_;

			ProccesPart(top, bottom);
		}
	}

	binared_->EndDirectAccess();
	confidence_->EndDirectAccess();
	grayscale_->EndDirectAccess();
}

/* se prelucreaza matricile mai mici */
void ImgBinarization::ProccesPart(const Point& top, const Point& bottom)
{
	int min = 256, max = 0;
	const int totalPixels = (bottom.x - top.x) * (bottom.y - top.y); 
	const int threshold   = GetTreshold(top, bottom, &min, &max);

	int global_treshold; //treshold global

	if( top.x < height_ / 2 )
	{
		if( top.y < width_ / 2 )
			global_treshold = t_l_treshold_;
		else
			global_treshold = t_r_treshold_;
	}
	else
	{
		if( top.y < width_ / 2 )
			global_treshold = b_l_treshold_;
		else
			global_treshold = b_r_treshold_;
	}

	if( max - min < 60 )
	{
		//culoarea alba este preponderenta => verifica superficiala(-5)
		if( threshold > global_treshold - 5 )
		{
			SetColor(top, bottom, true, threshold);
			return;
		}

		//negru doar in cazuri foarte sigure trebuie pus(-40)
		if( threshold < global_treshold - 40 )
		{
			SetColor(top, bottom, false, threshold);
			return;
		}
	}

	int whiteCount = 0;

	for(int i = top.y; i < bottom.y; ++i)
	{
		for(int j = top.x; j < bottom.x; ++j)
		{
			const BYTE pixel = grayscale_->Get8BPPPixel(i, j);

			if( pixel > threshold )
			{
				whiteCount++;
				binared_->Put1BPPPixel(i, j, true);
				confidence_->Put8BPPPixel(i, j, 127 + pixel - threshold);
			}
			else
			{
				binared_->Put1BPPPixel(i, j, false);
				confidence_->Put8BPPPixel(i, j, 127 + threshold - pixel);
			}
		}
	}

	//inlaturare zgomot
	const double whitePonder = (float)whiteCount / totalPixels * 100;

	if( whitePonder > 85 )
		SetColor(top, bottom, true, threshold);
	else 
		if( whitePonder < 15 )
			SetColor(top, bottom, false, threshold);
	
	finished_.store(true);
}

/* setare valoare pixel in imaginea binarizata si imaginea de confidente */
void ImgBinarization::SetColor(const Point& top, const Point& bottom, bool is_white, int threshold)
{
	for(int i = top.y; i < bottom.y; ++i)
	{
		for(int j = top.x; j < bottom.x; ++j)
		{
			binared_->Put1BPPPixel(i, j, is_white);
			const BYTE pixel = grayscale_->Get8BPPPixel(i, j);

			if(pixel > threshold  && pixel < (threshold + 50));
			{
				
				 confidence_->Put8BPPPixel(i, j, 127 - (pixel - threshold));
			}

			if(pixel < threshold  && pixel > (threshold - 50))
			{
				
				 confidence_->Put8BPPPixel(i, j, 127 - (threshold - pixel));
			}

			if((pixel < (threshold - 50)) || (pixel > (threshold + 50)))
			{
				confidence_->Put8BPPPixel(i, j, 127 + (threshold - pixel));
			}

		}
	}
}

shared_ptr<KImage> ImgBinarization::GetBinaredImage()
{
	if( finished_.load() )
		return binared_;
	return nullptr;
}

shared_ptr<KImage> ImgBinarization::GetConfidence()
{
	if( finished_.load() )
		return confidence_;
	return nullptr;
	
}

int ImgBinarization::GetTreshold(const Point& top, const Point& bottom)
{
	int min, max;
	return GetTreshold(top, bottom, &min, &max);
}

/* calculare valoare trashold 
 * ca fiind media aritmetica a valorilor minime si maxime din matrice  
 */
int ImgBinarization::GetTreshold(const Point& top, const Point& bottom, int* min, int* max)
{
	*min = 255;
	*max = 0;

	for(int i = top.y; i < bottom.y; ++i)
	{
		for(int j = top.x; j < bottom.x; ++j)
		{
			const BYTE pixel = grayscale_->Get8BPPPixel(i, j);

			if( *min > pixel )
				*min = pixel;

			if( *max < pixel )
				*max = pixel;
		}
	}

	return (*max + *min) / 2;
}