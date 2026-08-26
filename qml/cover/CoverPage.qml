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
            margins: Theme.paddingLarge
        }

        Label {
            text: qsTr("Ticker")
            font.pixelSize: Theme.fontSizeMedium
            font.family: Theme.fontFamilyHeading
            color: Theme.primaryColor
        }

        Item { width: 1; height: Theme.paddingMedium }

        Repeater {
            model: ticker.tickers

            Item {
                readonly property bool shown: index < ticker.coverRows

                width: content.width
                height: shown ? Theme.itemSizeSmall : 0
                visible: shown

                Row {
                    anchors.fill: parent
                    spacing: Theme.paddingSmall

                    Label {
                        width: parent.width - priceLabel.width - pctLabel.width - 2 * parent.spacing
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.symbol
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.primaryColor
                        elide: Text.ElideRight
                    }

                    Label {
                        id: priceLabel
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.price !== undefined && modelData.price.length > 0
                              ? modelData.price.split(" ")[0] : "—"
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.primaryColor
                    }

                    Label {
                        id: pctLabel
                        anchors.verticalCenter: parent.verticalCenter
                        visible: modelData.pct !== undefined && modelData.pct.length > 0
                        text: modelData.pct
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: modelData.up ? "#4caf50" : "#f44336"
                    }
                }
            }
        }
    }

    Label {
        visible: ticker.lastUpdated.length > 0
        anchors {
            left: parent.left
            bottom: parent.bottom
            leftMargin: Theme.paddingLarge
            bottomMargin: Theme.paddingMedium
        }
        text: ticker.lastUpdated.replace("T", " ")
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeExtraSmall
    }
}
