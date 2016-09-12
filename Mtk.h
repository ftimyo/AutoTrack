/*@author Timothy Yo (Yang You)*/
#include <opencv2/opencv.hpp>
#include <cstdint>
#include <string>
#include <atomic>
#include "Media.h"
#include "Pipe.h"
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "Stopwatch.h"
#include "Rect.h"
#ifndef MTK_H
#define MTK_H

class SpinBarrier {
private:
	unsigned const count_;
	boost::atomic<unsigned> spaces_;
	boost::atomic<unsigned> generation_;
public:
	explicit SpinBarrier(unsigned count):
		count_{count}, spaces_{count}, generation_{0}{}
	void wait() {
		unsigned const tc_generation = generation_;
		if (!--spaces_) {
			spaces_ = count_;
			++generation_;
		} else {
			while (generation_ == tc_generation) boost::this_thread::yield();
		}
	}
};

class MtkVehicleInfo : public VehicleInfo {
public:
	cv::Rect old_bb_;
	size_t old_fn_;
	size_t fn_;
	MtkVehicleInfo(const cv::Rect& bb, uint64_t id):VehicleInfo{bb,id}{}
	bool operator == (const MtkVehicleInfo& vinfo) const {
		return vinfo.id_ == id_;
	}
	bool operator == (const VehicleInfo& vinfo) const {
		return bb_.contains(vinfo.GetCenter()) || vinfo.bb_.contains(GetCenter());
	}
};

struct MtkOutput : public VehicleInfoOutput {
	std::vector<MtkVehicleInfo> vinfo;
	cv::Rect varea,marea;
	MtkOutput(VehicleInfoOutput&& output):VehicleInfoOutput{output}{}
	MtkOutput(){}
};

struct MtkVehicleInfoReq : public MtkVehicleInfo {
	MtkVehicleInfoReq(const cv::Rect& bb,uint64_t id,bool kill=false):
		MtkVehicleInfo{bb,id},kill_{kill}{}
	boost::shared_ptr<Media> pimg_;
	bool kill_;
	boost::thread td_;
};

class Mtk : public boost::enable_shared_from_this<Mtk> {
public:
	Pipe<boost::shared_ptr<MtkOutput>> output;
	Pipe<boost::shared_ptr<Media>> input;
private:
	std::atomic<uint64_t> id_pool_;
	boost::mutex cmds_mux_;
	std::vector<boost::shared_ptr<MtkVehicleInfoReq>> cmds_;
	std::vector<boost::shared_ptr<MtkVehicleInfoReq>> meta_;
	boost::shared_ptr<SpinBarrier> load_cp_, track_cp_;
	boost::shared_ptr<Media> media_cache_;

	boost::mutex marea_mux_;
	cv::Rect marea_cmd_;
	cv::Rect marea_; /*monitoring area*/
	cv::Rect varea_; /*visible area*/

	bool UpdateMeta();
	void UpdateMArea();
	void SyncThread();
	void TrackThread(boost::shared_ptr<MtkVehicleInfoReq>&);

	boost::thread sync_thread;
public:
	Mtk():id_pool_{0}{}
	void StartMtk();
	void StopMtk();
	uint64_t GenerateID() {return ++id_pool_;}
	void SendCmd(const boost::shared_ptr<MtkVehicleInfoReq>&);
	void SendMAreaCmd(const cv::Rect&);

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
