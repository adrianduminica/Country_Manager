#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <cmath>

// ----------------------------- ResourceStockpile -----------------------------
class ResourceStockpile {
    int oil, steel, manpower;
    static int clampNonNeg(int x) { return x < 0 ? 0 : x; }
public:
    ResourceStockpile(int oil, int steel, int manpower)
        : oil(oil), steel(steel), manpower(manpower) {}
    ResourceStockpile(const ResourceStockpile& other)
        : oil(other.oil), steel(other.steel), manpower(other.manpower) {}
    ResourceStockpile& operator=(const ResourceStockpile& other) {
        if (this != &other) { oil=other.oil; steel=other.steel; manpower=other.manpower; }
        return *this;
    }
    ~ResourceStockpile() {}

    int getOil() const { return oil; }
    int getSteel() const { return steel; }
    int getManpower() const { return manpower; }

    void add(int dOil, int dSteel, int dManpower) {
        oil += dOil; steel += dSteel; manpower += dManpower;
        oil = clampNonNeg(oil); steel = clampNonNeg(steel); manpower = clampNonNeg(manpower);
    }
    bool canAfford(int needOil, int needSteel, int needManpower) const {
        return oil >= needOil && steel >= needSteel && manpower >= needManpower;
    }
    bool consume(int needOil, int needSteel, int needManpower) {
        if (!canAfford(needOil, needSteel, needManpower)) return false;
        oil -= needOil; steel -= needSteel; manpower -= needManpower; return true;
    }

    friend std::ostream& operator<<(std::ostream& os, const ResourceStockpile& r) {
        return os << "Oil=" << r.oil << ", Steel=" << r.steel << ", Manpower=" << r.manpower;
    }
};

// --------------------------------- Province ---------------------------------
class Province {
    std::string name;
    int population;
    int civFactories;
    int milFactories;
    int infrastructure; // 0..10

    double infraMultiplier() const { return 0.6 + 0.05 * infrastructure; } // 0.6..1.1
public:
    Province(std::string name, int population, int civ, int mil, int infra)
        : name(std::move(name)), population(population),
          civFactories(civ), milFactories(mil), infrastructure(infra) {}

    const std::string& getName() const { return name; }
    int getPopulation()  const { return population; }
    int getCiv()         const { return civFactories; }
    int getMil()         const { return milFactories; }
    int getInfra()       const { return infrastructure; }

    void addCiv(int x){ civFactories = std::max(0, civFactories + x); }
    void addMil(int x){ milFactories = std::max(0, milFactories + x); }
    void addInfra(int x){ infrastructure = std::min(10, std::max(0, infrastructure + x)); }

    // Producție simplificată pe ZI (exemple numerice)
    ResourceStockpile productionPerDay() const {
        double mult = infraMultiplier(); //voi adauga alte bonusuri mai tarziu
        int oilOut   = int(std::round((civFactories * 0.3 + milFactories * 0.2) * mult));
        int steelOut = int(std::round((civFactories * 0.2 + milFactories * 0.3) * mult));
        int mpOut    = int(std::round(population * 0.0001 * mult)); // mic aport zilnic
        return ResourceStockpile(oilOut, steelOut, mpOut);
    }

    friend std::ostream& operator<<(std::ostream& os, const Province& p) {
        os << "Province(" << p.name << ") pop=" << p.population
           << ", INFRA=" << p.infrastructure
           << ", CIV=" << p.civFactories
           << ", MIL=" << p.milFactories;
        return os;
    }
};

// ------------------------------- Construction --------------------------------
enum class BuildType { Infra, Civ, Mil, Refinery };

struct ConstructionTask {
    BuildType type;
    int provinceIndex;
    double remainingBP;   // build points rămase
    double baseCostBP;    // pentru raportare
};

std::ostream& operator<<(std::ostream& os, const ConstructionTask& t){
    const char* tn = t.type==BuildType::Infra?"INFRA":
                     t.type==BuildType::Civ?"CIV":
                     t.type==BuildType::Mil?"MIL":"REFINERY";
    os << tn << "(prov=" << t.provinceIndex << ", left=" << std::fixed << std::setprecision(1)
       << t.remainingBP << "/" << t.baseCostBP << ")";
    return os;
}

// ---------------------------------- Country ----------------------------------
class Country {
    std::string name;
    std::string ideology;
    std::vector<Province> provinces;
    ResourceStockpile stock;

    std::vector<ConstructionTask> queue;

    // ——— Parametri de joc (ușor de ajustat) ———
    static constexpr double CIV_OUTPUT_PER_DAY = 1.0; // fiecare CIV produce 1 BP/zi
    static constexpr double INFRA_COST   = 5.0;
    static constexpr double CIV_COST     = 25.0;
    static constexpr double MIL_COST     = 25.0;
    static constexpr double REFINERY_COST= 20.0;

