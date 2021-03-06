/*@author Timothy Yo (Yang You)*/
#include <boost/thread.hpp>
#include <queue>
/* 
 * Pipe for communication between threads
 * Blocks on Read, if EOF flag not set.
 * Recommend use shared_ptr.
 */
#ifndef PIPE_H
#define PIPE_H
template<typename T>
class Pipe : public std::queue<T> {
protected:
	using Q = std::queue<T>;
	boost::mutex mux_;
	bool eof_;
	bool InternalRead(T& elem) {
		boost::lock_guard<boost::mutex> lk{mux_};
		if (Q::empty()) return false;
		elem = Q::front();
		Q::pop();
		return true;
	}
public:
	/*Nonblock Write*/
	void Write(const T& elem) {
		boost::lock_guard<boost::mutex> lk{mux_};
		Q::push(elem);
	}
	/*Blocking Read*/
	bool Read(T& elem) {
		while(!InternalRead(elem)) {
			if (eof_) return false;
			boost::this_thread::yield();
		}
		return true;
	}
	/*Signal the reader EOF*/
	void SetEOF() {eof_ = true;}

	Pipe():eof_{false}{}
};

template<typename T>
class ShortPipe : public Pipe<T> {
	using Q = std::queue<T>;
	using P = Pipe<T>;
	const size_t len_;
	bool InternalWrite(const T& elem) {
		boost::lock_guard<boost::mutex> lk{P::mux_};
		if (Q::size() < len_) {Q::push(elem);return true;}
		else {return false;}
	}
public:
	/*block Write*/
	void Write(const T& elem) {
		while(!InternalWrite(elem)) boost::this_thread::yield();
	}
	ShortPipe(size_t len):P{},len_{len}{}
};
#endif
