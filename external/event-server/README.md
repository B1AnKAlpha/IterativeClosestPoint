# Incident Event Server

A lightweight Socket.IO event stream for the WidgetQuadtree incident monitoring page. Provides:

- `/publish` REST endpoint for producers to push incident payloads
- WebSocket channel broadcasting `incident` events and `updated` acknowledgements
- Optional bootstrap of recent events when a client connects

## Usage

```bash
cd external/event-server
npm install
npm start
```

Default port: `4000`. Override with `PORT=5000 npm start`.

Payload format example when calling `/publish`:

```json
{
  "level": "高",
  "vehicleId": "A231",
  "message": "持续超速超过 15km/h",
  "timestamp": "2025-10-11T01:30:00Z"
}
```

Any omitted fields will be auto-filled.
