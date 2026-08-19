#include "tickercontroller.h"

#include <SailfishApp.h>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace {
const char *WATCHLIST_FILE = "watchlist.json";
const char *SNAPSHOT_FILE = "snapshot.json";
const int MAX_SYMBOLS = 20;
const int BASE_BACKOFF_MS = 60 * 1000;
const int MAX_BACKOFF_MS = 30 * 60 * 1000;

QString dataPath(const QString &fileName)
{
    return SailfishApp::appDataDir() + fileName;
}

QString fmtPrice(double v, const QString &currency)
{
    QLocale locale(QLocale::English);
    QString s = locale.toString(v, 'f', 2);
    if (!currency.isEmpty())
        s += " " + currency;
    return s;
}
} // namespace

TickerController::TickerController(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);

    loadState();

    m_timer = new QTimer(this);
    m_timer->setInterval(30 * 1000);
    connect(m_timer, &QTimer::timeout, this, &TickerController::pollDue);
    m_timer->start();

    emitChanged();
    refresh();
}

TickerController::~TickerController()
{
    persistWatchlist();
}

void TickerController::loadState()
{
    QFile watch(dataPath(WATCHLIST_FILE));
    QStringList ids;
    if (watch.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(watch.readAll()).array();
        for (const QJsonValue &v : arr)
            ids << v.toString();
    } else {
        ids = QStringList{ "^GSPC", "^NDX", "^DJI", "AAPL", "MSFT" };
    }

    QFile snap(dataPath(SNAPSHOT_FILE));
    QVariantMap byId;
    if (snap.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(snap.readAll()).array();
        for (const QJsonValue &v : arr) {
            QJsonObject o = v.toObject();
            QVariantMap m = o.toVariantHash();
            byId.insert(o.value("symbol").toString(), m);
        }
    }

    m_symbols.clear();
    for (const QString &id : ids) {
        Symbol s;
        s.id = id;
        s.data = byId.value(id).toMap();
        m_symbols.append(s);
    }
}

void TickerController::persistWatchlist() const
{
    QJsonArray arr;
    for (const Symbol &s : m_symbols)
        arr.append(s.id);
    QFile f(dataPath(WATCHLIST_FILE));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(arr).toJson());
}

void TickerController::refresh()
{
    tick();
}

void TickerController::addSymbol(const QString &raw)
{
    const QString id = cleanSymbol(raw);
    if (id.isEmpty())
        return;
    for (const Symbol &s : m_symbols) {
        if (s.id.compare(id, Qt::CaseInsensitive) == 0)
            return;
    }
    if (m_symbols.size() >= MAX_SYMBOLS)
        return;
    Symbol s;
    s.id = id;
    m_symbols.append(s);
    persistWatchlist();
    emitChanged();
    refresh();
}

void TickerController::removeSymbol(const QString &symbol)
{
    for (int i = 0; i < m_symbols.size(); ++i) {
        if (m_symbols[i].id.compare(symbol, Qt::CaseInsensitive) == 0) {
            m_symbols.removeAt(i);
            persistWatchlist();
            emitChanged();
            return;
        }
    }
}

void TickerController::moveSymbol(int from, int to)
{
    if (from < 0 || from >= m_symbols.size() || to < 0 || to >= m_symbols.size())
        return;
    Symbol s = m_symbols.takeAt(from);
    m_symbols.insert(to, s);
    persistWatchlist();
    emitChanged();
}

void TickerController::setIntervalMinutes(int minutes)
{
    if (minutes < 1)
        minutes = 1;
    if (minutes > 30)
        minutes = 30;
    if (minutes == m_intervalMinutes)
        return;
    m_intervalMinutes = minutes;
    emit intervalMinutesChanged();
}

void TickerController::pollDue()
{
    const qint64 dueMs = qint64(m_intervalMinutes) * 60 * 1000 - 12000;
    if (m_lastUpdated.isEmpty()) {
        tick();
        return;
    }
    QDateTime t = QDateTime::fromString(m_lastUpdated, Qt::ISODate);
    if (t.isValid() && t.msecsTo(QDateTime::currentDateTime()) >= dueMs)
        tick();
}

void TickerController::tick()
{
    if (m_refreshing || m_symbols.isEmpty())
        return;
    for (Symbol &s : m_symbols)
        s.pending = false;
    const bool anyMissing = std::any_of(m_symbols.begin(), m_symbols.end(),
                                        [](const Symbol &s) { return s.data.value("price").toString().isEmpty(); });
    if (!anyMissing)
        return;
    m_refreshing = true;
    emit refreshingChanged();
    m_cursor = 0;
    fetchNext();
}

