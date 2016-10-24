/*@author Timothy Yo (Yang You)*/
#include "FBOF.h"
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include "Stopwatch.h"
#include <cstdio>
const int FBOF::MAX_KERNEL_LENGTH = 31;
const int FBOF::MIN_KERNEL_LENGTH = 3;

template <typename T>
static double CalcRatio(T x, T y) {
	return static_cast<double>(x)/static_cast<double>(y);
}

void FBOF::GetCC(cv::UMat& ubimg,cv::Mat&) {
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(ubimg,contours,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);
	auto& bbs = cache_meta_cur_->bbs_;
	for (const auto& contour : contours) {
		auto bb = cv::boundingRect(contour);
		bb += bb.size()/2;
		bb += bb.tl()/2;
		/*check validity of bb*/
		auto sideLen = std::minmax(bb.width,bb.height);
		if (sideLen.first == 0
				|| sideLen.first < i_min_sideLen_
				|| sideLen.second > i_max_sideLen_
				|| CalcRatio(sideLen.second,sideLen.first) > i_max_ratio_
				|| (marea_ & bb) != bb) {
			continue;
		}
		bbs.emplace_back(bb);
	}
}

template <typename... Ts>
void FBOF::CalcOF(Ts... params) {
	if (cache_pre2_.empty() || cache_cur_.empty()) return;
	/*Calc farneback optical flow*/
	cv::UMat uflow;
	cv::calcOpticalFlowFarneback(cache_pre2_,cache_cur_,uflow,params...);

	/*Get Flow values in orthodoxical manner*/
	std::vector<cv::UMat> udxy;
	cv::split(uflow,udxy);

	/*Calc magnitude of displacement for each pixel*/
	cv::UMat mimg;
	cv::magnitude(udxy[0],udxy[1],mimg);

	/*Update Thresh, if 0, using mean value*/
	double thresh;
	if (i_thresh_ == 0) {
		double max,min;
		cv::minMaxIdx(mimg,&max,&min);
		thresh = (max+min)/2;
	} else {
		thresh = i_thresh_;
	}

	/*Get Motion Binary image*/
	cv::UMat ubimg;
	cv::threshold(mimg,ubimg,thresh,255,cv::THRESH_BINARY);
	ubimg.convertTo(ubimg,CV_8UC1);
	cv::Mat flow;uflow.copyTo(flow);

	GetCC(ubimg,flow);
	return;
}

void FBOF::SetBypass(bool bypass) {
	bypass_ = bypass;
}

void FBOF::Run() {
	std::shared_ptr<Media> media_cache;
	while (input.Read(media_cache)) {
		marea_ = cv::Rect{1,1,media_cache->img.cols-2,media_cache->img.rows-2};
		cache_meta_cur_ = std::make_shared<BBS>(std::move(media_cache));

		if (bypass_) {
			cache_pre1_ = cv::UMat{}; cache_cur_ = cv::UMat{};
		} else {
			UpdateMeta();
			GaussianBlur(cache_meta_cur_->context_->gray,cache_cur_,
					cv::Size(i_gwin_,i_gwin_),0,0);
			/*Process Frames*/
			CalcOF(0.5,3,15,3,5,1.2,0);
			std::swap(cache_pre2_,cache_pre1_);
			std::swap(cache_pre1_,cache_cur_);
		}
		output.WriteBlocking(std::make_shared<MSG>(std::move(cache_meta_cur_),nullptr));
	}
	output.SetEOF();
}

void FBOF::SetGaussianWindow(int win) {
	boost::lock_guard<boost::mutex> lk{mux_};
	if (win < MIN_KERNEL_LENGTH) win = 3;
	if (win % 2 == 0) win += 1;
	if (win > MAX_KERNEL_LENGTH) win = 31;
	gwin_ = win;
}
void FBOF::SetMaxRatio(float) {
	boost::lock_guard<boost::mutex> lk{mux_};
}
void FBOF::SetMaxSideLen(int) {
	boost::lock_guard<boost::mutex> lk{mux_};
}
void FBOF::SetMinSideLen(int) {
	boost::lock_guard<boost::mutex> lk{mux_};
}
void FBOF::SetThresh(double thresh) {
	boost::lock_guard<boost::mutex> lk{mux_};
	thresh_ = thresh;
}

void FBOF::UpdateMeta() {
	boost::lock_guard<boost::mutex> lk{mux_};
	i_gwin_ = gwin_;
	i_max_ratio_ = max_ratio_;
	i_max_sideLen_ = max_sideLen_;
	i_min_sideLen_ = min_sideLen_;
	i_thresh_ = thresh_;
}

void FBOF::StartFback() {
	if (tfback_.joinable()) return;
	auto self = shared_from_this();
	tfback_ = boost::thread{
		[this,self]() {Run();}
	};
}
void FBOF::StopFback() {
	if (tfback_.joinable()) tfback_.join();
}
