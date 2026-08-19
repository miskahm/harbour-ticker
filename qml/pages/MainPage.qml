import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.ticker 1.0

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Add symbol")
                onClicked: addDialog.open()
            }
            MenuItem {
                text: qsTr("Refresh now")
                onClicked: ticker.refresh()
            }
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: 0

            PageHeader {
                title: "Harbour Ticker"
            }

            Label {
                visible: ticker.lastUpdated.length > 0
                x: Theme.horizontalPageMargin
                text: qsTr("Last updated %1").arg(ticker.lastUpdated)
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            Repeater {
                model: ticker.tickers
                SilicaHorizontalDivider { topVisible: true }
                ItemDelegate {
                    height: Theme.itemSizeMedium
                    text: modelData.symbol
                    subtext: modelData.name !== undefined && modelData.name.length > 0 ? modelData.name : ""
                    description: modelData.price
                    icon.sourceIcon: "qtc"

                    Label {
                        id: pctLabel
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.rightMargin: Theme.paddingMedium
                        visible: modelData.pct !== undefined && modelData.pct.length > 0
                        text: modelData.pct
                        color: modelData.up ? "#4caf50" : "#f44336"
                        font.pixelSize: Theme.fontSizeSmall
                    }

                    Menu {
                        MenuItem {
                            text: qsTr("Remove")
                            onClicked: ticker.removeSymbol(modelData.symbol)
                        }
                    }
                }
            }

            Label {
                visible: ticker.tickers.length === 0
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No symbols in watchlist.\nPull down to add one.")
                color: Theme.secondaryColor
            }
        }
    }

    Dialog {
        id: addDialog
        title: qsTr("Add symbol")
        modality: Qt.Modal
        standardButtons: Dialog.Cancel
        onAccepted: ticker.addSymbol(symbolField.text)

        Column {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingSmall

            TextField {
                id: symbolField
                width: parent.width
                placeholderText: "AAPL, ^GSPC, EURUSD=X"
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                onAccepted: addDialog.accept()
            }

            Label {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Stocks (AAPL), indexes (^GSPC, ^NDX, ^DJI), FX (EURUSD=X) or crypto (BTC-USD).")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }
}
