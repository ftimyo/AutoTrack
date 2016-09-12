/*@author Timothy Yo (Yang You)*/
#include <opencv2/opencv.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdio>
#ifndef MEDIA_H
#define MEDIA_H
struct Media {
	cv::UMat img;
	size_t fn;
};

struct VehicleInfo {
	cv::Rect bb_;	/*bounding box*/
	uint64_t id_; /*vehicle ID*/
	cv::Point2f v_; /*vehicle velocity*/
public:
	VehicleInfo(const cv::Rect& bb,uint64_t id,
			const cv::Point2f& v = cv::Point2f{}):bb_{bb},id_{id},v_{v}{}
	cv::Point GetCenter() const {return (bb_.tl()+bb_.br())/2;}
};

struct VehicleInfoOutput {
	boost::shared_ptr<Media> context;
};
#endif
