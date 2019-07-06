/****************************************************************************
**
** Copyright (C) 2019 Aliaksei Stratsilatau
** Contact: http://docs.uavos.com
**
** This file is part of APX.
**
** All rights reserved.
**
****************************************************************************/

import qbs.FileInfo

Module {
    Depends { name: "texttemplate"}
    Depends { name: "apx" }
    Group {
        name: "version.h.in"
        files: [name]
        texttemplate.outputFileName: FileInfo.joinPaths(product.buildDirectory,"include",FileInfo.completeBaseName(name))
        texttemplate.dict: apx.git
    }
    texttemplate.outputTag: "hpp"

    Depends { name: "cpp" }
    cpp.includePaths: [FileInfo.joinPaths(product.buildDirectory,"include")]
}
