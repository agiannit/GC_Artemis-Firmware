# Flight Radar (work in progress)

Development branch: `feature/flight-radar`, based on `feature/bluetooth-scanner`.

## Stage 1: watch UI scaffold
- Menu entry and LVGL screen for the 128 x 128 display.
- Alt returns to the main menu.
- No Wi-Fi, HTTP, RTC or background tasks yet.
- This is a placeholder screen, not a functioning aircraft tracker.

## Next steps
1. Choose aircraft data source and geographic reference (GPS, configured coordinates, or phone).
2. Implement a dedicated on-demand Wi-Fi station session, separate from the Wi-Fi scanner.
3. Fetch a small test payload using bounded timeouts; record internal free heap before/after Wi-Fi and HTTPS.
4. Stop and deinitialize Wi-Fi on exit, even after errors; test repeated entry/exit and sleep.
5. Parse only the fields needed for a compact aircraft list: callsign, distance, altitude, heading.
6. Add aircraft detail view and refresh controls.

## Guardrails
- Never update the RTC or system time from this app.
- Never connect automatically at watch startup.
- Avoid simultaneous BLE scanning and Wi-Fi connection where possible.
- Do not commit credentials or API tokens.
- Test sleep/wake before considering the feature stable.
