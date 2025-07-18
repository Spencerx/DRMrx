#
/*
 *    Copyright (C) 2010, 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the drm2.
 *
 *    drm2 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    drm2 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with drm2; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * 	This particular driver is a very simple wrapper around the
 * 	librtlsdr. The librtlsdr is osmocom software and all rights
 * 	are greatly acknowledged
 *
 *	To keep things simple, we just load the librtlsdr library
 *	and wrap around it
 */

#define READLEN_DEFAULT 8192

#include	<QThread>
#include	"rtlsdr-handler.h"
#include	<stdio.h>
#include	<QSettings>
//
//	The naming of functions for accessing shared libraries
//	differ between Linux and Windows
#ifdef __MINGW32__
#define	GETPROCADDRESS	GetProcAddress
#else
#define	GETPROCADDRESS	dlsym
#endif
//
//	for handling the events in libusb, we need a controlthread
//	whose sole purpose is to process the rtlsdr_read_async function
//	from the lib.

	dll_driver::dll_driver (rtlsdrHandler *d) {
	theStick	= d;
	start ();
}

	dll_driver::~dll_driver (void) {
}


static 
float convTable [256];

static
std::complex<float> ibuf [READLEN_DEFAULT / 2];

//	For the callback, we do need some environment 
//	This is the user-side call back function
void	dll_driver::RTLSDRCallBack (unsigned char *buf,
	                            uint32_t len, void *ctx) {
rtlsdrHandler	*theStick = (rtlsdrHandler *)ctx;
int cnt	= 0;
std::complex<float> localBuf [READLEN_DEFAULT / 2];

	if ((len != READLEN_DEFAULT) || (theStick == NULL))
	   return;

	for (int i = 0; i < READLEN_DEFAULT / 2; i ++) {
	   std::complex<float> tmp =
	               std::complex<float> (convTable [buf [2 * i]],
	                                    convTable [buf [2 * i + 1]]);
	   std::complex<float> tmp2;
	   if (theStick -> filter_1 -> Pass (tmp, &tmp2))
	      if (theStick -> filter_2 -> Pass (tmp2, &tmp2)) {
	         if (theStick -> directSamplingMode)
	            localBuf [cnt] = theStick -> theFilter. Pass (5 *real (tmp2));
	         else
	            localBuf [cnt] = tmp2;
                    cnt ++;
	      }
	}
	theStick -> _I_Buffer ->  putDataIntoBuffer (localBuf, cnt);
	if ((int)(theStick -> _I_Buffer -> GetRingBufferReadAvailable ()) >
	                                           theStick -> outputRate / 10)
	   theStick -> newdataAvailable (theStick -> outputRate / 10);
}

void	dll_driver::run () {
	(theStick -> rtlsdr_read_async) (theStick -> device,
	                          (rtlsdr_read_async_cb_t)&RTLSDRCallBack,
	                          (void *)theStick,
	                          0,
	                          READLEN_DEFAULT);
}
//
//	Our wrapper is a simple classs
//	to keep things simple, we just load the osmocom function
//	dynamically
	rtlsdrHandler::rtlsdrHandler (RadioInterface *mr,
                                      QSettings      *s,
                                      RingBuffer<std::complex<float>> *r):
                                               myFrame (nullptr),
	                                       theFilter (2 * 8192) {
int	res;
int16_t	deviceIndex;
int16_t	i;
int	inputRate;
int	k;
QString	temp;

	outputRate		= 96000;
	inputRate		= 24 * outputRate;
	setupUi (&myFrame);
        myFrame. show ();
        this    -> _I_Buffer   = r;
        this    -> rtlsdrSettings        = s;
//
//	The mapping from 8 bit samples  to floats is done, using a
//	simple mapping table, after all, only 256 entries are needed
	for (i = 0; i < 256; i ++)
	   convTable [i] = (i - 128.0) / 128.0;
//
	workerHandle	= nullptr;

	statusLabel	-> setText ("setting up");
#ifdef	__MINGW32__
	Handle		= LoadLibrary ((wchar_t *)L"rtlsdr.dll");
#else
	Handle		= dlopen ("librtlsdr.so", RTLD_NOW);
#endif
	if (Handle == NULL) {
	   fprintf (stderr, "Failed to open rtlsdr.dll\n");
	   statusLabel	-> setText ("no rtlsdr lib");
	   throw (21);
	}
//
//	library is loaded
//	and widget is visible
	fprintf (stderr, "rtlsdr is loaded\n");

	if (!load_rtlFunctions	()) {
	   statusLabel	-> setText ("load failed");
	   goto err;
	}

//	vfoFrequency is the VFO frequency
	vfoFrequency		= KHz (220);	// just a dummy
	deviceCount 		= (this -> rtlsdr_get_device_count) ();
	if (deviceCount == 0) {
	   statusLabel	-> setText ("no device");
	   fprintf (stderr, "No devices found\n");
	   goto err;
	}

	deviceIndex = 0;	// default
	if (deviceCount > 1) {
	   dongleSelector	= new dongleSelect ();
	   for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex ++) {
	      dongleSelector ->
	           addtoDongleList (rtlsdr_get_device_name (deviceIndex));
	   }
	   deviceIndex = dongleSelector -> QDialog::exec ();
	   delete dongleSelector;
	}
