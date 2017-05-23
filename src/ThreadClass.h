#pragma once

#include <thread>
#include <chrono>

class MyClass
{

public:
	/* Explicitly using the default constructor to
	* underline the fact that it does get called */
	MyClass() : the_thread() {}
	~MyClass() {
		stop_thread = true;
		if (the_thread.joinable()) the_thread.join();
	}

	void Start() {
		// This will start the thread. Notice move semantics!
		the_thread = std::thread(&MyClass::ThreadMain, this);
	}

	void Stop() {
		stop_thread = true;
		if (the_thread.joinable()) the_thread.join();
	}

private:
	std::thread the_thread;
	
	bool stop_thread = false; // Super simple thread stopping.

	void ThreadMain()
	{
		while (!stop_thread)
		{
			std::cout << "thread iteration" << std::endl;
			// Do something useful, e.g:
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	/****/
};


class ThreadCamera : public Camera
{

public:
	/* Explicitly using the default constructor to
	* underline the fact that it does get called */
	ThreadCamera(int deviceIndex) : the_thread()
	{
		_deviceID = deviceIndex;
	}
	~ThreadCamera() {
		stop_thread = true;
		if (the_thread.joinable()) the_thread.join();
	}

	// implement camera interface

	bool initialize()
	{

		_resolution.x = 320;
		_resolution.y = 240;
		_framerate = 125;
		_numColorChannels = 3;
		//_deviceID = 0;


		return true;
	};
	void deinitialize() {  };

	bool startCapture()
	{
		// This will start the thread. Notice move semantics!
		the_thread = std::thread(&ThreadCamera::ThreadMain, this);
		
		return true;
	};
	bool stopCapture()
	{
		stop_thread = true;
		if (the_thread.joinable()) the_thread.join();

		return true;
	};

	cv::Mat receiveFrame()
	{
		return _latestCamFrame;
	};

	unsigned int getCameraWidth() { return _resolution.x; };
	unsigned int getCameraHeight() { return _resolution.y; };
	unsigned int getFramerate() { return _framerate; }
	unsigned int getNumColorChannels() { return _numColorChannels; }

private:
	std::thread the_thread;

	bool stop_thread = false; // Super simple thread stopping.

	bool initCamera()
	{
		using namespace ps3eye;

		// list out the devices
		std::vector<PS3EYECam::PS3EYERef> devices(PS3EYECam::getDevices());
		LOGCON("Found %d cameras.\n", (int)devices.size());

		bool initializationResult = false;

		if (devices.size() > 0)
		{
			_cameraPtr = devices.at(_deviceID);

			if (_numColorChannels == 3)
				initializationResult = _cameraPtr->init(_resolution.x, _resolution.y, _framerate, ps3eye::PS3EYECam::EOutputFormat::BGR);
			else
				initializationResult = _cameraPtr->init(_resolution.x, _resolution.y, _framerate, ps3eye::PS3EYECam::EOutputFormat::Bayer);

			if (initializationResult)
			{

				LOGCON("Initialization of PS3 Eye Cam successful !\n");

				_resolution.x = _cameraPtr->getWidth();
				_resolution.y = _cameraPtr->getHeight();
				_framerate = _cameraPtr->getFrameRate();
				_numColorChannels = _cameraPtr->getOutputBytesPerPixel();

				// allocate memory for output frame
				//frame_bgr = new uint8_t[_resolution.x * _resolution.y * _numColorChannels];

				if (_numColorChannels == 3)
					_latestCamFrame = cv::Mat(_resolution.y, _resolution.x, CV_8UC3);
				else
					_latestCamFrame = cv::Mat(_resolution.y, _resolution.x, CV_8UC1);

			}
			else {
				LOGERROR("Initialization of PS3 Eye Cam faild !!\n");
				return false;
			}
		}

		frame_bgr = new uint8_t[_resolution.x * _resolution.y * _numColorChannels];

		return _cameraPtr->isInitialized();

	}

	void resetFrameCounter()
	{
		// reset frame counter
		_currentfps = 0;
		_fpsCount = 0;
		_lastTime = time(0);
	}
	
	void updateFrameCounter()
	{
		// current FPS computation
		_fpsCount++;
		if (time(0) > _lastTime) {
			_currentfps = _fpsCount;
			_fpsCount = 0;
			_lastTime = time(0);
			printf("cam %d fps = %d\n", _deviceID, _currentfps);
		}
	}

	void ThreadMain()
	{

		// initialize camera
		if (!initCamera())
			return;
		
		_cameraPtr->start();
		
		while (!stop_thread)
		{
			//std::cout << "thread iteration" << std::endl;
			
			// Do something useful, e.g:
			//std::this_thread::sleep_for(std::chrono::seconds(1));

			_cameraPtr->getFrame(frame_bgr);

			updateFrameCounter();
		}

		_cameraPtr->stop();

	}

	// camera members
	uint8_t	*frame_bgr;
	cv::Mat _latestCamFrame;

	ps3eye::PS3EYECam::PS3EYERef _cameraPtr;
	unsigned int	_deviceID;

	cv::Point2i		_resolution;
	unsigned int	_framerate;
	unsigned int	_numColorChannels;

	int _currentfps, _fpsCount = 0;
	time_t _lastTime = time(0);

public:

	int				_exposure = 50;
	int				_gain = 0;
	int				_brightness = 0;
	int				_contrast = 0;
	int				_sharpness = 0;

	bool			_autogain = false;
	bool			_autowhitebalance = false;

	bool			_flipHorizontally = false;
	bool			_flipVertically = false;

	void ThreadCamera::updateCameraSettings() {

		uint8_t exposure = _cameraPtr->getExposure();
		uint8_t gain = _cameraPtr->getGain();
		uint8_t brightness = _cameraPtr->getBrightness();
		uint8_t contrast = _cameraPtr->getContrast();
		uint8_t sharpness = _cameraPtr->getSharpness();

		bool flipVertical = _cameraPtr->getFlipV();
		bool flipHorizontal = _cameraPtr->getFlipH();

		bool autoGain = _cameraPtr->getAutogain();
		bool autoWhiteBalance = _cameraPtr->getAutoWhiteBalance();

		if (exposure != _exposure) {
			_cameraPtr->setExposure(uint8_t(_exposure));
			//LOGCON("Setting expsure to %d\n", uint8_t(_exposure));
		}
		if (gain != _gain) {
			_cameraPtr->setGain(uint8_t(_gain));
			//LOGCON("Setting gain to %d\n", uint8_t(_gain));
		}
		if (brightness != _brightness) {
			_cameraPtr->setBrightness(uint8_t(_brightness));
			LOGCON("Setting brightness to %d\n", uint8_t(_gain));
			LOGWARNING("brightness filter not implemented yet!\n");
		}
		if (contrast != _contrast) {
			_cameraPtr->setContrast(uint8_t(_contrast));
			LOGCON("Setting contrast to %d\n", uint8_t(_gain));
			LOGWARNING("contrast filter not implemented yet!\n");
		}
		if (sharpness != _sharpness) {
			_cameraPtr->setSharpness(uint8_t(_sharpness));
			LOGCON("Setting sharpness to %d\n", uint8_t(_gain));
			LOGWARNING("sharpness filter not implemented yet!\n");
		}

		if (autoGain != _autogain) {
			_cameraPtr->setAutogain(_autogain);
			LOGCON("Setting autogain to %d\n", _autogain);
		}

		if (autoWhiteBalance != _autowhitebalance) {
			_cameraPtr->setAutoWhiteBalance(_autowhitebalance);
			LOGCON("Setting autoWhiteBalance to %d\n", _autowhitebalance);
		}

	}


	/****/
};