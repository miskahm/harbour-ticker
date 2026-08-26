#include "tickercontroller.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>
#include <QVariantHash>

namespace {
const char *WATCHLIST_FILE = "watchlist.json";
const char *SNAPSHOT_FILE = "snapshot.json";
const int MAX_SYMBOLS = 20;
const int MIN_COVER_ROWS = 1;
const int MAX_COVER_ROWS = 10;
const int BASE_BACKOFF_MS = 60 * 1000;
const int MAX_BACKOFF_MS = 30 * 60 * 1000;

QString dataPath(const QString &fileName)
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + QLatin1Char('/') + fileName;
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
    qInfo() << "controller: constructing, dataPath=" << dataPath(WATCHLIST_FILE);
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    m_nam = new QNetworkAccessManager(this);

    loadState();
    qInfo() << "controller: loaded" << m_symbols.size() << "symbols";

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
        const QJsonDocument doc = QJsonDocument::fromJson(watch.readAll());
        const QJsonArray arr = doc.isObject()
                ? doc.object().value("symbols").toArray()
                : doc.array();
        if (doc.isObject()) {
            m_intervalMinutes = qBound(1, doc.object().value("intervalMinutes").toInt(m_intervalMinutes), 30);
            m_coverRows = qBound(MIN_COVER_ROWS, doc.object().value("coverRows").toInt(m_coverRows), MAX_COVER_ROWS);
            if (doc.object().contains("showCoverTimestamp"))
                m_showCoverTimestamp = doc.object().value("showCoverTimestamp").toBool(m_showCoverTimestamp);
            if (doc.object().contains("showCoverCurrency"))
                m_showCoverCurrency = doc.object().value("showCoverCurrency").toBool(m_showCoverCurrency);
            if (doc.object().contains("showCoverPrice"))
                m_showCoverPrice = doc.object().value("showCoverPrice").toBool(m_showCoverPrice);
            if (doc.object().contains("coverScale"))
                m_coverScale = qBound(0.7, doc.object().value("coverScale").toDouble(m_coverScale), 1.6);
            if (doc.object().contains("finnhubApiKey"))
                m_finnhubApiKey = doc.object().value("finnhubApiKey").toString();
            if (doc.object().contains("providerMode"))
                m_providerMode = qBound(0, doc.object().value("providerMode").toInt(m_providerMode), 3);
        }
        for (const QJsonValue &v : arr)
            ids << v.toString();
    } else {
        // No default tickers — fresh install starts empty (user adds via Browse/Add)
        ids = QStringList{};
        qInfo() << "controller: no watchlist found, starting empty";
    }

    QFile snap(dataPath(SNAPSHOT_FILE));
    QVariantMap byId;
    if (snap.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(snap.readAll()).array();
        for (const QJsonValue &v : arr) {
            QJsonObject o = v.toObject();
            QVariantHash h = o.toVariantHash();
            QVariantMap m;
            for (auto it = h.constBegin(); it != h.constEnd(); ++it)
                m.insert(it.key(), it.value());
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

    persistWatchlist();
}

void TickerController::persistWatchlist() const
{
    QJsonArray arr;
    for (const Symbol &s : m_symbols)
        arr.append(s.id);
    QJsonObject root;
    root.insert("intervalMinutes", m_intervalMinutes);
    root.insert("coverRows", m_coverRows);
    root.insert("showCoverTimestamp", m_showCoverTimestamp);
    root.insert("showCoverCurrency", m_showCoverCurrency);
    root.insert("showCoverPrice", m_showCoverPrice);
    root.insert("coverScale", m_coverScale);
    root.insert("finnhubApiKey", m_finnhubApiKey);
    root.insert("providerMode", m_providerMode);
    root.insert("symbols", arr);
    QFile f(dataPath(WATCHLIST_FILE));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(root).toJson());
    else
        qWarning() << "controller: FAILED to open watchlist file" << dataPath(WATCHLIST_FILE);
}

