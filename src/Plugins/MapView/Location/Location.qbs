import qbs
import ApxApp

ApxApp.ApxPlugin {

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
