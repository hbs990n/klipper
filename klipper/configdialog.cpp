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
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqpushbutton.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>
#include <tqvbuttongroup.h>
#include <assert.h>

#include <kiconloader.h>
#include <tdelocale.h>
#include <tdepopupmenu.h>
#include <twinmodule.h>
#include <kregexpeditorinterface.h>
#include <tdeparts/componentfactory.h>

#include "configdialog.h"

ConfigDialog::ConfigDialog( const ActionList *list, TDEGlobalAccel *accel,
                            bool isApplet )
    : KDialogBase( Tabbed, i18n("Configure"),
                    Ok | Cancel | Help,
                    Ok, 0L, "config dialog" )
{
    if ( isApplet )
        setHelp( TQString::null, "klipper" );

    TQFrame *w = 0L; // the parent for the widgets

    w = addVBoxPage( i18n("&General") );
    generalWidget = new GeneralWidget( w, "general widget" );

    w = addVBoxPage( i18n("Ac&tions") );
    actionWidget = new ActionWidget( list, this, w, "actions widget" );

    w = addVBoxPage( i18n("Global &Shortcuts") );
    keysWidget = new KKeyChooser( accel, w );
}


ConfigDialog::~ConfigDialog()
{
}

// prevent huge size due to long regexps in the action-widget
void ConfigDialog::show()
{
    if ( !isVisible() ) {
	TWinModule module(0, TWinModule::INFO_DESKTOP);
	TQSize s1 = sizeHint();
	TQSize s2 = module.workArea().size();
	int w = s1.width();
	int h = s1.height();

	if ( s1.width() >= s2.width() )
	    w = s2.width();
	if ( s1.height() >= s2.height() )
	    h = s2.height();

 	resize( w, h );
    }

    KDialogBase::show();
}

void ConfigDialog::commitShortcuts()
{
    keysWidget->commitChanges();
}

/////////////////////////////////////////
////


GeneralWidget::GeneralWidget( TQWidget *parent, const char *name )
    : TQVBox( parent, name )
{
    setSpacing(KDialog::spacingHint());

    cbMousePos = new TQCheckBox( i18n("&Popup menu at mouse-cursor position"),
                                this );
    cbSaveContents = new TQCheckBox( i18n("Save clipboard contents on e&xit"),
                                    this );
    cbStripWhitespace = new TQCheckBox( i18n("Remove whitespace when executing actions"), this );
    TQWhatsThis::add( cbStripWhitespace,
                     i18n("Sometimes, the selected text has some whitespace at the end, which, if loaded as URL in a browser would cause an error. Enabling this option removes any whitespace at the beginning or end of the selected string (the original clipboard contents will not be modified).") );

    cbReplayAIH = new TQCheckBox( i18n("&Replay actions on an item selected from history"),
                                    this );

    cbNoNull = new TQCheckBox( i18n("Pre&vent empty clipboard"), this );
    TQWhatsThis::add( cbNoNull,
                     i18n("Selecting this option has the effect, that the "
                          "clipboard can never be emptied. E.g. when an "
                          "application exits, the clipboard would usually be "
                          "emptied.") );

    cbIgnoreSelection = new TQCheckBox( i18n("&Ignore selection"), this );
    TQWhatsThis::add( cbIgnoreSelection,
                     i18n("This option prevents the selection being recorded "
                          "in the clipboard history. Only explicit clipboard "
                          "changes are recorded.") );

    TQVButtonGroup *group = new TQVButtonGroup( i18n("Clipboard/Selection Behavior"), this );
    group->setExclusive( true );

    TQWhatsThis::add( group,
     i18n("<qt>There are two different clipboard buffers available:<br><br>"
          "<b>Clipboard</b> is filled by selecting something "
          "and pressing Ctrl+C, or by clicking \"Copy\" in a toolbar or "
          "menubar.<br><br>"
          "<b>Selection</b> is available immediately after "
          "selecting some text. The only way to access the selection "
          "is to press the middle mouse button.<br><br>"
          "You can configure the relationship between Clipboard and Selection."
          "</qt>" ));

    cbSynchronize = new TQRadioButton(
        i18n("Sy&nchronize contents of the clipboard and the selection"),
        group );
    TQWhatsThis::add( cbSynchronize,
      i18n("Selecting this option synchronizes these two buffers.") );

    cbSeparate = new TQRadioButton(
        i18n("Separate clipboard and selection"), group );
    TQWhatsThis::add(
        cbSeparate,
        i18n("Using this option will only set the selection when highlighting "
             "something and the clipboard when choosing e.g. \"Copy\" "
             "in a menubar.") );

    cbSeparate->setChecked( !cbSynchronize->isChecked() );

    popupTimeout = new KIntNumInput( this );
    popupTimeout->setLabel( i18n( "Tim&eout for action popups:" ) );
    popupTimeout->setRange( 0, 200, 1, true );
    popupTimeout->setSuffix( i18n( " sec" ) );
    TQToolTip::add( popupTimeout, i18n("A value of 0 disables the timeout") );

    maxItems = new KIntNumInput( this );
    maxItems->setLabel(  i18n( "C&lipboard history size:" ) );
    maxItems->setRange( 2, 2048, 1, true );
    connect( maxItems, TQ_SIGNAL( valueChanged( int )),
             TQ_SLOT( historySizeChanged( int ) ));

    connect( group, TQ_SIGNAL( clicked( int )),
             TQ_SLOT( slotClipConfigChanged() ));
    slotClipConfigChanged();

    // Add some spacing at the end
    TQWidget *dummy = new TQWidget( this );
    setStretchFactor( dummy, 1 );
}

