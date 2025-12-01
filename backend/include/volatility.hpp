#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>

namespace derivx {

/**
 * Структура OHLCV данных
 */
struct OHLCV {
    std::string date;
    double open;
    double high;
    double low;
    double close;
    double volume;
};

/**
 * Класс для расчета волатильности из OHLCV данных
 */
class VolatilityCalculator {
public:
    /**
     * Загрузка OHLCV данных из CSV файла
     */
    static std::vector<OHLCV> loadOHLCVFromCSV(const std::string& filepath);
    
    /**
     * Расчет исторической волатильности (стандартное отклонение логарифмических доходностей)
     * 
     * @param ohlcv_data Вектор OHLCV данных
     * @param period Период для расчета (в днях), по умолчанию 30
     * @return Годовая волатильность (в долях, не процентах)
     */
    static double calculateHistoricalVolatility(
        const std::vector<OHLCV>& ohlcv_data,
        int period = 30
    );
    
    /**
     * Расчет волатильности по методу Паркинсона (использует high/low)
     */
    static double calculateParkinsonVolatility(
        const std::vector<OHLCV>& ohlcv_data,
        int period = 30
    );
    
    /**
     * Получение текущей цены из последней свечи
     */
    static double getCurrentPrice(const std::vector<OHLCV>& ohlcv_data);
    
    /**
     * Получение последних N свечей
     */
    static std::vector<OHLCV> getLastNCandles(
        const std::vector<OHLCV>& ohlcv_data,
        int n
    );

private:
    /**
     * Расчет логарифмических доходностей
     */
    static std::vector<double> calculateReturns(const std::vector<OHLCV>& ohlcv_data);
    
    /**
     * Расчет стандартного отклонения
     */
    static double calculateStandardDeviation(const std::vector<double>& values);
};

} // namespace derivx

