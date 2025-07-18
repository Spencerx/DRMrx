#
/*
 *    Copyright (C) 2017 .. 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the drm-receiver
 *
 *    drm-receiver is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    drm-receiver is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with drm-receiver; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#pragma once

#include	<QString>
#include	<stdint.h>
#include	<math.h>
#include	<vector>
#include	"radio-constants.h"

class	agcHandler {
public:
			agcHandler	(int32_t, int16_t);
			~agcHandler	(void);
	void		set_bitDepth	(int);
	void		doAgc		(DSPCOMPLEX *, int32_t);
	float		doAgc		(DSPCOMPLEX);
	void		setMode		(uint8_t);
	void		setThreshold	(int16_t s);
	enum Mode {
	   AGC_OFF      = 0,
	   AGC_SLOW     = 1,
	   AGC_FAST     = 2
	};
private:
	float		get_db 		(float x);
	float		decayingAverage (float, float, float);
	float		gain_for	(float v);


	int32_t		sampleRate;
	int16_t		bitDepth;
        uint32_t	normalizer;
	uint8_t		agcMode;
	float		currentPeak;
	float		attackAverage;
	float		decayAverage;
	int32_t		bufferSize;
	std::vector<float> buffer;
	int32_t		bufferP;
	float		ThresholdValue;
	float		agcHangtime;
	int16_t		agcHangInterval;

	void		setLevel	(int16_t L);
};

