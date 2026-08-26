import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    CoverActionList {
        CoverAction {
            iconSource: "image://theme/icon-m-refresh"
            onTriggered: ticker.refresh()
        }
    }

    Column {
        id: content
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: Theme.paddingSmall
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.paddingMedium
        }
        spacing: 1

        Repeater {
            model: ticker.tickers

            Item {
                readonly property bool shown: index < ticker.coverRows
                readonly property real s: ticker.coverScale

                width: content.width
                height: shown ? Math.round(30 * s) : 0
                visible: shown

                Row {
                    anchors.fill: parent
                    spacing: Theme.paddingSmall

                    Label {
                        width: {
                            var w = parent.width - parent.spacing
                            if (pctLabel.visible) w -= pctLabel.width + parent.spacing
                            if (ticker.showCoverPrice && priceLabel.visible) w -= priceLabel.width + parent.spacing
                            return w
                        }
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.symbol
                        font.pixelSize: Math.round(Theme.fontSizeExtraSmall * s)
                        color: Theme.primaryColor
                        elide: Text.ElideRight
                    }

                    Label {
                        id: priceLabel
                        anchors.verticalCenter: parent.verticalCenter
                        visible: ticker.showCoverPrice
                        width: visible ? implicitWidth : 0
                        text: {
                            if (modelData.price === undefined || modelData.price.length === 0) return "—"
                            if (ticker.showCoverCurrency) return modelData.price
                            return modelData.price.split(" ")[0]
                        }
                        font.pixelSize: Math.round(Theme.fontSizeExtraSmall * s)
                        color: Theme.primaryColor
                    }

                    Label {
                        id: pctLabel
                        anchors.verticalCenter: parent.verticalCenter
                        visible: modelData.pct !== undefined && modelData.pct.length > 0
                        width: visible ? implicitWidth : 0
                        text: modelData.pct
                        font.pixelSize: Math.round(Theme.fontSizeExtraSmall * s)
                        color: modelData.up ? "#4caf50" : "#f44336"
                    }
                }
            }
        }
    }

    Label {
        visible: ticker.showCoverTimestamp && ticker.lastUpdated.length > 0
        anchors {
            left: parent.left
            bottom: parent.bottom
            leftMargin: Theme.paddingMedium
            bottomMargin: Theme.paddingSmall
        }
        text: ticker.lastUpdated.replace("T", " ")
        color: Theme.secondaryColor
        font.pixelSize: Math.round(Theme.fontSizeTiny * ticker.coverScale)
    }
}
