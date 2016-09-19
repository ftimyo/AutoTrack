/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "FarnebackVehicleDetect.h"
#include "FrameStream.h"
#include <atomic>
#include <string>
#include <sstream>
#include <list>
struct GUI {
private:
	boost::shared_ptr<Mtk> mtk_;
	boost::shared_ptr<FarnebackVehicleDetect> fback_;
	boost::shared_ptr<FrameStream> fs_;

	boost::mutex mux_;
	boost::shared_ptr<FarnebackVehicleDetectOutput> cache_;
	void UpdateCache(const boost::shared_ptr<FarnebackVehicleDetectOutput>&);
	bool SendFarnebackVehicleInfo(const cv::Point&);
	bool SendMtkKillVehicleInfo(const cv::Point&);

/*Mouse Action Begin*/
	enum class MAction {NOP,MADD,MDEL};
	std::atomic<MAction> mouse_action_;
	static void setNop(int,void*);
	static void setAddAction(int,void*);
	static void setDelAction(int,void*);
	static void onMouse(int event, int x, int y, int, void* user);
/*Mouse Action END*/

/*Motion Thresh BEGIN*/
	int motion_thresh_dv;
	static void MotionThresh(int,void*);
/*Motion Thresh END*/

/*Display Param BEGIN*/
	std::list<std::string> out_;
	cv::Mat vconsole_;
	void ShowVConsole();
/*Display Param END*/

/*Text ON Image BEGIN*/
	static std::string MtkInfo2StringID(const MtkVehicleInfo&);
	static std::string MtkInfo2StringMotion(const MtkVehicleInfo&v);
	static void Tag(cv::Mat&, const cv::Rect&, const std::string&);
/*Text ON Image END*/

	void SetupGUIEnv(const std::string&);

public:
	GUI(const boost::shared_ptr<Mtk>& mtk,
			const boost::shared_ptr<FarnebackVehicleDetect>& fback,
			const boost::shared_ptr<FrameStream>& fs):mtk_{mtk},fback_{fback},fs_{fs},
			mouse_action_{MAction::NOP},motion_thresh_dv{0}{}

	void Start(const std::string& wn = "video");
};
