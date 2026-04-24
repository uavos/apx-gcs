set(QRC_QML
    ColorChoiceFact.qml
    ColorChooser.qml
    FilterFact.qml
    FilterRegistry.qml
    MenuItem.qml
    MenuPage.qml
    MenuSet.qml
    MenuSets.qml
    PageButton.qml
    SignalButton.qml
    Signals.qml
    SignalsModel.qml
    SignalsPlugin.qml
    SignalsView.qml
)

apx_plugin(SRCS ${QRC_QML})

apx_qrc(${MODULE} PREFIX ${MODULE} SRCS ${QRC_QML})
