import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.ticker 1.0

CoverBackground {
    id: cover

    TickerController {
        id: ticker
    }

    CoverActionList {
        id: coverActions

        CoverAction {
            iconSource: "image://theme/icon-m-refresh"
            onTriggered: ticker.refresh()
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        Row {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            height: Theme.itemSizeSmall

            Label {
                text: "Harbour Ticker"
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.primaryColor
                anchors.verticalCenter: parent.verticalCenter
            }

            Item { width: 1 }

            Label {
                visible: ticker.refreshing
                text: qsTr("updating…")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Repeater {
            model: ticker.tickers
            Row {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: Theme.itemSizeSmall

                Label {
                    text: modelData.symbol
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.primaryColor
                    anchors.verticalCenter: parent.verticalCenter
                    width: 90
                    elide: Text.ElideRight
                }

                Item { width: 1 }

                Label {
                    text: modelData.price !== undefined && modelData.price.length > 0 ? modelData.price : "—"
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.primaryColor
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: 1 }

                Label {
                    visible: modelData.pct !== undefined && modelData.pct.length > 0
                    text: modelData.pct
                    font.pixelSize: Theme.fontSizeSmall
                    color: modelData.up ? "#4caf50" : "#f44336"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        Item { width: 1; height: 1 }

        Label {
            visible: ticker.lastUpdated.length > 0
            x: Theme.horizontalPageMargin
            text: qsTr("updated %1").arg(ticker.lastUpdated)
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
        }
    }
}
