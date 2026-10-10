// ==========================================
// BASILMART FRONTEND SCRIPT
// ==========================================

// Use configurable API_URL from config.js or fallback
const API_URL = (typeof window !== "undefined" && window.API_URL) ? window.API_URL : "http://localhost:8080";


// ==========================================
// REGISTER USER
// ==========================================

async function registerUser(event) {
    event.preventDefault();

    const name = document.getElementById("registerName").value;
    const email = document.getElementById("registerEmail").value;
    const password = document.getElementById("registerPassword").value;

    if (name === "" || email === "" || password === "") {
        alert("Please fill all fields!");
        return;
    }

    try {
        const response = await fetch(
            `${API_URL}/register?name=${encodeURIComponent(name)}&email=${encodeURIComponent(email)}&password=${encodeURIComponent(password)}`,
            {
                method: "POST"
            }
        );

        const data = await response.json();

        if (data.success) {
            alert("Registration successful!");

            window.location.href = "login.html";
        } else {
            alert(data.message);
        }
    }
    catch (error) {
        console.error("Register Error:", error);

        alert("Unable to connect to BasilMart server!");
    }
}


// ==========================================
// LOGIN USER
// ==========================================

async function loginUser(event) {
    event.preventDefault();

    const email = document.getElementById("loginEmail").value;
    const password = document.getElementById("loginPassword").value;

    if (email === "" || password === "") {
        alert("Please enter email and password!");
        return;
    }

    try {
        const response = await fetch(
            `${API_URL}/login?email=${encodeURIComponent(email)}&password=${encodeURIComponent(password)}`,
            {
                method: "POST"
            }
        );

        const data = await response.json();

        if (data.success) {

            localStorage.setItem(
                "user",
                JSON.stringify(data.user)
            );

            alert("Login successful!");

            window.location.href = "index.html";
        }
        else {
            alert(data.message);
        }
    }
    catch (error) {
        console.error("Login Error:", error);

        alert("Unable to connect to BasilMart server!");
    }
}


// ==========================================
// LOGOUT USER
// ==========================================

function logoutUser() {
    localStorage.removeItem("user");

    window.location.href = "login.html";
}


// ==========================================
// GET LOGGED-IN USER
// ==========================================

function getLoggedInUser() {
    const user = localStorage.getItem("user");

    if (user) {
        return JSON.parse(user);
    }

    return null;
}


// ==========================================
// CHECK LOGIN
// ==========================================

function checkLogin() {
    const user = getLoggedInUser();

    if (!user) {
        window.location.href = "login.html";
    }
}


// ==========================================
// GET PRODUCTS
// ==========================================

async function getProducts() {

    try {

        const response = await fetch(
            `${API_URL}/products`
        );

        const data = await response.json();

        if (data.success) {
            return data.products;
        }

        console.log(data.message);

        return [];

    }
    catch (error) {

        console.error(
            "Products Error:",
            error
        );

        return [];
    }
}


// ==========================================
// ADD PRODUCT TO CART
// ==========================================

async function addToCart(productId, quantity = 1) {

    const user = getLoggedInUser();

    if (!user) {
        alert("Please login first!");
        window.location.href = "login.html";
        return;
    }

    try {

        const response = await fetch(
            `${API_URL}/cart/add?user_id=${user.id}&product_id=${productId}&quantity=${quantity}`,
            {
                method: "POST"
            }
        );

        const data = await response.json();

        if (data.success) {

            alert("Product added to cart!");

        }
        else {

            alert(data.message);

        }

    }
    catch (error) {

        console.error(
            "Cart Error:",
            error
        );

        alert(
            "Unable to connect to BasilMart server!"
        );
    }
}


// ==========================================
// GET CART
// ==========================================

async function getCart() {

    const user = getLoggedInUser();

    if (!user) {
        return [];
    }

    try {

        const response = await fetch(
            `${API_URL}/cart/${user.id}`
        );

        const data = await response.json();

        if (data.success) {
            return data.cart;
        }

        return [];

    }
    catch (error) {

        console.error(
            "Cart Error:",
            error
        );

        return [];
    }
}