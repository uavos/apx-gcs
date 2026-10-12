import QtQuick
import QtLocation

// ATS overlay: antenna beam of every GCS unit
MapItemGroup {
    id: overlay

    readonly property var f_overlay: apx.tools.ats.overlay

    readonly property bool showBeam: f_overlay.show_beam.value
    readonly property real beamDistance: f_overlay.beam_distance.value * 1000 // m

    readonly property real beamHalfAngle: 2 // deg

    MapItemView {
        model: apx.fleet.model
        delegate: AtsGcsItem { }
    }
}
