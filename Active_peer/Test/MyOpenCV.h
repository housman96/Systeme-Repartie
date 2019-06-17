#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>  
#include <vector>
#include <thread> 

using namespace std;
using namespace cv;

namespace myOpenCV
{
	int luminanceEqualization(Mat& src, Mat& res);

	int luminanceNormalisation(Mat& src, Mat& res);

	int saturationSetting(Mat& src, Mat& res, int param);

	int saturationSettingLocalThreaded(int param, int place, vector<Mat>& stack, Mat& img);

	int saturationSettingThreaded(Mat& src, Mat& res, int param);

	int saturationSettingSocket(Mat& src, Mat& res, int param);


}
