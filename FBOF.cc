/*@author Timothy Yo (Yang You)*/
#include "FBOF.h"
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include "Stopwatch.h"
#include <cstdio>
constexpr int FBOF::MAX_KERNEL_LENGTH = 31;
constexpr int FBOF::MIN_KERNEL_LENGTH = 3;
constexpr int FBOF::MAX_RATIO_MAX = 5;
constexpr int FBOF::SIDELEN_MAX = 500;
constexpr int FBOF::SIDELEN_MIN = 1;
constexpr int FBOF::MAX_THRESH = 150;

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
	if (i_thresh_ < 0.01) {
		double max,min;
		cv::minMaxIdx(mimg,&max,&min);
		thresh = (max+min)/2;
	} else {
		thresh = i_thresh_;
	}

	/*Get Motion Binary image*/
	cv::UMat ubimg;
	cv::threshold(mimg,ubimg,thresh,255,cv::THRESH_BINARY);
	/*Convert Binary image to single Channel*/
	cv::UMat tmp;
	ubimg.convertTo(tmp,CV_8UC1);
	std::swap(ubimg,tmp);
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
double FBOF::SpeedToPixel(int speed, int height) {
	if (height == 0) height = 10;
	return speed / height;
}
int FBOF::SetGaussianWindow(int win) {
	if (win < MIN_KERNEL_LENGTH) win = MIN_KERNEL_LENGTH;
	if (win % 2 == 0) win += 1;
	if (win > MAX_KERNEL_LENGTH) win = MAX_KERNEL_LENGTH;
	boost::lock_guard<boost::mutex> lk{mux_};
	gwin_ = win;
	return gwin_;
}
int FBOF::SetMaxRatio(int ratio) {
	if (ratio < 1) ratio = 1;
	if (ratio > MAX_RATIO_MAX) ratio = MAX_RATIO_MAX;
	boost::lock_guard<boost::mutex> lk{mux_};
	max_ratio_ = ratio;
	return max_ratio_;
}
int FBOF::SetMaxSideLen(int xlen) {
	if (xlen > SIDELEN_MAX) xlen = SIDELEN_MAX;
	if (xlen < SIDELEN_MIN) xlen = SIDELEN_MIN;
	boost::lock_guard<boost::mutex> lk{mux_};
	max_sideLen_ = xlen;
	return max_sideLen_;
}
int FBOF::SetMinSideLen(int mlen) {
	if (mlen > SIDELEN_MAX) mlen = SIDELEN_MAX;
	if (mlen < SIDELEN_MIN) mlen = SIDELEN_MIN;
	boost::lock_guard<boost::mutex> lk{mux_};
	min_sideLen_ = mlen;
	return min_sideLen_;
}
int FBOF::SetThresh(int thresh, int height) {
	boost::lock_guard<boost::mutex> lk{mux_};
	thresh_ = SpeedToPixel(thresh,height);
	return thresh;
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
