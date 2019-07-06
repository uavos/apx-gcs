import qbs
import qbs.FileInfo

Product {
    type: [ "deployment" ]

    name: "DeployLibs"

    property var outputs: []

    condition: qbs.buildVariant.contains("release")

    Depends { name: "apx" }

    Depends {
        productTypes: [
            "bundle.content",
            "application",
            "loadablemodule",
            "dynamiclibrary",
        ]
    }
}
