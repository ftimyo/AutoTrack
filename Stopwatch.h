/*@author Timothy Yo (Yang You)*/
#include <chrono>
#ifndef STOPWATCH
#define STOPWATCH
class Stopwatch {
public:
	using Time = std::chrono::high_resolution_clock;
	using Duration = std::chrono::duration<double>;
private:
	Time::time_point birth_;
	Time::time_point death_;
public:
	Stopwatch():birth_{Time::now()},death_{birth_}{}

	/*Update lifespan*/
	void Aging() {death_ = Time::now();}
	/*Growth since last Aging*/
	double Growth() const {
		return std::chrono::duration_cast<Duration>(Time::now() - death_).count();
	}
	/*Growth since birth*/
	double Age() const {
		return std::chrono::duration_cast<Duration>(death_ - birth_).count();
	}
};
#endif
