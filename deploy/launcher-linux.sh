#!/bin/sh
export LD_LIBRARY_PATH=$(dirname $0)/../lib/uavos/Qt/lib
export QT_PLUGIN_PATH=$(dirname $0)/../lib/uavos/Qt/plugins
export QML2_IMPORT_PATH=$(dirname $0)/../lib/uavos/Qt/qml
$(dirname $0)/../lib/uavos/$(basename $0) $*
