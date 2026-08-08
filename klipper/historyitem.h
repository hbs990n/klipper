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
#ifndef _HISTORYITEM_H_
#define _HISTORYITEM_H_
#include <tqpixmap.h>

class TQString;
class TQMimeSource;
class TQDataStream;

/**
 * An entry in the clipboard history.
 */
class HistoryItem
{
public:
    HistoryItem( );
    virtual ~HistoryItem();

    /**
     * Return the current item as text
     * An image would be returned as a descriptive
     * text, such as 32x43 image.
     */
    virtual TQString text() const = 0;

    /**
     * Return the current item as text
     * A text would be returned as a null pixmap,
     * which is also the default implementation
     */
    inline virtual const TQPixmap& image() const;

    /**
     * Returns TQMimeSource suitable for TQClipboard::setData().
     */
    virtual TQMimeSource* mimeSource() const = 0;

    /**
     * Write object on datastream
     */
    virtual void write( TQDataStream& stream ) const = 0;

    /**
     * Equality.
     */
    virtual bool operator==(const HistoryItem& rhs) const = 0;

    /**
     * Create an HistoryItem from MimeSources (i.e., clipboard data)
     * returns null if create fails (e.g, unsupported mimetype)
     */
    static HistoryItem* create( const TQMimeSource& aSource );

    /**
     * Create an HistoryItem from MimeSources (i.e., clipboard data)
     * returns null if creation fails. In this case, the datastream
     * is left in an undefined state.
     */
    static HistoryItem* create( TQDataStream& aSource );
};

inline
const TQPixmap& HistoryItem::image() const {
    static TQPixmap nullPixmap;
    return nullPixmap;
}

inline
TQDataStream& operator<<( TQDataStream& lhs, HistoryItem const * const rhs ) {
    if ( rhs ) {
        rhs->write( lhs );
    }
    return lhs;

}

#endif
