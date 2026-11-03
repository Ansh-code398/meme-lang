// Benchmark 2: CSV Processing & Financial Calculations in JavaScript (Node/Bun)
const fs = require('fs');

const start = performance.now();

const content = fs.readFileSync('benchmarks/data/sales_records.csv', 'utf8');
const lines = content.split('\n');
const headers = lines[0].trim().split(',');

let highValueOrders = 0;
let totalNetRevenue = 0.0;
const regionalRevenue = {};
let rowCount = 0;

for (let i = 1; i < lines.length; i++) {
    const line = lines[i].trim();
    if (!line) continue;
    rowCount++;
    const parts = line.split(',');
    // headers: order_id,customer_id,region,product,units_sold,unit_price,discount
    const reg = parts[2];
    const units = parseFloat(parts[4]);
    const price = parseFloat(parts[5]);
    const disc = parseFloat(parts[6]);
    const net = units * price * (1.0 - disc);

    totalNetRevenue += net;

    if (net > 500.0) {
        highValueOrders++;
        if (!regionalRevenue[reg]) regionalRevenue[reg] = 0.0;
        regionalRevenue[reg] += net;
    }
}

const elapsed = performance.now() - start;
const runtime = typeof Bun !== 'undefined' ? 'Bun      ' : 'Node.js  ';
console.log(`${runtime}| Rows: ${rowCount} | HighValue: ${highValueOrders} | NetRev: ${Math.floor(totalNetRevenue)} | Regions: ${Object.keys(regionalRevenue).length} | Time: ${elapsed.toFixed(2)}ms`);
