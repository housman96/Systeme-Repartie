#include <opencv2/opencv.hpp>
#include <opencv2/core/cuda.hpp>
#include <iostream>
#include <algorithm>  
#include <vector>

using namespace std;
using namespace cv;
using namespace cv::cuda;

namespace myOpenCV {
	int luminanceEqualization(Mat& src, Mat& res);
}

class Main
{
public:
	Main();
	~Main();

};


