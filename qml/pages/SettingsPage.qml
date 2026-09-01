import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: 0

            PageHeader {
                title: qsTr("Settings")
            }

            ValueButton {
                width: parent.width
                label: qsTr("Refresh interval")
                description: ticker.intervalMinutes + qsTr(" min")
                onClicked: intervalDialog.open()
            }

            ValueButton {
                width: parent.width
                label: qsTr("Cover rows")
                description: ticker.coverRows + qsTr(" visible tickers")
                onClicked: rowsDialog.open()
            }

            TextSwitch {
                text: qsTr("Show timestamp on cover")
                description: qsTr("Last updated time at bottom of cover")
                checked: ticker.showCoverTimestamp
                onClicked: ticker.setShowCoverTimestamp(!ticker.showCoverTimestamp)
            }

            TextSwitch {
                text: qsTr("Show currency on cover")
                description: qsTr("e.g. USD/EUR suffix next to price")
                checked: ticker.showCoverCurrency
                onClicked: ticker.setShowCoverCurrency(!ticker.showCoverCurrency)
            }

            TextSwitch {
                text: qsTr("Show price on cover")
                description: qsTr("Off = % only, more room for symbols")
                checked: ticker.showCoverPrice
                onClicked: ticker.setShowCoverPrice(!ticker.showCoverPrice)
            }

            Slider {
                width: parent.width
                label: qsTr("Cover scale")
                minimumValue: 0.7
                maximumValue: 1.6
                stepSize: 0.1
                value: ticker.coverScale
                valueText: Math.round(value * 100) + "%"
                onValueChanged: {
                    if (Math.abs(value - ticker.coverScale) > 0.01)
                        ticker.setCoverScale(value)
                }
            }

            SectionHeader {
                text: qsTr("Data provider")
            }

            ComboBox {
                width: parent.width
                label: qsTr("Provider")
                value: ticker.providerModeName()
                currentIndex: ticker.providerMode
                menu: ContextMenu {
                    MenuItem { text: qsTr("Auto (Yahoo → Nordic → Finnhub)"); onClicked: ticker.setProviderMode(0) }
                    MenuItem { text: qsTr("Yahoo only"); onClicked: ticker.setProviderMode(1) }
                    MenuItem { text: qsTr("Nordic only (no key)"); onClicked: ticker.setProviderMode(2) }
                    MenuItem { text: qsTr("Finnhub only"); onClicked: ticker.setProviderMode(3) }
                }
            }

            TextField {
                id: finnhubField
                width: parent.width
                label: qsTr("Finnhub API key")
                placeholderText: qsTr("Paste key from finnhub.io (free)")
                text: ticker.finnhubApiKey
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    ticker.setFinnhubApiKey(text)
                    focus = false
                }
                onActiveFocusChanged: {
                    if (!activeFocus && text !== ticker.finnhubApiKey)
                        ticker.setFinnhubApiKey(text)
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("Get a free key at finnhub.io → Docs. Auto tries Yahoo, then Nordic CDN, then Finnhub if key is set. See https://finnhub.io/docs/api")
                linkColor: Theme.highlightColor
                onLinkActivated: Qt.openUrlExternally(link)
            }

            SectionHeader {
                text: qsTr("Ideas for next")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("• Price alerts / thresholds\n• Change display: % / absolute / both\n• Sort: manual vs alphabetical\n• Decimals: 2 vs dynamic\n• Stale indicator after offline N min\n• Compact vs detailed cover density")
            }

            Separator {
                width: parent.width
                color: Theme.primaryColor
                horizontalAlignment: Qt.AlignHCenter
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: qsTr("Data: Yahoo (keyless) → Nordic CDN (no key) → Finnhub (if key set). Prices update on the refresh interval while the app is running or its cover is active.")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }

            Separator {
                width: parent.width
                color: Theme.primaryColor
                horizontalAlignment: Qt.AlignHCenter
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: "harbour-ticker 0.1.0\nData: Yahoo (unofficial) + Nordic CDN + Finnhub.io"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }

        VerticalScrollDecorator {}
    }

    Dialog {
        id: intervalDialog
        canAccept: !isNaN(parseInt(intervalField.text))
        onAccepted: ticker.setIntervalMinutes(parseInt(intervalField.text))

        Column {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingSmall

            DialogHeader {
                title: qsTr("Refresh interval")
            }

            TextField {
                id: intervalField
                width: parent.width
                text: String(ticker.intervalMinutes)
                inputMethodHints: Qt.ImhDigitsOnly
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: intervalDialog.accept()
            }

            Label {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("1–30 minutes.")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }

    Dialog {
        id: rowsDialog
        canAccept: !isNaN(parseInt(rowsField.text)) && parseInt(rowsField.text) >= 1 && parseInt(rowsField.text) <= 20
        onAccepted: ticker.setCoverRows(parseInt(rowsField.text))

        Column {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingSmall

            DialogHeader {
                title: qsTr("Cover rows")
            }

            TextField {
                id: rowsField
                width: parent.width
                text: String(ticker.coverRows)
                inputMethodHints: Qt.ImhDigitsOnly
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: rowsDialog.accept()
            }

            Label {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("How many tickers to show on the home screen cover (1–20).")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }
}