void TickerController::refresh()
{
    tick();
}

void TickerController::addSymbol(const QString &raw)
{
    const QString id = cleanSymbol(raw);
    qInfo() << "controller: addSymbol request" << raw << "->" << id;
    if (id.isEmpty()) {
        qWarning() << "controller: addSymbol rejected empty after clean";
        return;
    }
    for (const Symbol &s : m_symbols) {
        if (s.id.compare(id, Qt::CaseInsensitive) == 0) {
            qInfo() << "controller: addSymbol already exists" << id;
            return;
        }
    }
    if (m_symbols.size() >= MAX_SYMBOLS) {
        qWarning() << "controller: addSymbol at max" << MAX_SYMBOLS;
        return;
    }
    Symbol s;
    s.id = id;
    m_symbols.append(s);
    qInfo() << "controller: addSymbol added" << id << "total" << m_symbols.size();
    persistWatchlist();
    emitChanged();
    refresh();
    if (m_refreshing) {
        qInfo() << "controller: addSymbol during refresh, will be picked up by ongoing fetch chain";
    }
}

void TickerController::addSymbols(const QStringList &symbols)
{
    bool changed = false;
    for (const QString &raw : symbols) {
        const QString id = cleanSymbol(raw);
        if (id.isEmpty())
            continue;
        bool exists = false;
        for (const Symbol &s : m_symbols) {
            if (s.id.compare(id, Qt::CaseInsensitive) == 0) { exists = true; break; }
        }
        if (exists)
            continue;
        if (m_symbols.size() >= MAX_SYMBOLS)
            break;
        Symbol s;
        s.id = id;
        m_symbols.append(s);
        changed = true;
    }
    if (!changed)
        return;
    persistWatchlist();
    emitChanged();
    refresh();
}

void TickerController::addSymbolsVariant(const QVariantList &symbols)
{
    QStringList list;
    list.reserve(symbols.size());
    for (const QVariant &v : symbols)
        list << v.toString();
    addSymbols(list);
}

