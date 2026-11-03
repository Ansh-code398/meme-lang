// Benchmark 1: JSON Data Processing & Aggregation in JavaScript (Node/Bun)
const fs = require('fs');

const start = performance.now();

const raw = fs.readFileSync('benchmarks/data/transactions.json', 'utf8');
const txns = JSON.parse(raw);

let completedCount = 0;
let totalRevenue = 0.0;
const categoryTotals = {};
const categoryCounts = {};

for (const t of txns) {
    if (t.status === 'completed') {
        completedCount++;
        const amt = t.amount;
        totalRevenue += amt;
        const cat = t.category;
        if (!categoryTotals[cat]) {
            categoryTotals[cat] = 0.0;
            categoryCounts[cat] = 0;
        }
        categoryTotals[cat] += amt;
        categoryCounts[cat]++;
    }
}

const elapsed = performance.now() - start;
const runtime = typeof Bun !== 'undefined' ? 'Bun      ' : 'Node.js  ';
console.log(`${runtime}| Completed: ${completedCount} | Revenue: ${Math.floor(totalRevenue)} | Categories: ${Object.keys(categoryTotals).length} | Time: ${elapsed.toFixed(2)}ms`);
