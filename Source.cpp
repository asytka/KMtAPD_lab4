// lab4_variant3_pipes.cpp
#include <windows.h>
#include <iostream>
#include <vector>

// === Дані ===
std::vector<int> poly1 = { 1, 2, 3 };       // 1 + 2x + 3x^2
std::vector<int> poly2 = { 2, 4, 1 };       // 2 + 4x + 1x^2
std::vector<int> poly3 = { 3, 0, 5 };       // 3 + 0x + 5x^2

// === Об'єкти синхронізації ===
HANDLE hEventAtoB;
HANDLE hEventBtoC;

// === Pipe handles ===
HANDLE hReadAB, hWriteAB;
HANDLE hReadBC, hWriteBC;

// === Функція множення многочленів ===
std::vector<int> multiply(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> res(a.size() + b.size() - 1, 0);
    for (size_t i = 0; i < a.size(); ++i)
        for (size_t j = 0; j < b.size(); ++j)
            res[i + j] += a[i] * b[j];
    return res;
}

// === Потік A ===
DWORD WINAPI ThreadA(LPVOID) {
    std::vector<int> result1 = multiply(poly1, poly2);
    std::cout << "[Thread A] Multiplication 1 complete.\n";

    DWORD bytesWritten;
    WriteFile(hWriteAB, result1.data(), result1.size() * sizeof(int), &bytesWritten, NULL);

    SetEvent(hEventAtoB);
    return 0;
}

// === Потік B ===
DWORD WINAPI ThreadB(LPVOID) {
    WaitForSingleObject(hEventAtoB, INFINITE);

    DWORD bytesRead;
    int buffer1[64];
    ReadFile(hReadAB, buffer1, sizeof(buffer1), &bytesRead, NULL);
    std::vector<int> result1(buffer1, buffer1 + bytesRead / sizeof(int));

    std::vector<int> result2 = multiply(result1, poly3);
    std::cout << "[Thread B] Multiplication 2 complete.\n";

    DWORD bytesWritten;
    WriteFile(hWriteBC, result2.data(), result2.size() * sizeof(int), &bytesWritten, NULL);

    SetEvent(hEventBtoC);
    return 0;
}

// === Потік C ===
DWORD WINAPI ThreadC(LPVOID) {
    WaitForSingleObject(hEventBtoC, INFINITE);

    DWORD bytesRead;
    int buffer2[128];
    ReadFile(hReadBC, buffer2, sizeof(buffer2), &bytesRead, NULL);
    std::vector<int> result2(buffer2, buffer2 + bytesRead / sizeof(int));

    std::cout << "[Thread C] Final result: ";
    for (int coef : result2) std::cout << coef << " ";
    std::cout << "\n";

    return 0;
}

int main() {
    // Ініціалізація подій
    hEventAtoB = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEventBtoC = CreateEvent(NULL, FALSE, FALSE, NULL);

    // Створення пайпів
    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    CreatePipe(&hReadAB, &hWriteAB, &saAttr, 0);
    CreatePipe(&hReadBC, &hWriteBC, &saAttr, 0);

    // Створення потоків
    HANDLE hThreads[3];
    hThreads[0] = CreateThread(NULL, 0, ThreadA, NULL, 0, NULL);
    hThreads[1] = CreateThread(NULL, 0, ThreadB, NULL, 0, NULL);
    hThreads[2] = CreateThread(NULL, 0, ThreadC, NULL, 0, NULL);

    // Очікування завершення всіх потоків
    WaitForMultipleObjects(3, hThreads, TRUE, INFINITE);

    // Закриття дескрипторів
    for (int i = 0; i < 3; ++i) CloseHandle(hThreads[i]);
    CloseHandle(hEventAtoB);
    CloseHandle(hEventBtoC);
    CloseHandle(hReadAB); CloseHandle(hWriteAB);
    CloseHandle(hReadBC); CloseHandle(hWriteBC);

    return 0;
}
