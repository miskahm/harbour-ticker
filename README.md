# harbour-ticker

Native SailfishOS app whose **cover** shows live stock and index tickers.
Data from Yahoo Finance (keyless `v8/finance/chart`), editable watchlist,
configurable refresh interval (1–30 min, default 5).

Default watchlist: `^GSPC`, `^NDX`, `^DJI`, `AAPL`, `MSFT`.

## Build

Requires the SailfishOS platform SDK docker image and `mb2`:

```sh
podman pull coderus/sailfishos-platform-sdk
# mb2 (installed on host or via pip) pointed at the container:
mb2 build
```

The RPM lands in `.mb2/build/` as `harbour-ticker-*.rpm`.

## Install

```sh
sfdk deploy -e harbour-ticker-0.1.0-1.aarch64.rpm
```

or copy the RPM to the phone and install with an RPM manager.

## Notes

- Cover rows update while the app is resident (SailfishOS cover semantics).
- Watchlist and last snapshot persist under the app data dir; edits survive restarts.
- 429/401 responses trigger exponential backoff per symbol.
- Distribution target: OpenRepos.
