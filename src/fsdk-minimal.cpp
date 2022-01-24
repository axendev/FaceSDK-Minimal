#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <ctime>

#include <cstdlib>

#include <facerec/import.h>
#include <facerec/libfacerec.h>

#include "Database.h"

#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <ctime>
#include <chrono>
#include <thread>
#include "json.hpp"


//#define USE_DATABASE_DIR
#define SKIP_FRAMES 1


#ifndef USE_DATABASE_DIR
std::vector<pbio::VideoWorker::DatabaseElement> vw_elements;
#endif

using json = nlohmann::json;

using namespace cv;
using namespace std;

namespace chrono_my = std::chrono;



typedef chrono_my::high_resolution_clock::time_point time_point;


const long MAX_GRAB_RETRIES = 200;
const long GRAB_INTERVAL_US = 10000;


const float recognition_distance_threshold = 7400.0;
Database *database;
pbio::VideoWorker::Ptr video_worker;
pbio::FacerecService::Ptr service;

time_point init_c;
VideoCapture *cap;
	


time_point get_time_point(){
	return chrono_my::high_resolution_clock::now();
}

double milliseconds_from(const time_point& a){
	return chrono_my::duration_cast<chrono_my::microseconds>(
		chrono_my::high_resolution_clock::now() - a).count() / (double)1000;
}




		


// match found callback function
//  userdata is supposed to be pointer to this Worker
static void MatchFoundCallback(
		const pbio::VideoWorker::MatchFoundCallbackData &data,
		void* const userdata)
{
	if(data.search_results.empty())
	{
		std::cout << "MatchFoundCallback: Search result empty" << std::endl;
	}
	else
	{
		std::cout << "MatchFoundCallback: Search results size: " << data.search_results.size() << std::endl;
		
		for(auto const& result: data.search_results) 
		{		
			uint64_t _person_id = result.person_id;
			
			if(_person_id == pbio::VideoWorker::MATCH_NOT_FOUND_ID)
			{
				cout << "MatchFoundCallback: Unknown person "
					 << " frame#" << data.frame_id
					 //<<", ts: " << milliseconds_from(init_c) << " ms]"
					 << " sample_id " << data.sample->getID()
					 << endl;
				
				// face was recognized but did not matched with any one from the base
			}
			else
			{
				cout << "MatchFoundCallback: PersonID: " 
					 << _person_id
					 << " frame#" << data.frame_id
					 //<<", ts: " << milliseconds_from(init_c) << "ms]"
					 << " sample_id " << data.sample->getID()
					 << endl;
					 
				// face was recognized and matched with one from the base
				
			}
		}
	}
}
		
static void TemplateCreatedCallback(
		const pbio::VideoWorker::TemplateCreatedCallbackData &data,
		void *const userdata)
{
	std::cout << "Template created" << " at " << data.frame_id << std::endl;
}

static void TrackingCallback(
		const pbio::VideoWorker::TrackingCallbackData &data,
		void *const userdata)
{
	for (int i = 0; i < data.samples.size(); i++)
	{
		auto& sample = data.samples[i];

		auto rect = sample->getFaceCutRectangle(pbio::RawSample::FACE_CUT_BASE); // FACE_CUT_BASE FACE_CUT_TOKEN_FRONTAL

		auto leftEye = sample->getLeftEye();
		auto rightEye = sample->getRightEye();

		auto angels = sample->getAngles();

		float quality = data.samples_quality[i];
		int light_and_blur = data.samples_good_light_and_blur[i];
		int good_angles = data.samples_good_angles[i];


		std::cout << "Tracking event at frame#" << data.frame_id
			<< " track_id: " << data.samples_track_id[i]
			<< " left-eye pos: (" << leftEye.x << ", " << leftEye.y << ")"
			<< " right-eye pos: (" << rightEye.x << ", " << rightEye.y << ")"
			<< std::endl;
	}
			  
}


static void TrackingLostCallback(
		const pbio::VideoWorker::TrackingLostCallbackData &data,
		void *const userdata)
{
	std::cout << "TrackingLost event at frames[" 
			  << data.first_frame_id << ", " << data.last_frame_id
			  << "] track_id: " << data.track_id << std::endl;

}


