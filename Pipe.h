/*@author Timothy Yo (Yang You)*/
#include <boost/thread.hpp>
#include <queue>
#include <memory>
#include <boost/thread.hpp>
#include <cstdlib>
/* 
 * Pipe for communication between threads
 * Blocks on Read, if EOF flag not set.
 * Recommend use shared_ptr.
 */
#ifndef PIPE_H
#define PIPE_H

/*Pipe with theoretically infinite length*/
template<typename T>
class Pipe : public std::queue<T> {
protected:
	using Q = std::queue<T>;
	mutable boost::mutex mux_;
	bool eof_;
	size_t wb_len; /*write blocking length*/
	bool InternalRead(T& elem) {
		boost::lock_guard<boost::mutex> lk{mux_};
		if (Q::empty()) return false;
		elem = std::move(Q::front());
		Q::pop();
		return true;
	}
public:
	/*Nonblock Write*/
	void Write(const T& elem) {
		boost::lock_guard<boost::mutex> lk{mux_};
		Q::push(elem);
	}
	void Write(T&& elem) {
		boost::lock_guard<boost::mutex> lk{mux_};
		Q::push(std::move(elem));
	}
	/*Length Dependant Blocking Write*/
	void WriteBlocking(const T& elem) {
		while (Length() > wb_len) boost::this_thread::yield();
		Write(elem);
	}
	void WriteBlocking(T&& elem) {
		while (Length() > wb_len) boost::this_thread::yield();
		Write(std::move(elem));
	}
	/*Blocking Read*/
	bool Read(T& elem) {
		while(!InternalRead(elem)) {
			if (eof_) return false;
			boost::this_thread::yield();
		}
		return true;
	}
	/*Get Length of the Pipe*/
	size_t Length() const {
		boost::lock_guard<boost::mutex> lk{mux_};
		return Q::size();
	}
	/*Signal the reader EOF*/
	void SetEOF() {eof_ = true;}

	explicit Pipe(size_t len = 2):eof_{false},wb_len{len}{}
};

template<typename T>
class DupSplit {
	using P = Pipe<T>;
	P *in_, *out1_, *out2_;
	boost::thread td_;
public:
	DupSplit(P& in, P& out1, P& out2) : in_{&in},out1_{&out1},out2_{&out2}{}
	void Connect() {
		td_ = boost::thread{[](auto& in, auto& out1, auto& out2){
			T tmp;
			while (in.Read(tmp)) {
				out1.Write(tmp);
				out2.Write(std::move(tmp));
			}
			out1.SetEOF();
			out2.SetEOF();
		},boost::ref(*in_),boost::ref(*out1_),boost::ref(*out2_)};
	}
	void Stop() {
		if (td_.joinable()) td_.join();
	}
};

#endif
