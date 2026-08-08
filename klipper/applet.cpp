/* This file is part of the KDE project
   Copyright (C) by Andrew Stanley-Jones
   Copyright (C) 2000 by Carsten Pfeiffer <pfeiffer@kde.org>

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
#include "applet.h"

#include <tdeaboutapplication.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <dcopclient.h>
#include <tdeconfig.h>

#include "toplevel.h"
#include "history.h"
#include "klipperpopup.h"

extern "C"
{
    TDE_EXPORT KPanelApplet* init(TQWidget *parent, const TQString& configFile)
    {
        TDEGlobal::locale()->insertCatalogue("klipper");
        int actions = KPanelApplet::Preferences | KPanelApplet::About | KPanelApplet::Help;
        return new KlipperApplet(configFile, KPanelApplet::Normal, actions, parent, "klipper");
    }
}

KlipperApplet::KlipperApplet(const TQString& configFile, Type t, int actions,
                         TQWidget *parent, const char *name)
    : KPanelApplet(configFile, t, actions, parent, name)
{
    KlipperWidget::createAboutData();
    move( 0, 0 );
    setBackgroundMode(TQWidget::X11ParentRelative);
    widget = new KlipperAppletWidget( this );
    setCustomMenu(widget->history()->popup());
    centerWidget();
    widget->show();
}

KlipperApplet::~KlipperApplet()
{
    widget->saveSession();
    delete widget;
    KlipperWidget::destroyAboutData();
}

int KlipperApplet::widthForHeight(int) const
{
    return widget->width();
}

int KlipperApplet::heightForWidth(int) const
{
    return widget->height();
}

void KlipperApplet::resizeEvent( TQResizeEvent* ev )
{
    widget->adjustSize();
    KPanelApplet::resizeEvent( ev );
    centerWidget();
}

void KlipperApplet::centerWidget()
{
    int x = (width() - widget->width())/2;
    int y = (height() - widget->height())/2;
    widget->move( x, y );
}

void KlipperApplet::preferences()
{
    widget->slotConfigure();
}

void KlipperApplet::help()
{
    tdeApp->invokeHelp(TQString::null, TQString::fromLatin1("klipper"));
}

void KlipperApplet::about()
{
    TDEAboutApplication about(this, 0);
    about.exec();
}

KlipperAppletWidget::KlipperAppletWidget( TQWidget* parent )
// init() is called first, before KlipperWidget is called with ( parent, tdeconfig )
    : KlipperWidget( ( init(), parent ), new TDEConfig( "klipperrc" ))
{
}

// this needs to be called before KlipperWidget ctor, because it performs already some
// operations with the clipboard, and the other running instance could notice that
// and request data while this instance is waiting in the DCOP call
void KlipperAppletWidget::init()
{
    // if there's klipper process running, quit it
    TQByteArray arg1, arg2;
    TQCString str;
    // call() - wait for finishing
    tdeApp->dcopClient()->call("klipper", "klipper", "quitProcess()", arg1, str, arg2 );
    // register ourselves, so if klipper process is started,
    // it will quit immediately (TDEUniqueApplication)
    s_dcop = new DCOPClient;
    s_dcop->registerAs( "klipper", false );
}

KlipperAppletWidget::~KlipperAppletWidget()
{
    delete s_dcop;
    s_dcop = 0;
}

DCOPClient* KlipperAppletWidget::s_dcop = 0;

// this is just to make klipper process think we're TDEUniqueApplication
// (AKA ugly hack)
int KlipperAppletWidget::newInstance()
{
    return 0;
}


#include "applet.moc"
