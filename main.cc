/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "Stopwatch.h"
#include "FBOF.h"
#include "FrameStream.h"
#include "GUI.h"
#include <memory>
#include <cstdio>

int main(int,char* argv[]) {
	boost::asio::io_service ios;
	auto mtk = std::make_shared<Mtk>();
	auto fbof = std::make_shared<FBOF>(false);
	auto fs = std::make_shared<FrameStream>(argv[1]);
	GUI a{mtk,fbof,fs,ios};
	a.Start(argv[1]);
	return 0;
}