bool TickerController::containsSymbol(const QString &symbol) const
{
    const QString id = cleanSymbol(symbol);
    for (const Symbol &s : m_symbols) {
        if (s.id.compare(id, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
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
    persistWatchlist();
    emit intervalMinutesChanged();
}

void TickerController::setCoverRows(int rows)
{
    rows = qBound(MIN_COVER_ROWS, rows, MAX_COVER_ROWS);
    if (rows == m_coverRows)
        return;
    m_coverRows = rows;
    persistWatchlist();
    emit coverRowsChanged();
}

void TickerController::setShowCoverTimestamp(bool v)
{
    if (v == m_showCoverTimestamp)
        return;
    m_showCoverTimestamp = v;
    persistWatchlist();
    emit showCoverTimestampChanged();
}

void TickerController::setShowCoverCurrency(bool v)
{
    if (v == m_showCoverCurrency)
        return;
    m_showCoverCurrency = v;
    persistWatchlist();
    emit showCoverCurrencyChanged();
}

void TickerController::setShowCoverPrice(bool v)
{
    if (v == m_showCoverPrice)
        return;
    m_showCoverPrice = v;
    persistWatchlist();
    emit showCoverPriceChanged();
}

void TickerController::setCoverScale(double v)
{
    v = qBound(0.7, v, 1.6);
    if (qFuzzyCompare(v, m_coverScale))
        return;
    m_coverScale = v;
    persistWatchlist();
    emit coverScaleChanged();
}

void TickerController::setFinnhubApiKey(const QString &key)
{
    const QString k = key.trimmed();
    if (k == m_finnhubApiKey)
        return;
    m_finnhubApiKey = k;
    persistWatchlist();
    emit finnhubApiKeyChanged();
    qInfo() << "controller: finnhub key" << (m_finnhubApiKey.isEmpty() ? "cleared" : "set");
}

void TickerController::setProviderMode(int mode)
{
    mode = qBound(0, mode, 3);
    if (mode == m_providerMode)
        return;
    m_providerMode = mode;
    persistWatchlist();
    emit providerModeChanged();
    qInfo() << "controller: providerMode" << m_providerMode << providerModeName();
}

QString TickerController::providerModeName() const
{
    switch (m_providerMode) {
    case 0: return QStringLiteral("Auto");
    case 1: return QStringLiteral("Yahoo");
    case 2: return QStringLiteral("Nordic");
    case 3: return QStringLiteral("Finnhub");
    default: return QStringLiteral("Auto");
    }
}

bool TickerController::shouldUseYahoo() const
{
    return m_providerMode == 0 || m_providerMode == 1;
}

bool TickerController::shouldUseNordic() const
{
    return m_providerMode == 0 || m_providerMode == 2;
}

bool TickerController::shouldUseFinnhub() const
{
    return (m_providerMode == 0 || m_providerMode == 3) && !m_finnhubApiKey.isEmpty();
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

bool TickerController::isNordicSymbol(const QString &symbol) const
{
    const QString s = symbol.toUpper();
    return s.endsWith(".HE") || s.endsWith(".ST") || s.endsWith(".CO") || s.endsWith(".OL")
            || s.startsWith("^OMX") || s.startsWith("^OMXH") || s.startsWith("^OMXS") || s.startsWith("^OMXC");
}

bool TickerController::needsNordic() const
{
    // No-key CDN is small and cacheable — fetch once per tick if any watchlist exists
    // so any Yahoo-miss (e.g. First North plain symbols like EASOR) can fallback
    return !m_symbols.isEmpty();
}

void TickerController::tick()
{
    if (m_refreshing || m_symbols.isEmpty())
        return;
    for (Symbol &s : m_symbols)
        s.pending = false;
    m_refreshing = true;
    emit refreshingChanged();
    m_cursor = 0;
    m_nordicCache.clear();
    if (shouldUseNordic() && needsNordic()) {
        fetchNordicSnapshot();
    } else {
        fetchNext();
    }
}

void TickerController::fetchNordicSnapshot()
{
    if (m_nordicFetching)
        return;
    m_nordicFetching = true;
    qInfo() << "controller: fetching Nordic snapshot cdn.opennordicstocks.net";
    QUrl url(QStringLiteral("https://cdn.opennordicstocks.net/data/latest.json"));
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
    // short timeout via attribute if needed
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        onNordicFinished(status, reply->readAll());
        reply->deleteLater();
    });
}

void TickerController::onNordicFinished(int httpStatus, const QByteArray &payload)
{
    m_nordicFetching = false;
    qInfo() << "controller: Nordic snapshot finished http=" << httpStatus << "bytes=" << payload.size();
    if (httpStatus == 200 && !payload.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        QJsonObject root = doc.object();
        QJsonArray stocks = root.value("stocks").toArray();
        // fallback if root itself is array
        if (stocks.isEmpty() && doc.isArray())
            stocks = doc.array();
        // some CDN variants put stocks directly
        if (stocks.isEmpty())
            stocks = root.value("data").toArray();
        qInfo() << "controller: Nordic stocks count" << stocks.size();
        for (const QJsonValue &v : stocks) {
            QJsonObject o = v.toObject();
            QVariantMap m = parseNordicStock(o);
            if (m.isEmpty())
                continue;
            const QString sym = m.value("symbol").toString().toUpper();
            m_nordicCache.insert(sym, m);
            // also insert stripped version without suffix for lookup
            QString base = sym;
            // store base without .HE/.ST etc if present? keep as is for now
            // also store without suffix
            int dot = base.lastIndexOf('.');
            if (dot > 0) {
                const QString stripped = base.left(dot);
                if (!m_nordicCache.contains(stripped))
                    m_nordicCache.insert(stripped, m);
            }
        }
        qInfo() << "controller: Nordic cache populated" << m_nordicCache.size() << "entries";
    } else {
        qWarning() << "controller: Nordic snapshot failed, continuing with Yahoo only";
    }
    fetchNext();
}

QVariantMap TickerController::parseNordicStock(const QJsonObject &obj) const
{
    // Expected: {symbol, name, price, market, currency, volume, change, changePercent, timestamp}
    const QString symbol = obj.value("symbol").toString();
    if (symbol.isEmpty())
        return {};
    double price = obj.value("price").toDouble(-1);
    if (price <= 0)
        price = obj.value("regularMarketPrice").toDouble(-1);
    if (price <= 0)
        return {};
    const QString currency = obj.value("currency").toString();
    const QString name = obj.value("name").toString(obj.value("longName").toString(symbol));
    QVariantMap m;
    m["symbol"] = symbol;
    m["name"] = name.isEmpty() ? symbol : name;
    m["price"] = fmtPrice(price, currency);
    m["priceValue"] = price;
    m["currency"] = currency;
    double change = obj.value("change").toDouble(0);
    // if change not present, try to compute from changePercent
    double pct = obj.value("changePercent").toDouble(0);
    if (obj.contains("changePercent") && pct == 0)
        pct = obj.value("change_percent").toDouble(0);
    // Yahoo style: change/changepct derived, but CDN may already have them
    if (pct == 0 && obj.contains("changePct"))
        pct = obj.value("changePct").toDouble(0);
    if (change != 0 || pct != 0) {
        QLocale locale(QLocale::English);
        const QString sign = change >= 0 ? "+" : "";
        // if change is 0 but pct non-zero, estimate change = price * pct/100
        if (change == 0 && pct != 0)
            change = price * pct / 100.0;
        if (pct == 0 && change != 0)
            pct = change / (price - change) * 100.0;
        m["change"] = sign + locale.toString(change, 'f', 2);
        m["pct"] = sign + locale.toString(pct, 'f', 2) + "%";
        m["up"] = change >= 0;
    } else {
        m["change"] = QString();
        m["pct"] = QString();
        m["up"] = true;
    }
    // optional fields
    long long vol = obj.value("volume").toVariant().toLongLong();
    if (vol > 0) {
        QLocale locale(QLocale::English);
        m["volume"] = locale.toString(vol);
    }
    return m;
}

QVariantMap TickerController::nordicLookup(const QString &symbol) const
{
    const QString id = symbol.toUpper();
    auto it = m_nordicCache.find(id);
    if (it != m_nordicCache.end())
        return it.value();
    // try stripped .HE/.ST etc
    int dot = id.lastIndexOf('.');
    if (dot > 0) {
        const QString stripped = id.left(dot);
        it = m_nordicCache.find(stripped);
        if (it != m_nordicCache.end())
            return it.value();
        // try with dash vs dot: Helsinki sometimes uses - vs .
        const QString dash = stripped + "-" + id.mid(dot+1);
        it = m_nordicCache.find(dash);
        if (it != m_nordicCache.end())
            return it.value();
    }
    // try with suffix added if plain
    if (!id.contains('.')) {
        for (auto suf : {".HE", ".ST", ".CO", ".OL"}) {
            it = m_nordicCache.find(id + suf);
            if (it != m_nordicCache.end())
                return it.value();
        }
    }
    return {};
}

void TickerController::fetchFinnhubQuote(const QString &symbol)
{
    if (m_finnhubApiKey.isEmpty()) {
        qWarning() << "controller: Finnhub key empty, cannot fetch" << symbol;
        // mark failure and continue
        for (Symbol &s : m_symbols) {
            if (s.id.compare(symbol, Qt::CaseInsensitive)==0) {
                s.failures++;
                break;
            }
        }
        fetchNext();
        return;
    }
    QUrl url(QStringLiteral("https://finnhub.io/api/v1/quote?symbol=%1&token=%2")
                  .arg(QString::fromLatin1(QUrl::toPercentEncoding(symbol)), m_finnhubApiKey));
    qInfo() << "controller: fetching Finnhub" << symbol;
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, symbol]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        onFinnhubFinished(symbol, status, reply->readAll());
        reply->deleteLater();
    });
}

