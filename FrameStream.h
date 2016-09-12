/*@author Timothy Yo (Yang You)*/
#include "Media.h"
#include "Mtk.h"
#include <opencv2/highgui.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>
#ifndef FSTREAM
#define FSTREAM
struct FrameStream : public boost::enable_shared_from_this<FrameStream>{
	cv::VideoCapture vs_;
	boost::shared_ptr<Mtk> mtk_;
	boost::thread tdfs_;
	void Run() {
		size_t fn = 0;
		auto& output = mtk_->input;
		while (true) {
			++fn;
			auto sp = boost::make_shared<Media>();
			sp->fn = fn;
			if (!vs_.read(sp->img)) break;
			output.Write(sp);
		}
		output.SetEOF();
	}
public:
	FrameStream(const std::string& fname, const boost::shared_ptr<Mtk>& mtk):
		vs_{fname},mtk_{mtk}{}
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
