import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Browse tickers")
                onClicked: pageStack.push(Qt.resolvedUrl("TickerBrowserPage.qml"))
            }
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
                title: qsTr("ticker")
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

                ListItem {
                    id: rowItem
                    contentHeight: Theme.itemSizeMedium

                    menu: ContextMenu {
                        MenuItem {
                            text: qsTr("Remove")
                            onClicked: rowItem.remorseDelete(function () {
                                ticker.removeSymbol(modelData.symbol)
                            })
                        }
                    }

                    Column {
                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            leftMargin: Theme.horizontalPageMargin
                            rightMargin: Theme.horizontalPageMargin
                        }

                        Row {
                            width: parent.width
                            spacing: Theme.paddingSmall

                            Label {
                                width: parent.width - priceLabel.width - pctLabel.width - 2 * parent.spacing
                                text: modelData.symbol
                                font.pixelSize: Theme.fontSizeMedium
                                color: rowItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                                elide: Text.ElideRight
                            }

                            Label {
                                id: priceLabel
                                text: modelData.price !== undefined && modelData.price.length > 0 ? modelData.price : "—"
                                font.pixelSize: Theme.fontSizeMedium
                                color: Theme.primaryColor
                            }

                            Label {
                                id: pctLabel
                                visible: modelData.pct !== undefined && modelData.pct.length > 0
                                text: modelData.pct
                                font.pixelSize: Theme.fontSizeSmall
                                color: modelData.up ? "#4caf50" : "#f44336"
                            }
                        }

                        Label {
                            width: parent.width
                            visible: modelData.name !== undefined && modelData.name.length > 0 && modelData.name !== modelData.symbol
                            text: modelData.name !== undefined ? modelData.name : ""
                            font.pixelSize: Theme.fontSizeExtraSmall
                            color: Theme.secondaryColor
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Label {
                visible: ticker.tickers.length === 0
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No symbols in watchlist.\nPull down to browse or add one.")
                color: Theme.secondaryColor
            }
        }
    }

    Dialog {
        id: addDialog
        canAccept: symbolField.text.trim().length > 0
        onAccepted: {
            ticker.addSymbol(symbolField.text)
            symbolField.text = ""
        }
        onRejected: symbolField.text = ""

        Column {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingSmall

            DialogHeader {
                title: qsTr("Add symbol")
            }

            TextField {
                id: symbolField
                width: parent.width
                placeholderText: "AAPL, ^GSPC, EURUSD=X"
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: addDialog.accept()
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