GeneralWidget::~GeneralWidget()
{
}

void GeneralWidget::historySizeChanged( int value )
{
    // Note there is no %n in this string, because value is not supposed
    // to be put into the suffix of the spinbox.
    maxItems->setSuffix( i18n( " entry", " entries", value ) );
}

void GeneralWidget::slotClipConfigChanged()
{
    cbIgnoreSelection->setEnabled( !cbSynchronize->isChecked() );
}

/////////////////////////////////////////
////

void ListView::rename( TQListViewItem* item, int c )
{
  bool gui = false;
  if ( item->childCount() != 0 && c == 0) {
    // This is the regular expression
    if ( _configWidget->useGUIRegExpEditor() ) {
      gui = true;
    }
  }

  if ( gui ) {
    if ( ! _regExpEditor )
      _regExpEditor = KParts::ComponentFactory::createInstanceFromQuery<TQDialog>( "KRegExpEditor/KRegExpEditor", TQString(), this );
    KRegExpEditorInterface *iface = static_cast<KRegExpEditorInterface *>( _regExpEditor->tqt_cast( "KRegExpEditorInterface" ) );
    assert( iface );
    iface->setRegExp( item->text( 0 ) );

    bool ok = _regExpEditor->exec();
    if ( ok )
      item->setText( 0, iface->regExp() );

  }
  else
    TDEListView::rename( item ,c );
}


