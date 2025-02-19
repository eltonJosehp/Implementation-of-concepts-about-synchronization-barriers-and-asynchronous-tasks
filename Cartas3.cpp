#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <algorithm>
#include <random>

enum Palo { TREBOL, CORAZON, ESPADA, DIAMANTE };

struct Carta {
    int numero;
    Palo palo;
};

// Generador de cartas aleatorias
std::vector<Carta> generarCartas(int cantidad) {
    std::vector<Carta> mazo;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> num_dist(1, 13);
    std::uniform_int_distribution<int> palo_dist(0, 3);

    for (int i = 0; i < cantidad; ++i) {
        mazo.push_back({num_dist(gen), static_cast<Palo>(palo_dist(gen))});
    }
    return mazo;
}

std::mutex mtx;
std::condition_variable cv;
int hilos_terminados = 0;

// Función para mostrar las cartas
void mostrarCartas(const std::vector<Carta> &cartas) {
    std::lock_guard<std::mutex> lock(mtx);
    for (size_t i = 0; i < std::min(cartas.size(), size_t(50)); ++i) {
        std::cout << cartas[i].numero << " de " << (cartas[i].palo == TREBOL ? "Trébol" :
                                                    cartas[i].palo == CORAZON ? "Corazón" :
                                                    cartas[i].palo == ESPADA ? "Espada" : "Diamante")
                  << std::endl;
    }
}

// Comparador para ordenar las cartas
bool compararCartas(const Carta &a, const Carta &b) {
    return (a.palo < b.palo) || (a.palo == b.palo && a.numero < b.numero);
}

// Función para ordenar un segmento y sincronizar con los demás
void ordenarSegmento(std::vector<Carta> &cartas, int inicio, int fin) {
    std::sort(cartas.begin() + inicio, cartas.begin() + fin, compararCartas);

    // Sincronización manual
    std::unique_lock<std::mutex> lock(mtx);
    hilos_terminados++;
    if (hilos_terminados == 4) {
        cv.notify_one(); // Notifica al hilo principal
    }
}

// Función para ordenar el mazo completo
void ordenarMazo(std::vector<Carta> &mazo) {
    int n = mazo.size();
    int segmento = n / 4;

    // Lanzamos 4 tareas en paralelo
    auto futuro1 = std::async(std::launch::async, ordenarSegmento, std::ref(mazo), 0, segmento);
    auto futuro2 = std::async(std::launch::async, ordenarSegmento, std::ref(mazo), segmento, 2 * segmento);
    auto futuro3 = std::async(std::launch::async, ordenarSegmento, std::ref(mazo), 2 * segmento, 3 * segmento);
    auto futuro4 = std::async(std::launch::async, ordenarSegmento, std::ref(mazo), 3 * segmento, n);

    // Esperamos a que todos los hilos terminen
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [] { return hilos_terminados == 4; });

    // Fusionamos los segmentos ordenados
    std::inplace_merge(mazo.begin(), mazo.begin() + segmento, mazo.begin() + 2 * segmento, compararCartas);
    std::inplace_merge(mazo.begin() + 2 * segmento, mazo.begin() + 3 * segmento, mazo.end(), compararCartas);
    std::inplace_merge(mazo.begin(), mazo.begin() + 2 * segmento, mazo.end(), compararCartas);
}

int main() {
    std::vector<Carta> mazo = generarCartas(100000);

    std::cout << "Cartas desordenadas (primeras 20):" << std::endl;
    mostrarCartas(mazo);

    ordenarMazo(mazo);

    std::cout << "\nCartas ordenadas (primeras 20):" << std::endl;
    mostrarCartas(mazo);

    return 0;
}


