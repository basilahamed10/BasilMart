-- =========================================================
-- BasilMart Seed Data
-- Safe, repeatable product seed script (will not duplicate data)
-- =========================================================

INSERT INTO public.products (id, name, price, stock, description)
VALUES 
    (1, 'Men T-Shirt', 499.00, 20, 'Comfortable cotton T-shirt'),
    (2, 'Women Dress', 999.00, 15, 'Stylish casual dress'),
    (3, 'Running Shoes', 1499.00, 10, 'Lightweight running shoes'),
    (4, 'Smart Watch', 2499.00, 8, 'Fitness and smart watch'),
    (5, 'Backpack', 799.00, 12, 'Durable everyday backpack'),
    (6, 'Formal Shirt', 899.00, 10, 'Premium formal shirt'),
    (7, 'GIRLS JACKET', 400.00, 10, 'nice dress')
ON CONFLICT (id) DO UPDATE 
SET 
    name = EXCLUDED.name,
    price = EXCLUDED.price,
    stock = EXCLUDED.stock,
    description = EXCLUDED.description;

-- Ensure auto-increment sequence is synced past existing IDs
SELECT setval(
    pg_get_serial_sequence('public.products', 'id'),
    COALESCE((SELECT MAX(id) FROM public.products), 1)
);
