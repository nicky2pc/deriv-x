#include "../include/volatility.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace derivx {

std::vector<OHLCV> VolatilityCalculator::loadOHLCVFromCSV(const std::string& filepath) {
    std::vector<OHLCV> data;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        return data;
    }
    
    std::string line;
    bool isFirstLine = true;
    
    while (std::getline(file, line)) {
        if (isFirstLine) {
            isFirstLine = false;
            continue; // Пропускаем заголовок
        }
        
        if (line.empty()) {
            continue;
        }
        
        std::istringstream iss(line);
        std::string token;
        OHLCV ohlcv;
        
        // Парсим CSV (date,open,high,low,close,volume)
        std::vector<std::string> tokens;
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 6) {
            ohlcv.date = tokens[0];
            ohlcv.open = std::stod(tokens[1]);
            ohlcv.high = std::stod(tokens[2]);
            ohlcv.low = std::stod(tokens[3]);
            ohlcv.close = std::stod(tokens[4]);
            ohlcv.volume = std::stod(tokens[5]);
            
            data.push_back(ohlcv);
        }
    }
    
    file.close();
    return data;
}

std::vector<double> VolatilityCalculator::calculateReturns(const std::vector<OHLCV>& ohlcv_data) {
    std::vector<double> returns;
    
    if (ohlcv_data.size() < 2) {
        return returns;
    }
    
    returns.reserve(ohlcv_data.size() - 1);
    
    for (size_t i = 1; i < ohlcv_data.size(); ++i) {
        if (ohlcv_data[i-1].close > 0.0) {
            double ret = std::log(ohlcv_data[i].close / ohlcv_data[i-1].close);
            returns.push_back(ret);
        }
    }
    
    return returns;
}

double VolatilityCalculator::calculateStandardDeviation(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    
    // Среднее значение
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    
    // Дисперсия
    double variance = 0.0;
    for (double value : values) {
        variance += (value - mean) * (value - mean);
    }
    variance /= values.size();
    
    return std::sqrt(variance);
}

double VolatilityCalculator::calculateHistoricalVolatility(
    const std::vector<OHLCV>& ohlcv_data,
    int period
) {
    if (ohlcv_data.size() < 2) {
        return 0.2; // Значение по умолчанию
    }
    
    // Берем последние N свечей
    int n = std::min(static_cast<int>(ohlcv_data.size()), period);
    std::vector<OHLCV> recent_data(
        ohlcv_data.end() - n,
        ohlcv_data.end()
    );
    
    // Рассчитываем логарифмические доходности
    std::vector<double> returns = calculateReturns(recent_data);
    
    if (returns.empty()) {
        return 0.2; // Значение по умолчанию
    }
    
    // Стандартное отклонение доходностей
    double stdDev = calculateStandardDeviation(returns);
    
    // Годовая волатильность (умножаем на sqrt(252) для дневных данных)
    // Предполагаем, что данные дневные
    double annualVolatility = stdDev * std::sqrt(252.0);
    
    return annualVolatility;
}

double VolatilityCalculator::calculateParkinsonVolatility(
    const std::vector<OHLCV>& ohlcv_data,
    int period
) {
    if (ohlcv_data.size() < 1) {
        return 0.2;
    }
    
    int n = std::min(static_cast<int>(ohlcv_data.size()), period);
    std::vector<OHLCV> recent_data(
        ohlcv_data.end() - n,
        ohlcv_data.end()
    );
    
    double sum = 0.0;
    int count = 0;
    
    for (const auto& candle : recent_data) {
        if (candle.low > 0.0 && candle.high > candle.low) {
            double hl_ratio = std::log(candle.high / candle.low);
            sum += hl_ratio * hl_ratio;
            count++;
        }
    }
    
    if (count == 0) {
        return 0.2;
    }
    
    // Формула Паркинсона
    double variance = (1.0 / (4.0 * std::log(2.0))) * (sum / count);
    double dailyVolatility = std::sqrt(variance);
    
    // Годовая волатильность
    return dailyVolatility * std::sqrt(252.0);
}

double VolatilityCalculator::getCurrentPrice(const std::vector<OHLCV>& ohlcv_data) {
    if (ohlcv_data.empty()) {
        return 0.0;
    }
    
    return ohlcv_data.back().close;
}

std::vector<OHLCV> VolatilityCalculator::getLastNCandles(
    const std::vector<OHLCV>& ohlcv_data,
    int n
) {
    if (ohlcv_data.size() <= static_cast<size_t>(n)) {
        return ohlcv_data;
    }
    
    return std::vector<OHLCV>(
        ohlcv_data.end() - n,
        ohlcv_data.end()
    );
}

} // namespace derivx

