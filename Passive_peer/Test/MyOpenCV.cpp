#include "pch.h"
#include "MyOpenCV.h"


namespace myOpenCV
{
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

	int saturationSettingLocalThreaded(int param, int firstRow, int lastRow, Mat& img) {

		int channels = img.channels();
		int endCols = (img.cols) * channels;
		int startCols = 0;
		if (img.isContinuous())
		{

			startCols = firstRow * endCols;
			endCols *= lastRow;
			//cout << startCols << "  " << endCols << endl;
			firstRow = 0;
			lastRow = 0;
		}

		uchar* p;

		float localParam = (float)param / 255;

		for (int i = firstRow; i <= lastRow; i++)
		{
			p = img.ptr<uchar>(i);

			for (int j = startCols; j <= endCols - channels; j += channels) {

				/*BGR 2 HSV*/

				uchar& bU = p[j + 2];
				uchar& gU = p[j + 1];
				uchar& rU = p[j];

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
				if (saturation + localParam < 1) {
					saturation += localParam;
				}
				else {
					saturation = 1;
				}

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
		}
		return 0;
	}

	int saturationSettingThreadedSetup(int param, int firstRow, int lastRow, Mat & img, int threadNumber, vector<thread> & threads) {
		int mid1;
		int mid2;
		if (threadNumber >= 2) {
			mid1 = firstRow + (lastRow - firstRow) / 2;
			mid2 = mid1 + 1;
		}

		if (threadNumber > 2) {

			saturationSettingThreadedSetup(param, firstRow, mid1, img, threadNumber / 2, threads);
			saturationSettingThreadedSetup(param, mid2, lastRow, img, (threadNumber % 2) + (threadNumber / 2), threads);
		}
		else {
			if (threadNumber == 1) {
				threads.push_back(thread(saturationSettingLocalThreaded, param, firstRow, lastRow, ref(img)));
			}
			if (threadNumber == 2) {
				threads.push_back(thread(saturationSettingLocalThreaded, param, firstRow, mid1, ref(img)));
				threads.push_back(thread(saturationSettingLocalThreaded, param, mid2, lastRow, ref(img)));
			}
		}
		return 0;
	}

	int saturationSettingThreaded(Mat & src, Mat & res, int param) {
		unsigned int n = std::thread::hardware_concurrency();
		if (&res != &src)
			res = src.clone();
		vector<thread> threads = vector<thread>();
		saturationSettingThreadedSetup(param, 0, res.rows, res, n, threads);
		for (vector<thread>::iterator itThreads = threads.begin(); itThreads != threads.end(); itThreads++) {
			(*itThreads).join();
		}

		return 0;
	}

	int saturationSettingSocket(Mat & src, Mat & res, int param) {
		if (&res != &src) {
			res = src.clone();
		}
			
		Mat img1;
		Mat img2;

		Rect rect1 = Rect(0, 0, res.cols, res.rows / 2);
		img1 = res(rect1).clone();

		Rect rect2 = Rect(0, (res.rows / 2) + 1, res.cols, res.rows - 1 - (res.rows / 2));
		img2 = res(rect2).clone();

		saturationSettingThreaded(img1, img1, param);/*ICI TU DOIS REMPLACER CETTE FONCTION PAR TA SOCKET QUI DOIT APPELER CETTE FONCTION*/
		saturationSettingThreaded(img2, img2, param);

		//imshow("imageAfter", img1);
		//imshow("image", img2);

		res = Mat(0, 0, res.type());

		res.push_back(img1);
		res.push_back(img2);

		return 0;
	}
}