void TickerController::fetchYahooVariant(const QString &original, const QString &variant)
{
    QUrl url(QStringLiteral("https://query1.finance.yahoo.com/v8/finance/chart/%1")
                  .arg(QString::fromLatin1(QUrl::toPercentEncoding(variant))));
    qInfo() << "controller: fetching Yahoo variant" << original << "->" << variant;
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, original, variant]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        onYahooVariantFinished(original, variant, status, reply->readAll());
        reply->deleteLater();
    });
}

void TickerController::onYahooVariantFinished(const QString &original, const QString &variant, int httpStatus, const QByteArray &payload)
{
    qInfo() << "controller: Yahoo variant finished" << original << "->" << variant << "http=" << httpStatus << payload.left(200);
    for (Symbol &s : m_symbols) {
        if (s.id.compare(original, Qt::CaseInsensitive)!=0)
            continue;
        s.pending = false;
        if (httpStatus == 200) {
            QVariantMap meta = parseMeta(payload);
            if (!meta.isEmpty()) {
                // keep original symbol id for display, but use variant data
                meta["symbol"] = s.id;
                s.data = meta;
                s.failures = 0;
                s.retryAfterMs = 0;
                qInfo() << "controller: Yahoo variant ok" << original << "->" << variant;
                break;
            }
        }
        // variant miss — fall through to Nordic/Finnhub chain
        bool isNordic = shouldUseNordic();
        if (isNordic) {
            QVariantMap nordic = nordicLookup(original);
            if (!nordic.isEmpty()) {
                nordic["symbol"] = s.id;
                s.data = nordic;
                s.failures = 0;
                s.retryAfterMs = 0;
                qInfo() << "controller: Nordic fallback after variant miss ok for" << original;
                break;
            }
        }
        if (shouldUseFinnhub()) {
            qInfo() << "controller: Yahoo variant miss for" << original << "trying Finnhub";
            s.pending = true;
            fetchFinnhubQuote(original);
            return;
        }
        s.failures++;
        const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 3)), MAX_BACKOFF_MS);
        s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
        break;
    }
    fetchNext();
}

