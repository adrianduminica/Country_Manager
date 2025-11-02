#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

// ----------------------------- ResourceStockpile -----------------------------
class ResourceStockpile {
    int fuel;
    int manpower;

    static int clampNonNeg(int x) { return x < 0 ? 0 : x; }

public:
    ResourceStockpile(int fuel, int manpower)
        : fuel(fuel), manpower(manpower) {}

    ResourceStockpile(const ResourceStockpile& other)
        : fuel(other.fuel), manpower(other.manpower) {}

    ResourceStockpile& operator=(const ResourceStockpile& other) {
        if (this != &other) {
            fuel = other.fuel;
            manpower = other.manpower;
        }
        return *this;
    }

    ~ResourceStockpile() {}

    int getFuel() const { return fuel; }
    int getManpower() const { return manpower; }

    void add(int dFuel, int dManpower) {
        fuel     = clampNonNeg(fuel     + dFuel);
        manpower = clampNonNeg(manpower + dManpower);
    }
};

std::ostream& operator<<(std::ostream& os, const ResourceStockpile& r) {
    os << "Fuel=" << r.getFuel() << ", Manpower=" << r.getManpower();
    return os;
}

// --------------------------------- Province ---------------------------------
class Province {
    std::string name;
    int population;
    int civFactories;
    int milFactories;
    int infrastructure;
    int steelReserve;
    int tungstenReserve;
    int chromiumReserve;
    int aluminiumReserve;
    int oilReserve;

public:
    Province(std::string name, int population, int civ, int mil, int infra,
             int steelReserve,int tungstenReserve, int chromiumReserve, int aluminiumReserve, int oilReserve)
        : name(std::move(name)), population(population),
          civFactories(civ), milFactories(mil),
          infrastructure(infra),
          steelReserve(steelReserve), tungstenReserve(tungstenReserve),chromiumReserve(chromiumReserve),aluminiumReserve(aluminiumReserve),
          oilReserve(oilReserve) {}

    const std::string& getName() const { return name; }
    int getPopulation()  const { return population; }
    int getCiv()         const { return civFactories; }
    int getMil()         const { return milFactories; }
    int getInfra()       const { return infrastructure; }
    int getSteelReserve()const { return steelReserve; }
    int getTungstenReserve()const { return tungstenReserve; }
    int getChromiumReserve()const { return chromiumReserve; }
    int getAluminiumReserve()const { return aluminiumReserve; }
    int getOilReserve()  const { return oilReserve; }

    void addCiv(int x){ civFactories = std::max(0, civFactories + x); }
    void addMil(int x){ milFactories = std::max(0, milFactories + x); }
    void addInfra(int x){ infrastructure = std::min(10, std::max(0, infrastructure + x)); }
};

std::ostream& operator<<(std::ostream& os, const Province& p) {
    os << "Province(" << p.getName()
       << ") pop=" << p.getPopulation()
       << ", INFRA=" << p.getInfra()
       << ", CIV=" << p.getCiv()
       << ", MIL=" << p.getMil()
       << ", STEEL=" << p.getSteelReserve()
       << ", TUNGSTEN=" << p.getTungstenReserve()
       << ", CHROMIUM=" << p.getChromiumReserve()
       << ", ALUMINIUM=" << p.getAluminiumReserve()
       << ", OIL=" << p.getOilReserve();
    return os;
}

// ------------------------------- Construction --------------------------------
enum class BuildType { Infra, Civ, Mil, Refinery };

struct ConstructionTask {
    BuildType type;
    int provinceIndex;
    double remainingBP;
    double baseCostBP;
};

std::ostream& operator<<(std::ostream& os, const ConstructionTask& t){
    const char* tn = t.type==BuildType::Infra?"INFRA":
                     t.type==BuildType::Civ?"CIV":
                     t.type==BuildType::Mil?"MIL":"REFINERY";
    os << tn << "(prov=" << t.provinceIndex
       << ", left=" << std::fixed << std::setprecision(1)
       << t.remainingBP << "/" << t.baseCostBP << ")";
    return os;
}

// ---------------------------------- Tara ----------------------------------
class Country {
    std::string name;
    std::string ideology;
    std::vector<Province> provinces;
    ResourceStockpile stock;
    std::vector<ConstructionTask> queue;
    int refineries = 0;

    static constexpr int OIL_TO_FUEL_RATIO = 5;
    static constexpr int REFINERY_FUEL_BONUS_PER_DAY = 10;
    static constexpr double CIV_OUTPUT_PER_DAY = 1.0;
    static constexpr double INFRA_COST = 5.0;
    static constexpr double CIV_COST = 25.0;
    static constexpr double MIL_COST = 25.0;
    static constexpr double REFINERY_COST = 20.0;

public:
    Country(std::string name, std::string ideology, std::vector<Province> provs, ResourceStockpile stock)
        : name(std::move(name)), ideology(std::move(ideology)),
          provinces(std::move(provs)), stock(std::move(stock)) {}

    // --- Getteri ---
    const std::string& getName() const { return name; }
    const std::string& getIdeology() const { return ideology; }
    const std::vector<Province>& getProvinces() const { return provinces; }
    const ResourceStockpile& getStock() const { return stock; }
    int getRefineries() const { return refineries; }
    const std::vector<ConstructionTask>& getQueue() const { return queue; }

