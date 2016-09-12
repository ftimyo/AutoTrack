/* 
 * Struck: Structured Output Tracking with Kernels
 * 
 * Code to accompany the paper:
 *   Struck: Structured Output Tracking with Kernels
 *   Sam Hare, Amir Saffari, Philip H. S. Torr
 *   International Conference on Computer Vision (ICCV), 2011
 * 
 * Copyright (C) 2011 Sam Hare, Oxford Brookes University, Oxford, UK
 * 
 * This file is part of Struck.
 * 
 * Struck is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Struck is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Struck.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "ImageRep.h"

#include <cassert>

#include <opencv/highgui.h>


#include "util.h"
#include <stdio.h>

using namespace std;
using namespace cv;

static const int kNumBins = 16;

#if 1
static const int kLiveBoxWidth = 80;
static const int kLiveBoxHeight = 80;

float M_INF=1000000000;
float M_NINF=-1000000000;
int   M_NMAX=255;
int   M_NMIN=0;
#endif




ImageRep::ImageRep(const cv::Mat & image, bool colour):m_channels(colour ? 3 : 1),
	m_rect(0, 0, image.cols, image.rows)
{
	for (int i = 0; i < m_channels; ++i)
	{
		m_images.push_back(Mat(image.rows, image.cols, CV_8UC1));		
	}
	
	if (colour)
	{
		assert(image.channels() == 3);
		split(image, m_images);

//		imwrite("b.jpg",m_images[0]);
//		imwrite("g.jpg",m_images[1]);
//		imwrite("r.jpg",m_images[2]);
		/*m_rgbImages.push_back(m_images[2]);
		m_rgbImages.push_back(m_images[1]);
		m_rgbImages.push_back(m_images[0]);*/

		Mat o1 = Mat::zeros(image.rows,image.cols,CV_32FC1);
		Mat o2 = Mat::zeros(image.rows,image.cols,CV_32FC1);
		Mat o3 = Mat::zeros(image.rows,image.cols,CV_32FC1);
  //   	Mat o1o3 = Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat o2o3 = Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat hue = Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat nr=Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat ng=Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat nb=Mat::zeros(image.rows,image.cols,CV_32FC1);
		///*Mat r=Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat g=Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat b=Mat::zeros(image.rows,image.cols,CV_32FC1);*/
		//Mat tr=Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat tg=Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat tb=Mat::zeros(image.rows,image.cols,CV_32FC1);


		//Mat floatsrc = Mat::zeros(image.rows,image.cols,CV_32FC1);
		//Mat floathsv = Mat::zeros(image.rows,image.cols,CV_32FC1);
		//std::vector<cv::Mat> floathsvs;
		//for(int i=0;i<3;i++)
		//	floathsvs.push_back(Mat(image.rows, image.cols, CV_32FC1));

		//Mat floatlabsrc = Mat::zeros(image.rows,image.cols,CV_8UC1);
		//Mat floatlab = Mat::zeros(image.rows,image.cols,CV_8UC1);
		//std::vector<cv::Mat> floatlabs;
		//for(int i=0;i<3;i++)
		//	floatlabs.push_back(Mat(image.rows, image.cols, CV_8UC1));

		float min_1,max_1,min_2,max_2,min_3,max_3;


		for(int i=0;i<image.rows;i++){
			for(int j=0;j<image.cols;j++){
				//cout<<(int)m_images[2].at<uchar>(i,j)<<"  "<<(int)m_images[1].at<uchar>(i,j)<<endl;
				o1.at<float>(i,j)=((int)m_images[2].at<uchar>(i,j)-(int)m_images[1].at<uchar>(i,j))/sqrt(2.0);
				//cout<<o1.at<float>(i,j)<<endl;
				o2.at<float>(i,j)=((int)m_images[2].at<uchar>(i,j)+(int)m_images[1].at<uchar>(i,j)-2*(int)m_images[0].at<uchar>(i,j))/sqrt(6.0);
				//cout<<o2.at<float>(i,j)<<endl;
				o3.at<float>(i,j)=((int)m_images[0].at<uchar>(i,j)+(int)m_images[1].at<uchar>(i,j)+(int)m_images[2].at<uchar>(i,j))/sqrt(3.0);
				//cout<<o3.at<float>(i,j)<<endl;
			}
		}

		min_1=min_2=min_3=M_INF;
		max_1=max_2=max_3=M_NINF;
		for(int i=0;i<image.rows;i++){
			for(int j=0;j<image.cols;j++){
				float vo1 = o1.at<float>(i,j);
				float vo2 = o2.at<float>(i,j);
				float vo3=  o3.at<float>(i,j);
				if(vo1>max_1){
					max_1=vo1;
				}
				if(vo1<min_1){
					min_1=vo1;
				}
				if(vo2>max_2){
					max_2=vo2;
				}
				if(vo2<min_2){
					min_2=vo2;
				}
				if(vo3>max_3){
					max_3=vo3;
				}
				if(vo3<min_3){
					min_3=vo3;
				}

			}
		}
		for(int i=0;i<image.rows;i++){
			for(int j=0;j<image.cols;j++){
				if(min_1==max_1){
					o1.at<float>(i,j)=0.5;
				}
				else{
					o1.at<float>(i,j)=(o1.at<float>(i,j)-min_1)/(max_1-min_1);
				}
				o1.at<float>(i,j)=o1.at<float>(i,j)*(M_NMAX-M_NMIN)+M_NMIN;
				if(min_2==max_2){
					o2.at<float>(i,j)=0.5;
				}
				else{
					o2.at<float>(i,j)=(o2.at<float>(i,j)-min_2)/(max_2-min_2);
				}
				o2.at<float>(i,j)=o2.at<float>(i,j)*(M_NMAX-M_NMIN)+M_NMIN;
				if(min_3==max_3){
					o3.at<float>(i,j)=0.5;
				}
				else{
					o3.at<float>(i,j)=(o3.at<float>(i,j)-min_3)/(max_3-min_3);
				}
				o3.at<float>(i,j)=o3.at<float>(i,j)*(M_NMAX-M_NMIN)+M_NMIN;
			}
		}
		m_oppoImages.push_back(o1);
		m_oppoImages.push_back(o2);
		m_oppoImages.push_back(o3);
	}
	else
	{
		assert(image.channels() == 1 || image.channels() == 3);
		if (image.channels() == 3)
		{
			cvtColor(image, m_images[0], CV_RGB2GRAY);
		}
		else if (image.channels() == 1)
		{
			image.copyTo(m_images[0]);
		}
	}
}

