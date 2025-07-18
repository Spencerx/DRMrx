#
/*
 *    Copyright (C) 2014 .. 2025
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the drmradio
 *
 *    swradio is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    swradio is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with swradio; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#
#ifndef	__DEVICE_SELECT__
#define	__DEVICE_SELECT__
#
#include	<QDialog>
#include	<QLabel>
#include	<QListView>
#include	<QStringListModel>
#include	<QStringList>
#include	<cstdint>

class	deviceSelect: public QDialog {
Q_OBJECT
public:
			deviceSelect	();
			~deviceSelect	();
	void		addtoList	(const char *);
	void		addList		(const QStringList &);
private:
	QLabel		*toptext;
	QListView	*selectorDisplay;
	QStringListModel deviceList;
	QStringList	Devices;
	int16_t		selectedItem;
private slots:
void	select_device	(QModelIndex);
};

#endif

