#include "GUI.h"
std::string GUI::MtkInfo2StringID(const MtkVehicleInfo& v) {
	std::stringstream ss;
	ss << "ID: " << v.id_;
	return ss.str();
}
std::string GUI::MtkInfo2StringMotion(const MtkVehicleInfo&v) {
	std::stringstream ss;
	auto d = v.bb_.tl() - v.old_bb_.tl(); auto n = sqrt(d.x*d.x+d.y*d.y);
	ss << "Motion: " << n;
	return ss.str();
}
void GUI::Tag(cv::Mat& canvas, const cv::Rect& bb, const std::string& text) {
	auto pt = bb.tl() - cv::Point(10,10);
	auto font = cv::fontQt("Times",16,cv::Scalar(0,255,0));
	cv::addText(canvas,text,pt, font);
}
void GUI::ShowVConsole() {
	vconsole_ = cv::Mat::zeros(800,300,CV_8UC3);		
	auto font = cv::fontQt("Times",16,cv::Scalar(155,255,0));
	int step = 50;int y = step;
	for (const auto& s : out_) {
		cv::addText(vconsole_,s, cv::Point(50,y), font);
		y += step;
	}
	out_.clear();
	imshow("vconsole",vconsole_);
}
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
		std::stringstream ss; ss << "Motion Thresh: " << op->motion_thresh_;
		out_.push_back(ss.str());
		for (const auto& v : op->fbvinfo) {
			cv::rectangle(canvas,v.bb_,cv::Scalar(255,0,0),2);
		}
		for (const auto& v : op->vinfo) {
			auto sid = MtkInfo2StringID(v);
			auto smotion = MtkInfo2StringMotion(v);
			cv::rectangle(canvas,v.bb_,cv::Scalar(0,0,255),2);
			Tag(canvas,v.bb_,sid);
			out_.push_back(sid+"\t"+smotion);
		}
		cv::imshow(wname,canvas);
		ShowVConsole();
		cv::waitKey(30);
	}
}
void GUI::MotionThresh(int dv, void *user) {
	auto gui = static_cast<GUI*>(user);
	double cv = dv/10.0;
	gui->fback_->SetThresh(cv);
}

void GUI::SetupGUIEnv(const std::string& wname) {
	cv::namedWindow(wname);
	cv::namedWindow("vconsole", CV_GUI_NORMAL|cv::WINDOW_AUTOSIZE);
	cv::moveWindow(wname,0,0);
	cv::setMouseCallback(wname,GUI::onMouse,this);
	cv::createButton("nop",GUI::setNop,this,CV_RADIOBOX);
	cv::createButton("add",GUI::setAddAction,this,CV_RADIOBOX,1);
	cv::createButton("del",GUI::setDelAction,this,CV_RADIOBOX);
	cv::createTrackbar("Motion Thresh","",&motion_thresh_dv,150,GUI::MotionThresh,this);
}
