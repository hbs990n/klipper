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
#include <tqmime.h>
#include <tqdragobject.h>
#include <tqmap.h>
#include <tqstring.h>
#include <tqpixmap.h>
#include <kurldrag.h>

#include <kdebug.h>

#include "historyitem.h"
#include "historystringitem.h"
#include "historyimageitem.h"
#include "historyurlitem.h"

HistoryItem::HistoryItem() {

}

HistoryItem::~HistoryItem() {

}

HistoryItem* HistoryItem::create( const TQMimeSource& aSource )
{
#if 0
    int i=0;
    while ( const char* f = aSource.format( i++ ) ) {
        kdDebug() << "format(" << i <<"): " << f << endl;
    }
#endif
    if( KURLDrag::canDecode( &aSource )) {
        KURL::List urls;
        TQMap<TQString,TQString> metaData;
        if( KURLDrag::decode( &aSource, urls, metaData )) {
            // this is from KonqDrag (libkonq)
            TQByteArray a = aSource.encodedData( "application/x-tde-cutselection" );
            bool cut = !a.isEmpty() && (a.at(0) == '1'); // true if 1
            return new HistoryURLItem( urls, metaData, cut );
        }
    }
    if ( TQTextDrag::canDecode( &aSource ) ) {
        TQString text;
        if( TQTextDrag::decode( &aSource, text ))
            return text.isNull() ? 0 : new HistoryStringItem( text );
    }
    if ( TQImageDrag::canDecode( &aSource ) ) {
        TQPixmap image;
        if( TQImageDrag::decode( &aSource, image ))
            return image.isNull() ? 0 : new HistoryImageItem( image );
    }
    return 0; // Failed.
}

HistoryItem* HistoryItem::create( TQDataStream& aSource ) {
    if ( aSource.atEnd() ) {
        return 0;
    }
    TQString type;
    aSource >> type;
    if ( type == "url" ) {
        KURL::List urls;
        TQMap< TQString, TQString > metaData;
        int cut;
        aSource >> urls;
        aSource >> metaData;
        aSource >> cut;
        return new HistoryURLItem( urls, metaData, cut );
    }
    if ( type == "string" ) {
        TQString text;
        aSource >> text;
        return new HistoryStringItem( text );
    }
    if ( type == "image" ) {
        TQPixmap image;
        aSource >> image;
        return new HistoryImageItem( image );
    }
    kdWarning() << "Failed to restore history item: Unknown type \"" << type << "\"" << endl;
    return 0;
}

