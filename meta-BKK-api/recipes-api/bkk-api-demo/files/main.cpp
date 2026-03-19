#include <exception>
#include <iostream>
#include <string>

#include <bkk_api/bkk_api.hpp>

int main() {
    try {
        BkkApi api;

        const std::string stop_id = "F02262";
        const auto arrivals = api.get_arrivals_for_station(stop_id);

        std::cout << "Fetched " << arrivals.size() << " arrivals for stop " << stop_id << '\n';
        api.display_arrivals(arrivals);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "bkk-api-demo failed: " << ex.what() << '\n';
        return 1;
    }
}
