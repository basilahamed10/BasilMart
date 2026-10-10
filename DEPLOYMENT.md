# BasilMart — Render Deployment Guide

This guide walks you through deploying the **BasilMart** full-stack e-commerce project to **Render** using a free PostgreSQL database, a containerized C++ Drogon web service, and a Static Site for the frontend.

---

## Architecture Overview

```
                                      +-------------------------------+
                                      |     BasilMart Frontend        |
                                      | (Render Static Site: frontend)|
                                      +---------------+---------------+
                                                      |
                                     HTTPS REST API   |
                                                      v
+-------------------------------+     +---------------+---------------+
|    Render PostgreSQL DB       | <== |     BasilMart Backend         |
|   (Internal Connection URL)   |     |  (Render Docker Web Service)  |
+-------------------------------+     +-------------------------------+
```

- **Frontend**: Render Static Site (Publish directory: `frontend`).
- **Backend**: Render Web Service (Runtime: `Docker`, health check: `/`).
- **Database**: Render PostgreSQL database (Version 16+).

---

## Step 1: Database Initialization on Render

1. Log in to your [Render Dashboard](https://dashboard.render.com/).
2. Click **New +** -> **PostgreSQL**.
3. Fill in the details:
   - **Name**: `basilmart-db`
   - **Database**: `basilmart`
   - **User**: `basilmart`
   - **Region**: Choose the region closest to you (e.g., Oregon or Frankfurt).
   - **Plan**: Free
4. Click **Create Database**.
5. Once created, click on `basilmart-db` and copy the **External Database URL** (for running migrations from your machine) and **Internal Database URL** (for backend communication within Render).

### Run the schema and seed data migration:
From your local terminal, initialize tables and seed items using `psql`:

```bash
# Set external connection string (replace with your Render external database URL):
psql "<RENDER_EXTERNAL_DATABASE_URL>" -f db/schema.sql
psql "<RENDER_EXTERNAL_DATABASE_URL>" -f db/seed.sql
```

> **Note:** `schema.sql` uses `CREATE TABLE IF NOT EXISTS` and `seed.sql` uses `ON CONFLICT (id) DO UPDATE`, so you can run them repeatedly without losing existing orders or users.

---

## Step 2: Deploy the C++ Drogon Backend

1. Push your repository to GitHub or GitLab.
2. In Render Dashboard, click **New +** -> **Web Service**.
3. Connect your repository.
4. Configure the Web Service:
   - **Name**: `basilmart-backend`
   - **Region**: Same region as `basilmart-db`
   - **Branch**: `main` (or your default branch)
   - **Runtime**: `Docker`
   - **Dockerfile Path**: `./Dockerfile`
   - **Docker Context**: `.`
   - **Health Check Path**: `/`
5. In the **Environment Variables** section, add:
   - `PORT`: `10000` (or leave default; Render automatically routes traffic to this port)
   - `DATABASE_URL`: Paste the **Internal Database URL** from your Render PostgreSQL instance (format: `postgresql://basilmart:<password>@dpg-xxx-a/basilmart`).
6. Click **Create Web Service**.
7. Once deployed, Render will provide a public URL like:
   ```
   https://basilmart-backend.onrender.com
   ```
8. Verify backend health by visiting `https://basilmart-backend.onrender.com/` in your browser. You should see:
   ```json
   {
     "message": "BasilMart Backend Server is Running!",
     "status": "OK",
     "success": true
   }
   ```

---

## Step 3: Deploy the Frontend Static Site

1. In Render Dashboard, click **New +** -> **Static Site**.
2. Connect the same repository.
3. Configure the Static Site:
   - **Name**: `basilmart-frontend`
   - **Branch**: `main`
   - **Build Command**: *(Leave empty)*
   - **Publish Directory**: `frontend`
4. Click **Create Static Site**.
5. Once deployed, Render assigns a public URL like:
   ```
   https://basilmart-frontend.onrender.com
   ```

---

## Step 4: Link Frontend to Production Backend

You can connect your frontend to the production backend in either of two ways:

### Option A: Via `frontend/config.js` (Recommended)
Edit `frontend/config.js` to define your backend URL:
```javascript
const DEFAULT_API_URL = "https://basilmart-backend.onrender.com";
```
Commit and push this change. Render will automatically redeploy the Static Site within seconds.

### Option B: Runtime configuration without modifying files
Open browser DevTools on your deployed frontend and run:
```javascript
localStorage.setItem("basilmart_api_url", "https://basilmart-backend.onrender.com");
location.reload();
```

---

## Step 5: Verification Checklist

| Test Case | Route / Page | Expected Result |
| :--- | :--- | :--- |
| **Backend Health** | `GET /` | Returns `{"success": true, "status": "OK"}` |
| **Products List** | `GET /products` / `products.html` | Lists seeded apparel and accessories |
| **User Registration** | `POST /register` / `register.html` | Registers user into Render PostgreSQL |
| **User Sign-in** | `POST /login` / `login.html` | Authenticates and returns user session |
| **Add to Cart** | `POST /cart/add` / `cart.html` | Cart updates dynamically |
| **Admin Stats** | `GET /admin/stats` / `admin.html` | Displays orders, sales and user count |
| **Admin Orders** | `GET /admin/orders` / `admin.html` | Displays customer orders table |
| **Order Status Update** | `PUT /admin/orders/status` | Updates order state (Placed, Delivered, etc.) |

---

## Local Development (Unchanged)
Local development works seamlessly with no extra configuration:
```bash
# 1. Run local PostgreSQL service
# 2. Start compiled Windows binary:
.\build\Release\BasilMart.exe
# 3. Open frontend/index.html in your browser
```
If `DATABASE_URL` is not set, the server automatically reads `config.json` and connects to `127.0.0.1:5432`.
