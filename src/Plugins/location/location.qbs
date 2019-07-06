import qbs
import apx.Application as APX

APX.ApxPlugin {

    /*Depends { name: "GeoPlugin" }

    Depends {
        name: "Qt";
        submodules: [
            "core",
        ]
    }*/

    //cpp.visibility: "default"


    /*files: [
        "LocationPlugin.cpp", "LocationPlugin.h",
    ]*/



    Depends {
        name: "Qt";
        submodules: [
            "sql" ,
            "network" ,
            "location-private" ,
            "positioning-private" ,
        ]
    }

    Depends { name: "ApxCore" }
    Depends { name: "ApxData" }

    cpp.includePaths: ["GeoPlugin"]

    Group {
        name: "GeoPlugin"
        prefix: name+"/"
        files: [
            "GeoMapReply.cpp", "GeoMapReply.h",
            "GeoPlugin.cpp", "GeoPlugin.h",
            "GeoPlugin.json",
            "GeoTileFetcher.cpp", "GeoTileFetcher.h",
            "GeoTiledMappingManagerEngine.cpp", "GeoTiledMappingManagerEngine.h",
            "TileLoader.cpp", "TileLoader.h",
            "MapsDB.cpp", "MapsDB.h",
        ]
    }
}