    ResourceStockpile sumProductionPerDay() const {
        int o=0,s=0,m=0;
        for (const auto& p : provinces) {
            auto out = p.productionPerDay();
            o += out.getOil(); s += out.getSteel(); m += out.getManpower();
        }
        return ResourceStockpile(o,s,m);
    }
    int totalCiv() const { int c=0; for (const auto& p:provinces) c+=p.getCiv(); return c; }
    int totalMil() const { int m=0; for (const auto& p:provinces) m+=p.getMil(); return m; }

public:
    Country(std::string name, std::string ideology, std::vector<Province> provs, ResourceStockpile stock)
        : name(std::move(name)), ideology(std::move(ideology)),
          provinces(std::move(provs)), stock(std::move(stock)) {}

    // API public
    bool startConstruction(BuildType type, int provIndex){
        if (provIndex < 0 || provIndex >= (int)provinces.size()) return false;
        double cost = 0.0;
        switch(type){
            case BuildType::Infra:    cost = INFRA_COST; break;
            case BuildType::Civ:      cost = CIV_COST; break;
            case BuildType::Mil:      cost = MIL_COST; break;
            case BuildType::Refinery: cost = REFINERY_COST; break;
        }
        queue.push_back(ConstructionTask{type, provIndex, cost, cost});
        return true;
    }

    // Simulează O ZI: producție + progres la construcții bazat pe CIV
    void simulateDay() {
        // 1) producție zilnică de resurse
        auto gain = sumProductionPerDay();
        stock.add(gain.getOil(), gain.getSteel(), gain.getManpower());

        // 2) construcții în timp funcție de CIV factories
        int civ = totalCiv();
        if (!queue.empty() && civ > 0){
            double throughput = civ * CIV_OUTPUT_PER_DAY; // BP/zi
            double perTask = throughput / queue.size();
            for (auto &t : queue) t.remainingBP = std::max(0.0, t.remainingBP - perTask);

            // finalizează task-urile terminate
            std::vector<ConstructionTask> next;
            for (auto &t : queue) {
                if (t.remainingBP > 0.0001) { next.push_back(t); continue; }
                auto &prov = provinces[t.provinceIndex];
                switch(t.type){
                    case BuildType::Infra:    prov.addInfra(+1); break;
                    case BuildType::Civ:      prov.addCiv(+1);   break;
                    case BuildType::Mil:      prov.addMil(+1);   break;
                    case BuildType::Refinery: stock.add(3,0,0);  break; // efect simplu
                }
            }
            queue.swap(next);
        }
    }


    friend std::ostream& operator<<(std::ostream& os, const Country& c) {
        os << "Country(" << c.name << ", " << c.ideology << ")\n";
        os << "  Factories: CIV=" << c.totalCiv() << " | MIL=" << c.totalMil() << "\n";
        os << "  Stock: " << c.stock << "\n";
        os << "  Provinces:\n";
        for (std::size_t i=0;i<c.provinces.size();++i) os << "    ["<<i<<"] " << c.provinces[i] << "\n";
        if (!c.queue.empty()){
            os << "  Construction Queue:\n";
            for (const auto &t : c.queue) os << "    - " << t << "\n";
        } else os << "  Construction Queue: (empty)\n";
        return os;
    }
};

// ------------------------------------ main ----------------------------------
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Provincii
    Province p1("Wallachia",    1800, 3, 1, 6);
    Province p2("Moldavia",     1500, 2, 2, 5);
    Province p3("Transylvania", 1600, 2, 1, 7);

    // Stocuri
    ResourceStockpile roStock(15, 20, 50);
    ResourceStockpile huStock(12, 18, 45);

    // Țări
    Country Romania("Romania", "Democratic", {p1, p2, p3}, roStock);
    Country Hungary("Hungary", "Authoritarian",
                    {Province("Alfold", 1400, 2, 2, 6),
                     Province("Transdanubia", 1200, 2, 1, 6)}, huStock);

    // Cozi de construcții (costurile sunt în BP și se plătesc în timp prin CIV_OUTPUT_PER_DAY)
    Romania.startConstruction(BuildType::Infra, 0); // +1 INFRA Wallachia
    Romania.startConstruction(BuildType::Civ,   2); // +1 CIV Transylvania
    Hungary.startConstruction(BuildType::Mil,   1); // +1 MIL Transdanubia

    std::cout << "=== INITIAL STATE ===\n";
    std::cout << Romania << "\n" << Hungary << "\n";

    // Simulăm 60 de zile
    for (int day=1; day<=60; ++day) {
        Romania.simulateDay();
        Hungary.simulateDay();
    }

    std::cout << "\n=== Dupa 60 zile ===\n";
    std::cout << Romania << "\n" << Hungary << "\n";

    return 0;
}
