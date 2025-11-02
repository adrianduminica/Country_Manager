#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>

// ============================================================================
//                               ResourceStockpile
// ============================================================================
class ResourceStockpile {
    int fuel;
    int manpower;

    static int clampNonNeg(int x) { return x < 0 ? 0 : x; }

public:
    ResourceStockpile() : fuel(0), manpower(0) {}
    ResourceStockpile(int fuel, int manpower)
        : fuel(fuel), manpower(manpower) {}

    int getFuel() const { return fuel; }
    int getManpower() const { return manpower; }

    void add(int dFuel, int dManpower) {
        fuel     = clampNonNeg(fuel     + dFuel);
        manpower = clampNonNeg(manpower + dManpower);
    }

    std::string toString() const {
        std::ostringstream ss;
        ss << "Fuel=" << fuel << ", Manpower=" << manpower;
        return ss.str();
    }
};

std::ostream& operator<<(std::ostream& os, const ResourceStockpile& r) {
    return os << r.toString();
}

// ============================================================================
//                               EquipmentStockpile
// ============================================================================
class EquipmentStockpile {
    long long guns = 0;
    long long artillery = 0;
    long long antiAir = 0;
    long long cas = 0;

public:
    void addGuns(long long n)      { if (n>0) guns      += n; }
    void addArtillery(long long n) { if (n>0) artillery += n; }
    void addAntiAir(long long n)   { if (n>0) antiAir   += n; }
    void addCAS(long long n)       { if (n>0) cas       += n; }

    long long getGuns() const      { return guns; }
    long long getArtillery() const { return artillery; }
    long long getAntiAir() const   { return antiAir; }
    long long getCAS() const       { return cas; }

    std::string toString() const {
        std::ostringstream ss;
        ss << "Guns=" << guns
           << ", Artillery=" << artillery
           << ", AA=" << antiAir
           << ", CAS=" << cas;
        return ss.str();
    }
};

std::ostream& operator<<(std::ostream& os, const EquipmentStockpile& e) {
    return os << e.toString();
}

// ============================================================================
//                                   Province
// ============================================================================
class Province {
    std::string name;
    int population;
    int civFactories;
    int milFactories;
    int infrastructure;
    int steelReserve;
    int oilReserve;

public:
    Province() = default;
    Province(std::string name, int population, int civ, int mil, int infra,
             int steelReserve, int oilReserve)
        : name(std::move(name)), population(population),
          civFactories(civ), milFactories(mil),
          infrastructure(infra),
          steelReserve(steelReserve), oilReserve(oilReserve) {}

    const std::string& getName() const { return name; }
    int getPopulation()  const { return population; }
    int getCiv()         const { return civFactories; }
    int getMil()         const { return milFactories; }
    int getInfra()       const { return infrastructure; }
    int getSteelReserve()const { return steelReserve; }
    int getOilReserve()  const { return oilReserve; }

    void addCiv(int x){ civFactories = std::max(0, civFactories + x); }
    void addMil(int x){ milFactories = std::max(0, milFactories + x); }
    void addInfra(int x){ infrastructure = std::min(10, std::max(0, infrastructure + x)); }

    std::string toString() const {
        std::ostringstream ss;
        ss << "Province(" << name
           << ") pop=" << population
           << ", INFRA=" << infrastructure
           << ", CIV=" << civFactories
           << ", MIL=" << milFactories
           << ", STEEL=" << steelReserve
           << ", OIL=" << oilReserve;
        return ss.str();
    }
};

std::ostream& operator<<(std::ostream& os, const Province& p) {
    return os << p.toString();
}

// ============================================================================
//                                   Construction
// ============================================================================
enum class BuildType { Infra, Civ, Mil, Refinery };

class ConstructionTask {
    BuildType type;
    int provinceIndex;
    double remainingBP;
    double baseCostBP;

public:
    ConstructionTask(BuildType type, int province_index, double remaining_bp, double base_cost_bp)
        : type(type), provinceIndex(province_index),
          remainingBP(remaining_bp), baseCostBP(base_cost_bp) {}

