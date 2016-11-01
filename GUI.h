/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "FBOF.h"
#include "Pipe.h"
#include "FrameStream.h"
#include "remote.h"
#include <boost/atomic.hpp>
#include <string>
#include <sstream>
#include <list>
#include <memory>
struct RtCtl {
	uchar bar{0};
};
struct Console {
	using strs = std::stringstream;
	cv::Mat canvas_;
	std::vector<strs> cache_;
	cv::Scalar color_;
	decltype(auto) NewEntry() {
		cache_.emplace_back();
		return cache_.back();
	}
	decltype(auto) GetLastEntry() {
		if (cache_.empty()) return NewEntry();
		return cache_.back();
	}
	inline void Show() {
		canvas_ = cv::Mat::zeros(canvas_.size(),CV_8UC3);
		auto font = cv::fontQt("",16,color_);
		int step = 50, y = 0;
		for (const auto& s : cache_) {
			cv::addText(canvas_,s.str() + '\n', cv::Point(50,y += step), font);
		}
		cache_.clear();
		cv::imshow("console", canvas_);
	}
	explicit Console(cv::Size&& sz = cv::Size(600,800),
			cv::Scalar&& color = cv::Scalar(200,200,200)):
		canvas_{std::move(sz),CV_8UC3},color_{std::move(color)}{}
};

struct GUI {
private:
	static const float tan4470;
	static const float vradius;
	static const float ms2mph;
	std::shared_ptr<Mtk> mtk_;
	std::shared_ptr<FBOF> fbof_;
	std::shared_ptr<FrameStream> fs_;

	boost::mutex mux_;
	std::shared_ptr<MSG> mtk_cache_;
	std::vector<std::pair<BBC,float>> mtk_old_bbs_cache_;
	std::shared_ptr<MSG> fbof_cache_;

	template <typename T>
	void UpdateCache(T&&, T&&);
	std::shared_ptr<MSG> GetMtkMSG();
	std::shared_ptr<MSG> GetFBOFMSG();
	void SendCmd(const cv::Point&,bool=false);

/*Mouse Action Begin*/
	enum class MAction {MADD,MDEL};
	boost::atomic<MAction> mouse_action_;
	static void setFBOFBypass(int,void*);
	static void setAddAction(int,void*);
	static void setDelAction(int,void*);
	static void onMouse(int event, int x, int y, int, void* user);
	static void Tag(cv::Mat&, const cv::Rect&, const std::stringstream&, const cv::Scalar&);
	static double PixelToMeter(cv::Point pixel,int height);
/*Mouse Action END*/

/*Motion Thresh BEGIN*/
	int motion_thresh_dv;
	static void MotionThresh(int,void*);
/*Motion Thresh END*/

/*OBJ Criteria BEGIN*/
	int maxlen_,minlen_,maxratio_,gaussianws_;
	static void SetMaxSideLen(int,void*);
	static void SetMinSideLen(int,void*);
	static void SetMaxRatio(int,void*);
	static void SetGaussianKernel(int,void*);
/*OBJ Criteria END*/

	void SetupGUIEnv(const std::string&);

/*Draw on Canvas BEGIN*/
	void DrawMtk(cv::Mat&);
	void DrawFBOF(cv::Mat&);
/*Draw on Canvas END*/

/*Remote Control Info BEGIN*/
	RtCtl rtctl_;
	TCPServer<RtCtl>::SRVPtr ctlsrv_;
	boost::thread iostd_;
/*Remote Control Info END*/

/*Console Simulate Display BEGIN*/
	std::unique_ptr<Console> con_;
/*Console Simulate Display END*/

public:
	GUI(const std::shared_ptr<Mtk>& mtk,
			const std::shared_ptr<FBOF>& fbof,
			const std::shared_ptr<FrameStream>& fs,
			boost::asio::io_service& ios,
			int port=8889):mtk_{mtk},fbof_{fbof},fs_{fs},
			mouse_action_{MAction::MADD},motion_thresh_dv{0},
			maxlen_{FBOF::SIDELEN_MAX},
			minlen_{FBOF::SIDELEN_MIN},
			maxratio_{FBOF::MAX_RATIO_MAX},
			gaussianws_{FBOF::MIN_KERNEL_LENGTH},
			ctlsrv_{TCPServer<RtCtl>::makeSRV(ios,port)},
			con_{std::make_unique<Console>(cv::Size(300,600),cv::Scalar(0,255,0))}{}

	void Start(const std::string&);
};
