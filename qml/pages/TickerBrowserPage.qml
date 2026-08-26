import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property var selected: []
    property var curated: [
        // US indices
        { sym: "^GSPC", name: "S&P 500", cat: "US Indices" },
        { sym: "^NDX", name: "Nasdaq 100", cat: "US Indices" },
        { sym: "^DJI", name: "Dow Jones", cat: "US Indices" },
        { sym: "^RUT", name: "Russell 2000", cat: "US Indices" },
        { sym: "^VIX", name: "VIX Volatility", cat: "US Indices" },
        // EU indices
        { sym: "^GDAXI", name: "DAX (Germany)", cat: "EU Indices" },
        { sym: "^FTSE", name: "FTSE 100 (UK)", cat: "EU Indices" },
        { sym: "^FCHI", name: "CAC 40 (France)", cat: "EU Indices" },
        { sym: "^STOXX50E", name: "Euro Stoxx 50", cat: "EU Indices" },
        { sym: "^OMXH25", name: "OMX Helsinki 25", cat: "EU Indices" },
        // Tech US
        { sym: "AAPL", name: "Apple", cat: "Tech" },
        { sym: "MSFT", name: "Microsoft", cat: "Tech" },
        { sym: "NVDA", name: "NVIDIA", cat: "Tech" },
        { sym: "GOOGL", name: "Alphabet (Google)", cat: "Tech" },
        { sym: "META", name: "Meta Platforms", cat: "Tech" },
        { sym: "TSLA", name: "Tesla", cat: "Tech" },
        { sym: "AMD", name: "AMD", cat: "Tech" },
        { sym: "INTC", name: "Intel", cat: "Tech" },
        { sym: "NFLX", name: "Netflix", cat: "Tech" },
        { sym: "ORCL", name: "Oracle", cat: "Tech" },
        // Helsinki
        { sym: "NOKIA.HE", name: "Nokia", cat: "Helsinki" },
        { sym: "KNEBV.HE", name: "KONE", cat: "Helsinki" },
        { sym: "UPM.HE", name: "UPM-Kymmene", cat: "Helsinki" },
        { sym: "NDA-FI.HE", name: "Nordea Bank", cat: "Helsinki" },
        { sym: "FORTUM.HE", name: "Fortum", cat: "Helsinki" },
        { sym: "ELISA.HE", name: "Elisa", cat: "Helsinki" },
        { sym: "WRT1V.HE", name: "W\u00e4rtsil\u00e4", cat: "Helsinki" },
        { sym: "STERV.HE", name: "Stora Enso", cat: "Helsinki" },
        // ETFs
        { sym: "SPY", name: "SPDR S&P 500 ETF", cat: "ETFs" },
        { sym: "QQQ", name: "Invesco QQQ", cat: "ETFs" },
        { sym: "VWCE.DE", name: "Vanguard FTSE All-World", cat: "ETFs" },
        { sym: "EUNL.DE", name: "iShares Core MSCI World", cat: "ETFs" },
        { sym: "IS3N.DE", name: "iShares Core MSCI EM", cat: "ETFs" },
        // FX
        { sym: "EURUSD=X", name: "EUR/USD", cat: "FX" },
        { sym: "EURSEK=X", name: "EUR/SEK", cat: "FX" },
        { sym: "USDJPY=X", name: "USD/JPY", cat: "FX" },
        { sym: "GBPUSD=X", name: "GBP/USD", cat: "FX" },
        { sym: "EURGBP=X", name: "EUR/GBP", cat: "FX" },
        // Crypto
        { sym: "BTC-USD", name: "Bitcoin", cat: "Crypto" },
        { sym: "ETH-USD", name: "Ethereum", cat: "Crypto" },
        { sym: "SOL-USD", name: "Solana", cat: "Crypto" },
        { sym: "DOGE-USD", name: "Dogecoin", cat: "Crypto" },
        // Commodities
        { sym: "GC=F", name: "Gold", cat: "Commodities" },
        { sym: "SI=F", name: "Silver", cat: "Commodities" },
        { sym: "CL=F", name: "Crude Oil", cat: "Commodities" }
    ]

    function isSelected(sym) { return selected.indexOf(sym) >= 0 }
    function isInWatchlist(sym) { return ticker.containsSymbol(sym) }

    function toggle(sym) {
        if (isInWatchlist(sym)) return
        var idx = selected.indexOf(sym)
        var copy = selected.slice(0)
        if (idx >= 0) copy.splice(idx, 1)
        else copy.push(sym)
        selected = copy
    }

    function filtered(filterText) {
        var f = (filterText !== undefined ? filterText : searchField.text).toLowerCase().trim()
        if (f.length === 0) return curated
        return curated.filter(function (e) {
            return e.sym.toLowerCase().indexOf(f) >= 0
                || e.name.toLowerCase().indexOf(f) >= 0
                || e.cat.toLowerCase().indexOf(f) >= 0
        })
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Select all filtered")
                onClicked: {
                    var list = filtered(searchField.text)
                    var copy = selected.slice(0)
                    for (var i = 0; i < list.length; i++) {
                        var s = list[i].sym
                        if (isInWatchlist(s)) continue
                        if (copy.indexOf(s) < 0) copy.push(s)
                    }
                    selected = copy
                }
            }
            MenuItem {
                text: qsTr("Clear selection")
                enabled: selected.length > 0
                onClicked: selected = []
            }
        }

        Column {
            id: col
            width: parent.width
            spacing: 0

            PageHeader {
                title: qsTr("Browse tickers")
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: qsTr("Search symbol or name")
                EnterKey.enabled: false
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: selected.length > 0
                      ? qsTr("%n selected — scroll down and tap Add", "", selected.length)
                      : qsTr("Tap rows to select multiple, then Add at bottom. Dimmed = already in watchlist.")
            }

            Item { width: 1; height: Theme.paddingSmall }

            Repeater {
                model: filtered(searchField.text)

                ListItem {
                    id: row
                    contentHeight: Theme.itemSizeMedium
                    enabled: !isInWatchlist(modelData.sym)
                    opacity: enabled ? 1.0 : 0.45
                    highlighted: isSelected(modelData.sym)

                    onClicked: toggle(modelData.sym)

                    Row {
                        anchors {
                            left: parent.left
                            right: checkIcon.left
                            verticalCenter: parent.verticalCenter
                            leftMargin: Theme.horizontalPageMargin
                            rightMargin: Theme.paddingSmall
                        }
                        spacing: Theme.paddingSmall

                        Column {
                            width: parent.width - catLabel.width - parent.spacing
                            anchors.verticalCenter: parent.verticalCenter

                            Label {
                                width: parent.width
                                text: modelData.sym
                                color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
                                font.pixelSize: Theme.fontSizeMedium
                                elide: Text.ElideRight
                            }
                            Label {
                                width: parent.width
                                text: modelData.name
                                color: Theme.secondaryColor
                                font.pixelSize: Theme.fontSizeExtraSmall
                                elide: Text.ElideRight
                            }
                        }

                        Label {
                            id: catLabel
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.cat
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeTiny
                        }
                    }

                    Image {
                        id: checkIcon
                        anchors {
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            rightMargin: Theme.horizontalPageMargin
                        }
                        source: isSelected(modelData.sym)
                                ? "image://theme/icon-m-acknowledge"
                                : isInWatchlist(modelData.sym)
                                  ? "image://theme/icon-m-acknowledge"
                                  : ""
                        visible: isSelected(modelData.sym) || isInWatchlist(modelData.sym)
                        opacity: isInWatchlist(modelData.sym) ? 0.4 : 1.0
                    }
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                enabled: selected.length > 0
                text: selected.length > 0
                      ? qsTr("Add %n ticker(s)", "", selected.length)
                      : qsTr("No selection")
                onClicked: {
                    ticker.addSymbolsVariant(selected)
                    pageStack.pop()
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.WordWrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("Can\u2019t find it? Use \u201cAdd symbol\u201d on the main page for any Yahoo symbol like BTC-USD, EURUSD=X or ^GSPC.")
            }
        }

        VerticalScrollDecorator {}
    }
}
