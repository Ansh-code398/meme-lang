
Bun.serve({
    port: 19878,
    hostname: '127.0.0.1',
    fetch(req) {
        return Response.json({ status: 'ok', user: 'Raju', role: 'admin' });
    }
});
