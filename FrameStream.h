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
struct FrameStream : public boost::enable_shared_from_this<FrameStream>{
private:
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
	bool read(cv::UMat& img) {
		DataF buf;
		try {
			boost::asio::read(sk_,boost::asio::buffer(&buf,sizeof(buf)));
			DataF::b2l(buf.w);DataF::b2l(buf.h);DataF::b2l(buf.bar);
			int len = (buf.w*buf.h);
			dbuf_ = new uint8_t[len];
			boost::asio::read(sk_,boost::asio::buffer(dbuf_,len));
		} catch(...) {
			return false;
		}
		cv::Mat y{buf.h,buf.w,CV_8UC1,dbuf_};
		cv::cvtColor(y,img,cv::COLOR_GRAY2RGB);
		return true;
	}

public:
	using tcp = boost::asio::ip::tcp;
	tcp::socket sk_;
	tcp::acceptor ask_;
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
		sk_{ios},ask_{ios,tcp::endpoint(tcp::v4(),8888)},mtk_{mtk}{}

	void Start() {
		auto& ios = ask_.get_io_service();
		tcp::resolver resolver(ios);
		tcp::resolver::query query(boost::asio::ip::host_name(), "");
		tcp::resolver::iterator iter = resolver.resolve(query);
		tcp::resolver::iterator end;
		tcp::endpoint ep = *iter;
		fprintf(stderr,"%s\n",ep.address().to_string().c_str());
		ask_.accept(sk_);
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
