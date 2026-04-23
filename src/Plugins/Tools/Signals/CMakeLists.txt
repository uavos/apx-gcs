set(SIGNALS_QML
    ChartsView.qml
    ColorChooser.qml
    FilterItem.qml
    FilterKalmanSimple.qml
    FilterRunningAvg.qml
    MenuColor.qml
    MenuFilterItemPage.qml
    MenuFilters.qml
    MenuFiltersPage.qml
    MenuItem.qml
    MenuPage.qml
    MenuSet.qml
    MenuSetPage.qml
    PageButton.qml
    Signals.qml
    SignalsModel.qml
    SignalsMenu.qml
    SignalsMenuPopup.qml
    SignalsPlugin.qml
)

apx_plugin(SRCS ${SIGNALS_QML})

apx_qrc(${MODULE} PREFIX ${MODULE} SRCS ${SIGNALS_QML})