void TickerController::fetchNext()
{
    while (m_cursor < m_symbols.size()) {
        Symbol &s = m_symbols[m_cursor];
        if (s.data.value("price").toString().isEmpty() && s.retryAfterMs > QDateTime::currentMSecsSinceEpoch()) {
            ++m_cursor;
            continue;
        }
        s.pending = true;
        QUrl url(QStringLiteral("https://query1.finance.yahoo.com/v8/finance/chart/%1")
                     .arg(QUrl::toPercentEncoding(s.id)));
        QNetworkRequest req(url);
        req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
        QNetworkReply *reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            onFetchFinished(m_symbols[m_cursor].id, status, reply->readAll());
            reply->deleteLater();
        });
        ++m_cursor;
        return;
    }
    finishTick();
}

void TickerController::onFetchFinished(const QString &symbol, int httpStatus, const QByteArray &payload)
{
    for (Symbol &s : m_symbols) {
        if (s.id.compare(symbol, Qt::CaseInsensitive) != 0)
            continue;
        s.pending = false;
        if (httpStatus == 200) {
            QVariantMap meta = parseMeta(payload);
            if (!meta.isEmpty()) {
                s.data = meta;
                s.failures = 0;
                s.retryAfterMs = 0;
            } else {
                s.failures++;
                const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 5)), MAX_BACKOFF_MS);
                s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
            }
        } else if (httpStatus == 429 || httpStatus == 401) {
            s.failures++;
            const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 5)), MAX_BACKOFF_MS);
            s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
        } else {
            s.failures++;
        }
        break;
    }
    fetchNext();
}

QVariantMap TickerController::parseMeta(const QByteArray &payload) const
{
    QJsonObject root = QJsonDocument::fromJson(payload).object();
    QJsonArray results = root.value("chart").toObject().value("result").toArray();
    if (results.isEmpty())
        return {};
    QJsonObject meta = results.first().toObject().value("meta").toObject();
    const double price = meta.value("regularMarketPrice").toDouble(-1);
    if (price <= 0)
        return {};

    double prev = meta.value("previousClose").toDouble(0);
    if (prev <= 0)
        prev = meta.value("chartPreviousClose").toDouble(0);

    const QString currency = meta.value("currency").toString();
    QVariantMap m;
    m["symbol"] = meta.value("symbol").toString();
    m["name"] = meta.value("longName").toString(meta.value("shortName").toString(m["symbol"].toString()));
    m["price"] = fmtPrice(price, currency);
    m["priceValue"] = price;
    m["currency"] = currency;
    if (prev > 0) {
        const double change = price - prev;
        const double pct = change / prev * 100.0;
        const QLocale locale(QLocale::English);
        const QString sign = change >= 0 ? "+" : "";
        m["change"] = sign + locale.toString(change, 'f', 2);
        m["pct"] = sign + locale.toString(pct, 'f', 2) + "%";
        m["up"] = change >= 0;
    } else {
        m["change"] = QString();
        m["pct"] = QString();
        m["up"] = true;
    }
    const double hi52 = meta.value("fiftyTwoWeekHigh").toDouble(0);
    const double lo52 = meta.value("fiftyTwoWeekLow").toDouble(0);
    if (hi52 > 0)
        m["hi52"] = fmtPrice(hi52, currency);
    if (lo52 > 0)
        m["lo52"] = fmtPrice(lo52, currency);
    const qlonglong volume = meta.value("regularMarketVolume").toVariant().toLongLong();
    if (volume > 0) {
        QLocale locale(QLocale::English);
        m["volume"] = locale.toString(volume);
    }
    return m;
}

void TickerController::finishTick()
{
    m_lastUpdated = QDateTime::currentDateTime().toString(Qt::ISODate);
    QFile f(dataPath(SNAPSHOT_FILE));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonArray arr;
        for (const Symbol &s : m_symbols) {
            if (s.data.isEmpty())
                continue;
            arr.append(QJsonDocument::fromVariant(s.data).object());
        }
        f.write(QJsonDocument(arr).toJson());
    }
    m_refreshing = false;
    emit refreshingChanged();
    emitChanged();
}

QVariantList TickerController::buildRows() const
{
    QVariantList rows;
    for (const Symbol &s : m_symbols) {
        if (s.data.isEmpty()) {
            QVariantMap row;
            row["symbol"] = s.id;
            row["name"] = s.id;
            row["price"] = QString();
            row["pct"] = QString();
            row["up"] = true;
            rows.append(row);
        } else {
            rows.append(s.data);
        }
    }
    return rows;
}

void TickerController::emitChanged()
{
    m_tickers = buildRows();
    emit tickersChanged();
}

QString TickerController::cleanSymbol(const QString &raw)
{
    const QString s = raw.trimmed().toUpper().remove(' ');
    return s;
}
