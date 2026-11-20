#include <iostream>
#include <fstream>
#include <string>
#include <limits>   // Для std::numeric_limits
#include <random>
#include <thread>
#include <locale>
#include <windows.h> 
#include <chrono>
#include <algorithm> // Для std::min

// Вспомогательные функции

int Dead_by_hunger(int people, int eaten) {
    int people_fed = eaten / 20;
    if (people_fed >= people) {
        return 0; // Все сыты
    } else {
        return people - people_fed; // Умершие от голода
    }
}

int Immigrants(int dead, int harvest, double bush) {
    // static_cast<int> лучше использовать чтобы контролировать округление
    int immig = (dead / 2) + static_cast<int>((5.0 - harvest) * bush / 600.0) + 1;
    if (immig < 0) return 0;
    if (immig > 50) return 50;
    return immig;
}

// Функции сохранения и загрузки

void SaveGame(int round, double bush, int acr, int people) {
    std::ofstream saveFile("save.txt");
    if (saveFile.is_open()) {
        saveFile << round << "\n"
                 << bush << "\n"
                 << acr << "\n"
                 << people << "\n";
        saveFile.close();
        std::cout << "\n>>> Игра успешно сохранена!\n";
    } else {
        std::cout << "\n>>> Ошибка: не удалось создать файл сохранения.\n";
    }
}

bool LoadGame(int& round, double& bush, int& acr, int& people) {
    std::ifstream loadFile("save.txt");
    if (loadFile.is_open() && (loadFile >> round >> bush >> acr >> people)) {
        loadFile.close();
        std::cout << "\n>>> Игра успешно загружена! Продолжаем с " << round << " года.\n";
        return true;
    }
    std::cout << "\n>>> Файл сохранения не найден или поврежден. Начинаем новую игру.\n";
    return false;
}

// Основной игровой цикл

