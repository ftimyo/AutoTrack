/*@author Timothy Yo (Yang You)*/
#include "Media.h"
#include "Mtk.h"
#include <opencv2/highgui.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <string>
#include <cstdio>
#ifndef FSTREAM
#define FSTREAM
const size_t MTU = 1024;
struct FrameStream : public boost::enable_shared_from_this<FrameStream>{
private:
	//using tcp = boost::asio::ip::tcp;
	using udp = boost::asio::ip::udp;
	struct DataF {
		int32_t w;
		int32_t h;
		int32_t bar;
		static inline void b2l(int32_t& bn) {
			auto ln = (uint8_t*)(&bn);
			std::swap(ln[0],ln[3]);
			std::swap(ln[1],ln[2]);
		}
	};
	uint8_t *dbuf_;
	int dbuf_size_;

	template <typename BUF>
	size_t udpRead(const BUF& buf, size_t len) {
		udp::endpoint sender;
		size_t idx = 0;
		while (idx < len) {
			auto tlen = idx + MTU < len ? MTU : len - idx;
			sk_.receive_from(boost::asio::buffer(buf+idx,tlen),sender);
			idx += tlen;
		}
		return idx;
	}

	bool read(cv::UMat& img) {
		DataF buf;
		try {
			auto hsize = udpRead(&buf,sizeof(buf));
			if (hsize != sizeof(buf)) return false;
			DataF::b2l(buf.w);DataF::b2l(buf.h);DataF::b2l(buf.bar);
			std::cout << buf.w <<','<<buf.h<<std::endl;
			size_t len = (buf.w*buf.h);
			dbuf_ = new uint8_t[len];
			auto size = udpRead(dbuf_,len);
			if (size != len) return false;
		} catch(...) {
			return false;
		}
		cv::Mat y{buf.h,buf.w,CV_8UC1,dbuf_};
		cv::cvtColor(y,img,cv::COLOR_GRAY2RGB);
		return true;
	}

public:
	udp::socket sk_;
	//tcp::acceptor ask_;
	boost::shared_ptr<Mtk> mtk_;
	boost::thread tdfs_;
	void Run() {
		size_t fn = 0;
		auto& output = mtk_->input;
		while (true) {
			++fn;
			auto sp = boost::make_shared<Media>();
			sp->fn = fn;
			if (!read(sp->img)) break;
			output.Write(sp);
		}
		output.SetEOF();
		sk_.close();
	}
public:
	FrameStream(boost::asio::io_service& ios, const boost::shared_ptr<Mtk>& mtk):
		sk_{ios,udp::endpoint(udp::v4(),8888)},mtk_{mtk}{}

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
