/*@author Timothy Yo (Yang You)*/
#include "Media.h"
#include "Pipe.h"
#include <opencv2/highgui.hpp>
#include <string>
#include <memory>
#ifndef FSTREAM
#define FSTREAM
struct FrameStream : public std::enable_shared_from_this<FrameStream>{
	Pipe<std::shared_ptr<Media>> output;
	cv::VideoCapture vs_;
	boost::thread tdfs_;
	void Run() {
		size_t fn = 0;
		while (true) {
			++fn;
			auto sp = std::make_shared<Media>();
			sp->fn = fn;
			if (!vs_.read(sp->img)) break;
			cv::cvtColor(sp->img,sp->gray,cv::COLOR_BGR2GRAY);
			cv::resize(sp->gray,sp->gray,cv::Size(),0.67,0.67);
			output.Write(sp);
		}
		output.SetEOF();
	}
public:
	FrameStream(const std::string& fname): vs_{fname}{}
	void Start() {
		if (tdfs_.joinable()) return;
		auto self = shared_from_this();
		tdfs_ = boost::thread{
			[this,self](){Run();}
		};
	}
	void Stop() {
		if (tdfs_.joinable()) tdfs_.join();
	}
};
#endif
