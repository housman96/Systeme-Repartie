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

	int saturationSettingLocalThreaded(int param, int firstRow, int lastRow, Mat& img);

	int saturationSettingThreadedSetup(int param, int firstRow, int lastRow, Mat & img, int threadNumber, vector<thread> & threads);

	int saturationSettingThreaded(Mat & src, Mat & res, int param);

	int saturationSettingSocket(Mat & src, Mat & res, int param);
}
