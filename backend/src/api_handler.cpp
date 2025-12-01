#include "../include/api_handler.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
using json = nlohmann::json;

namespace derivx {

void APIHandler::initialize(const std::string& dataDir) {
    dataDirectory_ = dataDir;
    ohlcvCache_.clear();
}

std::string APIHandler::symbolToFilename(const std::string& symbol) {
    std::string filename = symbol;
    std::replace(filename.begin(), filename.end(), '/', '_');
    return filename + "_ohlcv.csv";
}

std::string APIHandler::getDataFilePath(const std::string& symbol) {
    std::string filename = symbolToFilename(symbol);
    return dataDirectory_ + "/" + filename;
}

std::vector<OHLCV> APIHandler::loadOHLCVForSymbol(const std::string& symbol) {
    // Проверяем кэш
    auto it = ohlcvCache_.find(symbol);
    if (it != ohlcvCache_.end()) {
        return it->second;
    }
    
    // Загружаем из файла
    std::string filepath = getDataFilePath(symbol);
    std::vector<OHLCV> data = VolatilityCalculator::loadOHLCVFromCSV(filepath);
    
    // Если файл не найден, пробуем альтернативные варианты имен
    if (data.empty()) {
        // Пробуем заменить _ на /
        std::string altSymbol = symbol;
        std::replace(altSymbol.begin(), altSymbol.end(), '_', '/');
        std::string altFilepath = getDataFilePath(altSymbol);
        data = VolatilityCalculator::loadOHLCVFromCSV(altFilepath);
    }
    
    // Кэшируем
    ohlcvCache_[symbol] = data;
    
    return data;
}

std::string APIHandler::handleCalculateOption(const std::string& requestBody) {
    try {
        json request = json::parse(requestBody);
        
        // Парсим параметры
        std::string typeStr = request.value("type", "call");
        OptionType type = (typeStr == "put") ? OptionType::PUT : OptionType::CALL;
        
        double S = request.value("spotPrice", 100.0);
        double K = request.value("strike", 100.0);
        double T = request.value("timeToExpiration", 30.0) / 365.0; // Конвертируем дни в годы
        double sigma = request.value("volatility", 0.2) / 100.0; // Конвертируем проценты в доли
        double r = request.value("riskFreeRate", 5.0) / 100.0;
        double q = request.value("dividendYield", 0.0) / 100.0;
        
        // Валидация входных параметров
        if (S <= 0 || K <= 0 || T < 0 || sigma < 0) {
            json error;
            error["error"] = "Invalid parameters: spotPrice, strike, timeToExpiration, and volatility must be positive";
            return error.dump();
        }
        
        // Рассчитываем цену
        double price = OptionPricing::calculateBlackScholes(type, S, K, T, sigma, r, q);
        
        // Формируем ответ
        json response;
        response["price"] = price;
        response["type"] = typeStr;
        response["strike"] = K;
        response["spotPrice"] = S;
        response["volatility"] = sigma * 100.0;
        response["timeToExpiration"] = T * 365.0;
        
        return response.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Invalid request: ") + e.what();
        return error.dump();
    }
}

std::string APIHandler::handleCalculateStrategy(const std::string& requestBody) {
    try {
        json request = json::parse(requestBody);
        
        // Парсим опционы
        std::vector<Option> options;
        if (request.contains("options") && request["options"].is_array()) {
            for (const auto& optJson : request["options"]) {
                Option opt;
                std::string typeStr = optJson.value("type", "call");
                opt.type = (typeStr == "put") ? OptionType::PUT : OptionType::CALL;
                
                std::string posStr = optJson.value("position", "long");
                opt.position = (posStr == "short") ? OptionPosition::SHORT : OptionPosition::LONG;
                
                opt.strike = optJson.value("strike", 100.0);
                opt.premium = optJson.value("premium", 0.0);
                opt.quantity = optJson.value("quantity", 1);
                
                options.push_back(opt);
            }
        }
        
        // Параметры для графика
        double minPrice = request.value("minPrice", 0.0);
        double maxPrice = request.value("maxPrice", 200.0);
        int numPoints = request.value("numPoints", 200);
        
        // Автоматически определяем диапазон цен если не задан
        if (minPrice <= 0 || maxPrice <= minPrice) {
            // Находим минимальный и максимальный страйк
            double minStrike = std::numeric_limits<double>::max();
            double maxStrike = std::numeric_limits<double>::min();
            
            for (const auto& option : options) {
                minStrike = std::min(minStrike, option.strike);
                maxStrike = std::max(maxStrike, option.strike);
            }
            
            if (minStrike == std::numeric_limits<double>::max()) {
                minStrike = 0;
                maxStrike = 200;
            }
            
            minPrice = std::max(0.0, minStrike * 0.5);
            maxPrice = maxStrike * 1.5;
        }
        
        // Генерируем кривую payoff
        auto curve = OptionPricing::generatePayoffCurve(options, minPrice, maxPrice, numPoints);
        
        // Формируем ответ
        json response;
        json curveData = json::array();
        
        for (const auto& point : curve) {
            json pointJson;
            pointJson["price"] = point.first;
            pointJson["pnl"] = point.second;
            curveData.push_back(pointJson);
        }
        
        response["curve"] = curveData;
        response["numPoints"] = curve.size();
        response["minPrice"] = minPrice;
        response["maxPrice"] = maxPrice;
        
        return response.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Invalid request: ") + e.what();
        return error.dump();
    }
}

std::string APIHandler::handleCalculateGreeks(const std::string& requestBody) {
    try {
        json request = json::parse(requestBody);
        
        std::string typeStr = request.value("type", "call");
        OptionType type = (typeStr == "put") ? OptionType::PUT : OptionType::CALL;
        
        double S = request.value("spotPrice", 100.0);
        double K = request.value("strike", 100.0);
        double T = request.value("timeToExpiration", 30.0) / 365.0;
        double sigma = request.value("volatility", 0.2) / 100.0;
        double r = request.value("riskFreeRate", 5.0) / 100.0;
        double q = request.value("dividendYield", 0.0) / 100.0;
        
        // Валидация параметров
        if (S <= 0 || K <= 0 || T < 0 || sigma < 0) {
            json error;
            error["error"] = "Invalid parameters for Greeks calculation";
            return error.dump();
        }
        
        Greeks greeks = OptionPricing::calculateGreeks(type, S, K, T, sigma, r, q);
        
        json response;
        response["delta"] = greeks.delta;
        response["gamma"] = greeks.gamma;
        response["theta"] = greeks.theta;
        response["vega"] = greeks.vega;
        response["rho"] = greeks.rho;
        
        return response.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Invalid request: ") + e.what();
        return error.dump();
    }
}

std::string APIHandler::handleGetVolatility(const std::string& symbol) {
    try {
        std::vector<OHLCV> data = loadOHLCVForSymbol(symbol);
        
        if (data.empty()) {
            json error;
            error["error"] = "No data found for symbol: " + symbol;
            error["suggestion"] = "Make sure data file exists in data directory";
            return error.dump();
        }
        
        double volatility = VolatilityCalculator::calculateHistoricalVolatility(data, 30);
        
        json response;
        response["symbol"] = symbol;
        response["volatility"] = volatility; // В долях
        response["volatilityPercent"] = volatility * 100.0; // В процентах
        response["period"] = 30;
        response["dataPoints"] = data.size();
        
        return response.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Error: ") + e.what();
        return error.dump();
    }
}

std::string APIHandler::handleGetCurrentPrice(const std::string& symbol) {
    try {
        std::vector<OHLCV> data = loadOHLCVForSymbol(symbol);
        
        if (data.empty()) {
            json error;
            error["error"] = "No data found for symbol: " + symbol;
            return error.dump();
        }
        
        double price = VolatilityCalculator::getCurrentPrice(data);
        
        json response;
        response["symbol"] = symbol;
        response["price"] = price;
        response["lastUpdate"] = data.back().date;
        
        return response.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Error: ") + e.what();
        return error.dump();
    }
}

std::string APIHandler::handleGetOHLCV(const std::string& symbol, int limit) {
    try {
        std::vector<OHLCV> data = loadOHLCVForSymbol(symbol);
        
        if (data.empty()) {
            json error;
            error["error"] = "No data found for symbol: " + symbol;
            return error.dump();
        }
        
        // Берем последние N свечей
        int n = std::min(limit, static_cast<int>(data.size()));
        std::vector<OHLCV> recent = VolatilityCalculator::getLastNCandles(data, n);
        
        json response;
        json ohlcvArray = json::array();
        
        for (const auto& candle : recent) {
            json candleJson;
            candleJson["date"] = candle.date;
            candleJson["open"] = candle.open;
            candleJson["high"] = candle.high;
            candleJson["low"] = candle.low;
            candleJson["close"] = candle.close;
            candleJson["volume"] = candle.volume;
            ohlcvArray.push_back(candleJson);
        }
        
        response["symbol"] = symbol;
        response["data"] = ohlcvArray;
        response["count"] = ohlcvArray.size();
        
        return response.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["error"] = std::string("Error: ") + e.what();
        return error.dump();
    }
}

} // namespace derivx