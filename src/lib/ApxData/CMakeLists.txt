apx_lib(
    SRCS
    "**/*.[ch]*"
    INCLUDES
    .
    DEPENDS
    lib.ApxCore
    QT
    Sql
    Xml
)

# apx_install_res(${APX_RESOURCES_DIR} "templates/share")