    BuildType getType() const { return type; }
    int getProvinceIndex() const { return provinceIndex; }
    double getRemainingBP() const { return remainingBP; }
    double getBaseCostBP() const { return baseCostBP; }

    void setRemainingBP(double bp) { remainingBP = bp; }

    std::string toString() const {
        const char* tn = type==BuildType::Infra?"INFRA":
                         type==BuildType::Civ?"CIV":
                         type==BuildType::Mil?"MIL":"REFINERY";
        std::ostringstream ss;
        ss << tn << "(prov=" << provinceIndex
           << ", left=" << std::fixed << std::setprecision(1)
           << remainingBP << "/" << baseCostBP << ")";
        return ss.str();
    }
};

std::ostream& operator<<(std::ostream& os, const ConstructionTask& t){
    return os << t.toString();
}

// ============================================================================
//                            Military Production
// ============================================================================
enum class EquipmentType { Gun, Artillery, AntiAir, CAS };

class ProductionLine {
    EquipmentType type;
    int factoriesAssigned;
    double unitCost;

    // costuri implicite
    static constexpr double DEFAULT_COST_GUN       = 10.0;
    static constexpr double DEFAULT_COST_ARTILLERY = 50.0;
    static constexpr double DEFAULT_COST_ANTIAIR   = 40.0;
    static constexpr double DEFAULT_COST_CAS       = 200.0;

    static double defaultCost(EquipmentType t) {
        switch (t) {
            case EquipmentType::Gun:       return DEFAULT_COST_GUN;
            case EquipmentType::Artillery: return DEFAULT_COST_ARTILLERY;
            case EquipmentType::AntiAir:   return DEFAULT_COST_ANTIAIR;
            case EquipmentType::CAS:       return DEFAULT_COST_CAS;
        }
        return DEFAULT_COST_GUN;
    }

public:
    ProductionLine(EquipmentType type, int factories, double cost)
        : type(type),
          factoriesAssigned(std::max(0, factories)),
          unitCost(cost > 0 ? cost : defaultCost(type)) {}

    EquipmentType getType() const { return type; }
    int getFactories() const { return factoriesAssigned; }
    double getUnitCost() const { return unitCost; }

    void setFactories(int f) { factoriesAssigned = std::max(0, f); }
    void setUnitCost(double c) { unitCost = (c > 0 ? c : defaultCost(type)); }

    std::string toString() const {
        const char* tn = type==EquipmentType::Gun?"Gun":
                         type==EquipmentType::Artillery?"Artillery":
                         type==EquipmentType::AntiAir?"AA":"CAS";
        std::ostringstream ss;
        ss << tn << " [factories=" << factoriesAssigned
           << ", unitCost=" << unitCost << "]";
        return ss.str();
    }
};

std::ostream& operator<<(std::ostream& os, const ProductionLine& pl){
    return os << pl.toString();
}

// ============================================================================
//                                   Country
// ============================================================================
class Country {
    std::string name;
    std::string ideology;
    std::vector<Province> provinces;
    ResourceStockpile resources;
    EquipmentStockpile equipment;
    std::vector<ConstructionTask> queue;
    std::vector<ProductionLine> milLines;
    int refineries = 0;

    static constexpr int MIL_FACTORY_OUTPUT_PER_DAY = 1000;
    static constexpr int OIL_TO_FUEL_RATIO = 5;
    static constexpr int REFINERY_FUEL_BONUS_PER_DAY = 10;
    static constexpr double CIV_OUTPUT_PER_DAY = 1.0;

public:
    Country(std::string name, std::string ideology, std::vector<Province> provs, ResourceStockpile res)
        : name(std::move(name)), ideology(std::move(ideology)),
          provinces(std::move(provs)), resources(std::move(res)) {}

    int totalCiv() const { int c=0; for (auto& p:provinces) c+=p.getCiv(); return c; }
    int totalMil() const { int m=0; for (auto& p:provinces) m+=p.getMil(); return m; }
    int totalOil() const { int o=0; for (auto& p:provinces) o+=p.getOilReserve(); return o; }

