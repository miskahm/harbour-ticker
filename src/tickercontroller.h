#ifndef TICKERCONTROLLER_H
#define TICKERCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class QNetworkAccessManager;
class QTimer;

class TickerController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList tickers READ tickers NOTIFY tickersChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY tickersChanged)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshingChanged)
    Q_PROPERTY(int intervalMinutes READ intervalMinutes WRITE setIntervalMinutes NOTIFY intervalMinutesChanged)
    Q_PROPERTY(int coverRows READ coverRows WRITE setCoverRows NOTIFY coverRowsChanged)
    Q_PROPERTY(bool showCoverTimestamp READ showCoverTimestamp WRITE setShowCoverTimestamp NOTIFY showCoverTimestampChanged)
    Q_PROPERTY(bool showCoverCurrency READ showCoverCurrency WRITE setShowCoverCurrency NOTIFY showCoverCurrencyChanged)
    Q_PROPERTY(bool showCoverPrice READ showCoverPrice WRITE setShowCoverPrice NOTIFY showCoverPriceChanged)
    Q_PROPERTY(double coverScale READ coverScale WRITE setCoverScale NOTIFY coverScaleChanged)

public:
    explicit TickerController(QObject *parent = nullptr);
    ~TickerController();

    QVariantList tickers() const { return m_tickers; }
    QString lastUpdated() const { return m_lastUpdated; }
    bool refreshing() const { return m_refreshing; }
    int intervalMinutes() const { return m_intervalMinutes; }
    int coverRows() const { return m_coverRows; }
    bool showCoverTimestamp() const { return m_showCoverTimestamp; }
    bool showCoverCurrency() const { return m_showCoverCurrency; }
    bool showCoverPrice() const { return m_showCoverPrice; }
    double coverScale() const { return m_coverScale; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void addSymbol(const QString &symbol);
    Q_INVOKABLE void addSymbols(const QStringList &symbols);
    Q_INVOKABLE void addSymbolsVariant(const QVariantList &symbols);
    Q_INVOKABLE void removeSymbol(const QString &symbol);
    Q_INVOKABLE void moveSymbol(int from, int to);
    Q_INVOKABLE void setIntervalMinutes(int minutes);
    Q_INVOKABLE void setCoverRows(int rows);
    Q_INVOKABLE void setShowCoverTimestamp(bool v);
    Q_INVOKABLE void setShowCoverCurrency(bool v);
    Q_INVOKABLE void setShowCoverPrice(bool v);
    Q_INVOKABLE void setCoverScale(double v);
    Q_INVOKABLE bool containsSymbol(const QString &symbol) const;

signals:
    void tickersChanged();
    void refreshingChanged();
    void intervalMinutesChanged();
    void coverRowsChanged();
    void showCoverTimestampChanged();
    void showCoverCurrencyChanged();
    void showCoverPriceChanged();
    void coverScaleChanged();

private slots:
    void pollDue();
    void onFetchFinished(const QString &symbol, int httpStatus, const QByteArray &payload);
    void onNordicFinished(int httpStatus, const QByteArray &payload);

private:
    struct Symbol {
        QString id;
        QVariantMap data;
        bool pending = false;
        int failures = 0;
        qint64 retryAfterMs = 0;
    };

    void tick();
    void fetchNext();
    void fetchNordicSnapshot();
    void finishTick();
    QVariantMap parseMeta(const QByteArray &payload) const;
    QVariantMap parseNordicStock(const QJsonObject &obj) const;
    QVariantMap nordicLookup(const QString &symbol) const;
    bool isNordicSymbol(const QString &symbol) const;
    bool needsNordic() const;
    QVariantList buildRows() const;
    void persistWatchlist() const;
    void loadState();
    void emitChanged();
    static QString cleanSymbol(const QString &raw);

    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_timer = nullptr;
    QVector<Symbol> m_symbols;
    int m_cursor = 0;
    bool m_refreshing = false;
    bool m_nordicFetching = false;
    int m_intervalMinutes = 5;
    int m_coverRows = 5;
    bool m_showCoverTimestamp = true;
    bool m_showCoverCurrency = false;
    bool m_showCoverPrice = true;
    double m_coverScale = 1.0;
    QString m_lastUpdated;
    QVariantList m_tickers;
    QMap<QString, QVariantMap> m_nordicCache;
};

#endif
