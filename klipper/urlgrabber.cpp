/* This file is part of the KDE project
   Copyright (C) (C) 2000,2001,2002 by Carsten Pfeiffer <pfeiffer@kde.org>

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

#include <tqcursor.h>
#include <tqtimer.h>

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdialogbase.h>
#include <ktextedit.h>
#include <tdelocale.h>
#include <tdepopupmenu.h>
#include <kservice.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <netwm.h>
#include <kstringhandler.h>
#include <kmacroexpander.h>

#include <stdlib.h> // For getenv()

#include "urlgrabber.h"

// TODO:
// - script-interface?
// - Bug in TDEPopupMenu::clear() (insertTitle() doesn't go away sometimes)

#define URL_EDIT_ITEM 10
#define DO_NOTHING_ITEM 11
#define DISABLE_POPUP 12

URLGrabber::URLGrabber( TDEConfig* config )
 : m_config( config )
{
    if( m_config == NULL )
        m_config = tdeApp->config();
    myMenu = 0L;
    myPopupKillTimeout = 8;
    m_stripWhiteSpace = true;

    myActions = new ActionList();
    myActions->setAutoDelete( true );
    myMatches.setAutoDelete( false );

    readConfiguration( m_config );

    myPopupKillTimer = new TQTimer( this );
    connect( myPopupKillTimer, TQ_SIGNAL( timeout() ),
             TQ_SLOT( slotKillPopupMenu() ));

    // testing
    /*
    ClipAction *action;
    action = new ClipAction( "^http:\\/\\/", "Web-URL" );
    action->addCommand("kfmclient exec %s", "Open with Konqi", true);
    action->addCommand("netscape -no-about-splash -remote \"openURL(%s, new-window)\"", "Open with Netscape", true);
    myActions->append( action );

    action = new ClipAction( "^mailto:", "Mail-URL" );
    action->addCommand("kmail --composer %s", "Launch kmail", true);
    myActions->append( action );

    action = new ClipAction( "^\\/.+\\.jpg$", "Jpeg-Image" );
    action->addCommand("kuickshow %s", "Launch KuickShow", true);
    action->addCommand("kview %s", "Launch KView", true);
    myActions->append( action );
    */
}


URLGrabber::~URLGrabber()
{
    delete myActions;
}

//
// Called from Klipper::slotRepeatAction, i.e. by pressing Ctrl-Alt-R
// shortcut. I.e. never from clipboard monitoring
//
void URLGrabber::invokeAction( const TQString& clip )
{
    if ( !clip.isEmpty() )
	myClipData = clip;
    if ( m_stripWhiteSpace )
        myClipData = myClipData.stripWhiteSpace();

    actionMenu( false );
}


void URLGrabber::setActionList( ActionList *list )
{
    delete myActions;
    myActions = list;
}


const ActionList& URLGrabber::matchingActions( const TQString& clipData )
{
    myMatches.clear();
    ClipAction *action = 0L;
    ActionListIterator it( *myActions );
    for ( action = it.current(); action; action = ++it ) {
        if ( action->matches( clipData ) )
            myMatches.append( action );
    }

    return myMatches;
}


bool URLGrabber::checkNewData( const TQString& clipData )
{
    // kdDebug() << "** checking new data: " << clipData << endl;
    myClipData = clipData;
    if ( m_stripWhiteSpace )
        myClipData = myClipData.stripWhiteSpace();

    if ( myActions->isEmpty() )
        return false;

    actionMenu( true ); // also creates myMatches

    return ( !myMatches.isEmpty() &&
             (!m_config->readBoolEntry("Put Matching URLs in history", true)));
}


void URLGrabber::actionMenu( bool wm_class_check )
{
    if ( myClipData.isEmpty() )
        return;

    ActionListIterator it( matchingActions( myClipData ) );
    ClipAction *action = 0L;
    ClipCommand *command = 0L;

    if ( it.count() > 0 ) {
	// don't react on konqi's/netscape's urls...
        if ( wm_class_check && isAvoidedWindow() )
            return;

        TQString item;
        myCommandMapper.clear();
        myGroupingMapper.clear();

        myPopupKillTimer->stop();
        delete myMenu;
        myMenu = new TDEPopupMenu;
        connect( myMenu, TQ_SIGNAL( activated( int )),
                 TQ_SLOT( slotItemSelected( int )));

        for ( action = it.current(); action; action = ++it ) {
            TQPtrListIterator<ClipCommand> it2( action->commands() );
            if ( it2.count() > 0 )
                myMenu->insertTitle( SmallIcon( "klipper" ), action->description() +
				     i18n(" - Actions For: ") +
				     KStringHandler::csqueeze(myClipData, 45));
            for ( command = it2.current(); command; command = ++it2 ) {
                item = command->description;
                if ( item.isEmpty() )
                    item = command->command;

                int id;
                if ( command->pixmap.isEmpty() )
                    id = myMenu->insertItem( item );
                else
                    id = myMenu->insertItem( SmallIcon(command->pixmap), item);
                myCommandMapper.insert( id, command );
                myGroupingMapper.insert( id, action->capturedTexts() );
            }
        }

        // only insert this when invoked via clipboard monitoring, not from an
        // explicit Ctrl-Alt-R
        if ( wm_class_check )
        {
            myMenu->insertSeparator();
            myMenu->insertItem( i18n( "Disable This Popup" ), DISABLE_POPUP );
        }
        myMenu->insertSeparator();
        // add an edit-possibility
        myMenu->insertItem( SmallIcon("edit"), i18n("&Edit Contents..."),
                            URL_EDIT_ITEM );
        myMenu->insertItem( SmallIconSet("cancel"), i18n("&Cancel"), DO_NOTHING_ITEM );

        if ( myPopupKillTimeout > 0 )
            myPopupKillTimer->start( 1000 * myPopupKillTimeout, true );

        emit sigPopup( myMenu );
    }
}