void OpenCamera(string video_src)
{
	cout << "OpenCamera: Try to open " << video_src << std::endl;
	
	VideoCapture *new_cap = new VideoCapture(video_src); // open the default camera
	int retry_n = 5;
	
	while(--retry_n)
	{
		if(!new_cap->isOpened())  // check if we succeeded
		{
			cout << "OpenCamera: VideoCapture Not Opened. Retry..." << std::endl;
		}
		else
		{
			cout << "OpenCamera: VideoCapture Opened" << std::endl;
			cap = new_cap;
			return; // true;
		} 
	}
	cout << "OpenCamera: VideoCapture Not Opened. End." << std::endl;
	cap = nullptr;
	return; // false;
}

bool GrabCamera()
{
	bool grab_ret = false;
	long grab_cnt = 0;

	do{
		grab_ret = cap->grab();
		
		if(!grab_ret)
			this_thread::sleep_for(chrono::microseconds(GRAB_INTERVAL_US));
		
	}while((!grab_ret) && (++grab_cnt < MAX_GRAB_RETRIES));
	
	
	return grab_ret;
}

void video_thread(string video_src)
{

	bool ret;
	pbio::CVRawImage cvri;
	Mat frame, down;
	
	while(1)
	{
		try
		{
			// Open capturer 			
			OpenCamera(video_src);
			
			if(cap == nullptr)
			{
				cout << time(nullptr) << ": " << "VideoCapture Opening Error !!! Restart recogn_task in 10 sec..." << std::endl;	
				this_thread::sleep_for(chrono::milliseconds(10000));
				continue; // Restart cycle
			}				
					
			uint64_t cnt = 0;
					
			while(1)
			{
				if(!cap->isOpened())  // check if we succeeded
				{
					cout << time(nullptr) << ": " << "RecognTask: WhileWorking: VideoCapture Not Opened. Restart task..." << std::endl;
					throw std::invalid_argument( "Capture not opened, restart task..." );
				}
				
				
				ret = GrabCamera();
				if(!ret)
				{
					delete cap;
					throw std::invalid_argument( "Camera connection failure. Restart ..." );
				}
				
				// Get new frame
				ret = cap->retrieve(frame);
				if(!ret || frame.empty())
				{
					cout << time(nullptr) << ": " << "RecognTask: Frame is empty !" << std::endl;
					this_thread::sleep_for(chrono::milliseconds(20));
					continue;
				}
							
				
				if(cnt % SKIP_FRAMES == 0)
				{					
					// Send frame to FaceSDK
					cvri.mat() = frame;
					int frame_id = video_worker->addVideoFrame(cvri, 0);
					//cout << "Frame id:" << frame_id <<" ts: " << milliseconds_from(start_time) << "ms" << endl;		

					//pyrDown(frame, down);
					imshow("img", frame);
					
					waitKey(5);				
				}
				
				video_worker->checkExceptions();
				cnt++;			
			}
			
			//cap->release();		
		}
		catch( const pbio::Error &e )
		{
			std::cerr << time(nullptr) << ": " << "facerec exception catched: '" << e.what() << "' code: " << std::hex << e.code() << std::endl;
			//exit(-1);
		} catch( const std::exception &e )
		{
			std::cerr << time(nullptr) << ": " << "exception catched: '" << e.what() << "'" << std::endl;
		}
		
		this_thread::sleep_for(chrono::milliseconds(2000));
	}
	
	return;
}


