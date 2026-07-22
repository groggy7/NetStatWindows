# Privacy statement

NetStatBar operates entirely on the local Windows device.

## Data read

At the selected update interval, NetStatBar reads Windows' cumulative incoming and outgoing octet counters for the network interfaces selected by the user. It also reads local taskbar geometry, display scaling, theme, high-contrast state, power/session events, and its own per-user settings.

## Data not collected

NetStatBar does not:

- Capture packets.
- Inspect addresses, hostnames, protocols, payloads, or application traffic.
- Associate traffic with individual processes or users.
- Save network-rate history.
- Send network requests.
- Include analytics, advertising, crash upload, telemetry, or remote configuration.
- Require an account, administrator access, a service, or a driver.

## Local storage

The app stores only configuration values under:

```text
HKEY_CURRENT_USER\Software\NetStatBar
```

The unpackaged build may also create this opt-in startup value:

```text
HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run\NetStatBar
```

Disabling **Start at sign-in** removes that unpackaged startup value. Resetting settings replaces the stored values with validated defaults. An MSIX uninstall removes package-managed files; per-user registry settings outside package-managed storage may need to be removed manually if Windows does not virtualize them for the package.

## Interface totals

The physical mode attempts to include active hardware Wi-Fi and Ethernet interfaces. The all-active mode includes VPN and tunnel interfaces as well. Because the same bytes can cross a physical adapter and a tunnel, all-active totals can double-count VPN traffic. This behavior does not imply that traffic contents are being inspected.
