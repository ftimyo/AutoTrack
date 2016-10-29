/*@author Timothy Yo (Yang You)*/
#include <opencv2/opencv_modules.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include "Media.h"
#include "Mtk.h"
#include "Pipe.h"
#include <boost/thread.hpp>
#include <memory>
#ifndef FARNEBACK_H
#define FARNEBACK_H

class FBOF:public std::enable_shared_from_this<FBOF>{
	cv::UMat cache_pre1_,cache_pre2_,cache_cur_;
	std::shared_ptr<BBS> cache_meta_cur_;
	cv::Rect marea_;
	boost::thread tfback_;
	bool bypass_;

/*Start Two-level Meta Control Info*/
public:
	static const int MAX_KERNEL_LENGTH;
	static const int MIN_KERNEL_LENGTH;
	static const int MAX_RATIO_MAX;
	static const int SIDELEN_MAX;
	static const int SIDELEN_MIN;
	static const int MAX_THRESH;

private:
	int gwin_;
	float max_ratio_;
	int max_sideLen_;
	int min_sideLen_;
	double thresh_;

	int i_gwin_;
	float i_max_ratio_;
	int i_max_sideLen_;
	int i_min_sideLen_;
	double i_thresh_;

	boost::mutex mux_;
/*End Meta Control Info*/

private:
	void UpdateMeta();
	void GetCC(cv::UMat& ubimg,cv::Mat& flow);
	template<typename... Ts>
	void CalcOF(Ts... params);
	void Run();

public:
	Pipe<std::shared_ptr<MSG>> output;
	Pipe<std::shared_ptr<Media>> input;
public:
	explicit FBOF(bool bypass = false):bypass_{bypass},
		gwin_{17},max_ratio_{3},max_sideLen_{100},min_sideLen_{30},thresh_{0}{}
	void SetBypass(bool);
	void StartFback();
	void StopFback();

/*User Interface*/
	int SetGaussianWindow(int);
	int SetMaxRatio(int);
	int SetMaxSideLen(int);
	int SetMinSideLen(int);
	int SetThresh(int);
};
#endif
