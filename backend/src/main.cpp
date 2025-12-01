#include <iostream>
#include <string>
#include <fstream>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include "../include/api_handler.hpp"
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
using namespace std;

const string API_BASE_URL = "http://localhost:8080";
const string DATA_DIR = "../data";

derivx::APIHandler apiHandler;

// CORS headers
void addCorsHeaders(http_response& response) {
    response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
    response.headers().add(U("Access-Control-Allow-Methods"), U("GET, POST, OPTIONS"));
    response.headers().add(U("Access-Control-Allow-Headers"), U("Content-Type"));
}

// Health check endpoint
void handleHealth(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    
    json::value responseJson;
    responseJson[U("status")] = json::value::string(U("ok"));
    responseJson[U("service")] = json::value::string(U("DerivX API"));
    responseJson[U("version")] = json::value::string(U("1.0.0"));
    
    response.set_body(responseJson);
    request.reply(response);
}

// Calculate option price
void handleCalculateOption(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    
    request.extract_string()
        .then([&response](utility::string_t body) {
            try {
                string result = apiHandler.handleCalculateOption(utility::conversions::to_utf8string(body));
                response.set_body(utility::conversions::to_string_t(result));
                response.headers().set_content_type(U("application/json"));
            } catch (const exception& e) {
                response.set_status_code(status_codes::BadRequest);
                json::value errorJson;
                errorJson[U("error")] = json::value::string(utility::conversions::to_string_t(e.what()));
                response.set_body(errorJson);
            }
        })
        .wait();
    
    request.reply(response);
}

// Calculate strategy PNL
void handleCalculateStrategy(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    
    request.extract_string()
        .then([&response](utility::string_t body) {
            try {
                string result = apiHandler.handleCalculateStrategy(utility::conversions::to_utf8string(body));
                response.set_body(utility::conversions::to_string_t(result));
                response.headers().set_content_type(U("application/json"));
            } catch (const exception& e) {
                response.set_status_code(status_codes::BadRequest);
                json::value errorJson;
                errorJson[U("error")] = json::value::string(utility::conversions::to_string_t(e.what()));
                response.set_body(errorJson);
            }
        })
        .wait();
    
    request.reply(response);
}

// Calculate Greeks
void handleCalculateGreeks(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    
    request.extract_string()
        .then([&response](utility::string_t body) {
            try {
                string result = apiHandler.handleCalculateGreeks(utility::conversions::to_utf8string(body));
                response.set_body(utility::conversions::to_string_t(result));
                response.headers().set_content_type(U("application/json"));
            } catch (const exception& e) {
                response.set_status_code(status_codes::BadRequest);
                json::value errorJson;
                errorJson[U("error")] = json::value::string(utility::conversions::to_string_t(e.what()));
                response.set_body(errorJson);
            }
        })
        .wait();
    
    request.reply(response);
}

// Get volatility for symbol
void handleGetVolatility(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    
    // Извлекаем symbol из пути
    auto path = request.relative_uri().path();
    auto pathParts = uri::split_path(path);
    
    if (pathParts.size() < 3) {
        response.set_status_code(status_codes::BadRequest);
        json::value errorJson;
        errorJson[U("error")] = json::value::string(U("Symbol not specified"));
        response.set_body(errorJson);
        request.reply(response);
        return;
    }
    
    string symbol = utility::conversions::to_utf8string(pathParts[2]);
    cout << "Getting volatility for symbol: " << symbol << endl;
    
    string result = apiHandler.handleGetVolatility(symbol);
    
    response.set_body(utility::conversions::to_string_t(result));
    response.headers().set_content_type(U("application/json"));
    request.reply(response);
}

// Get current price for symbol
void handleGetCurrentPrice(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    
    auto path = request.relative_uri().path();
    auto pathParts = uri::split_path(path);
    
    if (pathParts.size() < 3) {
        response.set_status_code(status_codes::BadRequest);
        json::value errorJson;
        errorJson[U("error")] = json::value::string(U("Symbol not specified"));
        response.set_body(errorJson);
        request.reply(response);
        return;
    }
    
    string symbol = utility::conversions::to_utf8string(pathParts[2]);
    cout << "Getting current price for symbol: " << symbol << endl;
    
    string result = apiHandler.handleGetCurrentPrice(symbol);
    
    response.set_body(utility::conversions::to_string_t(result));
    response.headers().set_content_type(U("application/json"));
    request.reply(response);
}

