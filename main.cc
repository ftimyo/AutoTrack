/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "Stopwatch.h"
#include "FarnebackVehicleDetect.h"
#include "FrameStream.h"
#include "GUI.h"
#include <cstdio>

int main(int,char* argv[]) {
	boost::asio::io_service ios;
	auto mtk = boost::make_shared<Mtk>(1);
	auto fback = boost::make_shared<FarnebackVehicleDetect>(mtk);
	auto fs = boost::make_shared<FrameStream>(ios,mtk);
	GUI a{mtk,fback,fs};
	a.Start();
	return 0;
}

