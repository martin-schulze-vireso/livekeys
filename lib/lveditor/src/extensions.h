#ifndef LVEXTENSIONS_H
#define LVEXTENSIONS_H

#include <QObject>
#include <QQmlPropertyMap>

#include "live/lvviewglobal.h"
#include "live/liveextension.h"
#include "live/package.h"

namespace lv{

class ViewEngine;
class LV_EDITOR_EXPORT Extensions : public QObject{

    Q_OBJECT

public:
    Extensions(ViewEngine* engine, const QString& settingsPath, QObject *parent = 0);
    ~Extensions();

    QQmlPropertyMap *globals();

    void loadExtensions();

    QMap<std::string, LiveExtension*>::iterator begin();
    QMap<std::string, LiveExtension*>::iterator end();

private:
    LiveExtension* loadPackageExtension(const std::string& path);
    LiveExtension* loadPackageExtension(const Package::Ptr& package);

    QMap<std::string, LiveExtension*> m_extensions;

    QQmlPropertyMap*     m_globals;
    QString              m_path;
    ViewEngine*          m_engine;
};

/** Globals getter */
inline QQmlPropertyMap* Extensions::globals(){
    return m_globals;
}

/** Begin iterator of extensions */
inline QMap<std::string, LiveExtension*>::iterator Extensions::begin(){
    return m_extensions.begin();
}

/** End iterator of extensions */
inline QMap<std::string, LiveExtension*>::iterator Extensions::end(){
    return m_extensions.end();
}

} // namespace

#endif // LVEXTENSIONS_H