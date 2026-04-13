#ifndef classData_h 
#define classData_h

#include <vector>

struct WoWclass {
    public:
        std::vector<int> getDefSpells() const { return defSpells; }
        std::vector<int> getOffSpells() const { return offSpells; }
        std::vector<int> getMobSpells() const { return mobSpells; }
        std::vector<int> getRotSpells() const { return rotSpells; }
        std::vector<int> getUtilSpells() const { return utilSpells; }
        std::vector<int> getAllSpells() const {
            std::vector<int> allSpells;
            for (auto x : std::vector<std::vector<int>> {defSpells , offSpells , mobSpells , rotSpells , utilSpells}){
                for (auto y : x){
                    allSpells.push_back(y);
                }
            }
            return (allSpells);
        }

        void setDefSpells(const std::vector<int>& spells) { defSpells = spells; }
        void setOffSpells(const std::vector<int>& spells) { offSpells = spells; }
        void setMobSpells(const std::vector<int>& spells) { mobSpells = spells; }
        void setRotSpells(const std::vector<int>& spells) { rotSpells = spells; }
        void setUtilSpells(const std::vector<int>& spells) { utilSpells = spells; }

        void addDefSpell(int spell) { defSpells.push_back(spell); }
        void addOffSpell(int spell) { offSpells.push_back(spell); }
        void addMobSpell(int spell) { mobSpells.push_back(spell); }
        void addRotSpell(int spell) { rotSpells.push_back(spell); }
        void addUtilSpell(int spell) { utilSpells.push_back(spell); }

    private:
        std::vector<int> defSpells;
        std::vector<int> offSpells;
        std::vector<int> mobSpells;
        std::vector<int> rotSpells;
        std::vector<int> utilSpells;
};

#endif classData_h