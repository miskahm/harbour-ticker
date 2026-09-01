# Build & Deploy (developers)

Requires the **coderus/sailfishos-platform-sdk** podman image (SailfishOS 5.2.0.15).

> `mb2` (sb2) only sees bind-mounts at `/home/mersdk/project`.

```sh
# 1. Give the container user ownership and clean old artifacts
podman run --rm -u root -v "$PWD":/home/mersdk/project \
  coderus/sailfishos-platform-sdk \
  bash -c "chown -R 100000:100000 /home/mersdk/project && rm -rf /home/mersdk/project/.mb2 /home/mersdk/project/RPMS"

# 2. Build as the container user
podman run --rm -v "$PWD":/home/mersdk/project -w /home/mersdk/project \
  coderus/sailfishos-platform-sdk \
  bash -c "mb2 -t SailfishOS-5.2.0.15-aarch64 build-init && mb2 -t SailfishOS-5.2.0.15-aarch64 build"

# 3. Restore host ownership
podman run --rm -u root -v "$PWD":/home/mersdk/project \
  coderus/sailfishos-platform-sdk \
  chown -R 0:0 /home/mersdk/project
```

The RPM lands in `RPMS/harbour-ticker-*.rpm`.

## Deploy to device

```sh
ssh user@<device-ip> "pkill -f bin/harbour-ticker" 2>/dev/null
scp RPMS/harbour-ticker-*.rpm user@<device-ip>:/tmp/
ssh -tt user@<device-ip> "devel-su rpm -Uvh /tmp/harbour-ticker-*.rpm"
```

## Persistence & internals

- `watchlist.json` (`intervalMinutes`, `coverRows`, `showCoverTimestamp`, `showCoverCurrency`, `showCoverPrice`, `coverScale`, `symbols[]`) and `snapshot.json` under `~/.local/share/harbour-ticker/harbour-ticker/`.
- Cover layout: 30px × scale row height, `paddingSmall/Medium` margins, headerless dense layout.
- 429/401 → exponential backoff per symbol; `TickerController` is a single context property `ticker`.