//
//	OK, now open the hardware
	res	= this -> rtlsdr_open (&device, deviceIndex);
	if (res < 0) {
	   fprintf (stderr, "Opening dabstick failed\n");
	   statusLabel	-> setText ("opening failed");
	   goto err;
	}

	deviceName	-> setText (rtlsdr_get_device_name (deviceIndex));

	res	= (this -> rtlsdr_set_sample_rate) (device, inputRate);
	if (res < 0) {
	   fprintf (stderr, "Setting samplerate failed\n");
	   statusLabel	-> setText ("samplerate");
	   goto err;
	}

	gainsCount = rtlsdr_get_tuner_gains (device, NULL);
	{  int gains [gainsCount];
	   fprintf (stderr, "Supported gain values (%d): ", gainsCount);
	   gainsCount = rtlsdr_get_tuner_gains (device, gains);
	   for (i = gainsCount; i > 0; i--) {
	      fprintf (stderr, "%.1f ", gains [i - 1] / 10.0);
	      combo_gain -> addItem (QString::number (gains [i - 1]));
	   }

	   fprintf (stderr, "\n");
	}

	temp = rtlsdrSettings -> value ("externalGain", "10"). toString ();
        k       = combo_gain -> findText (temp);
        if (k != -1) {
           combo_gain   -> setCurrentIndex (k);
           theGain      = temp. toInt ();
        }
	else {
	   combo_gain	-> setCurrentIndex (gainsCount / 2);
	   theGain	= (combo_gain -> currentText ()). toInt ();
	}
	 
	rtlsdr_set_tuner_gain_mode (device, 1);
	rtlsdr_set_tuner_gain (device, theGain);
	
	connect (combo_gain, SIGNAL (textActivated (const QString &)),
	         this, SLOT (setExternalGain (const QString &)));
	connect (f_correction, SIGNAL (valueChanged (int)),
	         this, SLOT (setCorrection (int)));
	connect (checkAgc, SIGNAL (stateChanged (int)),
	         this, SLOT (setAgc (int)));
	connect (directSamplingSwitch, SIGNAL (stateChanged (int)),
	         this, SLOT (handle_directSamplingSwitch (int)));
//
//	since the connections are made, the settings will trigger
//	the functions associated with the control
	if (rtlsdrSettings != NULL) {
	   rtlsdrSettings	-> beginGroup ("Stick");
	   k	= rtlsdrSettings	-> value ("dab_correction", 0). toInt ();
	   f_correction		-> setValue (k);

	   rtlsdrSettings	-> endGroup ();
	}

	statusLabel	-> setText ("Loaded");
	filter_1	= new decimatingFIR (2 * 4 + 1,
                                             + outputRate / 2,
                                             inputRate, 4);
	filter_2        = new decimatingFIR (2 * 8 + 1,
                                             outputRate / 2,
                                             inputRate / 4, 6);

	directSamplingMode	= false;
	return;
err:
	rtlsdr_close (device);
