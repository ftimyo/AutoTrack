/*@author Timothy Yo (Yang You)*/
#include <opencv2/opencv.hpp>
#include <cstdio>
#include <memory>
#ifndef MEDIA_H
#define MEDIA_H
struct Media {
	cv::UMat img;
	cv::UMat gray;
	size_t fn;
};
using MediaPtr = std::shared_ptr<Media>;

/*Bounding box with ID*/
struct BB {
	cv::Rect bb_;
	uint64_t id_;
	cv::Point GetCenter() const {return (bb_.tl()+bb_.br())/2;}
	bool Covers(const BB& bb) const {return bb_.contains(bb.GetCenter());}
	explicit BB(const cv::Rect& bb, uint64_t id = 0):bb_{bb},id_{id}{}
	BB(const BB&) = default;
	BB(BB&&) = default;
	BB& operator= (const BB&) = default;
	BB& operator= (BB&&) = default;
	virtual ~BB() = default;
};
/*BB with Context*/
struct BBC : public BB {
	std::shared_ptr<Media> context_;
	BBC(const BB& bb, const std::shared_ptr<Media>& context):
		BB{bb},context_{context}{}
	BBC(BB&& bb, const std::shared_ptr<Media>& context):
		BB{std::move(bb)},context_{context}{}
	BBC(const BBC&) = default;
	BBC(BBC&&) = default;
	BBC& operator= (const BBC&) = default;
	BBC& operator= (BBC&&) = default;
};
/*Vector of BBs with Context*/
struct BBS {
	std::shared_ptr<Media> context_;
	std::vector<BB> bbs_;
	explicit BBS(const std::shared_ptr<Media>& context):context_{context}{}
	explicit BBS(std::shared_ptr<Media>&& context):context_{std::move(context)}{}
	BBS(const BBS&) = default;
	BBS(BBS&&) = default;
	BBS& operator= (const BBS&) = default;
	BBS& operator= (BBS&&) = default;
	void RemoveOverlap(BBS& bbs) {
		auto& bbs2 = bbs.bbs_;
		bbs_.erase(
			remove_if(std::begin(bbs_),std::end(bbs_),
				[&bbs2=bbs2](const auto& b){
					for (auto& bb : bbs2) if (b.Covers(bb)) return true;
					return false;
				}),
			std::end(bbs_));
	}
};
/*Message Pass through the System*/
struct MSG {
	std::shared_ptr<BBS> cur_;
	std::shared_ptr<BBS> old_;
	MSG(const std::shared_ptr<BBS>& cur, const std::shared_ptr<BBS>& old):
		cur_{cur},old_{old}{}
	MSG(std::shared_ptr<BBS>&& cur, std::shared_ptr<BBS>&& old):
		cur_{std::move(cur)},old_{std::move(old)}{}
	MSG(const MSG&) = default;
	MSG(MSG&&) = default;
	MSG& operator= (const MSG&) = default;
	MSG& operator= (MSG&&) = default;
};

#endif
