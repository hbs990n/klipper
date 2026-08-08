/* This file is part of the KDE project
   Copyright (C) by Andrew Stanley-Jones

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef NOSLOTS
# define DEF( name, key3, key4, fnSlot ) \
   keys->insert( name, i18n(name), TQString(), key3, key4, this, TQ_SLOT(fnSlot) )
#else
# define DEF( name, key3, key4, fnSlot ) \
   keys->insert( name, i18n(name), TQString(), key3, key4 )
#endif
#define WIN KKey::QtWIN

	keys->insert( "Program:klipper", i18n("Clipboard") );

	DEF( I18N_NOOP("Show Klipper Popup-Menu"), ALT+CTRL+TQt::Key_V, WIN+CTRL+TQt::Key_V, slotPopupMenu() );
	DEF( I18N_NOOP("Manually Invoke Action on Current Clipboard"), ALT+CTRL+TQt::Key_R, WIN+CTRL+TQt::Key_R, slotRepeatAction() );
	DEF( I18N_NOOP("Enable/Disable Clipboard Actions"), ALT+CTRL+TQt::Key_X, WIN+CTRL+TQt::Key_X, toggleURLGrabber() );

#undef DEF
#undef WIN
