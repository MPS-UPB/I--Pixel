#pragma once

#include <atomic>
#include <memory>

class KImage;

struct Point
{
	unsigned x;
	unsigned y;
	Point(){};
	Point(unsigned x_val, unsigned y_val) :
		x(x_val), y(y_val){}
	~Point(){};
};

using std::atomic_bool;
using std::shared_ptr;

class ImgBinarization
{
	public:
		//Grayscale image  for binarization
		ImgBinarization(KImage* image);
		~ImgBinarization(void);
		void operator()(KImage* image);

		shared_ptr<KImage> GetBinaredImage();
		shared_ptr<KImage> GetConfidence();

	private:
		KImage*			   grayscale_;
		shared_ptr<KImage> binared_;
		shared_ptr<KImage> confidence_;
		atomic_bool		   finished_;
		int				   width_;
		int				   height_;
		int				   t_l_treshold_;
		int				   t_r_treshold_;
		int				   b_l_treshold_;
		int				   b_r_treshold_;

		void StartBinarization();
		void ProccesPart(const Point& top, const Point& bottom);
		void SetColor(const Point& top, const Point& bottom, bool is_white, int treshold);
		int  GetTreshold(const Point& top, const Point& bottom);
		int  GetTreshold(const Point& top, const Point& bottom, int* min, int* max);
		void ComputeTresholds();//compute's t_l_treshold_, t_r_treshold_, b_l_treshold_ and b_r_treshold_
};