int init_faceid(json config)
{
	try
	{
		string method_config = config["facesdk"]["method_config"].get<string>();
		string vworker_config = config["facesdk"]["vworker_config"].get<string>();
		string capturer_config = config["facesdk"]["capturer_config"].get<string>();

		cout << "FaceRec: Use method config: " << method_config << endl;
		cout << "FaceRec: Use vworker config: " << vworker_config << endl;

		// create facerec servcie
		service = pbio::FacerecService::createService(
			config["facesdk"]["dll_path"].get<string>(),
			config["facesdk"]["conf_dir_path"].get<string>(),
			config["facesdk"]["license_dir"].get<string>()
		);

		cout << "FaceRec: FacerecService version: " << service->getVersion() << endl;

		pbio::FacerecService::Config vw_config(vworker_config);



		vw_config.overrideParameter("not_found_match_found_callback", 1);

		vw_config.overrideParameter("single_match_mode", config["facesdk"]["vw_config_override"]["single_match_mode"].get<int>());
		vw_config.overrideParameter("search_k", config["facesdk"]["vw_config_override"]["search_k"].get<int>());
		vw_config.overrideParameter("consecutive_match_count_for_match_found_callback", config["facesdk"]["vw_config_override"]["consecutive_match_count_for_match_found_callback"].get<int>());
		vw_config.overrideParameter("downscale_rawsamples_to_preferred_size", config["facesdk"]["vw_config_override"]["downscale_rawsamples_to_preferred_size"].get<int>());
		vw_config.overrideParameter("min_size", config["facesdk"]["vw_config_override"]["min_size"].get<int>());
		vw_config.overrideParameter("good_blur_threshold", config["facesdk"]["vw_config_override"]["good_blur_threshold"].get<float>());
		//vw_config.overrideParameter("timestamp_distance_threshold_in_microsecs", 60000);
		//vw_config.overrideParameter("enable_active_liveness", 1);

		// create one VideoWorker
		video_worker = service->createVideoWorker(
			vw_config,
			method_config,
			1,   // streams_count
			1,   // processing_threads_count
			1);  // matching_threads_count



#ifdef USE_DATABASE_DIR
		string database_dir = config["facesdk"]["database_dir"].get<string>();

		// create database
		database = new Database(
			database_dir,
			*service->createRecognizer(method_config, true, true),
			*service->createCapturer(capturer_config),
			recognition_distance_threshold);

		// set database
		video_worker->setDatabase(database->vw_elements);
#else
		video_worker->setDatabase(vw_elements);
#endif


		video_worker->addMatchFoundCallbackU(MatchFoundCallback, NULL);
		video_worker->addTemplateCreatedCallbackU(TemplateCreatedCallback, NULL);
		video_worker->addTrackingCallbackU(TrackingCallback, NULL);
		video_worker->addTrackingLostCallbackU(TrackingLostCallback, NULL);

		//ready_ = true;
		cout << "FaceRec: Init facesdk OK" << endl;
	}
	catch (const pbio::Error& e)
	{
		cerr << "FaceRec: Init facesdk: exception catched: '" << e.what() << "' code: " << std::hex << e.code() << endl;
		return -1;
	}
	catch (const exception& e)
	{
		cerr << "FaceRec: Init facesdk: exception catched: '" << e.what() << "'" << endl;
		return -1;
	}


	return 0;
}

json load_config(std::string path)
{
	json config;
	ifstream pc_in(path.c_str(), ifstream::binary);
	if (pc_in.is_open())
	{
		try
		{
			config = json::parse(pc_in);
		}
		catch (json::exception e)
		{
			cout << "Error parse config file" << endl;
			exit(-1);
		}
	}

	else
	{
		cout << "Error openning config  file ..." << endl;
	}
	pc_in.close();
	return config;
}

int main(int argc, char const *argv[])
{
	cout << "Version 1.12 ..." << endl;
	
	if (argc < 2)
	{
		cout << "Usage: fsdk-minimal [path_to_config]" << endl;
		exit(-1);
	}
	else
	{
		cout << "Using config file at " << argv[1] << endl;
	}

	json config = load_config(argv[1]);

	cout << "Init FaceID Lib ..." << endl;
	if (init_faceid(config) != 0)
	{
		// Error while init faceid lib
		cout << time(nullptr) << ": " << "Init FaceID Lib Error. Terminate..." << endl;
		return -1;
	}
	cout << "Init FaceID Lib OK" << endl;
	
	
	
	std::thread video_thr(video_thread, config["RGB_camera"]["device"].get<string>());
	video_thr.join();
}
