#include "../include/option_pricing.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;
namespace derivx {

// Константа для нормального распределения
const double INV_SQRT_2PI = 1.0 / std::sqrt(2.0 * M_PI);

double OptionPricing::normalCDF(double x) {
    // Аппроксимация нормального CDF (формула Абрамовица и Стигана)
    const double a1 =  0.254829592;
    const double a2 = -0.284496736;
    const double a3 =  1.421413741;
    const double a4 = -1.453152027;
    const double a5 =  1.061405429;
    const double p  =  0.3275911;
    
    int sign = 1;
    if (x < 0) {
        sign = -1;
        x = -x;
    }
    
    double t = 1.0 / (1.0 + p * x);
    double y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * std::exp(-x * x / 2.0);
    
    return 0.5 * (1.0 + sign * y);
}

double OptionPricing::normalPDF(double x) {
    return INV_SQRT_2PI * std::exp(-0.5 * x * x);
}

double OptionPricing::calculateD1(double S, double K, double T, double sigma, double r, double q) {
    if (T <= 0.0 || sigma <= 0.0) {
        return 0.0;
    }
    return (std::log(S / K) + (r - q + 0.5 * sigma * sigma) * T) / (sigma * std::sqrt(T));
}

double OptionPricing::calculateD2(double d1, double sigma, double T) {
    return d1 - sigma * std::sqrt(T);
}

double OptionPricing::calculateBlackScholes(
    OptionType type,
    double S,
    double K,
    double T,
    double sigma,
    double r,
    double q
) {
    // Если время истекло, возвращаем внутреннюю стоимость
    if (T <= 0.0) {
        if (type == OptionType::CALL) {
            return std::max(S - K, 0.0);
        } else {
            return std::max(K - S, 0.0);
        }
    }

    cout << "Volatility: " << sigma << endl;
    
    if (sigma <= 0.0) {
        // Если волатильность нулевая, возвращаем внутреннюю стоимость
        if (type == OptionType::CALL) {
            return std::max(S * std::exp(-q * T) - K * std::exp(-r * T), 0.0);
        } else {
            return std::max(K * std::exp(-r * T) - S * std::exp(-q * T), 0.0);
        }
    }
    
    double d1 = calculateD1(S, K, T, sigma, r, q);
    double d2 = calculateD2(d1, sigma, T);
    
    double N_d1 = normalCDF(d1);
    double N_d2 = normalCDF(d2);
    double N_neg_d1 = normalCDF(-d1);
    double N_neg_d2 = normalCDF(-d2);
    
    if (type == OptionType::CALL) {
        return S * std::exp(-q * T) * N_d1 - K * std::exp(-r * T) * N_d2;
    } else {
        return K * std::exp(-r * T) * N_neg_d2 - S * std::exp(-q * T) * N_neg_d1;
    }
}

Greeks OptionPricing::calculateGreeks(
    OptionType type,
    double S,
    double K,
    double T,
    double sigma,
    double r,
    double q
) {
    Greeks greeks;
    
    if (T <= 0.0 || sigma <= 0.0) {
        // Если время истекло или волатильность нулевая, греки не определены
        return greeks;
    }
    
    double d1 = calculateD1(S, K, T, sigma, r, q);
    double d2 = calculateD2(d1, sigma, T);
    
    double N_d1 = normalCDF(d1);
    double N_d2 = normalCDF(d2);
    double pdf_d1 = normalPDF(d1);
    
    // Delta
    if (type == OptionType::CALL) {
        greeks.delta = std::exp(-q * T) * N_d1;
    } else {
        greeks.delta = std::exp(-q * T) * (N_d1 - 1.0);
    }
    
    // Gamma (одинакова для call и put)
    greeks.gamma = std::exp(-q * T) * pdf_d1 / (S * sigma * std::sqrt(T));
    
    // Theta (в день)
    if (type == OptionType::CALL) {
        greeks.theta = -(S * std::exp(-q * T) * pdf_d1 * sigma) / (2.0 * std::sqrt(T))
                       - r * K * std::exp(-r * T) * N_d2
                       + q * S * std::exp(-q * T) * N_d1;
    } else {
        greeks.theta = -(S * std::exp(-q * T) * pdf_d1 * sigma) / (2.0 * std::sqrt(T))
                       + r * K * std::exp(-r * T) * normalCDF(-d2)
                       - q * S * std::exp(-q * T) * normalCDF(-d1);
    }
    greeks.theta = greeks.theta / 365.0; // Перевод в дневное значение
    
    // Vega (на 1% изменения волатильности)
    greeks.vega = S * std::exp(-q * T) * pdf_d1 * std::sqrt(T) / 100.0;
    
    // Rho (на 1% изменения ставки)
    if (type == OptionType::CALL) {
        greeks.rho = K * T * std::exp(-r * T) * N_d2 / 100.0;
    } else {
        greeks.rho = -K * T * std::exp(-r * T) * normalCDF(-d2) / 100.0;
    }
    
    return greeks;
}

double OptionPricing::calculatePayoff(const Option& option, double spotPrice) {
    double intrinsicValue = 0.0;
    
    if (option.type == OptionType::CALL) {
        intrinsicValue = std::max(spotPrice - option.strike, 0.0);
    } else {
        intrinsicValue = std::max(option.strike - spotPrice, 0.0);
    }
    
    double payoff = intrinsicValue - option.premium;
    
    if (option.position == OptionPosition::SHORT) {
        payoff = -payoff;
    }
    
    return payoff * option.quantity;
}

double OptionPricing::calculateStrategyPNL(
    const std::vector<Option>& options,
    double spotPrice
) {
    double totalPNL = 0.0;
    
    for (const auto& option : options) {
        totalPNL += calculatePayoff(option, spotPrice);
    }
    
    return totalPNL;
}

std::vector<std::pair<double, double>> OptionPricing::generatePayoffCurve(
    const std::vector<Option>& options,
    double minPrice,
    double maxPrice,
    int numPoints
) {
    std::vector<std::pair<double, double>> curve;
    curve.reserve(numPoints);
    
    double step = (maxPrice - minPrice) / (numPoints - 1);
    
    for (int i = 0; i < numPoints; ++i) {
        double price = minPrice + i * step;
        double pnl = calculateStrategyPNL(options, price);
        curve.emplace_back(price, pnl);
    }
    
    return curve;
}

} // namespace derivx

