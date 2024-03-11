#include "Fact/Fact.h"
#include <QJSValue>
#include <QtCore>

namespace RG {

class UserDefinedData : public Fact
{
    Q_OBJECT

public:
    UserDefinedData(Fact *parent);
    Q_INVOKABLE void registerTextField(QString name, QString title);
    Q_INVOKABLE void registerEnum(QString name, QString title, QStringList enum_strings);
    Q_INVOKABLE QJSValue getFromField(QString field_name);

    Q_INVOKABLE void clearContext();

private:
    Fact *registerField(QString name, QString title, Flags flags);

    QHash<QString, Fact *> m_fact_map;
};

} // namespace RG
