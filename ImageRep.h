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

#ifndef IMAGE_REP_H
#define IMAGE_REP_H

#include "Rect.h"

#include <opencv/cv.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

#include <Eigen/Core>
//#include "Config.h"

//class Config;

class ImageRep
{
public:
	
//	ImageRep(const Config &conf, const cv::Mat& rImage, bool computeIntegral, bool computeIntegralHists, bool colour = false);
	ImageRep(const cv::Mat & rImage, bool colour = false);
	
//	int Sum(const IntRect& rRect, int channel = 0) const;
//	void Hist(const IntRect& rRect, Eigen::VectorXd& h) const;
	
	inline const cv::Mat& GetImage(int channel = 0) const { return m_images[channel]; }
	inline const cv::Mat& GetOppoImage(int channel = 0) const { return m_oppoImages[channel]; }
	inline const IntRect& GetRect() const { return m_rect; }

private:
	std::vector<cv::Mat> m_images;
	std::vector<cv::Mat> m_integralImages;
	std::vector<cv::Mat> m_integralHistImages;
	std::vector<cv::Mat> m_chromaticImages;
	std::vector<cv::Mat> m_chromaticIntegralImages;
	std::vector<cv::Mat> m_nChromaticImages;
	std::vector<cv::Mat> m_nChromaticIntegralImages;
	std::vector<cv::Mat> m_hueImages;
	std::vector<cv::Mat> m_hueIntegralImages;
	std::vector<cv::Mat> m_rgbImages;
	std::vector<cv::Mat> m_rgbIntegralImages;
	std::vector<cv::Mat> m_hsvImages;
	std::vector<cv::Mat> m_hsvIntegralImages;
	std::vector<cv::Mat> m_nRGBImages;
	std::vector<cv::Mat> m_nRGBIntegralImages;
	std::vector<cv::Mat> m_oppoImages;
	std::vector<cv::Mat> m_oppoIntegralImages;
	std::vector<cv::Mat> m_tRGBImages;
	std::vector<cv::Mat> m_tRGBIntegralImages;
	std::vector<cv::Mat> m_LABImages;
	std::vector<cv::Mat> m_LABIntegralImages;
	std::vector<cv::Mat> m_YCbCrImages;
	std::vector<cv::Mat> m_YCbCrIntegralImages;

	int m_channels;
	IntRect m_rect;
//	Config m_config;
};
#endif
