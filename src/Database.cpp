     /**
	\file video_recognition_demo/src/Database.cpp
*/

#include <fstream>

#include "Database.h"
#include "MAssert.h"
//#include "Worker.h"
#include <boost/filesystem.hpp>


cv::Mat Database::makeThumbnail(
	const pbio::RawSample &sample,
	const std::string name)
{
	const int thumbnail_size = 150;

	// buffer for the cutted image
	std::ostringstream stream;

	// make a cut in bmp format
	// so we don't waste time for encode/decode image
	// just copying it few times, which is irrelevant
	sample.cutFaceImage(
		stream,
		pbio::RawSample::IMAGE_FORMAT_BMP,
		pbio::RawSample::FACE_CUT_BASE);

	const std::string s_buffer(stream.str());

	const std::vector<uchar> buffer(s_buffer.begin(), s_buffer.end());

	cv::Mat temp = cv::imdecode(buffer, 1);

	// so we got an image

	// check it
	MAssert(!temp.empty(),);
	MAssert(temp.type() == CV_8UC3,);


	// and resize to the thumbnail_size

	cv::Rect res_rect;

	if(temp.rows >= temp.cols)
	{
		res_rect.height = thumbnail_size;
		res_rect.width = temp.cols * thumbnail_size / temp.rows;
	}
	else
	{
		res_rect.width = thumbnail_size;
		res_rect.height = temp.rows * thumbnail_size / temp.cols;
	}

	res_rect.x = (thumbnail_size - res_rect.width) / 2;
	res_rect.y = (thumbnail_size - res_rect.height) / 2;

	cv::Mat result(
		thumbnail_size,
		thumbnail_size,
		CV_8UC3,
		cv::Scalar::all(0));

	resize(
		temp,
		result(res_rect),
		res_rect.size());

	if(!name.empty())
	{
		result.rowRange(result.rows - 27, result.rows) *= 0.5f;

		cv::putText(
			result,
			name,
			cv::Point(0, result.rows - 7),
			cv::FONT_HERSHEY_DUPLEX,
			0.7,
			cv::Scalar::all(255),
			1);//,
			//LINE_AA);
	}

	return result;
}

Database::Database(
	const std::string database_dir_path,
	pbio::Recognizer &recognizer,
	pbio::Capturer &capturer,
	const float distance_threshold)
{
	// check paths
	MAssert(boost::filesystem::exists(database_dir_path),);

	MAssert(boost::filesystem::is_directory(database_dir_path),);

	// get directory content
	std::vector<boost::filesystem::path> path_l1;

	std::copy(
		boost::filesystem::directory_iterator(database_dir_path),
		boost::filesystem::directory_iterator(),
		std::back_inserter(path_l1));


	// ceck every element in that directory

	int element_id_counter = 0;

	for(size_t il1 = 0; il1 < path_l1.size(); ++il1)
	{
		// ignore files
		if(!boost::filesystem::is_directory(path_l1[il1]))
			continue;

		// so path_l1[il1] is supposed to be the path to the person directory

		// get files inside it
		std::vector<boost::filesystem::path> path_l2;
		std::copy(
			boost::filesystem::directory_iterator(path_l1[il1]),
			boost::filesystem::directory_iterator(),
			std::back_inserter(path_l2));

		std::string name;

		// search for the name.txt file
		for(size_t il2 = 0; il2 < path_l2.size(); ++il2)
		{
			if(path_l2[il2].filename() == "name.txt")
			{
				// put file content in the name
				std::ifstream file(path_l2[il2].c_str());

				for(;;)
				{
					const char c = file.get();
					if(!file.is_open() || !file.good())
						break;
					name += c;
				}
			}
		}

		// try to open each file as an image
		for(size_t il2 = 0; il2 < path_l2.size(); ++il2)
		{
			if(path_l2[il2].filename() == "name.txt")
				continue;

			std::cout << "processing '" << path_l2[il2] << "' name: '" << name << "'" << std::endl;
			
			// read image with opencv

			pbio::CVRawImage image;
			image.mat() = cv::imread(path_l2[il2].string());
			
			if(image.mat().empty() || image.mat().type() != CV_8UC3)
			{
				std::cout << "\n\nWARNING: cant read image '" << path_l2[il2] << "'\n\n" << std::endl;
				continue;
			}
			
			// capture the face
			const std::vector<pbio::RawSample::Ptr> captured_samples = capturer.capture(image);
			
			if(captured_samples.size() != 1)
			{
				std::cout << "\n\nWARNING: detected " << captured_samples.size() <<
					" faces on '" << path_l2[il2] << "' image instead of one, image ignored \n\n" << std::endl;
				continue;
			}

			const pbio::RawSample::Ptr sample = captured_samples[0];

			// make template
			const pbio::Template::Ptr templ = recognizer.processing(*sample);




			auto r1 = recognizer.verifyMatch(*templ, *templ);

			std::cout << "Match : D=" << r1.distance << ", S=" << r1.score << std::endl;


			// prepare data for VideoWorker
			pbio::VideoWorker::DatabaseElement vw_element;
			vw_element.element_id = element_id_counter++;
			vw_element.person_id = il1;
			vw_element.face_template = templ;
			vw_element.distance_threshold = distance_threshold;

			vw_elements.push_back(vw_element);

			samples.push_back(sample);

			thumbnails.push_back(makeThumbnail(*sample, name));

			names.push_back(name);
			linked_names.push_back((id_cell){.person_id = vw_element.person_id, .name = name});
			std::cout << "--> personID: " << vw_element.person_id << ", iname: '" << name << "'" << std::endl;
		}

	}

	MAssert(element_id_counter == (int) vw_elements.size(),);
	MAssert(element_id_counter == (int) samples.size(),);
	MAssert(element_id_counter == (int) thumbnails.size(),);
	MAssert(element_id_counter == (int) names.size(),);

	for(std::vector<id_cell>::iterator it = linked_names.begin(); it != linked_names.end(); ++it) {
		id_cell cell = *it;
		std::cout << "person_id: " << cell.person_id << ", name: '" << cell.name << "'" << std::endl;
	}
}
