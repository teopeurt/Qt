/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "tracer.h"

#include <QtCore/QDir>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDateTime>
#include <QtCore/QFileSystemWatcher>
#include <QtHelp/QHelpEngineCore>
#include "helpenginewrapper.h"
#include "qtdocinstaller.h"

QT_BEGIN_NAMESPACE

QtDocInstaller::QtDocInstaller(const QList<DocInfo> &docInfos)
    : m_abort(false), m_docInfos(docInfos)
{
    TRACE_OBJ
}

QtDocInstaller::~QtDocInstaller()
{
    TRACE_OBJ
    if (!isRunning())
        return;
    m_mutex.lock();
    m_abort = true;
    m_mutex.unlock();
    wait();
}

void QtDocInstaller::installDocs()
{
    TRACE_OBJ
    start(LowPriority);
}

void QtDocInstaller::run()
{
    TRACE_OBJ
    m_qchDir = QLibraryInfo::location(QLibraryInfo::DocumentationPath)
        + QDir::separator() + QLatin1String("qch");
    m_qchFiles = m_qchDir.entryList(QStringList() << QLatin1String("*.qch"));

    bool changes = false;
    foreach (const DocInfo &docInfo, m_docInfos) {
        changes |= installDoc(docInfo);
        m_mutex.lock();
        if (m_abort) {
            m_mutex.unlock();
            return;
        }
        m_mutex.unlock();
    }
    emit docsInstalled(changes);
}

bool QtDocInstaller::installDoc(const DocInfo &docInfo)
{
    TRACE_OBJ
    const QString &component = docInfo.first;
    const QStringList &info = docInfo.second;
    QDateTime dt;
    if (!info.isEmpty() && !info.first().isEmpty())
        dt = QDateTime::fromString(info.first(), Qt::ISODate);

    QString qchFile;
    if (info.count() == 2)
        qchFile = info.last();

    if (m_qchFiles.isEmpty()) {
        emit qchFileNotFound(component);
        return false;
    }
    foreach (const QString &f, m_qchFiles) {
        if (f.startsWith(component)) {
            QFileInfo fi(m_qchDir.absolutePath() + QDir::separator() + f);
            if (dt.isValid() && fi.lastModified().toTime_t() == dt.toTime_t()
                && qchFile == fi.absoluteFilePath())
                return false;
            emit registerDocumentation(component, fi.absoluteFilePath());
            return true;
        }
    }

    emit qchFileNotFound(component);
    return false;
}

QT_END_NAMESPACE
