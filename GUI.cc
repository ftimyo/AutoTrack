#include "GUI.h"
/*BB with motion vector*/
struct MBB : public BB {
	cv::Point2f v_;
	MBB(const BB& bb, cv::Point&& v):BB{bb},v_{std::move(v)}{}
};
struct MBBS {
	std::vector<MBB> bbs_;
	void BuildMBBS(const BBS& cur, const BBS& pre) {
		auto p = begin(pre.bbs_);
		auto e = end(pre.bbs_);
		double fndiff = cur.context_->fn - pre.context_->fn;
		for (auto& b : cur.bbs_) {
			while (p != e) {
				if (p->id_ == b.id_) {
					cv::Point2f v{b.bb_.tl() - p->bb_.tl()};
					bbs_.emplace_back(b,v/fndiff);
					break;
				}
				++p;
			}
		}
	}
};
template <typename T>
void GUI::UpdateCache(T&& mtk_tmp, T&& fbof_tmp) {
	boost::lock_guard<boost::mutex> lk{mux_};
	mtk_cache_ = std::forward<T>(mtk_tmp);
	fbof_cache_ = std::forward<T>(fbof_tmp);
}
std::shared_ptr<MSG> GUI::GetMtkMSG() {
	boost::lock_guard<boost::mutex> lk{mux_};
	return mtk_cache_;
}
std::shared_ptr<MSG> GUI::GetFBOFMSG() {
	boost::lock_guard<boost::mutex> lk{mux_};
	return fbof_cache_;
}
void GUI::onMouse(int event, int x, int y, int, void* user) {
	auto gui = static_cast<GUI*>(user);
	MAction action = gui->mouse_action_;
	auto pt = cv::Point(x,y);
	if (event == cv::EVENT_LBUTTONDOWN) {
		if (action == MAction::MADD) {
			gui->SendCmd(pt);
		} else if (action == MAction::MDEL) {
			gui->SendCmd(pt,true);
		}
	}
}
void GUI::SendCmd(const cv::Point& pt, bool kill) {
	auto mtkm = GetMtkMSG();
	auto bfofm = GetFBOFMSG();
	if (kill) {
		for (auto& b : mtkm->cur_->bbs_) {
			if (b.bb_.contains(pt)) mtk_->SendCmdBBC(BBC{b,mtkm->cur_->context_},kill);
		}
	} else {
		for (auto& b : bfofm->cur_->bbs_) {
			if (b.bb_.contains(pt)) mtk_->SendCmdBBC(BBC{b,bfofm->cur_->context_},kill);
		}
	}
}

void GUI::Start(const std::string& wname) {
	DupSplit<MediaPtr> splitpipe{fs_->output,mtk_->input,fbof_->input};
	fbof_->StartFback();
	mtk_->StartMtk();
	fs_->Start();
	splitpipe.Connect();
	std::shared_ptr<MSG> mtk_tmp,fbof_tmp;
	auto& mtk_input = mtk_->output;
	auto& fbof_input = fbof_->output;
	cv::Mat canvas;
	SetupGUIEnv(wname);
	MBBS mbbs;
	while (mtk_input.Read(mtk_tmp) && fbof_input.Read(fbof_tmp)) {
		UpdateCache(std::move(mtk_tmp),std::move(fbof_tmp));
		fbof_cache_->cur_->RemoveOverlap(*mtk_cache_->cur_);
		if (mtk_cache_->old_) mbbs.BuildMBBS(*mtk_cache_->cur_,*mtk_cache_->old_);
		mtk_cache_->cur_->context_->img.copyTo(canvas);
		DrawMtk(canvas);
		DrawFBOF(canvas);
		cv::imshow(wname,canvas);
		cv::waitKey(1);
	}
	splitpipe.Stop();
}

void GUI::DrawMtk(cv::Mat& canvas) {
	for (auto& b : mtk_cache_->cur_->bbs_) {
		cv::rectangle(canvas,b.bb_,cv::Scalar(0,0,255),2);
	}
}
void GUI::DrawFBOF(cv::Mat& canvas) {
	for (auto& b : fbof_cache_->cur_->bbs_) {
		cv::rectangle(canvas,b.bb_,cv::Scalar(255,0,0),2);
	}
}
void GUI::MotionThresh(int dv, void *user) {
	auto gui = static_cast<GUI*>(user);
	double cv = dv/10.0;
	gui->fbof_->SetThresh(cv);
}
void GUI::setAddAction(int,void* user) {
	auto gui = static_cast<GUI*>(user);
	gui->mouse_action_ = MAction::MADD;
}
void GUI::setDelAction(int,void* user) {
	auto gui = static_cast<GUI*>(user);
	gui->mouse_action_ = MAction::MDEL;
}

void GUI::SetupGUIEnv(const std::string& wname) {
	cv::namedWindow(wname);
	cv::moveWindow(wname,0,0);
	cv::setMouseCallback(wname,GUI::onMouse,this);
	cv::createButton("add",GUI::setAddAction,this,CV_RADIOBOX,1);
	cv::createButton("del",GUI::setDelAction,this,CV_RADIOBOX);
	cv::createTrackbar("Motion bar","",&motion_thresh_dv,150,GUI::MotionThresh,this);
}
