#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>  
#include <vector>

using namespace std;
using namespace cv;


namespace myOpenCV
{
	int luminanceEqualization(Mat& src, Mat& res);

	int luminanceNormalisation(Mat& src, Mat& res);

	int saturationSetting(Mat& src, Mat& res, int param);

	int saturationSettingOptimized(int param, Mat& img);

	int saturationSettingThreaded(Mat& src, Mat& res, int param);
}