ActionWidget::ActionWidget( const ActionList *list, ConfigDialog* configWidget, TQWidget *parent,
                            const char *name )
    : TQVBox( parent, name ),
      advancedWidget( 0L )
{
    Q_ASSERT( list != 0L );

    TQLabel *lblAction = new TQLabel(
	  i18n("Action &list (right click to add/remove commands):"), this );

    listView = new ListView( configWidget, this, "list view" );
    lblAction->setBuddy( listView );
    listView->addColumn( i18n("Regular Expression (see https://trinitydesktop.org/docs/qt3/qregexp.html#details)") );
    listView->addColumn( i18n("Description") );

    listView->setRenameable(0);
    listView->setRenameable(1);
    listView->setItemsRenameable( true );
    listView->setItemsMovable( false );
//     listView->setAcceptDrops( true );
//     listView->setDropVisualizer( true );
//     listView->setDragEnabled( true );

    listView->setRootIsDecorated( true );
    listView->setMultiSelection( false );
    listView->setAllColumnsShowFocus( true );
    listView->setSelectionMode( TQListView::Single );
    connect( listView, TQ_SIGNAL(executed( TQListViewItem*, const TQPoint&, int )),
             TQ_SLOT( slotItemChanged( TQListViewItem*, const TQPoint& , int ) ));
    connect( listView, TQ_SIGNAL( selectionChanged ( TQListViewItem * )),
             TQ_SLOT(selectionChanged ( TQListViewItem * )));
    connect(listView,
            TQ_SIGNAL(contextMenu(TDEListView *, TQListViewItem *, const TQPoint&)),
            TQ_SLOT( slotContextMenu(TDEListView*, TQListViewItem*, const TQPoint&)));

    ClipAction *action   = 0L;
    ClipCommand *command = 0L;
    TQListViewItem *item  = 0L;
    TQListViewItem *child = 0L;
    TQListViewItem *after = 0L; // QListView's default inserting really sucks
    ActionListIterator it( *list );

    const TQPixmap& doc = SmallIcon( "misc" );
    const TQPixmap& exec = SmallIcon( "application-x-executable" );

    for ( action = it.current(); action; action = ++it ) {
        item = new TQListViewItem( listView, after,
                                  action->regExp(), action->description() );
        item->setPixmap( 0, doc );

        TQPtrListIterator<ClipCommand> it2( action->commands() );
        for ( command = it2.current(); command; command = ++it2 ) {
            child = new TQListViewItem( item, after,
                                       command->command, command->description);
        if ( command->pixmap.isEmpty() )
            child->setPixmap( 0, exec );
        else
            child->setPixmap( 0, SmallIcon( command->pixmap ) );
            after = child;
        }
        after = item;
    }

    listView->setSorting( -1 ); // newly inserted items just append unsorted

    cbUseGUIRegExpEditor = new TQCheckBox( i18n("&Use graphical editor for editing regular expressions" ), this );
    if ( TDETrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty() )
    {
	cbUseGUIRegExpEditor->hide();
	cbUseGUIRegExpEditor->setChecked( false );
    }

    TQHBox *box = new TQHBox( this );
    box->setSpacing( KDialog::spacingHint() );
    TQPushButton *button = new TQPushButton( i18n("&Add Action"), box );
    connect( button, TQ_SIGNAL( clicked() ), TQ_SLOT( slotAddAction() ));

    delActionButton = new TQPushButton( i18n("&Delete Action"), box );
    connect( delActionButton, TQ_SIGNAL( clicked() ), TQ_SLOT( slotDeleteAction() ));

    TQLabel *label = new TQLabel(i18n("Click on a highlighted item's column to change it. \"%s\" in a command will be replaced with the clipboard contents."), box);
    label->setAlignment( WordBreak | AlignLeft | AlignVCenter );

    box->setStretchFactor( label, 5 );

    box = new TQHBox( this );
    TQPushButton *advanced = new TQPushButton( i18n("Advanced..."), box );
    advanced->setFixedSize( advanced->sizeHint() );
    connect( advanced, TQ_SIGNAL( clicked() ), TQ_SLOT( slotAdvanced() ));
    (void) new TQWidget( box ); // spacer

    delActionButton->setEnabled(listView->currentItem () !=0);
}

ActionWidget::~ActionWidget()
{
}

void ActionWidget::selectionChanged ( TQListViewItem * item)
{
    delActionButton->setEnabled(item!=0);
}