QVariantMap TickerController::parseFinnhubQuote(const QByteArray &payload, const QString &symbol) const
{
    QJsonObject o = QJsonDocument::fromJson(payload).object();
    // Finnhub quote: {c: current, d: change, dp: percent, h, l, o, pc, t}
    double price = o.value("c").toDouble(-1);
    if (price <= 0)
        return {};
    double change = o.value("d").toDouble(0);
    double pct = o.value("dp").toDouble(0);
    double prev = o.value("pc").toDouble(0);
    // pct may be already percent, change absolute
    QVariantMap m;
    m["symbol"] = symbol.toUpper();
    m["name"] = symbol.toUpper();
    // Finnhub doesn't give currency directly; assume USD for US, EUR for .HE etc.
    QString currency;
    if (symbol.toUpper().endsWith(".HE") || symbol.toUpper().endsWith(".ST") || symbol.toUpper().endsWith(".CO") || symbol.toUpper().endsWith(".OL"))
        currency = QStringLiteral("EUR");
    else if (symbol.contains("-USD") || symbol.endsWith("=X"))
        currency = QString();
    else
        currency = QStringLiteral("USD");
    // try to keep currency empty for FX/crypto
    if (symbol.endsWith("=X") || symbol.contains("-USD"))
        currency = QString();
    m["currency"] = currency;
    m["price"] = fmtPrice(price, currency);
    m["priceValue"] = price;
    if (prev <= 0 && pct != 0 && change != 0) {
        prev = price - change;
    }
    if (pct != 0 || change != 0) {
        QLocale locale(QLocale::English);
        const QString sign = change >= 0 ? "+" : "";
        // if pct is 0 but change known, compute pct
        if (pct == 0 && prev > 0)
            pct = change / prev * 100.0;
        if (change == 0 && pct != 0)
            change = price * pct / 100.0;
        m["change"] = sign + locale.toString(change, 'f', 2);
        m["pct"] = sign + locale.toString(pct, 'f', 2) + "%";
        m["up"] = change >= 0;
    } else {
        m["change"] = QString();
        m["pct"] = QString();
        m["up"] = true;
    }
    return m;
}

