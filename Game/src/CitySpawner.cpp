#include "CitySpawner.hpp"

#include <nlohmann/json.hpp>
#include <fstream>

void CitySpawner::load() {
    using json = nlohmann::json;

    std::ifstream airportFile(AIRPORTS_DATA_FILE);
    json airportData = json::parse(airportFile);

    for(auto& [k, v]: airportData.items()) {
        cities[k] = {};
        for(auto& e: v) {
            City c;
            c.name = e["name"].template get<std::string>();
            c.population = e["population"].template get<int>();
            auto coord = e["coords"].template get<std::vector<float>>();
            c.coord = {coord[0], coord[1]};
            c.capital = e["capital"].template get<bool>();
            c.country = k;

            cities[k].emplace_back(std::move(c));
        }
    }
}

std::optional<City> CitySpawner::getRandomCity() {
    std::optional<City> city;

    if(!pendingCities.empty()) {
        auto c = pendingCities.front();
        pendingCities.pop();
        return c;
    }

    if(possibleCountries.empty() || generator() % SPAWN_FREQUENCY != 0)
        return city;
    
    std::vector<int> populations(possibleCountries.size());
    for(size_t i=0; i<populations.size(); ++i)
        populations[i] = possibleCountries[i].population;
    std::discrete_distribution<int> distribution(populations.begin(), populations.end());

    auto countryIndex = distribution(generator);
    auto i = possibleCountries[countryIndex].currentCity;
    auto country = possibleCountries[countryIndex].name;
    if(i < cities[country].size()) {  
        city = cities[country][i];

        possibleCountries[countryIndex].population = city.value().population;
        possibleCountries[countryIndex].currentCity++;
    }

    return city;
}

void CitySpawner::addCountry(const std::string& country) {
    possibleCountries.emplace_back(country, 1, cities[country][0].population);
    pendingCities.push(cities[country][0]);
}

std::vector<std::pair<std::string, int>> CitySpawner::getCityIndices(const std::vector<City>& cities) const {
    std::vector<std::pair<std::string, int>> indices(cities.size());

    for(int i=0; i<int(cities.size()); ++i) {
        auto cnt = cities[i].country;
        indices[i].first = cities[i].country;
        const auto& cntCities = this->cities.at(cnt);
        indices[i].second = std::find_if(cntCities.begin(), cntCities.end(), 
            [&cities, i](auto c){ return c.name == cities[i].name; }) - cntCities.begin();
    }

    return indices;
}

std::vector<City> CitySpawner::getCityVector(const std::vector<std::pair<std::string, int>>& indices) const {
    std::vector<City> cities(indices.size());

    for(int i=0; i<int(indices.size()); ++i) {
        cities[i] = this->cities.at(indices[i].first).at(indices[i].second);
    }

    return cities;
}
