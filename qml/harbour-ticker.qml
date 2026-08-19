import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.ticker 1.0

ApplicationWindow {
    id: window

    TickerController {
        id: controller
    }

    initialPage: Component { MainPage {} }
    cover: Component { CoverPage {} }
}