void URLGrabber::slotItemSelected( int id )
{
    myMenu->hide(); // deleted by the timer or the next action

    switch ( id ) {
    case -1:
    case DO_NOTHING_ITEM:
        break;
    case URL_EDIT_ITEM:
        editData();
        break;
    case DISABLE_POPUP:
	emit sigDisablePopup();
	break;
    default:
        ClipCommand *command = myCommandMapper.find( id );
        TQStringList *backrefs = myGroupingMapper.find( id );
        if ( !command || !backrefs )
            tqWarning("Klipper: can't find associated action");
        else
            execute( command, backrefs );
    }
}


void URLGrabber::execute( const struct ClipCommand *command,
                          TQStringList *backrefs) const
{
    if ( command->isEnabled ) {
        TQMap<TQChar,TQString> map;
        map.insert( 's', myClipData );
		int brCounter = -1;
		TQStringList::Iterator it = backrefs->begin();
		while( it != backrefs->end() ) {
			map.insert( char(++brCounter + '0') , *it );
			++it;
		}
        TQString cmdLine = KMacroExpander::expandMacrosShellQuote( command->command, map );

        if ( cmdLine.isEmpty() )
            return;

        TDEProcess proc;
        const char *shell = getenv("KLIPPER_SHELL");
        if (shell==NULL) shell = getenv("SHELL");
        proc.setUseShell(true,shell);

        proc << cmdLine.stripWhiteSpace();

        if ( !proc.start(TDEProcess::DontCare, TDEProcess::NoCommunication ))
            tqWarning("Klipper: Couldn't start process!");
    }
}


void URLGrabber::editData()
{
    myPopupKillTimer->stop();
    KDialogBase *dlg = new KDialogBase( 0, 0, true,
                                        i18n("Edit Contents"),
                                        KDialogBase::Ok | KDialogBase::Cancel);
    KTextEdit *edit = new KTextEdit( dlg );
    edit->setText( myClipData );
    edit->setFocus();
    edit->setMinimumSize( 300, 40 );
    dlg->setMainWidget( edit );
    dlg->adjustSize();

    if ( dlg->exec() == TQDialog::Accepted ) {
        myClipData = edit->text();
        delete dlg;
        TQTimer::singleShot( 0, this, TQ_SLOT( slotActionMenu() ) );
    }
    else
    {
        delete dlg;
        myMenu->deleteLater();
        myMenu = 0L;
    }
}


void URLGrabber::readConfiguration( TDEConfig *kc )
{
    myActions->clear();
    kc->setGroup( "General" );
    int num = kc->readNumEntry("Number of Actions", 0);
    myAvoidWindows = kc->readListEntry("No Actions for WM_CLASS");
    myPopupKillTimeout = kc->readNumEntry( "Timeout for Action popups (seconds)", 8 );
    m_stripWhiteSpace = kc->readBoolEntry("Strip Whitespace before exec", true);
    TQString group;
    for ( int i = 0; i < num; i++ ) {
        group = TQString("Action_%1").arg( i );
        kc->setGroup( group );
        myActions->append( new ClipAction( kc ) );
    }
}


void URLGrabber::writeConfiguration( TDEConfig *kc )
{
    kc->setGroup( "General" );
    kc->writeEntry( "Number of Actions", myActions->count() );
    kc->writeEntry( "Timeout for Action popups (seconds)", myPopupKillTimeout);
    kc->writeEntry( "No Actions for WM_CLASS", myAvoidWindows );
    kc->writeEntry( "Strip Whitespace before exec", m_stripWhiteSpace );

    ActionListIterator it( *myActions );
    ClipAction *action;

    int i = 0;
    TQString group;
    while ( (action = it.current()) ) {
        group = TQString("Action_%1").arg( i );
        kc->setGroup( group );
        action->save( kc );
        ++i;
        ++it;
    }
}

