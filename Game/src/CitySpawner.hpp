#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <filesystem>
#include <random>
#include <queue>
#include <optional>

enum class CountryState { LOCKED, UNLOCKED, BANNED, HOVERED };

struct Country {
    std::string name;
    CountryState state;
};

struct City {
    std::string name;
    int population;
    bool capital;
    std::string country;
    
    glm::vec2 coord;
};

struct UnlockableCityData {
    std::string name;
    size_t currentCity;
    int population;
};

struct CitySpawnerSave {
    std::vector<UnlockableCityData> possibleCountries;
};

class CitySpawner {
private:
    //The inverse of the probability of a city spawn
    static constexpr int SPAWN_FREQUENCY = 50; //TODO: must change it probably
    inline static const std::filesystem::path AIRPORTS_DATA_FILE = "resources/airports.json";

public:
    //Time is more random in MinGW than the random_device
    CitySpawner(): generator(time(nullptr)) {}

    std::vector<std::pair<std::string, int>> getCityIndices(const std::vector<City>& cities) const;
    std::vector<City> getCityVector(const std::vector<std::pair<std::string, int>>& indices) const;

    void load();

    std::optional<City> getRandomCity();
    void addCountry(const std::string& country);

private:
    std::unordered_map<std::string, std::vector<City>> cities;
    std::mt19937_64 generator;

    std::queue<City> pendingCities;
    std::vector<UnlockableCityData> possibleCountries;

};