//int ImageRep::Sum(const IntRect& rRect, int channel) const
//{
//	assert(rRect.XMin() >= 0 && rRect.YMin() >= 0 && rRect.XMax() <= m_images[0].cols && rRect.YMax() <= m_images[0].rows);
//	/*return m_integralImages[channel].at<int>(rRect.YMin(), rRect.XMin()) +
//			m_integralImages[channel].at<int>(rRect.YMax(), rRect.XMax()) -
//			m_integralImages[channel].at<int>(rRect.YMax(), rRect.XMin()) -
//			m_integralImages[channel].at<int>(rRect.YMin(), rRect.XMax());*/
//
//	switch(m_config.colorKind){
//	case Config::kIntensity:
//		return m_integralImages[channel].at<int>(rRect.YMin(), rRect.XMin()) +
//			m_integralImages[channel].at<int>(rRect.YMax(), rRect.XMax()) -
//			m_integralImages[channel].at<int>(rRect.YMax(), rRect.XMin()) -
//			m_integralImages[channel].at<int>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kOPPONENT:
//		return m_oppoIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_oppoIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_oppoIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_oppoIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kChromatic:
//		return m_chromaticIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_chromaticIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_chromaticIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_chromaticIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kNChromatic:
//		return m_nChromaticIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_nChromaticIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_nChromaticIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_nChromaticIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kHue:
//		return m_hueIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_hueIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_hueIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_hueIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kRGB:
//		return m_rgbIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_rgbIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_rgbIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_rgbIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kNRGB:
//		return m_nRGBIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_nRGBIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_nRGBIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_nRGBIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kTRGB:
//		return m_tRGBIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_tRGBIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_tRGBIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_tRGBIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kHSV:
//		return m_hsvIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_hsvIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_hsvIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_hsvIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kLAB:
//		return m_LABIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_LABIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_LABIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_LABIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	case Config::kYCbCr:
//		return m_YCbCrIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMin()) +
//			m_YCbCrIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMax()) -
//			m_YCbCrIntegralImages[channel].at<float>(rRect.YMax(), rRect.XMin()) -
//			m_YCbCrIntegralImages[channel].at<float>(rRect.YMin(), rRect.XMax());
//		break;
//	default:
//		break;
//	}
//
//}
//
//void ImageRep::Hist(const IntRect& rRect, Eigen::VectorXd& h) const
//{
//	assert(rRect.XMin() >= 0 && rRect.YMin() >= 0 && rRect.XMax() <= m_images[0].cols && rRect.YMax() <= m_images[0].rows);
//	int norm = rRect.Area();
//	for (int i = 0; i < kNumBins; ++i)
//	{
//		int sum = m_integralHistImages[i].at<int>(rRect.YMin(), rRect.XMin()) +
//			m_integralHistImages[i].at<int>(rRect.YMax(), rRect.XMax()) -
//			m_integralHistImages[i].at<int>(rRect.YMax(), rRect.XMin()) -
//			m_integralHistImages[i].at<int>(rRect.YMin(), rRect.XMax());
//		h[i] = (float)sum/norm;
//	}
//}
