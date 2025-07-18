#
/*
 *    Copyright (C)  2018 - 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of drm2
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
 */

#pragma once

#include        <QMainWindow>
#include        <QTimer>
#include        <QWheelEvent>
#include        <QLineEdit>
#include	<QTimer>
#include	<sndfile.h>
#include        "ui_newradio.h"
#include        "radio-constants.h"
#include        "ringbuffer.h"
#include	"shifter.h"
#include	"fft-filters.h"
#include	"decimator.h"

class		deviceHandler;
class		drmDecoder;
class           QSettings;
class           fftScope;
class           fft_scope;
class           audioSink;
class           keyPad;
class           programList;
class		bandPlan;

class	RadioInterface:public QMainWindow,
	               private Ui_MainWindow {
Q_OBJECT
public:
		RadioInterface (QSettings	*sI,
	                        QString		stationList,
	                        bandPlan	*my_bandPlan,
	                        QWidget		*parent = NULL);
		~RadioInterface	();

	int32_t		get_selectedFrequency	();
	int32_t		get_centerFrequency	();
private:
        RingBuffer<std::complex<float> > inputData;
        RingBuffer<std::complex<float> > audioData;

	struct band {
           int32_t      lowF;
           int32_t      highF;
           int32_t      currentOffset;
        }theBand;

	int32_t		centerFrequency;
	int32_t		selectedFrequency;
	QSettings       *settings;
	bandPlan	*my_bandPlan;
        int32_t         inputRate;
	int32_t		decoderRate;
	int32_t		scopeWidth;
        int32_t         workingRate;
        int32_t         audioRate;
        int16_t         displaySize;
        int16_t         spectrumSize;
        double          *displayBuffer;
        audioSink       *audioHandler;
        int16_t         *outTable;
        deviceHandler	*theDevice;
	drmDecoder	*theDecoder;
        fftScope        *hfScope;
	fftFilter	hfFilter;
	shifter		hfShifter;
	decimator	theDecimator;
	int16_t		mouseIncrement;
	int16_t		delayCount;
	void		set_inMiddle	();
//
	SNDFILE		*dumpfilePointer;
	QTimer		secondsTimer;
	void            setupSoundOut   (QComboBox        *streamOutSelector,
                                         audioSink        *our_audioSink,
                                         int32_t          cardRate,
                                         int16_t          *table);

        keyPad          *mykeyPad;
        programList     *myList;
        QLineEdit       *myLine;
        void            adjust          (int32_t);
	
private slots:
        deviceHandler	*getDevice		(const QString &,
	                                         QSettings *,
	                                      RingBuffer<std::complex<float>> *);
        deviceHandler	*setDevice		(QSettings *,
	                                      RingBuffer<std::complex<float>> *);
        void            adjustFrequency_hz	(int);
        void            adjustFrequency_khz	(int);
        void            handle_myLine		(void);
        void            set_hfscopeLevel	(int);
	void		setFrequency		(int32_t);
        void            setStreamOutSelector	(int idx);
        void            handle_freqButton       (void);
        void            wheelEvent              (QWheelEvent *);
	void		set_mouseIncrement	(int);
	void		set_freqSave		(void);
	void		handle_quitButton	(void);
	void		switch_hfViewMode	(int);
	void		updateTime		(void);
	void		set_dumpButton		(void);
	void		closeEvent		(QCloseEvent *event);
public slots:
	void		quickStart		();
	void		sampleHandler		(int amount);
        void            processAudio		(int, int);
};

