# DerivX - Опционные стратегии для криптопар

Веб-приложение для построения и анализа опционных стратегий (страдлы, баттерфляи, кондоры и т.д.) с криптопарами.

## Архитектура

- **Backend**: C++ REST API - рассчитывает стоимость опционов, предоставляет данные для графиков PNL/payoff, использует реальные данные крипторынка (OHLCV) для расчета волатильности и модель Black-Scholes
- **Frontend**: Веб-интерфейс для добавления опционов, настройки параметров и визуализации графиков

## Структура проекта

```
deriv-x/
├── backend/          # C++ REST API
├── frontend/         # Веб-интерфейс
├── scripts/          # Python скрипты для получения OHLCV данных
├── data/            # OHLCV данные криптопар
└── CMakeLists.txt   # Сборка C++ проекта
```

## Требования

### Backend
- C++17 или выше
- CMake 3.15+
- Библиотека для REST API (cpprestsdk)
- JSON библиотека (nlohmann/json) - загружается автоматически через CMake

#### Установка cpprestsdk

**macOS:**
```bash
brew install cpprestsdk
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libcpprest-dev
```

**Из исходников:**
```bash
git clone https://github.com/Microsoft/cpprestsdk.git
cd cpprestsdk
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Frontend
- Современный браузер с поддержкой ES6+
- Chart.js (включен через CDN)
- Math.js (включен через CDN)

### Скрипты
- Python 3.7+
- ccxt библиотека

Установка зависимостей Python:
```bash
cd scripts
pip install -r requirements.txt
```

## Установка и запуск

### 1. Получение OHLCV данных

```bash
cd scripts
pip install -r requirements.txt
python fetch_ohlcv.py
```

Это создаст CSV файлы с историческими данными для основных криптопар в директории `data/`.

**Примечание:** Скрипт использует библиотеку `ccxt` для получения данных с биржи Binance. Убедитесь, что у вас есть интернет-соединение.

### 2. Сборка Backend

```bash
mkdir build
cd build
cmake ..
make
```

Если возникнут проблемы с поиском cpprestsdk, укажите путь явно:
```bash
cmake .. -Dcpprestsdk_DIR=/usr/local/lib/cmake/cpprestsdk
```

### 3. Запуск Backend

```bash
./derivx_api
```

Или с указанием пути к данным:
```bash
./derivx_api ../data
```

API будет доступен на `http://localhost:8080`

**Проверка работоспособности:**
```bash
curl http://localhost:8080/api/health
```

### 4. Запуск Frontend

Откройте `frontend/index.html` в браузере или используйте локальный веб-сервер:

```bash
cd frontend
python -m http.server 8000
```

Затем откройте `http://localhost:8000` в браузере.

## API Endpoints

- `GET /api/health` - Проверка работоспособности
- `POST /api/calculate-option` - Расчет цены опциона
  ```json
  {
    "type": "call",
    "strike": 100.0,
    "spotPrice": 100.0,
    "timeToExpiration": 30,
    "volatility": 20.0,
    "riskFreeRate": 5.0,
    "dividendYield": 0.0
  }
  ```
- `POST /api/calculate-strategy` - Расчет PNL стратегии
  ```json
  {
    "options": [
      {
        "type": "call",
        "position": "long",
        "strike": 100.0,
        "premium": 2.5,
        "quantity": 1
      }
    ],
    "minPrice": 50.0,
    "maxPrice": 150.0,
    "numPoints": 200
  }
  ```
- `POST /api/calculate-greeks` - Расчет греков
- `GET /api/volatility/{symbol}` - Получить волатильность для пары (например: `/api/volatility/BTC/USDT`)
- `GET /api/price/{symbol}` - Получить текущую цену пары
- `GET /api/ohlcv/{symbol}?limit=100` - Получить OHLCV данные

## Использование

1. Выберите криптопару
2. Система автоматически загрузит текущую цену и рассчитает волатильность из исторических данных
3. Добавьте опционы в стратегию
4. Настройте параметры (страйк, тип, позиция)
5. Просматривайте графики PNL и греки в реальном времени

