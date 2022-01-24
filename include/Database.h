/**
	\file video_recognition_demo/src/Database.h
*/

#ifndef __VIDEO_RECOGNITION_DEMO__DATABASE_H__
#define __VIDEO_RECOGNITION_DEMO__DATABASE_H__

#include <opencv2/opencv.hpp>

#include <facerec/libfacerec.h>
#include <pbio/example/CVRawImage.h>


typedef struct{
	uint64_t person_id;
	std::string name;
	} id_cell;


// database
class Database
{
public:

	// make a thumbnail of a sample
	static
	cv::Mat makeThumbnail(
		const pbio::RawSample &sample,
		const std::string name = std::string());

	// create the database
	Database(
		const std::string database_dir_path,
		pbio::Recognizer &recognizer,
		pbio::Capturer &capturer,
		const float distance_threshold);

	std::vector<pbio::VideoWorker::DatabaseElement> vw_elements;
	std::vector<pbio::RawSample::Ptr> samples;
	std::vector<cv::Mat> thumbnails;
	std::vector<std::string> names;
	std::vector<id_cell> linked_names;
};


#endif  // __VIDEO_RECOGNITION_DEMO__DATABASE_H__
