/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "Stopwatch.h"
#include "FarnebackVehicleDetect.h"
#include "FrameStream.h"
#include "GUI.h"
#include <cstdio>

int main(int,char* argv[]) {
	auto mtk = boost::make_shared<Mtk>();
	auto fback = boost::make_shared<FarnebackVehicleDetect>(mtk);
	auto fs = boost::make_shared<FrameStream>(argv[1],mtk);
	GUI a{mtk,fback,fs};
	a.Start(argv[1]);
	return 0;
}

