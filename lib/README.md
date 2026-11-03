# 📦 Pure MEMElang Standard Libraries (`lib/`)

These libraries are written **100% in pure MEMElang (`.meme`)** and are automatically available for import via `bula "<library>"`:

## Included Libraries

### 1. `dataframe` (`lib/dataframe.meme`)
Pandas-style tabular analytics and data manipulation engine.
```meme
bula "lib/dataframe" jaise { DataFrame, from_records };

pakad df = from_records([
    {"name": "Chai", "category": "Hot", "price": 20},
    {"name": "Lassi", "category": "Cold", "price": 40}
]);

pakad expensive = df.where(r => r.price > 25);
dikha f"Count: {expensive.count()}, Total Sum: {df.sum(\"price\")}";
```

### 2. `jwt` (`lib/jwt.meme`)
RFC 7519 JSON Web Token authentication library.
```meme
bula "lib/jwt" jaise jwt;

// Sign token (valid for 3600 seconds)
pakad token = jwt.sign({"userId": 101, "role": "admin"}, "secret_key_123", 3600);

// Verify signature and expiry
pakad user = jwt.verify(token, "secret_key_123");
dikha f"Authenticated user: {user.userId}";
```

### 3. `colors` (`lib/colors.meme`)
ANSI terminal styling and badge formatting (Chalk-equivalent).
```meme
bula "lib/colors" jaise colors;

dikha colors.green("Success!"), colors.yellow("Warning!"), colors.red("Error!");
dikha colors.badge("AUTH", "User session established", colors.cyan);
```
