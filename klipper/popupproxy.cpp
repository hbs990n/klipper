/* This file is part of the KDE project
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
#include <tqregexp.h>
#include <tqstyle.h>
#include <tqpixmap.h>
#include <tqimage.h>

#include <kstringhandler.h>
#include <tdelocale.h>
#include <kdebug.h>

#include "historyitem.h"
#include "popupproxy.h"
#include "history.h"
#include "klipperpopup.h"


PopupProxy::PopupProxy( KlipperPopup* parent, const char* name, int menu_height, int menu_width )
    : TQObject( parent, name ),
      proxy_for_menu( parent ),
      spillPointer( parent->history()->youngest() ),
      m_menu_height( menu_height ),
      m_menu_width( menu_width ),
      nextItemNumber( 0 )
{
    connect( parent->history(), TQ_SIGNAL( changed() ), TQ_SLOT( slotHistoryChanged() ) );
}

void PopupProxy::slotHistoryChanged() {
    deleteMoreMenus();

}

void PopupProxy::deleteMoreMenus() {
    const TDEPopupMenu* myParent = parent();
    if ( myParent != proxy_for_menu ) {
        const TDEPopupMenu* delme = proxy_for_menu;;
        proxy_for_menu = static_cast<TDEPopupMenu*>( proxy_for_menu->parent() );
        while ( proxy_for_menu != myParent ) {
            delme = proxy_for_menu;
            proxy_for_menu = static_cast<TDEPopupMenu*>( proxy_for_menu->parent() );
        }
        delete delme;
    }
}

int PopupProxy::buildParent( int index, const TQRegExp& filter ) {
    deleteMoreMenus();
    // Start from top of  history (again)
    spillPointer = parent()->history()->youngest();
    nextItemNumber = 0;
    if ( filter.isValid() ) {
        m_filter = filter;
    }

    return insertFromSpill( index );

}

KlipperPopup* PopupProxy::parent() {
    return static_cast<KlipperPopup*>( TQObject::parent() );
}

void PopupProxy::slotAboutToShow() {
    insertFromSpill();
}

void PopupProxy::tryInsertItem( HistoryItem const * const item,
                                int& remainingHeight,
                                const int index )
{

    // Insert item
    int id = -1;
    TQPixmap image( item->image() );
    if ( image.isNull() ) {
        // Squeeze text strings so that do not take up the entire screen (or more)
        TQString text( KStringHandler::cPixelSqueeze(item->text().simplifyWhiteSpace(),
                                                    proxy_for_menu->fontMetrics(),
                                                    m_menu_width).replace( "&", "&&" ) );
        id = proxy_for_menu->insertItem( text, -1, index );
    } else {
        const TQSize max_size( m_menu_width,m_menu_height/4 );
        if ( image.height() > max_size.height() || image.width() > max_size.width() ) {
            image.convertFromImage( TQImage(image.convertToImage()).smoothScale( max_size, TQImage::ScaleMin ) );
        }
        id = proxy_for_menu->insertItem( image,  -1,  index );
    }


    // Determine height of a menu item.
    Q_ASSERT( id != -1 ); // Be sure that the item was inserted.
    TQMenuItem* mi = proxy_for_menu->findItem( id );
    int fontheight = TQFontMetrics( proxy_for_menu->fontMetrics()  ).height();
    int itemheight = proxy_for_menu->style().sizeFromContents(TQStyle::CT_PopupMenuItem,
                                                              proxy_for_menu,
                                                              TQSize( 0, fontheight ),
                                                              TQStyleOption(mi,10,0) ).height();
    // Test if there was enough space
    remainingHeight -= itemheight;
    History* history = parent()->history();
    proxy_for_menu->connectItem(  id,
                                  history,
                                  TQ_SLOT( slotMoveToTop( int ) ) );
    proxy_for_menu->setItemParameter(  id,  nextItemNumber );

}

int PopupProxy::insertFromSpill( int index ) {

    // This menu is going to be filled, so we don't need the aboutToShow()
    // signal anymore
    disconnect( proxy_for_menu, 0, this, 0 );

    // Insert history items into the current proxy_for_menu,
    // discarding any that doesn't match the current filter.
    // stop when the total number of items equal m_itemsPerMenu;
    int count = 0;
    int remainingHeight = m_menu_height - proxy_for_menu->sizeHint().height();
    // Force at least one item to be inserted.
    remainingHeight = TQMAX( remainingHeight, 0 );
    for ( const HistoryItem* item = spillPointer.current();
          item && remainingHeight >= 0;
          nextItemNumber++, item = ++spillPointer )
    {
        if ( m_filter.search( item->text() ) == -1) {
            continue;
        }
        tryInsertItem( item, remainingHeight, index++ );
        count++;
    }

    // If there is more items in the history, insert a new "More..." menu and
    // make *this a proxy for that menu ('s content).
    if ( spillPointer.current() ) {
        TDEPopupMenu* moreMenu = new TDEPopupMenu( proxy_for_menu, "a more menu" );
        proxy_for_menu->insertItem( i18n( "&More" ),  moreMenu, -1, index );
        connect( moreMenu, TQ_SIGNAL( aboutToShow() ), TQ_SLOT( slotAboutToShow() ) );
        proxy_for_menu = moreMenu;
    }

    // Return the number of items inserted.
    return count;

}
#include "popupproxy.moc"