void ActionWidget::slotContextMenu( TDEListView *, TQListViewItem *item,
                                    const TQPoint& pos )
{
    if ( !item )
        return;

    int addCmd = 0, rmCmd = 0;
    TDEPopupMenu *menu = new TDEPopupMenu;
    addCmd = menu->insertItem( i18n("Add Command") );
    rmCmd = menu->insertItem( i18n("Remove Command") );
    if ( !item->parent() ) {// no "command" item
        menu->setItemEnabled( rmCmd, false );
        item->setOpen( true );
    }

    int id = menu->exec( pos );
    if ( id == addCmd ) {
        TQListViewItem *p = item->parent() ? item->parent() : item;
        TQListViewItem *cmdItem = new TQListViewItem( p, item,
                         i18n("Click here to set the command to be executed"),
                         i18n("<new command>") );
        cmdItem->setPixmap( 0, SmallIcon( "application-x-executable" ) );
    }
    else if ( id == rmCmd )
        delete item;

    delete menu;
}

void ActionWidget::slotItemChanged( TQListViewItem *item, const TQPoint&, int col )
{
    if ( !item->parent() || col != 0 )
        return;
    ClipCommand command( item->text(0), item->text(1) );
        item->setPixmap( 0, SmallIcon( command.pixmap.isEmpty() ?
                                                   "application-x-executable" : command.pixmap ) );
}

void ActionWidget::slotAddAction()
{
    TQListViewItem *item = new TQListViewItem( listView );
    item->setPixmap( 0, SmallIcon( "misc" ));
    item->setText( 0, i18n("Click here to set the regexp"));
    item->setText( 1, i18n("<new action>"));
}


void ActionWidget::slotDeleteAction()
{
    TQListViewItem *item = listView->currentItem();
    if ( item && item->parent() )
        item = item->parent();
    delete item;
}


ActionList * ActionWidget::actionList()
{
    TQListViewItem *item = listView->firstChild();
    TQListViewItem *child = 0L;
    ClipAction *action = 0L;
    ActionList *list = new ActionList;
    list->setAutoDelete( true );
    while ( item ) {
        action = new ClipAction( item->text( 0 ), item->text( 1 ) );
        child = item->firstChild();

        // add the commands
        while ( child ) {
            action->addCommand( child->text( 0 ), child->text( 1 ), true );
            child = child->nextSibling();
        }

        list->append( action );
        item = item->nextSibling();
    }

    return list;
}

void ActionWidget::slotAdvanced()
{
    KDialogBase dlg( 0L, "advanced dlg", true,
                     i18n("Advanced Settings"),
                     KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok );
    TQVBox *box = dlg.makeVBoxMainWidget();
    AdvancedWidget *widget = new AdvancedWidget( box );
    widget->setWMClasses( m_wmClasses );

    dlg.resize( dlg.sizeHint().width(),
                dlg.sizeHint().height() +40); // or we get an ugly scrollbar :(

    if ( dlg.exec() == TQDialog::Accepted ) {
        m_wmClasses = widget->wmClasses();
    }
}

AdvancedWidget::AdvancedWidget( TQWidget *parent, const char *name )
    : TQVBox( parent, name )
{
    editListBox = new KEditListBox( i18n("D&isable Actions for Windows of Type WM_CLASS"), this, "editlistbox", true, KEditListBox::Add | KEditListBox::Remove );

    TQWhatsThis::add( editListBox,
          i18n("<qt>This lets you specify windows in which Klipper should "
	       "not invoke \"actions\". Use<br><br>"
	       "<center><b>xprop | grep WM_CLASS</b></center><br>"
	       "in a terminal to find out the WM_CLASS of a window. "
	       "Next, click on the window you want to examine. The "
	       "first string it outputs after the equal sign is the one "
	       "you need to enter here.</qt>"));

    editListBox->setFocus();
}

AdvancedWidget::~AdvancedWidget()
{
}

void AdvancedWidget::setWMClasses( const TQStringList& items )
{
    editListBox->clear();
    editListBox->insertStringList( items );
}



///////////////////////////////////////////////////////
//////////

/*
KeysWidget::KeysWidget( TDEAccelActions &keyMap, TQWidget *parent, const char *name)
    : TQVBox( parent, name )
{
    keyChooser = new KKeyChooser( keyMap, this );
}

KeysWidget::~KeysWidget()
{
}
*/

#include "configdialog.moc"
