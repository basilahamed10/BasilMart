#include <drogon/drogon.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>

using namespace drogon;

int main()
{
    // ==========================================
    // LOAD CONFIGURATION
    // ==========================================

    app().loadConfigFile("config.json");


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

     ```cpp
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

  ```cpp
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
        << "Server running on port 8080..."
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