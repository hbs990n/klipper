/* This file is part of the KDE project
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
#ifndef URLGRABBER_H
#define URLGRABBER_H

#include <tqptrlist.h>
#include <tqintdict.h>
#include <tqregexp.h>
#include <tqstring.h>
#include <tqstringlist.h>

#include <tdeprocess.h>


class TQTimer;

class TDEConfig;
class TDEPopupMenu;

class ClipAction;
struct ClipCommand;
typedef TQPtrList<ClipAction> ActionList;
typedef TQPtrListIterator<ClipAction> ActionListIterator;

class URLGrabber : public TQObject
{
  TQ_OBJECT

public:
  URLGrabber( TDEConfig* config );
  ~URLGrabber();

  /**
   * Checks a given string whether it matches any of the user-defined criteria.
   * If it does, the configured action will be executed.
   * @returns false if the string should be put into the popupmenu or not,
   * otherwise true.
   */
  bool checkNewData( const TQString& clipData );
  void invokeAction( const TQString& clip = TQString::null );

  const ActionList * actionList() const { return myActions; }
  void setActionList( ActionList * );
  void readConfiguration( TDEConfig * );
  void writeConfiguration( TDEConfig * );

  int popupTimeout() const { return myPopupKillTimeout; }
  void setPopupTimeout( int timeout ) { myPopupKillTimeout = timeout; }

  const TQStringList& avoidWindows() const { return myAvoidWindows; }
  void setAvoidWindows( const TQStringList& list ) { myAvoidWindows = list; }

  bool stripWhiteSpace() const { return m_stripWhiteSpace; }
  void setStripWhiteSpace( bool enable ) { m_stripWhiteSpace = enable; }
    
private:
  const ActionList& matchingActions( const TQString& );
  void execute( const struct ClipCommand *command, 
                TQStringList *backrefs ) const;
  void editData();
  bool isAvoidedWindow() const;
  void actionMenu( bool wm_class_check );

  ActionList *myActions;
  ActionList myMatches;
  TQStringList myAvoidWindows;
  TQString myClipData;
  ClipAction *myCurrentAction;
  TQIntDict<ClipCommand> myCommandMapper;
  TQIntDict<TQStringList> myGroupingMapper;
  TDEPopupMenu *myMenu;
  TQTimer *myPopupKillTimer;
  int myPopupKillTimeout;
  bool m_stripWhiteSpace;
  TDEConfig* m_config;

private slots:
  void slotActionMenu() { actionMenu( true ); }
  void slotItemSelected( int );
  void slotKillPopupMenu();


signals:
    void sigPopup( TQPopupMenu * );
    void sigDisablePopup();

};


struct ClipCommand
{
    ClipCommand( const TQString &, const TQString &, bool = true, const TQString & = "" );
    TQString command;
    TQString description;
    bool isEnabled;
    TQString pixmap;
    //  int id; // the index reflecting the position in the list of commands
};

/**
 * Represents one configured action. An action consists of one regular
 * expression, an (optional) description and a list of ClipCommands
 * (a command to be executed, a description and an enabled/disabled flag).
 */
class ClipAction
{
public:
  ClipAction( const TQString& regExp, const TQString& description );
  ClipAction( const ClipAction& );
  ClipAction( TDEConfig *kc );
  ~ClipAction();

  void  setRegExp( const TQString& r) 	      { myRegExp = TQRegExp( r ); }
  TQString regExp() 			const { return myRegExp.pattern(); }
  inline bool matches( const TQString& string ) {
    int res = myRegExp.search( string ) ;
    if ( res != -1 ) {
      myCapturedTexts = myRegExp.capturedTexts();
      return true;
    } 
    return false;
  }

  void 	setDescription( const TQString& d)     { myDescription = d; }
  const TQString& description() 		const { return myDescription; }

  /**
   * Removes all ClipCommands associated with this ClipAction.
   */
  void clearCommands() { myCommands.clear(); }

  void  addCommand( const TQString& command, const TQString& description, bool, const TQString& icon = "" );
  const TQPtrList<ClipCommand>& commands() 	const { return myCommands; }

  /**
   * Saves this action to a a given TDEConfig object
   */
  void save( TDEConfig * ) const;

  /**
   * Returns the most recent list of matched group backreferences.
   * Note: you probably need to call matches() first.
   */
  inline const TQStringList* capturedTexts() const { return &myCapturedTexts; }

private:
  TQRegExp 		myRegExp;
  TQStringList	myCapturedTexts;
  TQString 		myDescription;
  TQPtrList<ClipCommand> 	myCommands;

};


#endif // URLGRABBER_H