void TickerController::onFinnhubFinished(const QString &symbol, int httpStatus, const QByteArray &payload)
{
    qInfo() << "controller: Finnhub finished" << symbol << "http=" << httpStatus << "bytes=" << payload.size() << payload.left(200);
    for (Symbol &s : m_symbols) {
        if (s.id.compare(symbol, Qt::CaseInsensitive)!=0)
            continue;
        s.pending = false;
        if (httpStatus == 200) {
            QVariantMap m = parseFinnhubQuote(payload, symbol);
            if (!m.isEmpty() && m.value("priceValue").toDouble(0) > 0) {
                // Finnhub returns 0 price for invalid symbols, treat as miss
                s.data = m;
                s.failures = 0;
                s.retryAfterMs = 0;
                qInfo() << "controller: Finnhub ok for" << symbol << m.value("price").toString();
            } else {
                qInfo() << "controller: Finnhub empty for" << symbol;
                s.failures++;
                const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 3)), MAX_BACKOFF_MS);
                s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
            }
        } else if (httpStatus == 429) {
            s.failures++;
            const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 5)), MAX_BACKOFF_MS);
            s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
        } else {
            s.failures++;
            const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 3)), MAX_BACKOFF_MS);
            s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
        }
        break;
    }
    fetchNext();
}

void TickerController::fetchNext()
{
    while (m_cursor < m_symbols.size()) {
        Symbol &s = m_symbols[m_cursor];
        if (s.data.value("price").toString().isEmpty() && s.retryAfterMs > QDateTime::currentMSecsSinceEpoch()) {
            // Plain Helsinki symbols like EASOR can be retried as EASOR.HE even in backoff
            bool plainRetry = !s.id.contains('.') && !s.id.startsWith("^") && shouldUseYahoo();
            if (!plainRetry) {
                qInfo() << "controller: skipping" << s.id << "in backoff until" << s.retryAfterMs;
                ++m_cursor;
                continue;
            } else {
                qInfo() << "controller: backoff but plain" << s.id << "will retry as .HE variant";
            }
        }
        s.pending = true;
        const QString symbol = s.id;
        // ProviderMode handling: if Yahoo disabled, try Nordic/Finnhub directly
        if (!shouldUseYahoo()) {
            if (shouldUseNordic()) {
                QVariantMap nordic = nordicLookup(symbol);
                if (!nordic.isEmpty()) {
                    qInfo() << "controller: Nordic direct hit for" << symbol;
                    nordic["symbol"] = s.id;
                    s.data = nordic;
                    s.failures = 0;
                    s.retryAfterMs = 0;
                    s.pending = false;
                    ++m_cursor;
                    continue;
                }
            }
            if (shouldUseFinnhub()) {
                qInfo() << "controller: fetching Finnhub direct for" << symbol << "cursor" << m_cursor << "of" << m_symbols.size();
                ++m_cursor;
                fetchFinnhubQuote(symbol);
                return;
            } else {
                qInfo() << "controller: no provider for" << symbol << "mode" << providerModeName();
                s.pending = false;
                s.failures++;
                ++m_cursor;
                continue;
            }
        }
        qInfo() << "controller: fetching Yahoo" << symbol << "cursor" << m_cursor << "of" << m_symbols.size() << "mode" << providerModeName();
        QUrl url(QStringLiteral("https://query1.finance.yahoo.com/v8/finance/chart/%1")
                      .arg(QString::fromLatin1(QUrl::toPercentEncoding(s.id))));
        QNetworkRequest req(url);
        req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");
        QNetworkReply *reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, symbol]() {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            onFetchFinished(symbol, status, reply->readAll());
            reply->deleteLater();
        });
        ++m_cursor;
        return;
    }
    finishTick();
}