#ifdef __MINGW32__
	FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
	throw (22);
}

	rtlsdrHandler::~rtlsdrHandler	() {
	rtlsdrSettings	-> beginGroup ("Stick");
	rtlsdrSettings	-> setValue ("rtlsdr_externalGain",
	                                          combo_gain -> currentText ());
	rtlsdrSettings	-> setValue ("rtlsdr_correction",
	                                          f_correction -> value ());
	rtlsdrSettings	-> endGroup ();

	stopReader ();
	rtlsdr_close (device);

	delete	filter_1;
	delete	filter_2;
#ifdef __MINGW32__
	FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
}

//
//
int32_t	rtlsdrHandler::getVFOFrequency	(void) {
	return (int32_t)rtlsdr_get_center_freq (device);
}
//
//
void	rtlsdrHandler::setVFOFrequency	(int32_t f) {
	vfoFrequency	= f;
	(void)rtlsdr_set_center_freq (device, f);
}
//
//
bool	rtlsdrHandler::restartReader	(void) {
int32_t	r;

	if (workerHandle != NULL)	// running already
	   return true;

	r = rtlsdr_reset_buffer (device);
	if (r < 0)
	   return false;

	rtlsdr_set_center_freq (device, vfoFrequency);
	workerHandle	= new dll_driver (this);
	return true;
}

void	rtlsdrHandler::stopReader	(void) {
	if (workerHandle == NULL)
	   return;

	rtlsdr_cancel_async (device);
	if (workerHandle != NULL) {
	   while (!workerHandle -> isFinished ()) 
	      usleep (10);

	   delete	workerHandle;
	}
	workerHandle	= NULL;
}

void	rtlsdrHandler::resetBuffer	() {
	_I_Buffer	-> FlushRingBuffer ();
}

//
//
void	rtlsdrHandler::setExternalGain	(const QString &s) {
        theGain         = s. toInt ();
	int xxx = rtlsdr_set_tuner_gain (device, theGain);
	fprintf (stderr, "setting the gain to %d gave %d\n", theGain, xxx);
}
//

