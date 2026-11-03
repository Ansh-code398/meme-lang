
const http = require('http');
const server = http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ status: 'ok', user: 'Raju', role: 'admin' }));
});
server.listen(19877, '127.0.0.1');