void PlayGame(int& round, double& bush, int& acr, int& people) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> price(17, 26);
    std::uniform_int_distribution<> harv(1, 6);
    std::uniform_real_distribution<> rat(0.0, 0.07);
    std::uniform_real_distribution<> plag(0, 100);

    while (round <= 10) {
        int acr_price = price(gen);
        
        std::cout << "\n=========================================\n";
        std::cout << "ГОД ПРАВЛЕНИЯ: " << round << "\n";
        std::cout << "В закромах: " << bush << " бушелей пшеницы\n";
        std::cout << "Земли: " << acr << " акров\n";
        std::cout << "Население: " << people << " человек\n";
        std::cout << "Текущая цена акра: " << acr_price << " бушелей\n";
        std::cout << "=========================================\n";

        if (round == 1) {
            std::cout << "Поздравляю вас, мой повелитель! Сегодня начинается ваше правление.\n";
            std::cout << "Наше главное богатство - пшеница. Люди, которым не хватит зерна, не доживут до следующего года.\n";
            // std::cout << "Посев 1 акра стоит 0.5 бушеля.\n";
        }

        int eaten = 0, bougt = 0, pos = 0;

        // Блок ввода с валидацией
        while (true) { 
            std::cout << "\nСколько бушелей пшеницы повелеваешь съесть? ";
            while (!(std::cin >> eaten) || eaten < 0) {
                std::cout << "Опечатка в документе. Введите целое число >= 0: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            std::cout << "Сколько акров повелеваешь купить? ";
            while (!(std::cin >> bougt) || bougt < 0) {
                std::cout << "Опечатка в документе. Введите целое число >= 0: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            std::cout << "Сколько акров повелеваешь засеять? ";
            while (!(std::cin >> pos) || pos < 0) {
                std::cout << "Опечатка в документе. Введите целое число >= 0: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }

            // Расход пшеницы
            double total_cost = eaten + (pos * 0.5) + (bougt * acr_price);
            
            // Проверяем хватит лизерна и земли для посева
            if (total_cost > bush || pos > (acr + bougt)) {
                std::cout << "\nПомилуйте, мой повелитель! У нас только " << people << " человек, " 
                          << acr << " акров земли и " << bush << " бушелей пшеницы.\n";
                std::cout << "Пожалуйста, пересмотрите свои приказы.\n";
                continue; // Возвращаемся к началу ввода
            } else {
                break; // Всё корректно, выходим из цикла ввода
            }
        }

        // Применение решений игрока
        bush -= (eaten + (pos * 0.5) + (bougt * acr_price));
        acr += bougt;

        // Isnt this a consequence of my decigions
        int dead = Dead_by_hunger(people, eaten);
        if (dead > 0) {
            std::cout << dead << " человек умерло с голоду.\n";
            // Проверка на бунт (если умерло больше 45% населения)
            if (dead * 100 > 45 * people) {
                std::cout << "Оставшиеся в живых подходят к дворцу с вилами. Нам некуда бежать, мой повелитель.\n";
                std::cout << "Ваше правление закончилось позором.\n";
                return; // Выход из функции
            }
        }
        people -= dead;

        // Чума (15% шанс)
        int plague = plag(gen);
        if (plague < 15) {
            int plague_deaths = people / 2;
            people -= plague_deaths;
            std::cout << "На нашу великую страну нашла ужасная чума, унесшая жизни " << plague_deaths << " человек!\n";
        }

        // Урожай
        // 1 человек может обработать 10 акров. Мы не можем собрать больше, чем засеяли (pos).
        int max_farmable_acres = people * 10;
        int actual_farmed = std::min(pos, max_farmable_acres);
        int one_acr_harv = harv(gen);
        int harvest = actual_farmed * one_acr_harv;
        
        bush += harvest; // Добавляем урожай в закрома
        std::cout << "Собрано " << harvest << " бушелей пшеницы (по " << one_acr_harv 
                  << " с акра). Засеяно было: " << actual_farmed << " акров.\n";

        // Ты крыса да я крыса
        double rat_chance = rat(gen);
        double eaten_by_rats = rat_chance * bush;
        bush -= eaten_by_rats;
        std::cout << "Крысы испортили и съели " << eaten_by_rats << " бушелей.\n";

        // Иммигранты
        int immig = Immigrants(dead, harvest, bush);
        if (immig > 0) {
            people += immig;
            std::cout << "?? " << immig << " человек прибыли в наш прекрасный город в поисках лучшей жизни.\n";
        }

        std::cout << "\n--- ИТОГИ ГОДА ---\n";
        std::cout << "Земли: " << acr << " акров | Население: " << people << " | Пшеница: " << bush << " бушелей.\n";

        //Логика сохранения
        std::cout << "\nЖелаете сохранить игру? (д/н): ";
        char save_choice;
        std::cin >> save_choice;
        if (save_choice == 'д' || save_choice == 'Д' || save_choice == 'y' || save_choice == 'Y') {
            SaveGame(round, bush, acr, people);
        }

        round++; // Переход к следующему году
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "\n=========================================\n";
    std::cout << "Поздравляем! Вы успешно правили 10 лет.\n";
    std::cout << "Финальный результат: " << people << " подданных, " << bush << " бушелей, " << acr << " акров земли.\n";
    std::cout << "=========================================\n";
}

int main() {
    // Установка русской локали для корректного вывода
    // SetConsoleOutputCP(65001);
    std::setlocale(LC_ALL, "ru_RU.UTF-8");
    
    // Начальные параметры
    int round = 1;
    double bush = 2800.0;
    int acr = 1000;
    int people = 100;

    std::cout << "Добро пожаловать в игру 'Хаммураби' by Karma!\n";
    std::cout << "1. Начать новую игру\n";
    std::cout << "2. Загрузить сохранение\n";
    std::cout << "Ваш выбор: ";
    
    int choice;
    std::cin >> choice;

    if (choice == 2) {
        // Пытаемся загрузить. Если не удалось, переменные останутся с начальными значениями, и round сбросится на 1
        if (!LoadGame(round, bush, acr, people)) {
            round = 1; // На случай, если загрузка провалилась, начинаем сначала
        }
    }

    // Передаем переменные по ссылке (&), чтобы функция PlayGame могла их изменять
    PlayGame(round, bush, acr, people);

    return 0;
}