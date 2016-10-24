/*@author Timothy Yo (Yang You)*/
#include <opencv2/opencv.hpp>
#include <cstdint>
#include <string>
#include "Media.h"
#include "Pipe.h"
#include <memory>
#include <boost/atomic.hpp>
#include "Stopwatch.h"
#include "Rect.h"
#ifndef MTK_H
#define MTK_H

class SpinBarrier;
struct CmdBBC;

class Mtk : public std::enable_shared_from_this<Mtk> {
public:
	Pipe<std::shared_ptr<Media>> input;
	Pipe<std::shared_ptr<MSG>> output;
private:
	boost::atomic<uint64_t> id_pool_;
	cv::Rect marea_;
	std::shared_ptr<SpinBarrier> load_cp_, track_cp_;
	std::shared_ptr<Media> media_cache_;
	std::shared_ptr<BBS> bbs_cache_;

	boost::mutex cmds_mux_;
	std::vector<std::shared_ptr<CmdBBC>> cmds_;
	std::vector<std::shared_ptr<CmdBBC>> meta_;

	bool UpdateMeta();
	void SyncThread();
	void TrackThread(std::shared_ptr<CmdBBC>);

	boost::thread sync_thread;
public:
	Mtk():id_pool_{0}{}
	void StartMtk();
	void StopMtk();
	uint64_t GenerateID() {return ++id_pool_;}
	void SendCmdBBC(const BBC&, bool);
	void SendCmdBBC(BBC&&, bool);

	static inline cv::Rect fbb2bb(const FloatRect& fb) {
		return cv::Rect{cv::Point(fb.XMin(),fb.YMin()),
			cv::Point(fb.XMax(),fb.YMax())};
	}
	static inline FloatRect bb2fbb(const cv::Rect& bb) {
		return FloatRect{float(bb.tl().x), float(bb.tl().y),
			float(bb.width), float(bb.height)};
	}
};
#endif