    // cu cost explicit
    bool addProductionLine(EquipmentType type, int factories, double cost) {
        int used = 0; for (auto& l: milLines) used += l.getFactories();
        if (used + factories > totalMil()) {
            std::cerr << "[!] Not enough military factories for new line in " << name << ".\n";
            return false;
        }
        milLines.emplace_back(type, factories, cost);
        return true;
    }

    // fără cost -> folosește implicit
    bool addProductionLine(EquipmentType type, int factories) {
        return addProductionLine(type, factories, -1.0);
    }

    bool checkMilitaryFactories() const {
        int used = 0; for (auto& l: milLines) used += l.getFactories();
        return used <= totalMil();
    }

    void simulateDay() {
        if (!checkMilitaryFactories()) {
            std::cerr << "[!] Factory overuse detected in " << name << ".\n";
        }

        int fuelGain = totalOil() * OIL_TO_FUEL_RATIO + refineries * REFINERY_FUEL_BONUS_PER_DAY;
        resources.add(fuelGain, 0);

        for (auto& l : milLines) {
            int used = std::min(l.getFactories(), totalMil());
            double totalPPD = used * MIL_FACTORY_OUTPUT_PER_DAY;
            long long units = static_cast<long long>(std::floor(totalPPD / l.getUnitCost()));

            switch(l.getType()) {
                case EquipmentType::Gun: equipment.addGuns(units); break;
                case EquipmentType::Artillery: equipment.addArtillery(units); break;
                case EquipmentType::AntiAir: equipment.addAntiAir(units); break;
                case EquipmentType::CAS: equipment.addCAS(units); break;
            }
        }
    }

    std::string toString() const {
        std::ostringstream ss;
        ss << "Country(" << name << ", " << ideology << ")\n";
        ss << "  Factories: CIV=" << totalCiv() << " | MIL=" << totalMil() << " | REFINERIES=" << refineries << "\n";
        ss << "  Resources: " << resources << "\n";
        ss << "  Equipment: " << equipment << "\n";
        if (!milLines.empty()) {
            ss << "  Military Lines:\n";
            for (auto& l : milLines) ss << "    - " << l << "\n";
        }
        ss << "  Provinces:\n";
        for (auto& p : provinces) ss << "    - " << p << "\n";
        return ss.str();
    }
};

std::ostream& operator<<(std::ostream& os, const Country& c) {
    return os << c.toString();
}

// ============================================================================
//                                      MAIN
// ============================================================================
int main() {
    // --- Romania ---
    Province p1("Wallachia",    1800, 3, 3, 6, 2, 3);
    Province p2("Moldavia",     1500, 2, 2, 5, 5, 1);
    Province p3("Transylvania", 1600, 2, 1, 7, 8, 2);
    ResourceStockpile roRes(0, 100);
    Country Romania("Romania", "Democratic", {p1,p2,p3}, roRes);
    Romania.addProductionLine(EquipmentType::Gun, 3);
    Romania.addProductionLine(EquipmentType::Artillery, 2);

    // --- Hungary ---
    Province h1("Alfold",       1400, 2, 2, 6, 4, 2);
    Province h2("Transdanubia", 1200, 2, 1, 6, 3, 1);
    ResourceStockpile huRes(0, 80);
    Country Hungary("Hungary", "Authoritarian", {h1,h2}, huRes);
    Hungary.addProductionLine(EquipmentType::Gun, 2);
    Hungary.addProductionLine(EquipmentType::AntiAir, 1);

    std::cout << "=== INITIAL STATE ===\n";
    std::cout << Romania << "\n" << Hungary << "\n";

    for (int day=1; day<=30; ++day) {
        Romania.simulateDay();
        Hungary.simulateDay();
    }

    std::cout << "\n=== AFTER 30 DAYS ===\n";
    std::cout << Romania << "\n" << Hungary << "\n";

    return 0;
}
