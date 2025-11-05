import express from 'express';
import http from 'http';
import cors from 'cors';
import { WebSocketServer } from 'ws';

const app = express();
const server = http.createServer(app);
const wss = new WebSocketServer({ server, path: '/stream' });

app.use(cors());
app.use(express.json());

const incidents = [];

app.get('/health', (req, res) => {
    res.json({ status: 'ok', incidents: incidents.length });
});

app.post('/publish', (req, res) => {
    const incident = buildIncident(req.body);
    incidents.unshift(incident);
    trimIncidents();
    broadcast({ type: 'incident', payload: incident });
    res.status(201).json(incident);
});

wss.on('connection', (socket) => {
    console.log('client connected');
    socket.send(JSON.stringify({ type: 'bootstrap', payload: incidents.slice(0, 20) }));

    socket.on('message', (data) => {
        try {
            const message = JSON.parse(data.toString());
            if (message.type === 'ack' && message.id) {
                const match = incidents.find((item) => item.id === message.id);
                if (match) {
                    match.status = 'resolved';
                    broadcast({ type: 'update', payload: match });
                }
            }
        } catch (err) {
            console.error('invalid message', err);
        }
    });

    socket.on('close', () => {
        console.log('client disconnected');
    });
});

function broadcast(message) {
    const data = JSON.stringify(message);
    wss.clients.forEach((client) => {
        if (client.readyState === 1) {
            client.send(data);
        }
    });
}

function buildIncident(payload = {}) {
    const now = new Date();
    return {
        id: payload.id || `${now.getTime()}`,
        level: payload.level || randomChoice(['高', '中', '低']),
        vehicleId: payload.vehicleId || randomChoice(['A231', 'H552', 'L878', 'M104']),
        message: payload.message || randomChoice([
            '持续超速超过 15km/h',
            '偏离指定区域 350m',
            '通信延迟异常',
            '燃油不足 10%'
        ]),
        timestamp: payload.timestamp || now.toISOString(),
        status: payload.status || 'pending'
    };
}

function trimIncidents() {
    while (incidents.length > 50) {
        incidents.pop();
    }
}

function randomChoice(list) {
    return list[Math.floor(Math.random() * list.length)];
}

const PORT = process.env.PORT || 4000;
server.listen(PORT, () => {
    console.log(`Incident event server listening on http://localhost:${PORT}`);
});
