# harbour-ticker

Native SailfishOS app whose **cover** shows live stock and index tickers on the
Home area.  Data from Yahoo Finance (keyless `v8/finance/chart` endpoint),
editable watchlist, configurable refresh interval and cover row count.

Default watchlist: `^GSPC`, `^NDX`, `^DJI`, `AAPL`, `MSFT`.

## Settings

| Setting | Range | Default | Notes |
|---------|-------|---------|-------|
| Refresh interval | 1–30 min | 5 | How often Yahoo is polled while the app runs or its cover is active |
| Cover rows | 1–10 | 5 | How many tickers appear on the Home screen cover |
| Show timestamp | on/off | on | Last updated time at bottom of cover |
| Show currency | on/off | off | e.g. `USD` suffix next to price |
| Show price | on/off | on | Off = % only, more room for symbols |
| Cover scale | 70–160% | 100% | Row height + font size on cover (Slider, step 10%) |

All persist in `watchlist.json` and survive restarts.

## Watchlist

- Pull-down **Add symbol** for free-text `AAPL`, `^GSPC`, `EURUSD=X`, `BTC-USD`.
- Pull-down **Browse tickers** for a curated 45-symbol picker (indices, Tech, Helsinki .HE, ETFs, FX, Crypto, Commodities) with search + multi-select. Already in watchlist is dimmed.

## Build

Requires the **coderus/sailfishos-platform-sdk** podman image.

> **Important:** `mb2` (sb2) can only see bind-mounts at `/home/mersdk/project`
> inside the container.  `$PWD` maps there.

```sh
# Restore container-root ownership for host access
podman run --rm -u root -v "$PWD":/home/mersdk/project \
  coderus/sailfishos-platform-sdk \
  bash -c "chown -R 100000:100000 /home/mersdk/project && rm -rf /home/mersdk/project/.mb2 /home/mersdk/project/RPMS"

# Build as the container user
podman run --rm -v "$PWD":/home/mersdk/project -w /home/mersdk/project \
  coderus/sailfishos-platform-sdk \
  bash -c "mb2 -t SailfishOS-5.2.0.15-aarch64 build-init && mb2 -t SailfishOS-5.2.0.15-aarch64 build"

# Restore host ownership
podman run --rm -u root -v "$PWD":/home/mersdk/project \
  coderus/sailfishos-platform-sdk \
  chown -R 0:0 /home/mersdk/project
```

The RPM lands in `dist/` as `harbour-ticker-*.rpm`.

## Deploy

```sh
ssh user@device-ip "pkill -f bin/harbour-ticker" 2>/dev/null
scp dist/harbour-ticker-*.rpm user@device-ip:/tmp/
printf 'password\n' | ssh -tt user@device-ip \
  "devel-su rpm -Uvh /tmp/harbour-ticker-*.rpm"
```

Replace `password` with the `devel-su` password for the device.

## Notes

- **Cover layout:** no header (freed for tickers), 30px × scale row height, `paddingSmall/Medium` margins — 10 rows fit; timestamp anchored bottom, currency toggle, price-only toggle, scale slider.
- **Persistence:** `watchlist.json` (JSON object with `intervalMinutes`,
  `coverRows`, `showCoverTimestamp`, `showCoverCurrency`, `showCoverPrice`, `coverScale`, `symbols[]`) and `snapshot.json` live under
  `~/.local/share/harbour-ticker/harbour-ticker/`.
- 429/401 responses trigger exponential backoff per symbol.
- Distribution target: OpenRepos.
