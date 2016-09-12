/*@author Timothy Yo (Yang You)*/
#include <opencv2/opencv_modules.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include "Media.h"
#include "Mtk.h"
#include "Pipe.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>
#ifndef FARNEBACK_H
#define FARNEBACK_H
class FarnebackVehicleDetect;

/*single vehicle information*/
class FarnebackVehicleInfo : public VehicleInfo {
public:
	FarnebackVehicleInfo(cv::Rect& bb, cv::Point2f& v): VehicleInfo{bb,0,v}{}
};

/*Group vehicle information from the same context*/
struct FarnebackVehicleDetectOutput : public MtkOutput{
	std::vector<FarnebackVehicleInfo> fbvinfo;
	FarnebackVehicleDetectOutput(MtkOutput&& input):MtkOutput{input}{}
};

class FarnebackVehicleDetect:public boost::enable_shared_from_this<FarnebackVehicleDetect>{
	cv::UMat cache_pre1_,cache_pre2_,cache_cur_;
	boost::shared_ptr<FarnebackVehicleDetectOutput> cache_meta_pre_;
	boost::shared_ptr<FarnebackVehicleDetectOutput> cache_meta_cur_;
	boost::shared_ptr<Mtk> mtk_;

	int gwin_;
	float max_ratio_;
	int max_sideLen_;
	int min_sideLen_;

	int i_gwin_;
	float i_max_ratio_;
	int i_max_sideLen_;
	int i_min_sideLen_;
	boost::mutex mux_;
	void UpdateMeta();
	static const int MAX_KERNEL_LENGTH;
	static const int MIN_KERNEL_LENGTH;
	boost::thread tfback_;
private:
	bool ValidateVehicleInfo(const FarnebackVehicleInfo&);
	void ExtractVehicleInfo(cv::UMat& ubimg,cv::Mat& flow);
	void MovingVehicleDetectInternal();
	void Run();
public:
	FarnebackVehicleDetect(boost::shared_ptr<Mtk>& mtk):mtk_{mtk},
		gwin_{17},max_ratio_{3},max_sideLen_{100},min_sideLen_{30}{}
	void StartFback();
	void StopFback();
	void SetGaussianWindow(int);
	void SetMaxRatio(float);
	void SetMaxSideLen(int);
	void SetMinSideLen(int);
	Pipe<boost::shared_ptr<FarnebackVehicleDetectOutput>> output;
};
#endif
