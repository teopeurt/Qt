/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QBYTEDATA_H
#define QBYTEDATA_H

#include <qbytearray.h>


// this class handles a list of QByteArrays. It is a variant of QRingBuffer
// that avoid malloc/realloc/memcpy.
class QByteDataBuffer
{
private:
    QList<QByteArray> buffers;
    qint64 bufferCompleteSize;
public:
    QByteDataBuffer() : bufferCompleteSize(0)
    {
    }

    ~QByteDataBuffer()
    {
        clear();
    }

    inline void append(QByteDataBuffer& other)
    {
        if (other.isEmpty())
            return;

        buffers.append(other.buffers);
        bufferCompleteSize += other.byteAmount();
    }


    inline void append(QByteArray& bd)
    {
        if (bd.isEmpty())
            return;

        buffers.append(bd);
        bufferCompleteSize += bd.size();
    }

    inline void prepend(QByteArray& bd)
    {
        if (bd.isEmpty())
            return;

        buffers.prepend(bd);
        bufferCompleteSize += bd.size();
    }

    // return the first QByteData. User of this function has to qFree() its .data!
    // preferably use this function to read data.
    inline QByteArray read()
    {
        bufferCompleteSize -= buffers.first().size();
        return buffers.takeFirst();
    }

    // return everything. User of this function has to qFree() its .data!
    // avoid to use this, it might malloc and memcpy.
    inline QByteArray readAll()
    {
        return read(byteAmount());
    }

    // return amount. User of this function has to qFree() its .data!
    // avoid to use this, it might malloc and memcpy.
    inline QByteArray read(qint64 amount)
    {
        amount = qMin(byteAmount(), amount);
        QByteArray byteData;
        byteData.resize(amount);
        read(byteData.data(), byteData.size());
        return byteData;
    }

    // return amount bytes. User of this function has to qFree() its .data!
    // avoid to use this, it will memcpy.
    qint64 read(char* dst, qint64 amount)
    {
        amount = qMin(amount, byteAmount());
        qint64 originalAmount = amount;
        char *writeDst = dst;

        while (amount > 0) {
            QByteArray first = buffers.takeFirst();
            if (amount >= first.size()) {
                // take it completely
                bufferCompleteSize -= first.size();
                amount -= first.size();
                memcpy(writeDst, first.constData(), first.size());
                writeDst += first.size();
                first.clear();
            } else {
                // take a part of it & it is the last one to take
                bufferCompleteSize -= amount;
                memcpy(writeDst, first.constData(), amount);

                qint64 newFirstSize = first.size() - amount;
                QByteArray newFirstData;
                newFirstData.resize(newFirstSize);
                memcpy(newFirstData.data(), first.constData() + amount, newFirstSize);
                buffers.prepend(newFirstData);

                amount = 0;
                first.clear();
            }
        }

        return originalAmount;
    }

    inline char getChar()
    {
        char c;
        read(&c, 1);
        return c;
    }

    inline void clear()
    {
        buffers.clear();
        bufferCompleteSize = 0;
    }

    // The byte count of all QByteArrays
    inline qint64 byteAmount() const
    {
        return bufferCompleteSize;
    }

    // the number of QByteArrays
    inline qint64 bufferCount() const
    {
        return buffers.length();
    }

    inline bool isEmpty() const
    {
        return byteAmount() == 0;
    }

    inline qint64 sizeNextBlock() const
    {
        if(buffers.isEmpty())
            return 0;
        else
            return buffers.first().size();
    }

    inline QByteArray& operator[](int i)
    {
        return buffers[i];
    }
};


#endif // QBYTEDATA_H