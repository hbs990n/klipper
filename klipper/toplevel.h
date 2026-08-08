/* This file is part of the KDE project
   Copyright (C) by Andrew Stanley-Jones
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>

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
#ifndef _TOPLEVEL_H_
#define _TOPLEVEL_H_

#include <tdeapplication.h>
#include <tdeglobalaccel.h>
#include <tdepopupmenu.h>
#include <tqmap.h>
#include <tqpixmap.h>
#include <dcopobject.h>
#include <tqtimer.h>

class TQClipboard;
class TDEToggleAction;
class TDEAboutData;
class URLGrabber;
class ClipboardPoll;
class TQTime;
class History;
class TDEAction;
class TQMimeSource;
class HistoryItem;
class KlipperSessionManaged;

class KlipperWidget : public TQWidget, public DCOPObject
{
  TQ_OBJECT
  K_DCOP

k_dcop:
    TQString getClipboardContents();
    void setClipboardContents(TQString s);
    void clearClipboardContents();
    void clearClipboardHistory();
    TQStringList getClipboardHistoryMenu();
    TQString getClipboardHistoryItem(int i);

public:
    KlipperWidget( TQWidget *parent, TDEConfig* config );
    ~KlipperWidget();

    virtual void adjustSize();
    TDEGlobalAccel *globalKeys;

    /**
     * Get clipboard history (the "document")
     */
    History* history() { return m_history; }

    static void updateTimestamp();
    static void createAboutData();
    static void destroyAboutData();
    static TDEAboutData* aboutData();

public slots:
    void saveSession();
    void slotSettingsChanged( int category );
    void slotHistoryTopChanged();
    void slotConfigure();

protected:
    /**
     * The selection modes
     *
     * Don't use 1, as I use that as a guard against passing
     * a boolean true as a mode.
     */
    enum SelectionMode { Clipboard = 2, Selection = 4 };

    void paintEvent(TQPaintEvent *);
    void mousePressEvent(TQMouseEvent *);
    void readProperties(TDEConfig *);
    void readConfiguration(TDEConfig *);

    /**
     * Loads history from disk.
     */
    bool loadHistory();

    /**
     * Save history to disk
     */
    void saveHistory();

    void writeConfiguration(TDEConfig *);
    /**
     * @returns the contents of the selection or, if empty, the contents of
     * the clipboard.
     */
    TQString clipboardContents( bool *isSelection = 0L );

    void removeFromHistory( const TQString& text );
    void setEmptyClipboard();

    void clipboardSignalArrived( bool selectionMode );

    /**
     * Check data in clipboard, and if it passes these checks,
     * store the data in the clipboard history.
     */
    void checkClipData( bool selectionMode );

    /**
     * Enter clipboard data in the history.
     */
    void applyClipChanges( const TQMimeSource& data );

    void setClipboard( const HistoryItem& item, int mode );
    bool ignoreClipboardChanges() const;

    TDEConfig* config() const { return m_config; }
    bool isApplet() const { return m_config != tdeApp->config(); }

protected slots:
    void slotPopupMenu();
    void showPopupMenu( TQPopupMenu * );
    void slotRepeatAction();
    void setURLGrabberEnabled( bool );
    void toggleURLGrabber();
    void disableURLGrabber();

private slots:
    void newClipData( bool selectionMode );
    void slotClearClipboard();

    void slotSelectionChanged() {
        clipboardSignalArrived( true );
    }
    void slotClipboardChanged() {
        clipboardSignalArrived( false );
    }

    void slotQuit();
    void slotStartHideTimer();
    void slotStartShowTimer();

    void slotClearOverflow();
    void slotCheckPending();
    void slotDelayedSetClipboard();

    /**
     * Set the flag that the user just picked an item from the history popup.
     */
    void markUserPastePending();

    /**
     * Auto-paste the newly selected history item into the widget
     * that currently has focus, by simulating a Ctrl+V keypress.
     */
    void simulatePaste();

private:

    TQClipboard *clip;

    TQTime *hideTimer;
    TQTime *showTimer;

    TQMimeSource* m_lastClipdata;
    int m_lastClipboard;
    int m_lastSelection;
    History* m_history;
    int m_overflowCounter;
    TDEToggleAction *toggleURLGrabAction;
    TDEAction* clearHistoryAction;
    TDEAction* configureAction;
    TDEAction* quitAction;
    TQPixmap m_pixmap;
    TQPixmap m_scaledpixmap;
    int m_iconOrigWidth;
    int m_iconOrigHeight;
    bool bPopupAtMouse :1;
    bool bKeepContents :1;
    bool bURLGrabber   :1;
    bool bReplayActionInHistory :1;
    bool bUseGUIRegExpEditor    :1;
    bool bNoNullClipboard       :1;
    bool bTearOffHandle         :1;
    bool bIgnoreSelection       :1;
    bool bSynchronize           :1;
    bool bSelectionTextOnly     :1;
    bool bIgnoreImages          :1;
    bool bSavedSelectionMode    :1;
    bool bCheckForEmpty         :1;

    /**
     * Avoid reacting to our own changes, using this
     * lock.
     * Don't manupulate this object directly... use the Ignore struct
     * instead
     */
    int locklevel;

    URLGrabber *myURLGrabber;
    TQString m_lastURLGrabberTextSelection;
    TQString m_lastURLGrabberTextClipboard;
    TDEConfig* m_config;
    TQTimer m_overflowClearTimer;
    TQTimer m_pendingCheckTimer;
    TQTimer m_setClipboardTimer;
    bool m_pendingContentsCheck;
    ClipboardPoll* poll;
    static TDEAboutData* about_data;

    bool blockFetchingNewData();
    KlipperSessionManaged* session_managed;

    /**
     * Set when the user picks an item from the history popup menu.
     * Consumed by slotHistoryTopChanged() to trigger an auto-paste.
     */
    bool m_pendingUserPaste;

    /**
     * Small delay before simulating the paste, so that the popup menu
     * is fully closed and keyboard focus is back on the target window.
     */
    TQTimer m_autoPasteTimer;
};

class Klipper : public KlipperWidget
{
    TQ_OBJECT
    K_DCOP
k_dcop:
    int newInstance();
    void quitProcess(); // not ASYNC
public:
    Klipper( TQWidget* parent = NULL );
};

#endif
