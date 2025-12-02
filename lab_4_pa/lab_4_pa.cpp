#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <random>

using namespace std;

// --- ПАРАМЕТРИ ВАРІАНТУ 1 (CRC-16) ---
const uint16_t POLYNOMIAL = 0x8005;     // x^16 + x^15 + x^2 + 1
const uint16_t REV_POLYNOMIAL = 0xA001; // Дзеркальний поліном для 0x8005

// Кількість експериментів для усереднення
const int NUM_EXPERIMENTS = 1000;

// Допоміжна функція: розворот бітів у байті
uint8_t reverseByte(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

// Допоміжна функція: розворот бітів у 16-бітному слові
uint16_t reverse16(uint16_t w) {
    uint8_t low = reverseByte(w & 0xFF);
    uint8_t high = reverseByte((w >> 8) & 0xFF);
    return (low << 8) | high;
}

class CRC16Calculator {
private:
    vector<uint16_t> table;
    vector<uint16_t> revTable;

public:
    CRC16Calculator() {
        // Ініціалізація таблиці для Прямого алгоритму
        for (int i = 0; i < 256; i++) {
            uint16_t crc = i << 8;
            for (int j = 0; j < 8; j++) {
                if (crc & 0x8000)
                    crc = (crc << 1) ^ POLYNOMIAL;
                else
                    crc = crc << 1;
            }
            table.push_back(crc);
        }

        // Ініціалізація таблиці для Дзеркального алгоритму
        for (int i = 0; i < 256; i++) {
            uint16_t crc = i;
            for (int j = 0; j < 8; j++) {
                if (crc & 1)
                    crc = (crc >> 1) ^ REV_POLYNOMIAL;
                else
                    crc = crc >> 1;
            }
            revTable.push_back(crc);
        }
    }

    // 1. Простий послідовний алгоритм
    uint16_t simpleSequential(const vector<uint8_t>& data) {
        uint16_t crc = 0;
        for (uint8_t b : data) {
            crc ^= (b << 8);
            for (int i = 0; i < 8; i++) {
                if (crc & 0x8000)
                    crc = (crc << 1) ^ POLYNOMIAL;
                else
                    crc = crc << 1;
            }
        }
        return crc;
    }

    // 2. Табличний алгоритм
    uint16_t tableAlgorithm(const vector<uint8_t>& data) {
        uint16_t crc = 0;
        for (uint8_t b : data) {
            uint8_t pos = (crc >> 8) ^ b;
            crc = (crc << 8) ^ table[pos];
        }
        return crc;
    }

    // 3. Дзеркальний послідовний алгоритм
    uint16_t mirrorSequential(const vector<uint8_t>& data) {
        uint16_t crc = 0;
        for (uint8_t b : data) {
            crc ^= b;
            for (int i = 0; i < 8; i++) {
                if (crc & 1)
                    crc = (crc >> 1) ^ REV_POLYNOMIAL;
                else
                    crc = crc >> 1;
            }
        }
        return crc;
    }

    // 4. Дзеркальний табличний алгоритм
    uint16_t mirrorTable(const vector<uint8_t>& data) {
        uint16_t crc = 0;
        for (uint8_t b : data) {
            uint8_t pos = (crc ^ b) & 0xFF;
            crc = (crc >> 8) ^ revTable[pos];
        }
        return crc;
    }

    // 5. Стандартизований алгоритм (CRC-16/ARC)
    uint16_t standardCRC16_ARC(const vector<uint8_t>& data) {
        uint16_t crc = 0x0000;
        for (uint8_t b : data) {
            uint8_t pos = (crc ^ b) & 0xFF;
            crc = (crc >> 8) ^ revTable[pos];
        }
        return crc ^ 0x0000;
    }
};

int main() {
    // Генерація вхідних даних
    const int DATA_SIZE = 100000;
    vector<uint8_t> data(DATA_SIZE);

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 255);

    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = (uint8_t)dis(gen);
    }

    CRC16Calculator calculator;

    cout << "CRC-16 Analysis (Variant 1: Poly 0x8005)" << endl;
    cout << "Data size: " << DATA_SIZE << " bytes" << endl;
    cout << "Averaging over " << NUM_EXPERIMENTS << " runs." << endl;
    cout << "--------------------------------------------------------" << endl;
    cout << "| Algorithm Type       | CRC Result | Avg Time          |" << endl;
    cout << "--------------------------------------------------------" << endl;

    //  Тест 1: Простий послідовний 
    uint16_t res1 = 0;
    long long totalTime1 = 0;
    // один прогін перед замірами
    calculator.simpleSequential(data);

    for (int i = 0; i < NUM_EXPERIMENTS; i++) {
        auto start = chrono::high_resolution_clock::now();
        res1 = calculator.simpleSequential(data); // Результат перезаписується, але він однаковий
        auto end = chrono::high_resolution_clock::now();
        totalTime1 += chrono::duration_cast<chrono::microseconds>(end - start).count();
    }
    double avg1 = (double)totalTime1 / NUM_EXPERIMENTS;
    cout << "| 1. Simple Sequential | 0x" << hex << uppercase << setw(4) << setfill('0') << res1
        << "     | " << dec << fixed << setprecision(2) << setw(15) << avg1 << " |" << endl;

    // Тест 2: Табличний 
    uint16_t res2 = 0;
    long long totalTime2 = 0;
    calculator.tableAlgorithm(data);

    for (int i = 0; i < NUM_EXPERIMENTS; i++) {
        auto start = chrono::high_resolution_clock::now();
        res2 = calculator.tableAlgorithm(data);
        auto end = chrono::high_resolution_clock::now();
        totalTime2 += chrono::duration_cast<chrono::microseconds>(end - start).count();
    }
    double avg2 = (double)totalTime2 / NUM_EXPERIMENTS;
    cout << "| 2. Table Direct      | 0x" << hex << uppercase << setw(4) << setfill('0') << res2
        << "     | " << dec << fixed << setprecision(2) << setw(15) << avg2 << " |" << endl;

    //  Тест 3: Дзеркальний послідовний 
    uint16_t res3 = 0;
    long long totalTime3 = 0;
    calculator.mirrorSequential(data);

    for (int i = 0; i < NUM_EXPERIMENTS; i++) {
        auto start = chrono::high_resolution_clock::now();
        res3 = calculator.mirrorSequential(data);
        auto end = chrono::high_resolution_clock::now();
        totalTime3 += chrono::duration_cast<chrono::microseconds>(end - start).count();
    }
    double avg3 = (double)totalTime3 / NUM_EXPERIMENTS;
    cout << "| 3. Mirror Sequential | 0x" << hex << uppercase << setw(4) << setfill('0') << res3
        << "     | " << dec << fixed << setprecision(2) << setw(15) << avg3 << " |" << endl;

    // Тест 4: Дзеркальний табличний 
    uint16_t res4 = 0;
    long long totalTime4 = 0;
    calculator.mirrorTable(data);

    for (int i = 0; i < NUM_EXPERIMENTS; i++) {
        auto start = chrono::high_resolution_clock::now();
        res4 = calculator.mirrorTable(data);
        auto end = chrono::high_resolution_clock::now();
        totalTime4 += chrono::duration_cast<chrono::microseconds>(end - start).count();
    }
    double avg4 = (double)totalTime4 / NUM_EXPERIMENTS;
    cout << "| 4. Mirror Table      | 0x" << hex << uppercase << setw(4) << setfill('0') << res4
        << "     | " << dec << fixed << setprecision(2) << setw(15) << avg4 << " |" << endl;

    // --- Тест 5: Стандартний (CRC-16/ARC) ---
    uint16_t res5 = 0;
    long long totalTime5 = 0;
    calculator.standardCRC16_ARC(data);

    for (int i = 0; i < NUM_EXPERIMENTS; i++) {
        auto start = chrono::high_resolution_clock::now();
        res5 = calculator.standardCRC16_ARC(data);
        auto end = chrono::high_resolution_clock::now();
        totalTime5 += chrono::duration_cast<chrono::microseconds>(end - start).count();
    }
    double avg5 = (double)totalTime5 / NUM_EXPERIMENTS;
    cout << "| 5. Standard (ARC)    | 0x" << hex << uppercase << setw(4) << setfill('0') << res5
        << "     | " << dec << fixed << setprecision(2) << setw(15) << avg5 << " |" << endl;

    cout << "--------------------------------------------------------" << endl;
    cout << "All times are in microseconds." << endl;

    return 0;
}