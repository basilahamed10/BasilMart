// ==========================================
// BASILMART API CONFIGURATION
// Configurable for local dev and Render production
// ==========================================

const DEFAULT_API_URL = "http://localhost:8080";

function getApiUrl() {
    // 1. Check window override if configured
    if (window.__BASILMART_API_URL__ && typeof window.__BASILMART_API_URL__ === "string") {
        return window.__BASILMART_API_URL__.replace(/\/+$/, "");
    }

    // 2. Check localStorage override (useful for runtime testing / deployment configuration)
    try {
        const stored = localStorage.getItem("basilmart_api_url");
        if (stored && stored.trim() !== "") {
            return stored.trim().replace(/\/+$/, "");
        }
    } catch (e) {
        // LocalStorage access restricted or unavailable
    }

    // 3. Fallback to default
    return DEFAULT_API_URL;
}

window.API_URL = getApiUrl();
