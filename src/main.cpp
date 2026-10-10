#include <drogon/drogon.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <cstdlib>
#include <string>
#include <regex>
#include <fstream>

using namespace drogon;

int main()
{
    // ==========================================
    // LOAD BASE CONFIGURATION
    // ==========================================

    std::ifstream configCheck("config.json");
    if (configCheck.good())
    {
        configCheck.close();
        app().loadConfigFile("config.json");
    }

    // ==========================================
    // DYNAMIC ENVIRONMENT VARIABLE CONFIGURATION
    // ==========================================

    // 1. Port and Listen Address (0.0.0.0 for containerized deployment)
    int serverPort = 8080;
    const char* portEnv = std::getenv("PORT");
    if (portEnv && *portEnv)
    {
        try {
            serverPort = std::stoi(portEnv);
        } catch (...) {
            serverPort = 8080;
        }
    }
    app().addListener("0.0.0.0", serverPort);

    // 2. PostgreSQL Database Configuration from Environment
    std::string dbHost;
    int dbPort = 5432;
    std::string dbName;
    std::string dbUser;
    std::string dbPass;

    const char* dbUrlEnv = std::getenv("DATABASE_URL");
    if (dbUrlEnv && *dbUrlEnv)
    {
        std::string dbUrl(dbUrlEnv);
        // Match: postgres[ql]://[user[:password]@]host[:port][/dbname]
        std::regex urlRegex(R"(^postgres(?:ql)?:\/\/(?:([^:]+)(?::([^@]+))?@)?([^:\/\?]+)(?::(\d+))?(?:\/([^?]+))?.*$)");
        std::smatch match;
        if (std::regex_match(dbUrl, match, urlRegex))
        {
            if (match[1].matched) dbUser = match[1].str();
            if (match[2].matched) dbPass = match[2].str();
            if (match[3].matched) dbHost = match[3].str();
            if (match[4].matched) dbPort = std::stoi(match[4].str());
            if (match[5].matched) dbName = match[5].str();
        }
    }

    const char* hostEnv = std::getenv("PGHOST");
    if (hostEnv && *hostEnv) dbHost = hostEnv;

    const char* pgPortEnv = std::getenv("PGPORT");
    if (pgPortEnv && *pgPortEnv)
    {
        try {
            dbPort = std::stoi(pgPortEnv);
        } catch (...) {}
    }

    const char* dbNameEnv = std::getenv("PGDATABASE");
    if (dbNameEnv && *dbNameEnv) dbName = dbNameEnv;

    const char* userEnv = std::getenv("PGUSER");
    if (userEnv && *userEnv) dbUser = userEnv;

    const char* passEnv = std::getenv("PGPASSWORD");
    if (passEnv && *passEnv) dbPass = passEnv;

    if (!dbHost.empty() && !dbName.empty())
    {
        app().createDbClient(
            "postgresql",
            dbHost,
            static_cast<unsigned short>(dbPort),
            dbName,
            dbUser,
            dbPass,
            4,
            "",
            "default"
        );
    }


    // ==========================================
    // CORS
    // ==========================================

    app().registerHttpResponseCreationAdvice(
        [](const HttpResponsePtr& response)
        {
            response->addHeader(
                "Access-Control-Allow-Origin",
                "*"
            );

            response->addHeader(
                "Access-Control-Allow-Methods",
                "GET, POST, PUT, DELETE, OPTIONS"
            );

            response->addHeader(
                "Access-Control-Allow-Headers",
                "Content-Type"
            );
        }
    );


    // ==========================================
    // HANDLE OPTIONS / PREFLIGHT REQUEST
    // ==========================================

    app().registerSyncAdvice(
        [](const HttpRequestPtr& req) -> HttpResponsePtr
        {
            if (req->method() == Options)
            {
                auto response =
                    HttpResponse::newHttpResponse();

                response->setStatusCode(k200OK);

                response->addHeader(
                    "Access-Control-Allow-Origin",
                    "*"
                );

                response->addHeader(
                    "Access-Control-Allow-Methods",
                    "GET, POST, PUT, DELETE, OPTIONS"
                );

                response->addHeader(
                    "Access-Control-Allow-Headers",
                    "Content-Type"
                );

                return response;
            }

            return nullptr;
        }
    );


    // ==========================================
    // TEST ROUTE
    // ==========================================

    app().registerHandler(
        "/",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback)
        {
            Json::Value json;

            json["success"] = true;

            json["message"] =
                "BasilMart Backend Server is Running!";

            json["status"] = "OK";

            auto response =
                HttpResponse::newHttpJsonResponse(json);

            callback(response);
        },
        {Get}
    );


    // ==========================================
    // REGISTER USER
    // ==========================================

    app().registerHandler(
        "/register",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback)
        {
            Json::Value json;

            auto name =
                req->getParameter("name");

            auto email =
                req->getParameter("email");

            auto password =
                req->getParameter("password");


            if (name.empty() ||
                email.empty() ||
                password.empty())
            {
                json["success"] = false;

                json["message"] =
                    "All fields are required!";

                auto response =
                    HttpResponse::newHttpJsonResponse(json);

                callback(response);
                return;
            }


            auto dbClient =
                app().getDbClient();


            dbClient->execSqlAsync(
                "INSERT INTO users "
                "(name, email, password) "
                "VALUES ($1, $2, $3)",

                [callback](const orm::Result& result)
                {
                    Json::Value json;

                    json["success"] = true;

                    json["message"] =
                        "Registration successful!";

                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                [callback](const orm::DrogonDbException& e)
                {
                    Json::Value json;

                    json["success"] = false;

                    json["message"] =
                        "Registration failed. "
                        "Email may already exist.";

                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                name,
                email,
                password
            );
        },
        {Post}
    );


    // ==========================================
    // LOGIN USER
    // ==========================================

    app().registerHandler(
        "/login",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback)
        {
            Json::Value json;

            auto email =
                req->getParameter("email");

            auto password =
                req->getParameter("password");


            if (email.empty() ||
                password.empty())
            {
                json["success"] = false;

                json["message"] =
                    "Email and password are required!";

                auto response =
                    HttpResponse::newHttpJsonResponse(json);

                callback(response);
                return;
            }


            auto dbClient =
                app().getDbClient();


            dbClient->execSqlAsync(
                "SELECT id, name, email "
                "FROM users "
                "WHERE email = $1 "
                "AND password = $2",

                [callback](const orm::Result& result)
                {
                    Json::Value json;


                    if (result.empty())
                    {
                        json["success"] = false;

                        json["message"] =
                            "Invalid email or password!";
                    }
                    else
                    {
                        json["success"] = true;

                        json["message"] =
                            "Login successful!";


                        json["user"]["id"] =
                            result[0]["id"].as<int>();


                        json["user"]["name"] =
                            result[0]["name"]
                                .as<std::string>();


                        json["user"]["email"] =
                            result[0]["email"]
                                .as<std::string>();
                    }


                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                [callback](const orm::DrogonDbException& e)
                {
                    Json::Value json;

                    json["success"] = false;

                    json["message"] =
                        "Database error!";

                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                email,
                password
            );
        },
        {Post}
    );


    // ==========================================
    // GET PRODUCTS
    // ==========================================

    app().registerHandler(
        "/products",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback)
        {
            auto dbClient =
                app().getDbClient();


            dbClient->execSqlAsync(
                "SELECT id, name, price, stock, description "
                "FROM products "
                "ORDER BY id",

                [callback](const orm::Result& result)
                {
                    Json::Value json;

                    json["success"] = true;

                    json["products"] =
                        Json::arrayValue;


                    for (const auto& row : result)
                    {
                        Json::Value product;


                        product["id"] =
                            row["id"].as<int>();


                        product["name"] =
                            row["name"]
                                .as<std::string>();


                        product["price"] =
                            row["price"].as<double>();


                        product["stock"] =
                            row["stock"].as<int>();


                        product["description"] =
                            row["description"]
                                .as<std::string>();


                        json["products"].append(product);
                    }


                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                [callback](const orm::DrogonDbException& e)
                {
                    Json::Value json;

                    json["success"] = false;

                    json["message"] =
                        "Unable to load products!";

                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                }
            );
        },
        {Get}
    );

    
// ===============================
// ADMIN - ADD NEW PRODUCT
// ===============================

app().registerHandler(
    "/admin/products",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& callback)
    {
        auto json = req->getJsonObject();

        Json::Value result;

        if (!json ||
            !json->isMember("name") ||
            !json->isMember("price") ||
            !json->isMember("stock") ||
            !json->isMember("description"))
        {
            result["success"] = false;
            result["message"] = "Please provide all product details.";

            auto response = HttpResponse::newHttpJsonResponse(result);
            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        std::string name = (*json)["name"].asString();
        double price = (*json)["price"].asDouble();
        int stock = (*json)["stock"].asInt();
        std::string description = (*json)["description"].asString();

        if (name.empty() || price <= 0 || stock < 0)
        {
            result["success"] = false;
            result["message"] = "Invalid product details.";

            auto response = HttpResponse::newHttpJsonResponse(result);
            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        auto dbClient = app().getDbClient();

        dbClient->execSqlAsync(
            "INSERT INTO products (name, price, stock, description) "
            "VALUES ($1, $2, $3, $4) RETURNING id",
            [callback](const orm::Result& dbResult)
            {
                Json::Value result;
                result["success"] = true;
                result["message"] = "Product added successfully!";
                result["id"] = dbResult[0]["id"].as<int>();

                auto response = HttpResponse::newHttpJsonResponse(result);
                callback(response);
            },
            [callback](const orm::DrogonDbException& e)
            {
                Json::Value result;
                result["success"] = false;
                result["message"] = "Database error while adding product.";

                auto response = HttpResponse::newHttpJsonResponse(result);
                response->setStatusCode(k500InternalServerError);
                callback(response);
            },
            name,
            price,
            stock,
            description
        );
    },
    {Post}
);

app().registerHandler(
    "/admin/stats",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& callback)
    {
        auto dbClient = app().getDbClient();

        dbClient->execSqlAsync(
            "SELECT "
            "(SELECT COUNT(*) FROM products) AS total_products, "
            "(SELECT COUNT(*) FROM users) AS total_users, "
            "(SELECT COUNT(*) FROM orders) AS total_orders, "
            "(SELECT COALESCE(SUM(total), 0) FROM orders) AS total_sales",
            [callback](const orm::Result& result)
            {
                Json::Value json;

                json["success"] = true;
                json["totalProducts"] =
                    result[0]["total_products"].as<int>();
                json["totalUsers"] =
                    result[0]["total_users"].as<int>();
                json["totalOrders"] =
                    result[0]["total_orders"].as<int>();
                json["totalSales"] =
                    result[0]["total_sales"].as<std::string>();

                auto response =
                    HttpResponse::newHttpJsonResponse(json);

                callback(response);
            },
            [callback](const orm::DrogonDbException& e)
            {
                Json::Value json;
                json["success"] = false;
                json["message"] = "Unable to load dashboard statistics.";

                auto response =
                    HttpResponse::newHttpJsonResponse(json);

                response->setStatusCode(k500InternalServerError);
                callback(response);
            }
        );
    },
    {Get}
);

app().registerHandler(
    "/admin/orders/status",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& callback)
    {
        auto json = req->getJsonObject();
        Json::Value result;

        if (!json ||
            !json->isMember("id") ||
            !json->isMember("status"))
        {
            result["success"] = false;
            result["message"] = "Order ID and status are required.";

            auto response =
                HttpResponse::newHttpJsonResponse(result);
            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        int orderId = (*json)["id"].asInt();
        std::string status = (*json)["status"].asString();

        const std::vector<std::string> validStatuses = {
            "Pending", "Placed", "Processing",
            "Shipped", "Delivered", "Cancelled"
        };

        bool valid = false;

        for (const auto& allowed : validStatuses)
        {
            if (status == allowed)
            {
                valid = true;
                break;
            }
        }

        if (orderId <= 0 || !valid)
        {
            result["success"] = false;
            result["message"] = "Invalid order ID or status.";

            auto response =
                HttpResponse::newHttpJsonResponse(result);
            response->setStatusCode(k400BadRequest);
            callback(response);
            return;
        }

        auto dbClient = app().getDbClient();

        dbClient->execSqlAsync(
            "UPDATE orders SET status = $1 "
            "WHERE id = $2 RETURNING id",

            [callback](const orm::Result& rows)
            {
                Json::Value result;

                if (rows.empty())
                {
                    result["success"] = false;
                    result["message"] = "Order not found.";

                    auto response =
                        HttpResponse::newHttpJsonResponse(result);
                    response->setStatusCode(k404NotFound);
                    callback(response);
                    return;
                }

                result["success"] = true;
                result["message"] =
                    "Order status updated successfully.";

                auto response =
                    HttpResponse::newHttpJsonResponse(result);
                callback(response);
            },

            [callback](const orm::DrogonDbException& e)
            {
                Json::Value result;
                result["success"] = false;
                result["message"] =
                    "Database error while updating order.";

                auto response =
                    HttpResponse::newHttpJsonResponse(result);
                response->setStatusCode(k500InternalServerError);
                callback(response);
            },

            status,
            orderId
        );
    },
    {Put}
);


app().registerHandler(
    "/admin/orders",
    [](const HttpRequestPtr& req,
       std::function<void(const HttpResponsePtr&)>&& callback)
    {
        auto dbClient = app().getDbClient();

        dbClient->execSqlAsync(
            "SELECT "
            "o.id AS id, "
            "COALESCE(o.user_id, 0) AS user_id, "
            "COALESCE(u.name, 'Guest') AS customer_name, "
            "COALESCE(u.email, '') AS customer_email, "
            "o.total AS total, "
            "o.status AS status, "
            "o.created_at::text AS created_at "
            "FROM orders o "
            "LEFT JOIN users u ON o.user_id = u.id "
            "ORDER BY o.id DESC",

            [callback](const orm::Result& rows)
            {
                Json::Value result;
                Json::Value orders(Json::arrayValue);

                for (const auto& row : rows)
                {
                    Json::Value order;

                    order["id"] = row["id"].as<int>();
                    order["user_id"] = row["user_id"].as<int>();
                    order["customer_name"] =
                        row["customer_name"].as<std::string>();
                    order["customer_email"] =
                        row["customer_email"].as<std::string>();
                    order["total"] =
                        row["total"].as<std::string>();
                    order["status"] =
                        row["status"].as<std::string>();
                    order["created_at"] =
                        row["created_at"].as<std::string>();

                    orders.append(order);
                }

                result["success"] = true;
                result["orders"] = orders;

                auto response =
                    HttpResponse::newHttpJsonResponse(result);

                callback(response);
            },

            [callback](const orm::DrogonDbException& e)
            {
                Json::Value result;
                result["success"] = false;
                result["message"] = "Unable to load orders.";

                auto response =
                    HttpResponse::newHttpJsonResponse(result);

                response->setStatusCode(k500InternalServerError);
                callback(response);
            }
        );
    },
    {Get}
);




// ===============================
// ADMIN - ADD NEW PRODUCT
// ===============================



    // ==========================================
    // ADD TO CART
    // ==========================================

    app().registerHandler(
        "/cart/add",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback)
        {
            Json::Value json;


            auto userId =
                req->getParameter("user_id");


            auto productId =
                req->getParameter("product_id");


            auto quantity =
                req->getParameter("quantity");


            if (userId.empty() ||
                productId.empty() ||
                quantity.empty())
            {
                json["success"] = false;

                json["message"] =
                    "User ID, product ID and quantity "
                    "are required!";


                auto response =
                    HttpResponse::newHttpJsonResponse(json);

                callback(response);
                return;
            }


            auto dbClient =
                app().getDbClient();


            dbClient->execSqlAsync(
                "INSERT INTO cart "
                "(user_id, product_id, quantity) "
                "VALUES ($1, $2, $3)",

                [callback](const orm::Result& result)
                {
                    Json::Value json;

                    json["success"] = true;

                    json["message"] =
                        "Product added to cart!";


                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                [callback](const orm::DrogonDbException& e)
                {
                    Json::Value json;

                    json["success"] = false;

                    json["message"] =
                        "Unable to add product to cart!";


                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                std::stoi(userId),
                std::stoi(productId),
                std::stoi(quantity)
            );
        },
        {Post}
    );


    // ==========================================
    // GET CART
    // ==========================================

    app().registerHandler(
        "/cart/{user_id}",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback,
           int userId)
        {
            auto dbClient =
                app().getDbClient();


            dbClient->execSqlAsync(
                "SELECT cart.id, "
                "cart.product_id, "
                "products.name, "
                "products.price, "
                "cart.quantity "

                "FROM cart "

                "JOIN products "
                "ON cart.product_id = products.id "

                "WHERE cart.user_id = $1",

                [callback](const orm::Result& result)
                {
                    Json::Value json;

                    json["success"] = true;

                    json["cart"] =
                        Json::arrayValue;


                    for (const auto& row : result)
                    {
                        Json::Value item;


                        item["id"] =
                            row["id"].as<int>();


                        item["product_id"] =
                            row["product_id"].as<int>();


                        item["name"] =
                            row["name"]
                                .as<std::string>();


                        item["price"] =
                            row["price"].as<double>();


                        item["quantity"] =
                            row["quantity"].as<int>();


                        json["cart"].append(item);
                    }


                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                [callback](const orm::DrogonDbException& e)
                {
                    Json::Value json;

                    json["success"] = false;

                    json["message"] =
                        "Unable to load cart!";


                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                userId
            );
        },
        {Get}
    );


    // ==========================================
    // CREATE ORDER
    // ==========================================

    app().registerHandler(
        "/orders",
        [](const HttpRequestPtr& req,
           std::function<void(const HttpResponsePtr&)>&& callback)
        {
            Json::Value json;


            auto userId =
                req->getParameter("user_id");

            auto total =
                req->getParameter("total");

            auto productIds =
                req->getParameter("product_ids");

            auto quantities =
                req->getParameter("quantities");

            auto prices =
                req->getParameter("prices");


            // ------------------------------------------
            // VALIDATE ORDER DATA
            // ------------------------------------------

            if (userId.empty() ||
                total.empty() ||
                productIds.empty() ||
                quantities.empty() ||
                prices.empty())
            {
                json["success"] = false;

                json["message"] =
                    "Order details are required!";


                auto response =
                    HttpResponse::newHttpJsonResponse(json);

                callback(response);
                return;
            }


            // ------------------------------------------
            // SPLIT PRODUCT IDs
            // ------------------------------------------

            std::vector<std::string> productIdList;

            std::stringstream ssProduct(productIds);

            std::string value;

            while (std::getline(
                ssProduct,
                value,
                ','))
            {
                productIdList.push_back(value);
            }


            // ------------------------------------------
            // SPLIT QUANTITIES
            // ------------------------------------------

            std::vector<std::string> quantityList;

            std::stringstream ssQuantity(quantities);

            while (std::getline(
                ssQuantity,
                value,
                ','))
            {
                quantityList.push_back(value);
            }


            // ------------------------------------------
            // SPLIT PRICES
            // ------------------------------------------

            std::vector<std::string> priceList;

            std::stringstream ssPrice(prices);

            while (std::getline(
                ssPrice,
                value,
                ','))
            {
                priceList.push_back(value);
            }


            // ------------------------------------------
            // CHECK ITEM COUNT
            // ------------------------------------------

            if (productIdList.size() != quantityList.size() ||
                productIdList.size() != priceList.size())
            {
                json["success"] = false;

                json["message"] =
                    "Invalid order items!";


                auto response =
                    HttpResponse::newHttpJsonResponse(json);

                callback(response);
                return;
            }


            // ------------------------------------------
            // DATABASE CLIENT
            // ------------------------------------------

            auto dbClient =
                app().getDbClient();


            // ------------------------------------------
            // INSERT INTO ORDERS
            // ------------------------------------------

            dbClient->execSqlAsync(
                "INSERT INTO orders "
                "(user_id, total, status) "
                "VALUES ($1, $2, 'Placed') "
                "RETURNING id",

                [callback,
                 productIdList,
                 quantityList,
                 priceList,
                 dbClient]
                (const orm::Result& result)
                {
                    Json::Value json;


                    if (result.empty())
                    {
                        json["success"] = false;

                        json["message"] =
                            "Unable to create order!";


                        auto response =
                            HttpResponse::newHttpJsonResponse(json);

                        callback(response);
                        return;
                    }


                    int orderId =
                        result[0]["id"].as<int>();


                    // ------------------------------------------
                    // INSERT ORDER ITEMS
                    // ------------------------------------------

                    auto insertItem =
                        std::make_shared<
                            std::function<void(size_t)>
                        >();


                    *insertItem =
                        [callback,
                         productIdList,
                         quantityList,
                         priceList,
                         dbClient,
                         orderId,
                         insertItem]
                        (size_t index)
                        {
                            // All items inserted
                            if (index >=
                                productIdList.size())
                            {
                                Json::Value json;

                                json["success"] =
                                    true;

                                json["message"] =
                                    "Order placed successfully!";

                                json["order_id"] =
                                    orderId;


                                auto response =
                                    HttpResponse::newHttpJsonResponse(
                                        json
                                    );

                                callback(response);

                                return;
                            }


                            // Insert current item
                            dbClient->execSqlAsync(
                                "INSERT INTO order_items "
                                "(order_id, product_id, quantity, price) "
                                "VALUES ($1, $2, $3, $4)",

                                [insertItem, index]
                                (const orm::Result& result)
                                {
                                    (*insertItem)(
                                        index + 1
                                    );
                                },

                                [callback]
                                (const orm::DrogonDbException& e)
                                {
                                    Json::Value json;

                                    json["success"] =
                                        false;

                                    json["message"] =
                                        "Unable to save order items!";


                                    auto response =
                                        HttpResponse::newHttpJsonResponse(
                                            json
                                        );

                                    callback(response);
                                },

                                orderId,

                                std::stoi(
                                    productIdList[index]
                                ),

                                std::stoi(
                                    quantityList[index]
                                ),

                                std::stod(
                                    priceList[index]
                                )
                            );
                        };


                    // Start inserting items
                    (*insertItem)(0);
                },

                [callback](const orm::DrogonDbException& e)
                {
                    Json::Value json;

                    json["success"] = false;

                    json["message"] =
                        "Unable to create order!";


                    auto response =
                        HttpResponse::newHttpJsonResponse(json);

                    callback(response);
                },

                std::stoi(userId),

                std::stod(total)
            );
        },
        {Post}
    );


    // ==========================================
    // SERVER START
    // ==========================================

    std::cout
        << "====================================="
        << std::endl;

    std::cout
        << "       BASILMART BACKEND SERVER"
        << std::endl;

    std::cout
        << "====================================="
        << std::endl;

    std::cout
        << "Server running on port " << serverPort << "..."
        << std::endl;

    std::cout
        << "CORS enabled..."
        << std::endl;

    std::cout
        << "Order API enabled..."
        << std::endl;

    std::cout
        << "====================================="
        << std::endl;


    // ==========================================
    // RUN SERVER
    // ==========================================

    app().run();


    return 0;
}