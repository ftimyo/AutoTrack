#include "GUI.h"
void GUI::setAddAction(int,void* user) {
	auto gui = static_cast<GUI*>(user);
	gui->mouse_action_ = MAction::MADD;
}
void GUI::setDelAction(int,void* user) {
	auto gui = static_cast<GUI*>(user);
	gui->mouse_action_ = MAction::MDEL;
}
void GUI::setNop(int,void* user) {
	auto gui = static_cast<GUI*>(user);
	gui->mouse_action_ = MAction::NOP;
}
void GUI::onMouse(int event, int x, int y, int, void* user) {
	auto gui = static_cast<GUI*>(user);
	MAction action = gui->mouse_action_;
	if (gui->mouse_action_ == MAction::NOP) return;
	auto pt = cv::Point(x,y);
	if (event == cv::EVENT_LBUTTONDOWN) {
		if (action == MAction::MADD) {
			gui->SendFarnebackVehicleInfo(pt);
		} else if (action == MAction::MDEL) {
			gui->SendMtkKillVehicleInfo(pt);
		}
	}
}
bool GUI::SendFarnebackVehicleInfo(const cv::Point& pt) {
	boost::lock_guard<boost::mutex> lk{mux_};
	const auto& info = cache_->fbvinfo;
	for (const auto& v : info) {
		if (v.bb_.contains(pt)) {
			auto req = boost::make_shared<MtkVehicleInfoReq>(v.bb_,mtk_->GenerateID());
			req->pimg_ = cache_->context;
			mtk_->SendCmd(req);
			return true;
		}
	}
	return false;
}
bool GUI::SendMtkKillVehicleInfo(const cv::Point& pt) {
	boost::lock_guard<boost::mutex> lk{mux_};
	const auto& info = cache_->vinfo;
	for (const auto& v : info) {
		if (v.bb_.contains(pt)) {
			auto req = boost::make_shared<MtkVehicleInfoReq>(v.bb_,v.id_,true);
			req->pimg_ = cache_->context;
			mtk_->SendCmd(req);
			return true;
		}
	}
	return false;
}

void GUI::UpdateCache(const boost::shared_ptr<FarnebackVehicleDetectOutput>& input) {
	boost::lock_guard<boost::mutex> lk{mux_};
	cache_ = input;
}
void GUI::Start(const std::string& wname) {
	fback_->StartFback();
	mtk_->StartMtk();
	fs_->Start();
	boost::shared_ptr<FarnebackVehicleDetectOutput> op;
	cv::Mat canvas;
	SetupGUIEnv(wname);
	while (fback_->output.Read(op)) {
		UpdateCache(op);
		op->context->img.copyTo(canvas);
		for (const auto& v : op->fbvinfo) {
			cv::rectangle(canvas,v.bb_,cv::Scalar(255,0,0),2);
		}
		for (const auto& v : op->vinfo) {
			cv::rectangle(canvas,v.bb_,cv::Scalar(0,0,255),2);
		}
		cv::imshow(wname,canvas);
		cv::waitKey(30);
	}
}
void GUI::SetupGUIEnv(const std::string& wname) {
	cv::namedWindow(wname);
	cv::setMouseCallback(wname,GUI::onMouse,this);
	cv::createButton("nop",GUI::setNop,this,CV_RADIOBOX,1);
	cv::createButton("add",GUI::setAddAction,this,CV_RADIOBOX);
	cv::createButton("del",GUI::setDelAction,this,CV_RADIOBOX);
}