// Get OHLCV data
void handleGetOHLCV(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    
    auto path = request.relative_uri().path();
    auto pathParts = uri::split_path(path);
    
    if (pathParts.size() < 3) {
        response.set_status_code(status_codes::BadRequest);
        json::value errorJson;
        errorJson[U("error")] = json::value::string(U("Symbol not specified"));
        response.set_body(errorJson);
        request.reply(response);
        return;
    }
    
    string symbol = utility::conversions::to_utf8string(pathParts[2]);
    
    // Получаем limit из query параметров
    int limit = 100;
    auto query = uri::split_query(request.relative_uri().query());
    if (query.find(U("limit")) != query.end()) {
        limit = stoi(utility::conversions::to_utf8string(query[U("limit")]));
    }
    
    cout << "Getting OHLCV data for symbol: " << symbol << " (limit: " << limit << ")" << endl;
    
    string result = apiHandler.handleGetOHLCV(symbol, limit);
    
    response.set_body(utility::conversions::to_string_t(result));
    response.headers().set_content_type(U("application/json"));
    request.reply(response);
}

// Handle OPTIONS requests for CORS
void handleOptions(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    request.reply(response);
}

// Handle root path - API info
void handleRoot(http_request request) {
    http_response response(status_codes::OK);
    addCorsHeaders(response);
    
    json::value apiInfo;
    apiInfo[U("service")] = json::value::string(U("DerivX API"));
    apiInfo[U("version")] = json::value::string(U("1.0.0"));
    apiInfo[U("status")] = json::value::string(U("running"));
    
    json::value endpoints;
    endpoints[U("health")] = json::value::string(U("GET /api/health"));
    endpoints[U("calculateOption")] = json::value::string(U("POST /api/calculate-option"));
    endpoints[U("calculateStrategy")] = json::value::string(U("POST /api/calculate-strategy"));
    endpoints[U("calculateGreeks")] = json::value::string(U("POST /api/calculate-greeks"));
    endpoints[U("getVolatility")] = json::value::string(U("GET /api/volatility/{symbol}"));
    endpoints[U("getPrice")] = json::value::string(U("GET /api/price/{symbol}"));
    endpoints[U("getOHLCV")] = json::value::string(U("GET /api/ohlcv/{symbol}"));
    
    apiInfo[U("endpoints")] = endpoints;
    apiInfo[U("note")] = json::value::string(U("Use BTC_USDT or BTC/USDT format for symbols"));
    
    response.set_body(apiInfo);
    response.headers().set_content_type(U("application/json"));
    request.reply(response);
}

int main(int argc, char* argv[]) {
    cout << "Starting DerivX API server..." << endl;
    
    // Инициализация API handler
    string dataDir = DATA_DIR;
    if (argc > 1) {
        dataDir = argv[1];
    }
    
    apiHandler.initialize(dataDir);
    cout << "Data directory: " << dataDir << endl;
    cout << "API Base URL: " << API_BASE_URL << endl;
    
    // Создание HTTP listener
    http_listener listener(utility::conversions::to_string_t(API_BASE_URL));
    
    // Регистрация обработчиков
    listener.support(methods::GET, [](http_request request) {
        auto path = request.relative_uri().path();
        
        if (path == U("/") || path == U("")) {
            handleRoot(request);
        } else if (path == U("/api/health")) {
            handleHealth(request);
        } else if (path.find(U("/api/volatility/")) == 0) {
            handleGetVolatility(request);
        } else if (path.find(U("/api/price/")) == 0) {
            handleGetCurrentPrice(request);
        } else if (path.find(U("/api/ohlcv/")) == 0) {
            handleGetOHLCV(request);
        } else {
            request.reply(status_codes::NotFound);
        }
    });
    
    listener.support(methods::POST, [](http_request request) {
        auto path = request.relative_uri().path();
        
        if (path == U("/api/calculate-option")) {
            handleCalculateOption(request);
        } else if (path == U("/api/calculate-strategy")) {
            handleCalculateStrategy(request);
        } else if (path == U("/api/calculate-greeks")) {
            handleCalculateGreeks(request);
        } else {
            request.reply(status_codes::NotFound);
        }
    });
    
    listener.support(methods::OPTIONS, handleOptions);
    
    try {
        listener.open()
            .then([]() {
                cout << "DerivX API server is listening on " << API_BASE_URL << endl;
                cout << "Available endpoints:" << endl;
                cout << "  GET  /api/health" << endl;
                cout << "  POST /api/calculate-option" << endl;
                cout << "  POST /api/calculate-strategy" << endl;
                cout << "  POST /api/calculate-greeks" << endl;
                cout << "  GET  /api/volatility/{symbol}" << endl;
                cout << "  GET  /api/price/{symbol}" << endl;
                cout << "  GET  /api/ohlcv/{symbol}" << endl;
            })
            .wait();
        
        cout << "Press Enter to exit..." << endl;
        string line;
        getline(cin, line);
        
        listener.close().wait();
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}