// find out whether the active window's WM_CLASS is in our avoid-list
// digged a little bit in netwm.cpp
bool URLGrabber::isAvoidedWindow() const
{
    Display *d = tqt_xdisplay();
    static Atom wm_class = XInternAtom( d, "WM_CLASS", true );
    static Atom active_window = XInternAtom( d, "_NET_ACTIVE_WINDOW", true );
    Atom type_ret;
    int format_ret;
    unsigned long nitems_ret, unused;
    unsigned char *data_ret;
    long BUFSIZE = 2048;
    bool ret = false;
    Window active = 0L;
    TQString wmClass;

    // get the active window
    if (XGetWindowProperty(d, DefaultRootWindow( d ), active_window, 0l, 1l,
                           False, XA_WINDOW, &type_ret, &format_ret,
                           &nitems_ret, &unused, &data_ret)
        == Success) {
        if (type_ret == XA_WINDOW && format_ret == 32 && nitems_ret == 1) {
            active = *((Window *) data_ret);
        }
        XFree(data_ret);
    }
    if ( !active )
        return false;

    // get the class of the active window
    if ( XGetWindowProperty(d, active, wm_class, 0L, BUFSIZE, False, XA_STRING,
                            &type_ret, &format_ret, &nitems_ret,
                            &unused, &data_ret ) == Success) {
        if ( type_ret == XA_STRING && format_ret == 8 && nitems_ret > 0 ) {
            wmClass = TQString::fromUtf8( (const char *) data_ret );
            ret = (myAvoidWindows.find( wmClass ) != myAvoidWindows.end());
        }

        XFree( data_ret );
    }

    return ret;
}


void URLGrabber::slotKillPopupMenu()
{
    if ( myMenu && myMenu->isVisible() )
    {
        if ( myMenu->geometry().contains( TQCursor::pos() ) &&
             myPopupKillTimeout > 0 )
        {
            myPopupKillTimer->start( 1000 * myPopupKillTimeout, true );
            return;
        }
    }

    delete myMenu;
    myMenu = 0L;
}

///////////////////////////////////////////////////////////////////////////
////////

ClipCommand::ClipCommand(const TQString &_command, const TQString &_description,
                         bool _isEnabled, const TQString &_icon)
    : command(_command),
      description(_description),
      isEnabled(_isEnabled)
{
    int len = command.find(" ");
    if (len == -1)
        len = command.length();

    if (!_icon.isEmpty())
        pixmap = _icon;
    else
    {
    KService::Ptr service= KService::serviceByDesktopName(command.left(len));
    if (service)
        pixmap = service->icon();
    else
        pixmap = TQString::null;
    }
}


ClipAction::ClipAction( const TQString& regExp, const TQString& description )
    : myRegExp( regExp ), myDescription( description )
{
    myCommands.setAutoDelete( true );
}


ClipAction::ClipAction( const ClipAction& action )
{
    myCommands.setAutoDelete( true );
    myRegExp      = action.myRegExp;
    myDescription = action.myDescription;

    ClipCommand *command = 0L;
    TQPtrListIterator<ClipCommand> it( myCommands );
    for ( ; it.current(); ++it ) {
        command = it.current();
        addCommand(command->command, command->description, command->isEnabled);
    }
}


ClipAction::ClipAction( TDEConfig *kc )
    : myRegExp( kc->readEntry( "Regexp" ) ),
      myDescription( kc->readEntry( "Description" ) )
{
    myCommands.setAutoDelete( true );
    int num = kc->readNumEntry( "Number of commands" );

    // read the commands
    TQString actionGroup = kc->group();
    for ( int i = 0; i < num; i++ ) {
        TQString group = actionGroup + "/Command_%1";
        kc->setGroup( group.arg( i ) );

        addCommand( kc->readPathEntry( "Commandline" ),
                    kc->readEntry( "Description" ), // i18n'ed
                    kc->readBoolEntry( "Enabled" ),
                    kc->readEntry( "Icon") );
    }
}


ClipAction::~ClipAction()
{
}


void ClipAction::addCommand( const TQString& command,
                             const TQString& description, bool enabled, const TQString& icon )
{
    if ( command.isEmpty() )
        return;

    struct ClipCommand *cmd = new ClipCommand( command, description, enabled, icon );
    //    cmd->id = myCommands.count(); // superfluous, I think...
    myCommands.append( cmd );
}


// precondition: we're in the correct action's group of the TDEConfig object
void ClipAction::save( TDEConfig *kc ) const
{
    kc->writeEntry( "Description", description() );
    kc->writeEntry( "Regexp", regExp() );
    kc->writeEntry( "Number of commands", myCommands.count() );

    TQString actionGroup = kc->group();
    struct ClipCommand *cmd;
    TQPtrListIterator<struct ClipCommand> it( myCommands );

    // now iterate over all commands of this action
    int i = 0;
    while ( (cmd = it.current()) ) {
        TQString group = actionGroup + "/Command_%1";
        kc->setGroup( group.arg( i ) );

        kc->writePathEntry( "Commandline", cmd->command );
        kc->writeEntry( "Description", cmd->description );
        kc->writeEntry( "Enabled", cmd->isEnabled );

        ++i;
        ++it;
    }
}

#include "urlgrabber.moc"
