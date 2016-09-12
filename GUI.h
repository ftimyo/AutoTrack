/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "FarnebackVehicleDetect.h"
#include "FrameStream.h"
#include <string>
struct GUI {
	boost::shared_ptr<Mtk> mtk_;
	boost::shared_ptr<FarnebackVehicleDetect> fback_;
	boost::shared_ptr<FrameStream> fs_;

	boost::mutex mux_;
	boost::shared_ptr<FarnebackVehicleDetectOutput> cache_;

public:
	GUI(const boost::shared_ptr<Mtk>& mtk,
			const boost::shared_ptr<FarnebackVehicleDetect>& fback,
			const boost::shared_ptr<FrameStream>& fs):mtk_{mtk},fback_{fback},fs_{fs}{}

	void Start(const std::string&);
	void UpdateCache(const boost::shared_ptr<FarnebackVehicleDetectOutput>&);
	bool SendFarnebackVehicleInfo(const cv::Point&);
	static void onMouse(int event, int x, int y, int, void* user);
	void SetupGUIEnv(const std::string&);
};
