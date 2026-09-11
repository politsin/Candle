#include "settingsprofileentry.h"
#include <QDebug>

QDataStream &operator<<(QDataStream &out, const SettingsProfileEntry &e)
{
    out  << e.name << e.settings;
    return out;
}

QDataStream &operator>>(QDataStream &in, SettingsProfileEntry &e)
{
    in  >> e.name >> e.settings;
    return in;
}

static void registerTypes()
{
    qRegisterMetaType<SettingsProfileEntry>();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<SettingsProfileEntry>("SettingsProfileEntry");
    QMetaType::registerDebugStreamOperator<SettingsProfileEntry>();
#endif
}
Q_CONSTRUCTOR_FUNCTION(registerTypes)
