/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "L1APG.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>

bool Mtk::UpdateMeta() {
	boost::lock_guard<boost::mutex> lk{cmds_mux_};
	if (cmds_.size() == 0) return false;
	for (const auto& cmd : cmds_) {
		auto id = cmd->id_;
		auto p = std::find_if(std::begin(meta_),std::end(meta_),
				[id](const boost::shared_ptr<MtkVehicleInfoReq>& x) {
					return x->id_ == id;
				});
		if (p == std::end(meta_)) {
			meta_.push_back(cmd);
		} else {
			(*p)->kill_ = cmd->kill_;
		}
	}
	cmds_.clear();
	return true;
}

void Mtk::SendMAreaCmd(const cv::Rect& marea) {
	boost::lock_guard<boost::mutex> lk{marea_mux_};
	marea_cmd_ = marea;
}
void Mtk::UpdateMArea() {
	boost::lock_guard<boost::mutex> lk{marea_mux_};
	if (marea_cmd_.width < (varea_.width >> 2) ||
			marea_cmd_.height < (varea_.height >> 2)) return;
	marea_ = marea_cmd_;
	marea_ &= varea_;
}

void Mtk::SendCmd(const boost::shared_ptr<MtkVehicleInfoReq>& req) {
	boost::lock_guard<boost::mutex> lk{cmds_mux_};
	cmds_.push_back(req);
}

void Mtk::StartMtk() {
	if (sync_thread.joinable()) return;
	auto self = shared_from_this();
	load_cp_ = boost::make_shared<SpinBarrier>(1);
	track_cp_ = boost::make_shared<SpinBarrier>(1);
	sync_thread = boost::thread{
		[this,self](){
			SyncThread();
		}
	};
}
void Mtk::StopMtk() {
	if (sync_thread.joinable()) sync_thread.join();
}

void Mtk::SyncThread() {
	bool update_meta = true;
	while (true) {
		if (!input.Read(media_cache_)) {
			media_cache_ = nullptr;
			load_cp_->wait();
			break;
		}
		varea_ = cv::Rect{1,1,media_cache_->img.cols-2,media_cache_->img.rows-2};
		marea_ = varea_;
		UpdateMArea();
		/*Media load checkpoint*/
		load_cp_->wait();
		update_meta = UpdateMeta();
		if (update_meta) {
			auto cnt = std::count_if(std::begin(meta_),std::end(meta_),
					[](const auto& x){return !x->kill_;});
			load_cp_ = boost::make_shared<SpinBarrier>(cnt + 1);
		}
		if (meta_.empty()) 
			boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
		/*Tracking checkpoint*/
		track_cp_->wait();
		if (update_meta) {
			for (auto& m : meta_) {
				if (m->kill_ && m->td_.joinable()){m->td_.join();}
				else if (m->pimg_) {
					auto self = shared_from_this();
					m->td_ = boost::thread{
						[this,self,m](){TrackThread(m);}
					};
				}
			}
			meta_.erase(
					remove_if(std::begin(meta_),std::end(meta_),[](const auto& x){return x->kill_;}),
					std::end(meta_));
			track_cp_ = boost::make_shared<SpinBarrier>(meta_.size() + 1);
		}
		auto out = boost::make_shared<MtkOutput>();
		out->marea = marea_; out->varea = varea_; out->context = media_cache_;
		for (auto& m : meta_) {out->vinfo.emplace_back(*m);}
		output.Write(out);
	}
	output.SetEOF();
}

void Mtk::TrackThread(boost::shared_ptr<MtkVehicleInfoReq> vinfo) {
	auto& bb = vinfo->bb_;
	auto& fn = vinfo->fn_;
	auto& old_bb = vinfo->old_bb_;
	auto& old_fn = vinfo->old_fn_;
	auto& kill = vinfo->kill_;

	cv::Mat img; vinfo->pimg_->img.copyTo(img);
	auto selfkill = false;
	old_fn = vinfo->pimg_->fn;
	vinfo->pimg_ = nullptr;
	auto tracker = L1APG {bb2fbb(bb),img,false};
	tracker.IntializeTpl(img);

	while (true) {
		/*Media load checkpoint*/
		load_cp_->wait();
		if (media_cache_ == nullptr) return;
		if (!selfkill) {
			old_fn = fn;
			old_bb = bb;
			media_cache_->img.copyTo(img);
			fn = media_cache_->fn;
			try {
			tracker.Track(img);
			} catch (...) {
				selfkill = true;
			}
			bb = fbb2bb(tracker.GetBB());
			if ((bb & marea_) != bb) {
				selfkill = true;
			}
		}
		/*Tracking checkpoint*/
		track_cp_->wait();
		if (kill) break;
		if (selfkill) {
			auto req = boost::make_shared<MtkVehicleInfoReq>(bb,vinfo->id_,true);
			SendCmd(req);
		}
	}
}
