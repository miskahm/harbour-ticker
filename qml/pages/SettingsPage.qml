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
        canAccept: !isNaN(parseInt(rowsField.text)) && parseInt(rowsField.text) >= 1 && parseInt(rowsField.text) <= 10
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
                text: qsTr("How many tickers to show on the home screen cover (1–10).")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }
        }
    }
}
