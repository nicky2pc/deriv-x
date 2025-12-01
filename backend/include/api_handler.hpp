#pragma once

#include "option_pricing.hpp"
#include "volatility.hpp"
#include <string>
#include <vector>
#include <map>

namespace derivx {

/**
 * Класс для обработки REST API запросов
 */
class APIHandler {
public:
    /**
     * Инициализация API handler
     * @param dataDir Путь к директории с OHLCV данными
     */
    void initialize(const std::string& dataDir);
    
    /**
     * Обработка запроса на расчет цены опциона
     */
    std::string handleCalculateOption(const std::string& requestBody);
    
    /**
     * Обработка запроса на расчет PNL стратегии
     */
    std::string handleCalculateStrategy(const std::string& requestBody);
    
    /**
     * Обработка запроса на расчет греков
     */
    std::string handleCalculateGreeks(const std::string& requestBody);
    
    /**
     * Обработка запроса на получение волатильности
     */
    std::string handleGetVolatility(const std::string& symbol);
    
    /**
     * Обработка запроса на получение текущей цены
     */
    std::string handleGetCurrentPrice(const std::string& symbol);
    
    /**
     * Обработка запроса на получение OHLCV данных
     */
    std::string handleGetOHLCV(const std::string& symbol, int limit = 100);

private:
    std::string dataDirectory_;
    std::map<std::string, std::vector<OHLCV>> ohlcvCache_;
    
    /**
     * Загрузка OHLCV данных для символа
     */
    std::vector<OHLCV> loadOHLCVForSymbol(const std::string& symbol);
    
    /**
     * Получение пути к файлу данных для символа
     */
    std::string getDataFilePath(const std::string& symbol);
    
    /**
     * Конвертация символа в имя файла
     */
    std::string symbolToFilename(const std::string& symbol);
};

} // namespace derivx

