/*@author Timothy Yo (Yang You)*/
#include "Mtk.h"
#include "Stopwatch.h"
#include "FBOF.h"
#include "FrameStream.h"
#include "GUI.h"
#include <memory>
#include <cstdio>

int main(int argc,char* argv[]) {
	boost::asio::io_service ios;
	if (argc > 2) {
		std::string s = std::string{"udp://@"}+std::string{argv[1]}
		+std::string{":8888?sync=ext&fflags=nobuffer&overrun_nonfatal=1&fifo_size=50000000"};
		auto mtk = std::make_shared<Mtk>();
		auto fbof = std::make_shared<FBOF>(true);
		auto fs = std::make_shared<FrameStream>(s);
		GUI a{mtk,fbof,fs,ios};
		a.Start(argv[1]);
	} else {
		auto mtk = std::make_shared<Mtk>();
		auto fbof = std::make_shared<FBOF>(true);
		auto fs = std::make_shared<FrameStream>(argv[1]);
		GUI a{mtk,fbof,fs,ios};
		a.Start(argv[1]);
	}
	return 0;
}

