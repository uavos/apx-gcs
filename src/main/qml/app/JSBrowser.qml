import QtQuick 2.2
import QtQuick.Controls 1.4
import QtQml.Models 2.11

TreeView {
    TableViewColumn {
        title: "Name"
        role: "name"
        width: 100
    }
    TableViewColumn {
        title: "Value"
        role: "text"
        width: 100
    }
    TableViewColumn {
        title: "Descr"
        role: "descr"
        //width: 100
    }
    model: jsTreeModel
}