    int totalCiv() const { int c=0; for (const auto& p:provinces) c+=p.getCiv(); return c; }
    int totalMil() const { int m=0; for (const auto& p:provinces) m+=p.getMil(); return m; }
    int totalSteel() const { int s=0; for (const auto& p:provinces) s+=p.getSteelReserve(); return s; }
    int totalTungsten() const { int t=0; for (const auto& p:provinces) t+=p.getTungstenReserve(); return t; }
    int totalChromium() const {int cr=0; for (const auto& p:provinces) cr+=p.getChromiumReserve(); return cr; }
    int totalAluminium() const {int a=0; for (const auto& p:provinces) a+=p.getAluminiumReserve(); return a; }
    int totalOil() const { int o=0; for (const auto& p:provinces) o+=p.getOilReserve(); return o; }

    // --- Funcții ---
    void startConstruction(BuildType type, int provIndex) {
        double cost = 0;
        switch(type){
            case BuildType::Infra: cost = INFRA_COST; break;
            case BuildType::Civ: cost = CIV_COST; break;
            case BuildType::Mil: cost = MIL_COST; break;
            case BuildType::Refinery: cost = REFINERY_COST; break;
        }
        queue.push_back({type, provIndex, cost, cost});
    }

    void simulateDay() {
        // Fuel zilnic
        int fuelGain = totalOil() * OIL_TO_FUEL_RATIO + refineries * REFINERY_FUEL_BONUS_PER_DAY;
        int manpowerGain = 0;
        for (const auto& p : provinces)
            manpowerGain += int(std::round(p.getPopulation() * 0.0001));
        const_cast<ResourceStockpile&>(stock).add(fuelGain, manpowerGain);

        // Construcții
        int civ = totalCiv();
        if (!queue.empty() && civ > 0){
            double throughput = civ * CIV_OUTPUT_PER_DAY;
            double perTask = throughput / queue.size();
            for (auto &t : queue) t.remainingBP = std::max(0.0, t.remainingBP - perTask);

            std::vector<ConstructionTask> next;
            for (auto &t : queue) {
                if (t.remainingBP > 0.0001) { next.push_back(t); continue; }
                switch(t.type){
                    case BuildType::Infra: provinces[t.provinceIndex].addInfra(+1); break;
                    case BuildType::Civ: provinces[t.provinceIndex].addCiv(+1); break;
                    case BuildType::Mil: provinces[t.provinceIndex].addMil(+1); break;
                    case BuildType::Refinery: refineries++; break;
                }
            }
            queue.swap(next);
        }
    }
};

std::ostream& operator<<(std::ostream& os, const Country& c) {
    os << "Country(" << c.getName() << ", " << c.getIdeology() << ")\n";
    os << "  Factories: CIV=" << c.totalCiv()
       << " | MIL=" << c.totalMil()
       << " | REFINERIES=" << c.getRefineries() << "\n";
    os << "  Steel=" << c.totalSteel()
       << " | Tungsten=" << c.totalTungsten()
        << " | Chromium=" << c.totalChromium()
        << " | Aluminium=" << c.totalAluminium()
       << ", Oil=" << c.totalOil() << "\n";
    os << "  Daily Fuel Gain = " << (c.totalOil() * 5 + c.getRefineries() * 10) << "\n";
    os << "  Stock: " << c.getStock() << "\n";
    os << "  Provinces:\n";
    for (const auto& p : c.getProvinces())
        os << "    - " << p << "\n";
    if (!c.getQueue().empty()) {
        os << "  Construction Queue:\n";
        for (const auto& t : c.getQueue())
            os << "    - " << t << "\n";
    } else os << "  Construction Queue: (empty)\n";
    return os;
}

// ------------------------------------ main ----------------------------------
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // --- Provincii România ---
    Province p1("Wallachia",    1800, 3, 1, 6,1,1,1, 10, 3);
    Province p2("Moldavia",     1500, 2, 2, 5, 6,1,1,1,  1);
    Province p3("Transylvania", 1600, 2, 1, 7, 8,1,1,1,2);

    // --- Provincii Ungaria ---
    Province h1("Alfold",       1400, 2, 2, 6, 7,1,1,1,1);
    Province h2("Transdanubia", 1200, 2, 1, 6, 5,1,1,1,2);

    ResourceStockpile roStock(0, 50);
    ResourceStockpile huStock(0, 45);

    Country Romania("Romania", "Democratic", {p1, p2, p3}, roStock);
    Country Hungary("Hungary", "Authoritarian", {h1, h2}, huStock);

    Romania.startConstruction(BuildType::Refinery, 0);
    Romania.startConstruction(BuildType::Civ, 2);
    Hungary.startConstruction(BuildType::Mil, 1);

    std::cout << "=== INITIAL STATE ===\n";
    std::cout << Romania << "\n" << Hungary << "\n";

    // simulăm 30 de zile
    for (int day=1; day<=30; ++day) {
        Romania.simulateDay();
        Hungary.simulateDay();
    }

    std::cout << "\n=== AFTER 30 DAYS ===\n";
    std::cout << Romania << "\n" << Hungary << "\n";

    return 0;
}
