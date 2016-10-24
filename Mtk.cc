/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "L1APG.h"
#include <algorithm>

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

struct CmdBBC : public BBC {
	bool kill_;
	boost::thread td_;
	CmdBBC(const BBC& bbc, bool kill): BBC{bbc},kill_{kill}{}
	CmdBBC(BBC&& bbc, bool kill): BBC{std::move(bbc)},kill_{kill}{}
	CmdBBC(const CmdBBC&) = default;
	CmdBBC(CmdBBC&&) = default;
	CmdBBC& operator= (const CmdBBC&) = default;
	CmdBBC& operator= (CmdBBC&&) = default;
};

bool Mtk::UpdateMeta() {
	boost::lock_guard<boost::mutex> lk{cmds_mux_};
	if (cmds_.empty()) return false;
	for (const auto& cmd : cmds_) {
		auto id = cmd->id_;
		auto p = std::find_if(std::begin(meta_),std::end(meta_),
				[id](const auto& m) {
					return m->id_ == id;
				});
		if (p == std::end(meta_)) {
			meta_.emplace_back(std::move(cmd));
		} else {
			(*p)->kill_ = cmd->kill_;
		}
	}
	cmds_.clear();
	return true;
}

void Mtk::SendCmdBBC(const BBC& bbc, bool kill) {
	boost::lock_guard<boost::mutex> lk{cmds_mux_};
	cmds_.emplace_back(std::make_shared<CmdBBC>(bbc,kill));
}
void Mtk::SendCmdBBC(BBC&& bbc, bool kill) {
	boost::lock_guard<boost::mutex> lk{cmds_mux_};
	if (!kill) bbc.id_ = GenerateID();
	bbc.bb_ -= bbc.bb_.size() / 3;
	bbc.bb_ -= bbc.bb_.tl() / 3;
	cmds_.emplace_back(std::make_shared<CmdBBC>(std::move(bbc),kill));
}

void Mtk::StartMtk() {
	if (sync_thread.joinable()) return;
	auto self = shared_from_this();
	load_cp_ = std::make_shared<SpinBarrier>(1);
	track_cp_ = std::make_shared<SpinBarrier>(1);
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
		marea_ = cv::Rect{1,1,media_cache_->img.cols-2,media_cache_->img.rows-2};
		/*Media load checkpoint*/
		load_cp_->wait();
		update_meta = UpdateMeta();
		if (update_meta) {
			auto cnt = std::count_if(std::begin(meta_),std::end(meta_),
					[](const auto& m){return !m->kill_;});
			load_cp_ = std::make_shared<SpinBarrier>(cnt + 1);
		}
		/*Tracking checkpoint*/
		track_cp_->wait();
		if (update_meta) {
			for (auto& m : meta_) {
				if (m->kill_ && m->td_.joinable()){m->td_.join();}
				else if (m->context_) {
					auto self = shared_from_this();
					m->td_ = boost::thread{
						[this,self,m](){TrackThread(m);}
					};
				}
			}
			meta_.erase(
					remove_if(std::begin(meta_),std::end(meta_),[](const auto& m){return m->kill_;}),
					std::end(meta_));
			track_cp_ = std::make_shared<SpinBarrier>(meta_.size() + 1);
		}
		auto out = std::make_shared<MSG>(
				std::make_shared<BBS>(std::move(media_cache_)),
				std::move(bbs_cache_)
				);
		bbs_cache_ = out->cur_;
		auto& bbs = bbs_cache_->bbs_;
		for (auto& m : meta_) {bbs.emplace_back(*m);}
		output.WriteBlocking(std::move(out));
	}
	output.SetEOF();
}

void Mtk::TrackThread(std::shared_ptr<CmdBBC> cmd) {
	auto& bb = cmd->bb_;
	auto& kill = cmd->kill_;

	cv::Mat img; cmd->context_->gray.copyTo(img);
	cmd->context_ = nullptr;
	auto selfkill = false;
	auto tracker = L1APG {bb2fbb(bb),img,false};
	tracker.IntializeTpl(img);

	while (true) {
		/*Media load checkpoint*/
		load_cp_->wait();
		if (media_cache_ == nullptr) return;
		if (!selfkill) {
			media_cache_->gray.copyTo(img);
			try {
			tracker.Track(img);
			} catch (...) {
				selfkill = true;
			}
			bb = fbb2bb(tracker.GetBB());
			bb += bb.size()/2;
			bb += bb.tl()/2;
			if ((bb & marea_) != bb) {
				selfkill = true;
			}
		}
		/*Tracking checkpoint*/
		track_cp_->wait();
		if (kill) break;
		if (selfkill) {
			SendCmdBBC(*cmd,true);
		}
	}
}
