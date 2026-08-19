import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.ticker 1.0

Page {
    id: page

    TickerController {
        id: ticker
    }

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

            ItemDelegate {
                height: Theme.itemSizeMedium
                text: qsTr("Refresh interval")
                description: ticker.intervalMinutes + qsTr(" min")
                onClicked: intervalDialog.open()
            }

            SilicaHorizontalDivider { topVisible: true }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: qsTr("Data from Yahoo Finance. Prices update on the refresh interval while the app is running or its cover is active.")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
            }

            SilicaHorizontalDivider { topVisible: true }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                text: "harbour-ticker 0.1.0\nData: Yahoo Finance (unofficial, keyless)"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }

    Dialog {
        id: intervalDialog
        title: qsTr("Refresh interval")
        modality: Qt.Modal
        standardButtons: Dialog.Save | Dialog.Cancel
        onAccepted: ticker.setIntervalMinutes(intervalField.text.trim().toInt())

        Column {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingSmall

            TextField {
                id: intervalField
                width: parent.width
                text: String(ticker.intervalMinutes)
                inputMethodHints: Qt.ImhDigitsOnly
                onAccepted: intervalDialog.accept()
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
}
