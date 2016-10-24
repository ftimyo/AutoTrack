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
	uint32_t bar;
};
struct GUI {
private:
	std::shared_ptr<Mtk> mtk_;
	std::shared_ptr<FBOF> fbof_;
	std::shared_ptr<FrameStream> fs_;

	boost::mutex mux_;
	std::shared_ptr<MSG> mtk_cache_;
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
/*Mouse Action END*/

/*Motion Thresh BEGIN*/
	int motion_thresh_dv;
	static void MotionThresh(int,void*);
/*Motion Thresh END*/

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

public:
	GUI(const std::shared_ptr<Mtk>& mtk,
			const std::shared_ptr<FBOF>& fbof,
			const std::shared_ptr<FrameStream>& fs,
			boost::asio::io_service& ios,
			int port=8889):mtk_{mtk},fbof_{fbof},fs_{fs},
			mouse_action_{MAction::MADD},motion_thresh_dv{0},
			ctlsrv_{TCPServer<RtCtl>::makeSRV(ios,port)}{}

	void Start(const std::string&);
};
