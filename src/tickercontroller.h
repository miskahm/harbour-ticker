#ifndef TICKERCONTROLLER_H
#define TICKERCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QElapsedTimer>

class QNetworkAccessManager;
class QTimer;

class TickerController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList tickers READ tickers NOTIFY tickersChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY tickersChanged)
    Q_PROPERTY(bool refreshing READ refreshing NOTIFY refreshingChanged)
    Q_PROPERTY(int intervalMinutes READ intervalMinutes WRITE setIntervalMinutes NOTIFY intervalMinutesChanged)

public:
    explicit TickerController(QObject *parent = nullptr);
    ~TickerController();

    QVariantList tickers() const { return m_tickers; }
    QString lastUpdated() const { return m_lastUpdated; }
    bool refreshing() const { return m_refreshing; }
    int intervalMinutes() const { return m_intervalMinutes; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void addSymbol(const QString &symbol);
    Q_INVOKABLE void removeSymbol(const QString &symbol);
    Q_INVOKABLE void moveSymbol(int from, int to);
    Q_INVOKABLE void setIntervalMinutes(int minutes);

signals:
    void tickersChanged();
    void refreshingChanged();
    void intervalMinutesChanged();

private slots:
    void pollDue();
    void onFetchFinished(const QString &symbol, int httpStatus, const QByteArray &payload);

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
    void finishTick();
    QVariantMap parseMeta(const QByteArray &payload) const;
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
    int m_intervalMinutes = 5;
    QString m_lastUpdated;
    QVariantList m_tickers;
};

#endif