void	rtlsdrHandler::setCorrection	(int ppm) {
	rtlsdr_set_freq_correction (device, ppm);
}
//
bool	rtlsdrHandler::load_rtlFunctions (void) {
//	link the required procedures
	rtlsdr_open	= (pfnrtlsdr_open)
	                       GETPROCADDRESS (Handle, "rtlsdr_open");
	if (rtlsdr_open == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_open\n");
	   return false;
	}
	rtlsdr_close	= (pfnrtlsdr_close)
	                     GETPROCADDRESS (Handle, "rtlsdr_close");
	if (rtlsdr_close == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_close\n");
	   return false;
	}

	rtlsdr_set_sample_rate =
	    (pfnrtlsdr_set_sample_rate)GETPROCADDRESS (Handle, "rtlsdr_set_sample_rate");
	if (rtlsdr_set_sample_rate == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_set_sample_rate\n");
	   return false;
	}

	rtlsdr_get_sample_rate	=
	    (pfnrtlsdr_get_sample_rate)
	               GETPROCADDRESS (Handle, "rtlsdr_get_sample_rate");
	if (rtlsdr_get_sample_rate == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_get_sample_rate\n");
	   return false;
	}

	rtlsdr_set_agc_mode	=
	    (pfnrtlsdr_set_agc_mode)
	               GETPROCADDRESS (Handle, "rtlsdr_set_agc_mode");
	if (rtlsdr_set_agc_mode == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_set_agc_mode\n");
	   return false;
	}

	rtlsdr_get_tuner_gains		= (pfnrtlsdr_get_tuner_gains)
	                     GETPROCADDRESS (Handle, "rtlsdr_get_tuner_gains");
	if (rtlsdr_get_tuner_gains == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_get_tuner_gains\n");
	   return false;
	}

	rtlsdr_set_tuner_gain_mode	= (pfnrtlsdr_set_tuner_gain_mode)
	                     GETPROCADDRESS (Handle, "rtlsdr_set_tuner_gain_mode");
	if (rtlsdr_set_tuner_gain_mode == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_set_tuner_gain_mode\n");
	   return false;
	}

	rtlsdr_set_tuner_gain	= (pfnrtlsdr_set_tuner_gain)
	                     GETPROCADDRESS (Handle, "rtlsdr_set_tuner_gain");
	if (rtlsdr_set_tuner_gain == NULL) {
	   fprintf (stderr, "Cound not find rtlsdr_set_tuner_gain\n");
	   return false;
	}

	rtlsdr_get_tuner_gain	= (pfnrtlsdr_get_tuner_gain)
	                     GETPROCADDRESS (Handle, "rtlsdr_get_tuner_gain");
	if (rtlsdr_get_tuner_gain == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_get_tuner_gain\n");
	   return false;
	}
	rtlsdr_set_center_freq	= (pfnrtlsdr_set_center_freq)
	                     GETPROCADDRESS (Handle, "rtlsdr_set_center_freq");
	if (rtlsdr_set_center_freq == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_set_center_freq\n");
	   return false;
	}

	rtlsdr_get_center_freq	= (pfnrtlsdr_get_center_freq)
	                     GETPROCADDRESS (Handle, "rtlsdr_get_center_freq");
	if (rtlsdr_get_center_freq == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_get_center_freq\n");
	   return false;
	}

	rtlsdr_reset_buffer	= (pfnrtlsdr_reset_buffer)
	                     GETPROCADDRESS (Handle, "rtlsdr_reset_buffer");
	if (rtlsdr_reset_buffer == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_reset_buffer\n");
	   return false;
	}

	rtlsdr_read_async	= (pfnrtlsdr_read_async)
	                     GETPROCADDRESS (Handle, "rtlsdr_read_async");
	if (rtlsdr_read_async == NULL) {
	   fprintf (stderr, "Cound not find rtlsdr_read_async\n");
	   return false;
	}

	rtlsdr_get_device_count	= (pfnrtlsdr_get_device_count)
	                     GETPROCADDRESS (Handle, "rtlsdr_get_device_count");
	if (rtlsdr_get_device_count == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_get_device_count\n");
	   return false;
	}

	rtlsdr_cancel_async	= (pfnrtlsdr_cancel_async)
	                     GETPROCADDRESS (Handle, "rtlsdr_cancel_async");
	if (rtlsdr_cancel_async == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_cancel_async\n");
	   return false;
	}

	rtlsdr_set_direct_sampling = (pfnrtlsdr_set_direct_sampling)
	                  GETPROCADDRESS (Handle, "rtlsdr_set_direct_sampling");
	if (rtlsdr_set_direct_sampling == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_set_direct_sampling\n");
	   return false;
	}

	rtlsdr_set_freq_correction = (pfnrtlsdr_set_freq_correction)
	                  GETPROCADDRESS (Handle, "rtlsdr_set_freq_correction");
	if (rtlsdr_set_freq_correction == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_set_freq_correction\n");
	   return false;
	}

	rtlsdr_get_device_name = (pfnrtlsdr_get_device_name)
	                   GETPROCADDRESS (Handle, "rtlsdr_get_device_name");
	if (rtlsdr_get_device_name == NULL) {
	   fprintf (stderr, "Could not find rtlsdr_get_device_name\n");
	   return false;
	}

	fprintf (stderr, "OK, functions seem to be loaded\n");
	return true;
}

int16_t	rtlsdrHandler::bitDepth	(void) {
	return 8;
}

void	rtlsdrHandler::newdataAvailable (int n) {
	dataAvailable (n);
}

void	rtlsdrHandler::setAgc	(int s) {
	(void) s;
	if (checkAgc -> isChecked ())
	   (void)rtlsdr_set_agc_mode (device, 1);
	else
	   (void)rtlsdr_set_agc_mode (device, 0);
}

void	rtlsdrHandler::handle_directSamplingSwitch	(int d) {
bool checked	= directSamplingSwitch -> isChecked ();
	(void)d;
int xxx = rtlsdr_set_direct_sampling (device, (int)checked);
	fprintf (stderr, "setting direct sampling to %d gave %d\n", 
	                        (int)checked, xxx);
	directSamplingMode	= directSamplingSwitch	-> isChecked ();
}
