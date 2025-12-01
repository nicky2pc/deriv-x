#pragma once

#include <string>
#include <vector>
#include <cmath>

namespace derivx {

/**
 * Тип опциона
 */
enum class OptionType {
    CALL,
    PUT
};

/**
 * Позиция по опциону
 */
enum class OptionPosition {
    LONG,
    SHORT
};

/**
 * Структура опциона
 */
struct Option {
    OptionType type;
    OptionPosition position;
    double strike;
    double premium;
    int quantity;
    
    Option() : type(OptionType::CALL), position(OptionPosition::LONG), 
               strike(0.0), premium(0.0), quantity(1) {}
};

/**
 * Параметры рынка для расчета опционов
 */
struct MarketParams {
    double spotPrice;
    double volatility;      // Годовая волатильность (в долях, не процентах)
    double riskFreeRate;   // Безрисковая ставка (в долях)
    double timeToExpiration; // Время до экспирации в годах
    double dividendYield;   // Дивидендная доходность (в долях)
    
    MarketParams() : spotPrice(100.0), volatility(0.2), riskFreeRate(0.05),
                     timeToExpiration(30.0/365.0), dividendYield(0.0) {}
};

/**
 * Греки опциона
 */
struct Greeks {
    double delta;
    double gamma;
    double theta;  // В день
    double vega;   // На 1% изменения волатильности
    double rho;    // На 1% изменения ставки
    
    Greeks() : delta(0.0), gamma(0.0), theta(0.0), vega(0.0), rho(0.0) {}
};

/**
 * Класс для расчета цен опционов по модели Black-Scholes
 */
class OptionPricing {
public:
    /**
     * Расчет цены опциона по модели Black-Scholes
     */
    static double calculateBlackScholes(
        OptionType type,
        double S,  // Spot price
        double K,  // Strike
        double T,  // Time to expiration (years)
        double sigma,  // Volatility
        double r,  // Risk-free rate
        double q = 0.0  // Dividend yield
    );
    
    /**
     * Расчет греков опциона
     */
    static Greeks calculateGreeks(
        OptionType type,
        double S,
        double K,
        double T,
        double sigma,
        double r,
        double q = 0.0
    );
    
    /**
     * Расчет payoff опциона при заданной цене базового актива
     */
    static double calculatePayoff(
        const Option& option,
        double spotPrice
    );
    
    /**
     * Расчет PNL стратегии (набор опционов) при заданной цене
     */
    static double calculateStrategyPNL(
        const std::vector<Option>& options,
        double spotPrice
    );
    
    /**
     * Генерация точек для графика PNL
     */
    static std::vector<std::pair<double, double>> generatePayoffCurve(
        const std::vector<Option>& options,
        double minPrice,
        double maxPrice,
        int numPoints = 200
    );

private:
    /**
     * Стандартное нормальное распределение (CDF)
     */
    static double normalCDF(double x);
    
    /**
     * Стандартное нормальное распределение (PDF)
     */
    static double normalPDF(double x);
    
    /**
     * Вспомогательные функции для Black-Scholes
     */
    static double calculateD1(double S, double K, double T, double sigma, double r, double q);
    static double calculateD2(double d1, double sigma, double T);
};

} // namespace derivx