void TickerController::onFetchFinished(const QString &symbol, int httpStatus, const QByteArray &payload)
{
    qInfo() << "controller: fetch finished" << symbol << "http=" << httpStatus
            << "bytes=" << payload.size() << "payload head:" << payload.left(200);
    for (Symbol &s : m_symbols) {
        if (s.id.compare(symbol, Qt::CaseInsensitive) != 0)
            continue;
        s.pending = false;
        bool yahooOk = false;
        if (httpStatus == 200) {
            QVariantMap meta = parseMeta(payload);
            if (!meta.isEmpty()) {
                s.data = meta;
                s.failures = 0;
                s.retryAfterMs = 0;
                yahooOk = true;
                qInfo() << "controller: Yahoo ok for" << symbol;
            }
        }
        if (!yahooOk && shouldUseYahoo() && !symbol.contains('.') && !symbol.startsWith("^")) {
            // Plain Helsinki First North symbol like EASOR → try EASOR.HE on Yahoo (free, covers Helsinki including First North)
            QString variant = symbol + ".HE";
            qInfo() << "controller: Yahoo miss for plain" << symbol << "trying variant" << variant;
            s.pending = true;
            fetchYahooVariant(symbol, variant);
            return;
        }
        if (!yahooOk && shouldUseNordic()) {
            // try Nordic fallback for any symbol (no-key CDN covers Helsinki/Stockholm/Copenhagen)
            QVariantMap nordic = nordicLookup(symbol);
            if (!nordic.isEmpty()) {
                nordic["symbol"] = s.id;
                s.data = nordic;
                s.failures = 0;
                s.retryAfterMs = 0;
                qInfo() << "controller: Nordic fallback ok for" << symbol << "price" << nordic.value("price").toString();
                yahooOk = true;
            } else if (!m_nordicCache.isEmpty()) {
                qInfo() << "controller: Nordic fallback missed for" << symbol << "cache size" << m_nordicCache.size();
            }
        }
        if (!yahooOk && shouldUseFinnhub()) {
            qInfo() << "controller: Yahoo+Nordic miss for" << symbol << "trying Finnhub";
            s.pending = true;
            fetchFinnhubQuote(symbol);
            return;
        }
        if (!yahooOk) {
            if (httpStatus == 429 || httpStatus == 401) {
                s.failures++;
                const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 5)), MAX_BACKOFF_MS);
                s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
                qInfo() << "controller: backoff for" << symbol << "429/401";
            } else if (httpStatus == 200) {
                // Yahoo 200 but empty and Nordic missed
                s.failures++;
                const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 5)), MAX_BACKOFF_MS);
                s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
            } else {
                s.failures++;
                // for 404 etc, also backoff but shorter? keep same
                const int backoff = qMin(BASE_BACKOFF_MS * (1 << qMin(s.failures, 3)), MAX_BACKOFF_MS);
                s.retryAfterMs = QDateTime::currentMSecsSinceEpoch() + backoff;
            }
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
    const QString snapPath = dataPath(SNAPSHOT_FILE);
    qInfo() << "controller: finishTick, writing snapshot to" << snapPath;
    QFile f(snapPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonArray arr;
        for (const Symbol &s : m_symbols) {
            if (s.data.isEmpty())
                continue;
            arr.append(QJsonDocument::fromVariant(s.data).object());
        }
        f.write(QJsonDocument(arr).toJson());
        qInfo() << "controller: snapshot written OK";
    } else {
        qWarning() << "controller: FAILED to open snapshot file" << snapPath;
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
