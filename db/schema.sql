-- =========================================================
-- BasilMart Database Schema
-- Safe, repeatable initialization script for Render PostgreSQL
-- =========================================================

-- USERS TABLE
CREATE TABLE IF NOT EXISTS public.users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(150) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    created_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- PRODUCTS TABLE
CREATE TABLE IF NOT EXISTS public.products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(150) NOT NULL,
    price NUMERIC(10,2) NOT NULL,
    stock INTEGER DEFAULT 0,
    description TEXT,
    created_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- ORDERS TABLE
CREATE TABLE IF NOT EXISTS public.orders (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES public.users(id),
    total NUMERIC(10,2) NOT NULL,
    status VARCHAR(50) DEFAULT 'Pending',
    created_at TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- ORDER ITEMS TABLE
CREATE TABLE IF NOT EXISTS public.order_items (
    id SERIAL PRIMARY KEY,
    order_id INTEGER REFERENCES public.orders(id),
    product_id INTEGER REFERENCES public.products(id),
    quantity INTEGER NOT NULL,
    price NUMERIC(10,2) NOT NULL
);

-- CART TABLE
CREATE TABLE IF NOT EXISTS public.cart (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES public.users(id),
    product_id INTEGER REFERENCES public.products(id),
    quantity INTEGER DEFAULT 1
);

-- Performance indices
CREATE INDEX IF NOT EXISTS idx_users_email ON public.users(email);
CREATE INDEX IF NOT EXISTS idx_orders_user_id ON public.orders(user_id);
CREATE INDEX IF NOT EXISTS idx_order_items_order_id ON public.order_items(order_id);
CREATE INDEX IF NOT EXISTS idx_cart_user_id ON public.cart(user_id);
