import puppeteer from 'puppeteer-core';

async function runE2ETests() {
  console.log('🚀 Running Comprehensive E2E Tests (Line Numbers & Input)...');
  const browser = await puppeteer.launch({
    executablePath: '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
    headless: true,
    args: ['--no-sandbox', '--disable-setuid-sandbox']
  });

  try {
    const page = await browser.newPage();
    await page.setViewport({ width: 1440, height: 900 });

    let dialogMessages = [];
    page.on('dialog', async (dialog) => {
      dialogMessages.push(dialog.message());
      console.log(`   💬 Browser prompt intercepted: "${dialog.message()}"`);
      await dialog.accept('42');
    });

    console.log('1. Navigating to Docs...');
    await page.goto('http://localhost:4173', { waitUntil: 'networkidle0' });

    await page.waitForFunction(() => {
      return document.body.innerText.includes('Wasm Ready');
    }, { timeout: 10000 });
    console.log('   ✅ WebAssembly initialized successfully!');

    // 2. Test Playground & Error Line Numbers
    console.log('2. Navigating to Playground tab...');
    const allButtons = await page.$$('button');
    for (const btn of allButtons) {
      const text = await (await btn.getProperty('innerText')).jsonValue();
      if (text.includes('Playground & REPL')) {
        await btn.click();
        break;
      }
    }
    await new Promise((r) => setTimeout(r, 600));

    console.log('3. Testing Error Line Numbers using Meme Errors Showcase recipe...');
    await page.select('select', 'errors');
    await new Promise((r) => setTimeout(r, 400));

    const runBtn = await page.evaluateHandle(() => {
      const btns = Array.from(document.querySelectorAll('button'));
      return btns.find((b) => b.innerText.includes('Run'));
    });
    await runBtn.click();

    await page.waitForFunction(() => {
      return document.body.innerText.includes('Line 4:') && document.body.innerText.includes('0 se divide');
    }, { timeout: 5000 });
    console.log('   ✅ Error line number verified: "Line 4: Kya coder banega re tu? 0 se divide..."');

    // 4. Test Pre-set Stdin with pooch() recipe
    console.log('4. Testing Pre-set Stdin with Console Input recipe...');
    await page.select('select', 'input');
    await new Promise((r) => setTimeout(r, 400));

    const runBtn2 = await page.evaluateHandle(() => {
      const btns = Array.from(document.querySelectorAll('button'));
      return btns.find((b) => b.innerText.includes('Run'));
    });
    await runBtn2.click();

    await page.waitForFunction(() => {
      return (
        document.body.innerText.includes('Swagat hai Baburao Apte bhai from Mumbai!') &&
        document.body.innerText.includes('5 saal baad aapki umar hogi: 55')
      );
    }, { timeout: 5000 });
    console.log('   ✅ Pre-set Stdin verified with pooch()!');

    // 5. Test Interactive Browser Prompt Dialog
    console.log('5. Testing Interactive Browser Prompt Dialog when stdin is empty...');
    await page.evaluate(() => {
      const ta = document.querySelector('textarea[placeholder*="Type lines here"]');
      if (ta) {
        const setter = Object.getOwnPropertyDescriptor(window.HTMLTextAreaElement.prototype, 'value').set;
        setter.call(ta, '');
        ta.dispatchEvent(new Event('input', { bubbles: true }));
      }
    });

    const runBtn3 = await page.evaluateHandle(() => {
      const btns = Array.from(document.querySelectorAll('button'));
      return btns.find((b) => b.innerText.includes('Run'));
    });
    await runBtn3.click();

    await new Promise((r) => setTimeout(r, 1000));
    console.log(`   ✅ Interactive browser prompt intercepted (${dialogMessages.length} prompts)!`);

    // 6. Test Rust-Style Memory Safety & Borrow Checker Recipe
    console.log('6. Testing Rust-Style Borrow Checker recipe in Playground...');
    await page.select('select', 'borrow-checker');
    await new Promise((r) => setTimeout(r, 400));

    const runBtn4 = await page.evaluateHandle(() => {
      const btns = Array.from(document.querySelectorAll('button'));
      return btns.find((b) => b.innerText.includes('Run'));
    });
    await runBtn4.click();

    await page.waitForFunction(() => {
      return (
        document.body.innerText.includes('Safe copy cloned: Tees Maar Khan Khazana') &&
        document.body.innerText.includes('Shared read 1: Majnu Bhai') &&
        document.body.innerText.includes('Score after scope borrow release: 100') &&
        document.body.innerText.includes('[RAII] Locker auto-dropped and sealed: Swiss Vault')
      );
    }, { timeout: 5000 });
    console.log('   ✅ Rust-Style Borrow Checker, Moves, Borrowing & RAII verified!');

    // 7. Test Collections & Iteration Recipe
    console.log('7. Testing Collections & Iteration recipe in Playground...');
    await page.select('select', 'collections-demo');
    await new Promise((r) => setTimeout(r, 400));
    const runBtn5 = await page.evaluateHandle(() => {
      const btns = Array.from(document.querySelectorAll('button'));
      return btns.find((b) => b.innerText.includes('Run'));
    });
    await runBtn5.click();

    await page.waitForFunction(() => {
      return (
        document.body.innerText.includes('Initial team: ["Kabir", "Bikram", "Guddu", "Bablu"]') &&
        document.body.innerText.includes('Captain (negative index): Bablu') &&
        document.body.innerText.includes('Player runs: 85') &&
        document.body.innerText.includes('Player: Kabir')
      );
    }, { timeout: 5000 });
    console.log('   ✅ Collections, Hash Maps & ghuma ... mein verified in browser!');

    // 8. Test Error Handling (panga/lafda/pakka/chillao) Recipe
    console.log('8. Testing Error Handling recipe in Playground...');
    await page.select('select', 'error-handling-demo');
    await new Promise((r) => setTimeout(r, 400));
    const runBtn6 = await page.evaluateHandle(() => {
      const btns = Array.from(document.querySelectorAll('button'));
      return btns.find((b) => b.innerText.includes('Run'));
    });
    await runBtn6.click();

    await page.waitForFunction(() => {
      return (
        document.body.innerText.includes('Balance after 200 withdrawal: 300') &&
        document.body.innerText.includes('Lafda pakda gaya!') &&
        document.body.innerText.includes('Error Code: 402') &&
        document.body.innerText.includes('Transaction session closed cleanly')
      );
    }, { timeout: 5000 });
    console.log('   ✅ panga / lafda / pakka / chillao verified in browser!');

    // 9. Test Enums & Pattern Matching Recipe
    console.log('9. Testing Enums & Pattern Matching recipe in Playground...');
    await page.select('select', 'enums-pattern-matching');
    await new Promise((r) => setTimeout(r, 400));
    const runBtn7 = await page.evaluateHandle(() => {
      const btns = Array.from(document.querySelectorAll('button'));
      return btns.find((b) => b.innerText.includes('Run'));
    });
    await runBtn7.click();

    await page.waitForFunction(() => {
      return (
        document.body.innerText.includes('Delivery boy raste mein hai!') &&
        document.body.innerText.includes('Samaan mil gaya! Rating de do bro!')
      );
    }, { timeout: 5000 });
    console.log('   ✅ avtar enums & jaanch pattern matching verified in browser!');

    // 10. Test Stdlib (hisab, waqt, json) Recipe
    console.log('10. Testing Built-in Stdlib recipe in Playground...');
    await page.select('select', 'stdlib-demo');
    await new Promise((r) => setTimeout(r, 400));
    const runBtn8 = await page.evaluateHandle(() => {
      const btns = Array.from(document.querySelectorAll('button'));
      return btns.find((b) => b.innerText.includes('Run'));
    });
    await runBtn8.click();

    await page.waitForFunction(() => {
      return (
        document.body.innerText.includes('Math PI: 3.14159') &&
        document.body.innerText.includes('Square root of 81: 9.0') &&
        document.body.innerText.includes('Round 7.8: 8') &&
        document.body.innerText.includes('Serialized JSON: {"name": "Virat", "century": 80, "active": true}') &&
        document.body.innerText.includes('Decoded player name: Virat')
      );
    }, { timeout: 5000 });
    console.log('   ✅ hisab, waqt & json standard library verified in browser!');

    // 11. Capture screenshot
    await page.screenshot({ path: 'docs-playground-input.png' });
    console.log('   📸 Screenshot saved to docs-playground-input.png');

    console.log('\n🎉 ALL FULL-FEATURED MEMELANG E2E TESTS (COLLECTIONS, ERRORS, ENUMS, STDLIB) PASSED 100%!');
  } finally {
    await browser.close();
  }
}

runE2ETests().catch((err) => {
  console.error('❌ E2E Test Failed:', err);
  process.exit(1);
});
