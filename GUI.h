/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "FarnebackVehicleDetect.h"
#include "FrameStream.h"
struct GUI {
	boost::shared_ptr<Mtk> mtk_;
	boost::shared_ptr<FarnebackVehicleDetect> fback_;
	boost::shared_ptr<FrameStream> fs_;
	GUI(const boost::shared_ptr<Mtk>& mtk,
			const boost::shared_ptr<FarnebackVehicleDetect>& fback,
			const boost::shared_ptr<FrameStream>& fs):mtk_{mtk},fback_{fback},fs_{fs}{}

	void Start() {
		fback_->StartFback();
		mtk_->StartMtk();
		fs_->Start();
		boost::shared_ptr<FarnebackVehicleDetectOutput> op;
		cv::Mat canvas;
		while (fback_->output.Read(op)) {
			op->context->img.copyTo(canvas);
			for (const auto& v : op->fbvinfo) {
				auto b = v.bb_;
				cv::rectangle(canvas,b,cv::Scalar(255,0,0),2);
			}
			cv::imshow("GUI",canvas);
			cv::waitKey(30);
		}
	}
};
