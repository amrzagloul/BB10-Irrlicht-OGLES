// Copyright (C) 2002-2011 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

//#define _IRR_COMPILE_WITH_BB10_DEVICE_

#ifdef _IRR_COMPILE_WITH_BB10_DEVICE_

#include "CIrrDeviceBB10.h"
#include "IEventReceiver.h"
#include "irrList.h"
#include "os.h"
#include "CTimer.h"
#include "irrString.h"
#include "Keycodes.h"
#include "COSOperator.h"
#include "SIrrCreationParameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include <EGL/egl.h>

#include <bps/bps.h>
#include <bps/event.h>
#include <bps/navigator.h>

namespace irr
{
	namespace video
	{

		#ifdef _IRR_COMPILE_WITH_OGLES1_
        	IVideoDriver* createOGLES1Driver(const SIrrlichtCreationParameters& params,
			video::SExposedVideoData& data, io::IFileSystem* io);
		#endif

		#ifdef	_IRR_COMPILE_WITH_OGLES2_
		IVideoDriver* createOGLES2Driver(const SIrrlichtCreationParameters& params,
		        video::SExposedVideoData& data, io::IFileSystem* io);
		#endif

	} // end namespace video

} // end namespace irr


namespace irr
{

//! constructor
CIrrDeviceBB10::CIrrDeviceBB10(const SIrrlichtCreationParameters& param)
	: CIrrDeviceStub(param),
	bspScreenWindow((screen_window_t)param.WindowId), MouseX(0), MouseY(0), MouseButtonStates(0),
	Width(param.WindowSize.Width), Height(param.WindowSize.Height),
	Resizable(false), WindowHasFocus(true), WindowMinimized(false)
{
	#ifdef _DEBUG
	setDebugName("CIrrDeviceBB10");
	#endif

	// Initialize SDL... Timer for sleep, video for the obvious, and
	// noparachute prevents SDL from catching fatal errors.
	if (bps_initialize() !=BPS_SUCCESS)
	{
		os::Printer::log( "Unable to initialize BSP !");
		Close = true;
	}

	//create screen context
	screen_create_context(&bspScreenCtx, 0);
	core::stringc devVersion = "BB10 Version ";
	Operator = new COSOperator(devVersion);
	os::Printer::log(devVersion.c_str(), ELL_INFORMATION);

	// create keymap
	createKeyMap();

	// create window
	if (CreationParams.DriverType != video::EDT_NULL)
	{
		// create the window, only if we do not use the null device
		createWindow();
	}

	// create cursor control
	CursorControl = new CCursorControl(this);

	// create driver
	createDriver();

	if (VideoDriver)
		createGUIAndScene();
}


//! destructor
CIrrDeviceBB10::~CIrrDeviceBB10()
{
	//TODO
}


bool CIrrDeviceBB10::createWindow()
{
	if ( Close )
		return false;

	int rc = screen_create_window(&bspScreenWindow, bspScreenCtx);
	if(rc)
	{
		os::Printer::log("Screen create window failed!");
		return false;
	}

	if(video::EDT_OGLES1 != CreationParams.DriverType && video::EDT_OGLES2 !=CreationParams.DriverType)
		return false;

	bspformat = SCREEN_FORMAT_RGBX8888;
	if(CreationParams.Bits==16)
		bspformat = SCREEN_FORMAT_RGBX4444;

	bspUsage = SCREEN_USAGE_OPENGL_ES1 | SCREEN_USAGE_ROTATION;
	if(video::EDT_OGLES2 ==CreationParams.DriverType)
		bspUsage = SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_ROTATION;

	//window properties
	rc = screen_set_window_property_iv(bspScreenWindow, SCREEN_PROPERTY_FORMAT, &bspformat);

	rc = screen_set_window_property_iv(bspScreenWindow, SCREEN_PROPERTY_USAGE, &bspUsage);

	screen_display_t screenDisplay;
	rc = screen_get_window_property_pv(bspScreenWindow, SCREEN_PROPERTY_DISPLAY, (void **)&screenDisplay);

	int screenResolution[2];
	rc = screen_get_display_property_iv(screenDisplay, SCREEN_PROPERTY_SIZE, screenResolution);

	int angle = atoi(getenv("ORIENTATION"));

	screen_display_mode_t screenDisplayMode;
	rc = screen_get_display_property_pv(screenDisplay, SCREEN_PROPERTY_MODE, (void**)&screenDisplayMode);

	int bufferSize[2];
	rc = screen_get_window_property_iv(bspScreenWindow, SCREEN_PROPERTY_BUFFER_SIZE, bufferSize);

	int newBufferSize[2] = {bufferSize[0], bufferSize[1]};
	if ((angle == 0) || (angle == 180)) 
	{
		if (((screenDisplayMode.width > screenDisplayMode.height) && (bufferSize[0] < bufferSize[1])) 
			|| ((screenDisplayMode.width < screenDisplayMode.height) && (bufferSize[0] > bufferSize[1])))
		{
			newBufferSize[1] = bufferSize[0];
			newBufferSize[0] = bufferSize[1];
		}
	}
	else if ((angle == 90) || (angle == 270))
	{
		if (((screenDisplayMode.width > screenDisplayMode.height) && (bufferSize[0] > bufferSize[1]))
			|| ((screenDisplayMode.width < screenDisplayMode.height && bufferSize[0] < bufferSize[1]))) 
		{
			newBufferSize[1] = bufferSize[0];
			newBufferSize[0] = bufferSize[1];
		}
	} 
	else
	{
		os::Printer::log("Navigator returned an unexpected orientation angle.\n");
		return false;
	}

	rc = screen_set_window_property_iv(bspScreenWindow, SCREEN_PROPERTY_BUFFER_SIZE, newBufferSize);

	rc = screen_set_window_property_iv(bspScreenWindow, SCREEN_PROPERTY_ROTATION, &angle);

	rc = screen_create_window_buffers(bspScreenWindow, 2);
	return true;
}


//! create the driver
void CIrrDeviceBB10::createDriver()
{
	video::SExposedVideoData data;
	switch(CreationParams.DriverType)
	{
	case video::EDT_OGLES1:
		#ifdef _IRR_COMPILE_WITH_OGLES1_
		
		data.OGLES.nativeWindow =bspScreenWindow;
		data.OGLES.displayID =EGL_DEFAULT_DISPLAY;
		VideoDriver = video::createOGLES1Driver(CreationParams, data, FileSystem);
		if (!VideoDriver)
		{
			os::Printer::log("Could not create OGLES1 Driver.", ELL_ERROR);
		}
		#else
		os::Printer::log("OGLES1 Driver was not compiled into this dll. Try another one.", ELL_ERROR);
		#endif // _IRR_COMPILE_WITH_OGLES2_

		break;

	case video::EDT_OGLES2:
		#ifdef _IRR_COMPILE_WITH_OGLES2_
		//video::SExposedVideoData data;
		data.OGLES.nativeWindow =bspScreenWindow;
		data.OGLES.displayID =EGL_DEFAULT_DISPLAY;
		VideoDriver = video::createOGLES2Driver(CreationParams, data, FileSystem);
		if (!VideoDriver)
		{
			os::Printer::log("Could not create OGLES2 Driver.", ELL_ERROR);
		}
		#else
		os::Printer::log("OGLES2 Driver was not compiled into this dll. Try another one.", ELL_ERROR);
		#endif // _IRR_COMPILE_WITH_OGLES2_

		break;

	case video::EDT_SOFTWARE:
		#ifdef _IRR_COMPILE_WITH_SOFTWARE_
		VideoDriver = video::createSoftwareDriver(CreationParams.WindowSize, CreationParams.Fullscreen, FileSystem, this);
		#else
		os::Printer::log("No Software driver support compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_BURNINGSVIDEO:
		#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
		VideoDriver = video::createBurningVideoDriver(CreationParams, FileSystem, this);
		#else
		os::Printer::log("Burning's video driver was not compiled in.", ELL_ERROR);
		#endif
		break;

	case video::EDT_NULL:
		VideoDriver = video::createNullDriver(FileSystem, CreationParams.WindowSize);
		break;

	default:
		os::Printer::log("Unable to create video driver of unknown type.", ELL_ERROR);
		break;
	}
}


//! runs the device. Returns false if device wants to be deleted
bool CIrrDeviceBB10::run()
{
	os::Timer::tick();
	
	//TODO: translate native event
	SEvent irrevent;
	bps_event_t *event = NULL;
	while ( !Close &&(bps_get_event(&event, 0) ==BPS_SUCCESS))
	{

		if ((event) && (bps_event_get_domain(event) == navigator_get_domain())
			&& (NAVIGATOR_EXIT == bps_event_get_code(event)))
		{
			Close =true;
			break;
		}

                if ((event) && (bps_event_get_domain(event) == navigator_get_domain())
                        && (NAVIGATOR_WINDOW_INACTIVE == bps_event_get_code(event)))
                {
			WindowHasFocus =false;
                        break;
                }
                if ((event) && (bps_event_get_domain(event) == navigator_get_domain())
                        && (NAVIGATOR_WINDOW_ACTIVE == bps_event_get_code(event)))
                {
			WindowHasFocus =true;
                        break;
                }

                if ((event) && (bps_event_get_domain(event) == navigator_get_domain())
                        && (NAVIGATOR_WINDOW_STATE == bps_event_get_code(event)))
                {
			if(navigator_event_get_window_state(event) == NAVIGATOR_WINDOW_INVISIBLE)
				WindowHasFocus =false;
			else
				WindowHasFocus =true;
                        break;
                }
		
		//TODO: transfer bps event to irr event
		//postEventFromUser(irrevent);
		break;

	} // end while

	return !Close;
}

//! pause execution temporarily
void CIrrDeviceBB10::yield()
{
	struct timespec ts = {0,0};
	nanosleep(&ts, NULL);
}


//! pause execution for a specified time
void CIrrDeviceBB10::sleep(u32 timeMs, bool pauseTimer)
{
	const bool wasStopped = Timer ? Timer->isStopped() : true;
	if (pauseTimer && !wasStopped)
		Timer->stop();

	struct timespec ts;
	ts.tv_sec = (time_t) (timeMs / 1000);
	ts.tv_nsec = (long) (timeMs % 1000) * 1000000;
	nanosleep(&ts, NULL);

	if (pauseTimer && !wasStopped)
		Timer->start();
}


//! sets the caption of the window
void CIrrDeviceBB10::setWindowCaption(const wchar_t* text)
{
	// do nothing

}


//! presents a surface in the client area
bool CIrrDeviceBB10::present(video::IImage* surface, void* windowId, core::rect<s32>* srcClip)
{
	return false;
}


//! notifies the device that it should close itself
void CIrrDeviceBB10::closeDevice()
{
	Close = true;
}


//! \return Pointer to a list with all video modes supported
video::IVideoModeList* CIrrDeviceBB10::getVideoModeList()
{
	if (!VideoModeList.getVideoModeCount())
	{
		// enumerate video modes.

	}

	return &VideoModeList;
}


//! Sets if the window should be resizable in windowed mode.
void CIrrDeviceBB10::setResizable(bool resize)
{
	// do nothing
}


//! Minimizes window if possible
void CIrrDeviceBB10::minimizeWindow()
{
	// do nothing
}


//! Maximize window
void CIrrDeviceBB10::maximizeWindow()
{
	// do nothing
}


//! Restore original window size
void CIrrDeviceBB10::restoreWindow()
{
	// do nothing
}


//! returns if window is active. if not, nothing need to be drawn
bool CIrrDeviceBB10::isWindowActive() const
{
	return (WindowHasFocus && !WindowMinimized);
}


//! returns if window has focus.
bool CIrrDeviceBB10::isWindowFocused() const
{
	return WindowHasFocus;
}


//! returns if window is minimized.
bool CIrrDeviceBB10::isWindowMinimized() const
{
	return WindowMinimized;
}


//! Set the current Gamma Value for the Display
bool CIrrDeviceBB10::setGammaRamp( f32 red, f32 green, f32 blue, f32 brightness, f32 contrast )
{
	// do nothing
	return false;
}

//! Get the current Gamma Value for the Display
bool CIrrDeviceBB10::getGammaRamp( f32 &red, f32 &green, f32 &blue, f32 &brightness, f32 &contrast )
{
	// do nothing
	return false;
}

//! returns color format of the window.
video::ECOLOR_FORMAT CIrrDeviceBB10::getColorFormat() const
{
	return CIrrDeviceStub::getColorFormat();
}


void CIrrDeviceBB10::createKeyMap()
{

}

} // end namespace irr

#endif // _IRR_COMPILE_WITH_SDL_DEVICE_

