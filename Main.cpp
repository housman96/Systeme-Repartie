#include "Main.h"

using namespace myOpenCV;
#include <ctime>
Main::Main()
{
}


Main::~Main()
{
}
const int alpha_slider_max = 100;
int alpha_slider;
double alpha;
Mat img;
Mat newImg;


namespace myOpenCV {
	int luminanceEqualization(Mat& src, Mat& res) {
		Mat imgTampon[3];
		cvtColor(src, res, COLOR_BGR2YCrCb);
		split(res, imgTampon);
		equalizeHist(imgTampon[0], imgTampon[0]);
		merge(imgTampon, 3, res);
		cvtColor(res, res, COLOR_YCrCb2BGR);
		return 0;
	}

	int luminanceNormalisation(Mat& src, Mat& res) {
		Mat imgTampon[3];
		cvtColor(src, res, COLOR_BGR2YCrCb);
		split(res, imgTampon);
		normalize(imgTampon[0], imgTampon[0], 0, 255, NORM_MINMAX);
		merge(imgTampon, 3, res);
		cvtColor(res, res, COLOR_YCrCb2BGR);
		return 0;
	}

	int saturationSetting(Mat& src, Mat& res, int param) {
		Mat imgTampon[3];
		cvtColor(src, res, COLOR_BGR2HSV);
		MatIterator_<Vec3b> it, end;
		for (it = res.begin<Vec3b>(), end = res.end<Vec3b>(); it != end; ++it)
		{
			(*it)[1] += param;
		}
		cvtColor(res, res, COLOR_HSV2BGR);
		return 0;
	}


	int saturationSettingLocalThreaded(int param, MatIterator_<Vec3b> it, MatIterator_<Vec3b> end) {
		float localParam = (float)param / 255;
		clock_t time;
		time = clock();
		for (it, end; it != end; ++it)
		{
			/*BGR 2 HSV*/
			uchar& bU = (*it)[0];
			uchar& gU = (*it)[1];
			uchar& rU = (*it)[2];

			float b = ((float)bU) / 255.f;
			float g = ((float)gU) / 255.f;
			float r = ((float)rU) / 255.f;

			float min;
			float max;

			if (r >= g && r >= b) {
				max = r;
			}
			else if (g >= b) {
				max = g;
			}
			else {
				max = b;
			}

			if (r <= g && r <= b) {
				min = r;
			}
			else if (g <= b) {
				min = g;
			}
			else {
				min = b;
			}


			float saturation = 0;

			float teinture;

			if (max != 0) {
				saturation = 1.f - (min / max);
			}

			if (max == min) {
				teinture = 0;
			}
			else if (max == r) {
				teinture = std::fmod(((60.f * (g - b)) / (max - min)) + 360.f, 360);
			}
			else if (max == g) {
				teinture = (60.f * (b - r) / (max - min)) + 120.f;
			}
			else if (max == b) {
				teinture = (60.f * (r - g) / (max - min)) + 240.f;
			}


			/*Modification de la saturation*/
			saturation += localParam;

			max = ((uchar)(max * 255)) / 255.f;
			saturation = ((uchar)(saturation * 255)) / 255.f;
			teinture = ((uchar)(teinture / 2)) * 2;

			/*HSV 2 BGR*/


			float C = max * saturation;
			float Hp = teinture / 60;
			float X = C * (1 - std::abs(std::fmod(Hp, 2) - 1));
			float m = max - C;

			if (0 <= Hp && Hp <= 1) {
				r = C;
				g = X;
				b = 0;
			}
			else if (1 < Hp && Hp <= 2) {
				r = X;
				g = C;
				b = 0;
			}
			else if (2 < Hp && Hp <= 3) {
				r = 0;
				g = C;
				b = X;
			}
			else if (3 < Hp && Hp <= 4) {
				r = 0;
				g = X;
				b = C;
			}
			else if (4 < Hp && Hp <= 5) {
				r = X;
				g = 0;
				b = C;
			}
			else if (5 < Hp && Hp <= 6) {
				r = C;
				g = 0;
				b = X;
			}

			r += m;
			g += m;
			b += m;

			rU = r * 255;
			gU = g * 255;
			bU = b * 255;
		}

		return 0;
	}

	int saturationSettingThreadedSetup(int param, MatIterator_<Vec3b> it, MatIterator_<Vec3b> end, int threadNumber, vector<thread> & threads) {
		MatIterator_<Vec3b> mid1;
		MatIterator_<Vec3b> mid2;
		if (threadNumber >= 2) {
			mid1 = it;
			ptrdiff_t shift = (end.lpos() - it.lpos()) / 2;
			mid1.operator+=(shift);
			mid2 = mid1;
			mid2++;
		}

		if (threadNumber > 2) {

			saturationSettingThreadedSetup(param, it, mid1, threadNumber / 2, threads);
			saturationSettingThreadedSetup(param, mid2, end, (threadNumber % 2) + (threadNumber / 2), threads);
		}
		else {
			if (threadNumber == 1) {
				threads.push_back(thread(saturationSettingLocalThreaded, param, it, end));
			}
			if (threadNumber == 2) {
				threads.push_back(thread(saturationSettingLocalThreaded, param, it, mid1));
				threads.push_back(thread(saturationSettingLocalThreaded, param, mid2, end));
			}
		}
		return 0;
	}

	int saturationSettingThreaded(Mat & src, Mat & res, int param) {
		unsigned int n = std::thread::hardware_concurrency();
		res = src.clone();
		MatIterator_<Vec3b> it = res.begin<Vec3b>(), end = res.end<Vec3b>();
		vector<thread> threads = vector<thread>();
		saturationSettingThreadedSetup(param, it, end, n, threads);
		for (vector<thread>::iterator itThreads = threads.begin(); itThreads != threads.end(); itThreads++) {
			(*itThreads).join();
		}

		return 0;
	}

	int saturationSettingSocket(Mat & src, Mat & res, int param) {
		res = src.clone();
		Mat img1;
		Mat img2;

		Rect rect1 = Rect(0, 0, res.cols, res.rows / 2);
		img1 = res(rect1).clone();

		Rect rect2 = Rect(0, (res.rows / 2) + 1, res.cols, res.rows - 1 - (res.rows / 2));
		img2 = res(rect2).clone();

		saturationSettingThreaded(img1, img1, param);/*ICI TU DOIS REMPLACER CETTE FONCTION PAR TA SOCKET QUI DOIT APPELER CETTE FONCTION*/
		saturationSettingThreaded(img2, img2, param);

		res = Mat(0, 0, res.type());

		res.push_back(img1);
		res.push_back(img2);

		return 0;
	}

	static void on_trackbar(int, void*)
	{
		clock_t time;
		time = clock();
		saturationSettingSocket(img, newImg, alpha_slider);
		cout << "Total Time" << ((float)clock() - time) / CLOCKS_PER_SEC << endl;
		imshow("imageAfter", newImg);
	}

}



int main(int argc, char* argv[]) {
	img = imread("Paysage.jpg");
	setUseOptimized(true);
	if (!img.empty()) {

		namedWindow("image", WINDOW_NORMAL);
		namedWindow("imageAfter", WINDOW_NORMAL);

		alpha_slider = 0;

		createTrackbar("test", "imageAfter", &alpha_slider, alpha_slider_max, on_trackbar);
		on_trackbar(alpha_slider, 0);
		imshow("image", img);

		waitKey(0);
	}
	return 0;
}
