/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "FarnebackVehicleDetect.h"
#include "FrameStream.h"
#include <atomic>
#include <string>
struct GUI {
private:
	enum class MAction {NOP,MADD,MDEL};
	boost::shared_ptr<Mtk> mtk_;
	boost::shared_ptr<FarnebackVehicleDetect> fback_;
	boost::shared_ptr<FrameStream> fs_;

	boost::mutex mux_;
	boost::shared_ptr<FarnebackVehicleDetectOutput> cache_;

	std::atomic<MAction> mouse_action_;

	static void setNop(int,void*);
	static void setAddAction(int,void*);
	static void setDelAction(int,void*);
public:
	GUI(const boost::shared_ptr<Mtk>& mtk,
			const boost::shared_ptr<FarnebackVehicleDetect>& fback,
			const boost::shared_ptr<FrameStream>& fs):mtk_{mtk},fback_{fback},fs_{fs},
			mouse_action_{MAction::NOP}{}

	void Start(const std::string&);
	void UpdateCache(const boost::shared_ptr<FarnebackVehicleDetectOutput>&);
	bool SendFarnebackVehicleInfo(const cv::Point&);
	bool SendMtkKillVehicleInfo(const cv::Point&);
	static void onMouse(int event, int x, int y, int, void* user);
	void SetupGUIEnv(const std::string&);
};
