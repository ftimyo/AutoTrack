/*@author Timothy Yo (Yang You)*/
#include "FarnebackVehicleDetect.h"
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include "Stopwatch.h"
#include <cstdio>
const int FarnebackVehicleDetect::MAX_KERNEL_LENGTH = 31;
const int FarnebackVehicleDetect::MIN_KERNEL_LENGTH = 3;
void FarnebackVehicleDetect::UpdateMeta() {
	boost::lock_guard<boost::mutex> lk{mux_};
	i_gwin_ = gwin_;
	i_max_ratio_ = max_ratio_;
	i_max_sideLen_ = max_sideLen_;
	i_min_sideLen_ = min_sideLen_;
}

void FarnebackVehicleDetect::StartFback() {
	if (tfback_.joinable()) return;
	auto self = shared_from_this();
	tfback_ = boost::thread{
		[this,self]() {Run();}
	};
}
void FarnebackVehicleDetect::StopFback() {
	if (tfback_.joinable()) tfback_.join();
}
void FarnebackVehicleDetect::Run() {
	auto& input = mtk_->output;
	boost::shared_ptr<MtkOutput> tmp;
	while (input.Read(tmp)) {
		cache_meta_cur_ = boost::make_shared<FarnebackVehicleDetectOutput>(std::move(*tmp));
		cv::UMat gray;
		cv::cvtColor(cache_meta_cur_->context->img,gray,cv::COLOR_BGR2GRAY);
		UpdateMeta();
		GaussianBlur(gray,cache_cur_,cv::Size(i_gwin_,i_gwin_),0,0);
		/*Process Frames*/
		MovingVehicleDetectInternal();
		output.Write(cache_meta_cur_);
		std::swap(cache_pre2_,cache_pre1_);
		std::swap(cache_pre1_,cache_cur_);
		std::swap(cache_meta_cur_,cache_meta_pre_);
	}
	output.SetEOF();
}
bool FarnebackVehicleDetect::ValidateVehicleInfo(const FarnebackVehicleInfo& vinfo) {
	const auto& bb = vinfo.bb_;
	auto sideLen = std::minmax(bb.width,bb.height);
	if (sideLen.first == 0
			|| sideLen.first < i_min_sideLen_
			|| sideLen.second > i_max_sideLen_
			|| static_cast<float>(sideLen.second)/static_cast<float>(sideLen.first) > i_max_ratio_) {
		return false;
	}
	if ((cache_meta_cur_->marea & bb) != bb) return false;
	const auto& mtkvinfo = cache_meta_cur_->vinfo;
	if (std::find(std::begin(mtkvinfo),std::end(mtkvinfo),vinfo) != std::end(mtkvinfo)) {
		return false;
	}
	return true;
}
void FarnebackVehicleDetect::ExtractVehicleInfo(cv::UMat& ubimg,cv::Mat& flow) {
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(ubimg,contours,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);
	auto& vinfo = cache_meta_cur_->fbvinfo;
	auto frame_diff = cache_meta_cur_->context->fn - cache_meta_pre_->context->fn;
	for (const auto& contour : contours) {
		auto bb = cv::boundingRect(contour);
		auto v = flow.at<cv::Point2f>((bb.tl()+bb.br())/2);
		auto dp = cv::Point(cvRound(v.x),cvRound(v.y));
		bb += dp;
		v /= static_cast<float>(frame_diff); /*TODO:check if frame_diff == 0*/
		/*check validity of bb*/
		FarnebackVehicleInfo tmp{bb,v};
		if (ValidateVehicleInfo(tmp)) vinfo.emplace_back(tmp);
	}
}

void FarnebackVehicleDetect::MovingVehicleDetectInternal() {
	if (cache_pre2_.empty() || cache_cur_.empty()) return;
	cv::UMat uflow;
	cv::calcOpticalFlowFarneback(cache_pre2_,cache_cur_,uflow,0.5,3,15,3,5,1.2,0);
	cv::UMat ubimg;
	std::vector<cv::UMat> udxy;
	cv::split(uflow,udxy);
	cv::UMat mimg;
	cv::magnitude(udxy[0],udxy[1],mimg);
	double max,min;
	cv::minMaxIdx(mimg,&max,&min);
	double thresh = (max+min)/2;
	cv::threshold(mimg,ubimg,thresh,255,cv::THRESH_BINARY);
	ubimg.convertTo(ubimg,CV_8UC1);
	cv::Mat flow;uflow.copyTo(flow);
	ExtractVehicleInfo(ubimg,flow);
	return;
}
void FarnebackVehicleDetect::SetGaussianWindow(int win) {
	boost::lock_guard<boost::mutex> lk{mux_};
	if (win < MIN_KERNEL_LENGTH) win = 3;
	if (win % 2 == 0) win += 1;
	if (win > MAX_KERNEL_LENGTH) win = 31;
	gwin_ = win;
}
void FarnebackVehicleDetect::SetMaxRatio(float) {
	boost::lock_guard<boost::mutex> lk{mux_};
}
void FarnebackVehicleDetect::SetMaxSideLen(int) {
	boost::lock_guard<boost::mutex> lk{mux_};
}
void FarnebackVehicleDetect::SetMinSideLen(int) {
	boost::lock_guard<boost::mutex> lk{mux_};
}
