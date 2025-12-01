#!/usr/bin/env python3
"""
Скрипт для получения OHLCV данных криптопар с бирж
Использует библиотеку ccxt для доступа к различным биржам
"""

import os
import sys
import datetime
import time
from pathlib import Path

try:
    import ccxt
    import pandas as pd
    from dateutil.parser import parse
except ImportError:
    print("Установите зависимости: pip install ccxt pandas python-dateutil")
    sys.exit(1)

# Основные криптопары для анализа
MAJOR_PAIRS = [
    "BTC/USDT",
    "ETH/USDT",
    "BNB/USDT",
    "SOL/USDT",
    "ADA/USDT",
    "XRP/USDT",
    "DOT/USDT",
    "DOGE/USDT",
    "AVAX/USDT",
    "MATIC/USDT"
]

# Биржа по умолчанию
DEFAULT_EXCHANGE = "binance"

# Таймфрейм по умолчанию (1 день)
DEFAULT_TIMEFRAME = "1d"

# Количество свечей (примерно 1 год данных)
DEFAULT_LIMIT = 365


def get_candle_diff_ms(timeframe):
    """Конвертирует таймфрейм в миллисекунды"""
    timeframe_map = {
        "1m": 60000,
        "3m": 3 * 60000,
        "5m": 5 * 60000,
        "15m": 15 * 60000,
        "30m": 30 * 60000,
        "1h": 3600000,
        "2h": 2 * 3600000,
        "4h": 4 * 3600000,
        "6h": 6 * 3600000,
        "8h": 8 * 3600000,
        "12h": 12 * 3600000,
        "1d": 86400000,
        "3d": 3 * 86400000,
        "1w": 604800000,
        "1M": 2629800000
    }
    return timeframe_map.get(timeframe, 86400000)


def fetch_ohlcv_data(
    exchange_name: str = DEFAULT_EXCHANGE,
    symbol: str = "BTC/USDT",
    timeframe: str = DEFAULT_TIMEFRAME,
    since=None,
    limit: int = DEFAULT_LIMIT,
) -> pd.DataFrame:
    """
    Получает OHLCV данные с биржи
    
    Parameters:
    -----------
    exchange_name : str
        Название биржи (например, "binance")
    symbol : str
        Торговая пара (например, "BTC/USDT")
    timeframe : str
        Таймфрейм ("1d", "1h", "4h" и т.д.)
    since : datetime или str
        Начальная дата (опционально)
    limit : int
        Количество свечей
    
    Returns:
    --------
    pd.DataFrame
        DataFrame с колонками: date, open, high, low, close, volume
    """
    
    # Конвертация since в timestamp
    if isinstance(since, str):
        since = parse(since)
    
    if isinstance(since, datetime.datetime):
        since = int(since.timestamp() * 1000)
    
    exchange_name = exchange_name.lower()
    
    # Создание экземпляра биржи
    try:
        exchange_class = getattr(ccxt, exchange_name)
        exchange = exchange_class({
            'enableRateLimit': True,
        })
    except AttributeError:
        raise ValueError(f"Биржа {exchange_name} не найдена")
    
    # Проверка поддержки таймфрейма
    exchange.load_markets()
    
    if timeframe not in exchange.timeframes:
        raise ValueError(
            f"Таймфрейм {timeframe} не поддерживается биржей {exchange_name}. "
            f"Доступные: {list(exchange.timeframes.keys())}"
        )
    
    # Проверка доступности символа
    if symbol not in exchange.symbols:
        raise ValueError(
            f"Пара {symbol} не найдена на бирже {exchange_name}"
        )
    
    # Получение данных
    print(f"Загрузка данных для {symbol} с {exchange_name}...")
    data = exchange.fetch_ohlcv(symbol, timeframe, since, limit)
    
    # Если данных меньше лимита, делаем дополнительные запросы
    rate_limit = exchange.rateLimit / 1000
    
    while len(data) < limit and len(data) > 0:
        since = data[-1][0] + get_candle_diff_ms(timeframe)
        time.sleep(rate_limit)
        
        new_data = exchange.fetch_ohlcv(symbol, timeframe, since, limit - len(data))
        if len(new_data) == 0:
            break
        data += new_data
    
    # Создание DataFrame
    header = ["timestamp", "open", "high", "low", "close", "volume"]
    df = pd.DataFrame(data, columns=header)
    
    # Конвертация timestamp в дату
    df["timestamp"] = df["timestamp"] / 1000
    df["date"] = pd.to_datetime(df["timestamp"], unit="s")
    
    # Переупорядочивание колонок
    df = df[["date", "open", "high", "low", "close", "volume"]]
    
    # Конвертация в числовые типы
    numeric_cols = ["open", "high", "low", "close", "volume"]
    for col in numeric_cols:
        df[col] = pd.to_numeric(df[col])
    
    return df


def save_ohlcv_data(df: pd.DataFrame, symbol: str, output_dir: str = "../data"):
    """Сохраняет OHLCV данные в CSV файл"""
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Создаем безопасное имя файла
    safe_symbol = symbol.replace("/", "_")
    filename = f"{safe_symbol}_ohlcv.csv"
    filepath = output_path / filename
    
    df.to_csv(filepath, index=False)
    print(f"Данные сохранены: {filepath}")
    return filepath


def main():
    """Основная функция"""
    print("=" * 60)
    print("Загрузка OHLCV данных для криптопар")
    print("=" * 60)
    
    # Создаем директорию для данных
    data_dir = Path(__file__).parent.parent / "data"
    data_dir.mkdir(parents=True, exist_ok=True)
    
    # Загружаем данные для каждой пары
    for symbol in MAJOR_PAIRS:
        try:
            df = fetch_ohlcv_data(
                exchange_name=DEFAULT_EXCHANGE,
                symbol=symbol,
                timeframe=DEFAULT_TIMEFRAME,
                limit=DEFAULT_LIMIT
            )
            
            save_ohlcv_data(df, symbol, str(data_dir))
            print(f"✓ {symbol}: {len(df)} свечей загружено")
            print()
            
            # Небольшая задержка между запросами
            time.sleep(1)
            
        except Exception as e:
            print(f"✗ Ошибка при загрузке {symbol}: {e}")
            print()
    
    print("=" * 60)
    print("Загрузка завершена!")
    print(f"Данные сохранены в: {data_dir}")
    print("=" * 60)


if __name__ == "__main__":
    main()

