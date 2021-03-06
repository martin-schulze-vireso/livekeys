/****************************************************************************
**
** Copyright (C) 2014-2019 Dinu SV.
** (contact: mail@dinusv.com)
** This file is part of Livekeys Application.
**
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
****************************************************************************/

#include "live/projectdocumentmodel.h"
#include "live/projectdocument.h"
#include "live/projectfile.h"
#include "projectfilemodel.h"
#include "live/project.h"

#include <QFileInfo>
#include <QFileSystemWatcher>

/**
 * \class lv::ProjectDocumentModel
 * \brief It's a model containing all of the open files in our Project
 *
 *
 * \ingroup lveditor
 */
namespace lv{

/** Default constructor */
ProjectDocumentModel::ProjectDocumentModel(Project *project)
    : QAbstractListModel(project)
    , m_fileWatcher(0)
{
    m_roles[ProjectDocumentModel::Name]   = "name";
    m_roles[ProjectDocumentModel::Path]   = "path";
    m_roles[ProjectDocumentModel::IsOpen] = "isOpen";
}

/** Default destructor */
ProjectDocumentModel::~ProjectDocumentModel(){
    closeDocuments();
    if ( m_fileWatcher )
        delete m_fileWatcher;
}

/** implementation of the function inherited from QAbstractListModel */
int ProjectDocumentModel::rowCount(const QModelIndex &) const{
    return m_openedFiles.size();
}


/** implementation of the function inherited from QAbstractListModel */
QVariant ProjectDocumentModel::data(const QModelIndex &index, int role) const{
    if ( index.row() < 0 || index.row() >= m_openedFiles.size() )
        return QVariant();
    QHash<QString, ProjectDocument*>::const_iterator it = m_openedFiles.constBegin() + index.row();

    if ( role == ProjectDocumentModel::Name ){
        return it.value()->file()->name();
    } else if ( role == ProjectDocumentModel::Path ){
        return it.value()->file()->path();
    } else if ( role == ProjectDocumentModel::IsOpen ){
        return true;
    }

    return QVariant();
}

/** implementation of the function inherited from QAbstractListModel */
QHash<int, QByteArray> ProjectDocumentModel::roleNames() const{
    return m_roles;
}


/** Adds a document to the model */
void ProjectDocumentModel::openDocument(const QString &path, ProjectDocument *document){
    beginResetModel();
    m_openedFiles[path] = document;
    if ( document->isMonitored() )
        fileWatcher()->addPath(path);
    endResetModel();
}

/** Changes the path of a document */
void ProjectDocumentModel::relocateDocument(const QString &path, const QString &newPath, ProjectDocument *document){
    beginResetModel();
    m_openedFiles.take(path);
    m_openedFiles[newPath] = document;
    if ( document->isMonitored() ){
        fileWatcher()->removePath(path);
        fileWatcher()->addPath(newPath);
    }
    endResetModel();
}

/** Closes all of the open documents */
void ProjectDocumentModel::closeDocuments(){
    if ( m_fileWatcher ){
        QObject::disconnect(m_fileWatcher, SIGNAL(fileChanged(QString)),
                            this, SLOT(monitoredFileChanged(QString)));
        m_fileWatcher->deleteLater();
        m_fileWatcher = 0;
    }

    for( QHash<QString, ProjectDocument*>::iterator it = m_openedFiles.begin(); it != m_openedFiles.end(); ++it ){
        emit aboutToClose(it.value());
        delete it.value();
    }
    m_openedFiles.clear();
}

/** Change the monitoring state of a particular documetn */
void ProjectDocumentModel::updateDocumentMonitoring(ProjectDocument *document, bool monitor){
    if ( document->isMonitored() != monitor ){
        if ( monitor ){
            fileWatcher()->addPath(document->file()->path());
            document->setIsMonitored(true);
        } else {
            fileWatcher()->removePath(document->file()->path());
            document->setIsMonitored(false);
        }
    }
}

/** Close all of the documents within a given folder path */
void ProjectDocumentModel::closeDocumentsInPath(const QString &path){
    if ( path.isEmpty() ){
        closeDocument(path);
        return;
    }

    QHash<QString, ProjectDocument*>::iterator it = m_openedFiles.begin();
    beginResetModel();
    while ( it != m_openedFiles.end() ){
        if ( it.key().startsWith(path) ){
            ProjectDocument* doc = it.value();
            it = m_openedFiles.erase(it);
            emit aboutToClose(it.value());
            delete doc;
        } else {
            ++it;
        }
    }

    endResetModel();
}

/**
 * \brief Close the specific file with a given path
 */
void ProjectDocumentModel::closeDocument(const QString &path){
    if ( m_openedFiles.contains(path) ){
        beginResetModel();
        ProjectDocument* document = m_openedFiles.take(path);
        emit aboutToClose(document);
        delete document;
        endResetModel();
    }
}

/** Check for changes inside the open files (by comparing last mod timestamps) */
void ProjectDocumentModel::rescanDocuments(){
    for( QHash<QString, ProjectDocument*>::iterator it = m_openedFiles.begin(); it != m_openedFiles.end(); ++it ){
        QDateTime modifiedDate = QFileInfo(it.key()).lastModified();
        if ( modifiedDate > it.value()->lastModified() && !it.value()->isMonitored() )
            emit documentChangedOutside(it.value());
    }
}

/** Re-reads content of a monitored file that changed */
void ProjectDocumentModel::monitoredFileChanged(const QString &path){
    ProjectDocument* doc = m_openedFiles[path];
    doc->readContent();
    emit monitoredDocumentChanged(doc);
}

/** Saves all changed documents */
bool ProjectDocumentModel::saveDocuments(){
    bool saved = true;
    for( QHash<QString, ProjectDocument*>::iterator it = m_openedFiles.begin(); it != m_openedFiles.end(); ++it ){
        if ( it.value()->isDirty() )
            if ( !it.value()->save() )
                saved = false;
    }
    return saved;
}

/** Returns last opened file */
ProjectDocument *ProjectDocumentModel::lastOpened(){
    if ( m_openedFiles.size() > 0 )
        return *m_openedFiles.begin();
    return nullptr;
}

/** Returns the document given it's path hash, using lv::ProjectEntry::hashPath() */
ProjectDocument *ProjectDocumentModel::documentByPathHash(const QString &hash){
    for( auto it = m_openedFiles.begin(); it != m_openedFiles.end(); ++it ){
        ProjectDocument* d = *it;
        if ( d->file()->hashPath() == hash )
            return d;
    }
    return nullptr;
}

/**
 * \brief Shows a list of all unsaved docs
 *
 * Useful for e.g. when we're closing a project
 */
QStringList ProjectDocumentModel::listUnsavedDocuments(){
    QStringList base;
    for( QHash<QString, ProjectDocument*>::iterator it = m_openedFiles.begin(); it != m_openedFiles.end(); ++it ){
        if ( it.value()->isDirty() )
            base.append(it.value()->file()->path());
    }
    return base;
}

/**
 * \brief Returns list of unsaved documents withing a folder
 *
 * Useful for e.g. when you're moving or deleting a folder within a project
 */
QStringList ProjectDocumentModel::listUnsavedDocumentsInPath(const QString &path){
    QStringList base;
    for( QHash<QString, ProjectDocument*>::iterator it = m_openedFiles.begin(); it != m_openedFiles.end(); ++it ){
        if ( it.key().startsWith(path) ){
            if ( it.value()->isDirty() )
                base.append(it.value()->file()->path());
        }
    }
    return base;
}

QFileSystemWatcher *ProjectDocumentModel::fileWatcher(){
    if ( !m_fileWatcher ){
        m_fileWatcher = new QFileSystemWatcher;
        QObject::connect(m_fileWatcher, SIGNAL(fileChanged(QString)),
                         this, SLOT(monitoredFileChanged(QString)));
    }
    return m_fileWatcher;
}

/**
 * \brief Returns if the file is opened
 *
 * By definition, if the file is inside this model, it's opened!
 */
ProjectDocument *ProjectDocumentModel::isOpened(const QString &path){
    if ( m_openedFiles.contains(path) )
        return m_openedFiles[path];
    return 0;
}


}// namespace

