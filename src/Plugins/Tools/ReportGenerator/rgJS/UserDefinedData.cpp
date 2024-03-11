#include "UserDefinedData.h"
#include "App/App.h"

RG::UserDefinedData::UserDefinedData(Fact *parent)
    : Fact(parent, "User defined data", "User defined data", "Values defined by user", Group | Count)
{}

Q_INVOKABLE void RG::UserDefinedData::registerTextField(QString name, QString title)
{
    registerField(name, title, Text);
}

Q_INVOKABLE void RG::UserDefinedData::registerEnum(QString name,
                                                   QString title,
                                                   QStringList enum_strings)
{
    auto f = registerField(name, title, Enum);
    f->setEnumStrings(enum_strings);
}

Q_INVOKABLE QJSValue RG::UserDefinedData::getFromField(QString field_name)
{
    auto child = this->findChild(field_name);

    if (child == nullptr)
        return QJSValue();

    return QJSValue(child->valueText());
}

Q_INVOKABLE void RG::UserDefinedData::clearContext()
{
    this->deleteChildren();

    m_fact_map.clear();
}

Fact *RG::UserDefinedData::registerField(QString name, QString title, Flags flags)
{
    auto temp_fact = new Fact(this, name, title, "", flags);

    m_fact_map.insert(name, temp_fact);

    return temp_fact;
}
