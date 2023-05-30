#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <array>
#include <set>
#include <list>
#include <map>
#include <vector>

using namespace std;

namespace SpringChallenge2023
{

struct Cell
{
    enum Type
    {
        Nothing,
        Egg,
        Crystal,
    };

    Type type;
    int initialResources; //  the amount of crystal/egg here
    std::array<int, 6> neighArr;

    int curResources;
    int myAnts;
    int oppAnts;

    void readInitState()
    {
        int cellType;
        cin >> cellType >> initialResources;
        type = static_cast<Cell::Type>(cellType);
        for (int nCnt = 0; nCnt < neighArr.size(); nCnt++)
        {
            cin >> neighArr[nCnt];
        }
    }

    void readTurnState()
    {
        cin >> curResources >> myAnts >> oppAnts;
        cin.ignore();
    }
};

struct Action
{
    Action() = default;
    virtual std::string stringify()
    {
        return "WAIT;";
    };
};
struct BeaconAction: public Action
{
    int index, strength;
    BeaconAction(int index, int strength):
        index{ index }, strength{ strength } {}
    std::string stringify() final
    {
        return "BEACON "
                + std::to_string(index) + " "
                + std::to_string(strength) + ";";
    }
};
struct LineAction: public Action
{
    int index1, index2, strength;
    LineAction(int index1, int index2, int strength) :
        index1{ index1 }, index2{ index2 }, strength{ strength } {}
    std::string stringify() final
    {
        return "LINE "
                + std::to_string(index1) + " "
                + std::to_string(index2) + " "
                + std::to_string(strength) + ";";
    }
};
struct MessageAction: public Action
{
    std::string message;
    MessageAction(const std::string& message):
        message{message} {}
    std::string stringify() final
    {
        return "MESSAGE " + message + ";";
    }
};

struct Field
{
    int numberOfCells;
    std::vector<Cell> cells;
    int numberOfBases;
    std::vector<int> friendlyBases;
    std::vector<int> opponentBases;

    int maxFieldDist{ 0 };

    // syntetic
    std::vector<int> crystalCells;
    std::vector<int> eggsCells;

    // map srcCellIdx -> map < dstCellIdx -> shortestPath >
    std::vector<std::map<int, std::list<int>>> pathMapVec;

    // map worth of cell to cell
    // worth = distFromNearestBase * curResources * cellTypeCoef
    // cellTypeCoef == 2 for Egg and 1 for Crystal
    std::map<int, int> turnCellWorthMap;

    std::set<int> friendlyCellsOnTurn;

    // map any cell to nearest friendly cell
    std::map<int, int> nearestFriendlyCellToAnyCells;

    std::map<Cell::Type, int> cellWorthCoef {
        {Cell::Egg,     10},
        {Cell::Crystal, 1},
    };

    void init()
    {
        cin >> numberOfCells;
        cin.ignore();

        cells.resize(numberOfCells);
        for (int i = 0; i < numberOfCells; i++)
        {
            cells[i].readInitState();

            if (cells[i].type == Cell::Crystal)
            {
                crystalCells.push_back(i);
            }

            if (cells[i].type == Cell::Egg)
            {
                eggsCells.push_back(i);
            }
        }
        cin.ignore();

        cin >> numberOfBases;
        cin.ignore();

        friendlyBases.resize(numberOfBases);
        opponentBases.resize(numberOfBases);

        for (int i = 0; i < numberOfBases; i++)
        {
            cin >> friendlyBases[i];
            cin.ignore();
        }

        for (int i = 0; i < numberOfBases; i++)
        {
            cin >> opponentBases[i];
            cin.ignore();
        }
    }

    void makeDfs(int curCellIdx, std::vector<int>& distances)
    {
        Cell& curCell = cells[curCellIdx];
        int nextDist = distances[curCellIdx] + 1;
        std::list<int> nextDfsNeighbours;
        for (int neighIdx = 0; neighIdx < curCell.neighArr.size(); neighIdx++)
        {
            int curNeighIdx = curCell.neighArr[neighIdx];
            if (curNeighIdx == -1 // no neigh
                || distances[curNeighIdx] <= nextDist && distances[curNeighIdx] != -1) // already visited
            {
                continue;
            }

            distances[curNeighIdx] = nextDist;
            nextDfsNeighbours.push_back(curNeighIdx);
        }

        for (int neighIdx : nextDfsNeighbours)
        {
            makeDfs(neighIdx, distances);
        }
    }

    void calcDistances()
    {
        pathMapVec.resize(numberOfCells);
        for (int cellIdx = 0; cellIdx < numberOfCells; cellIdx++)
        {
            pathMapVec[cellIdx] = makePathMap(cellIdx);

            // std::cerr << "---- PATH MAP FROM CELL " << cellIdx << " BUILDED ----\n";
            // const std::map<int, std::list<int>>& pathMapFromCell = pathMapVec.at(cellIdx);
            // for (const auto& pathPair : pathMapFromCell)
            // {
            //     std::cerr << "Path to " << pathPair.first << ": [";
            //     for (int cellIdx : pathPair.second)
            //     {
            //         std::cerr << cellIdx << " ";
            //     }
            //     std::cerr << "]\n";
            // }
        }

        // -------------------------------------
        const std::map<int, std::list<int>>& pathMapFromNullCell = pathMapVec.at(0);
        int farthestCellFromCenter = 0, maxDist = 0;
        for (const auto& pathPair : pathMapFromNullCell)
        {
            if (pathPair.second.size() > maxDist)
            {
                farthestCellFromCenter = pathPair.first;
                maxDist = pathPair.second.size();
            }
        }
        // std::cerr << "farthestCellFromCenter: " << farthestCellFromCenter << ", dist: " << maxDist << std::endl;
        const std::map<int, std::list<int>>& pathMapFromFarthestCell = pathMapVec.at(farthestCellFromCenter);
        for (const auto& pathPair : pathMapFromFarthestCell)
        {
            if (pathPair.second.size() > maxFieldDist)
            {
                maxFieldDist = pathPair.second.size();
            }
        }
        // std::cerr << "Max field distance: " << maxFieldDist << std::endl;
        // -------------------------------------

        // std::cerr << "---- INTRESTING CELLS DIST ----\n";
        // for (const auto& intrestingCellPair : intrestingDistances)
        // {
        //     std::cerr << "Distance from " << intrestingCellPair.first << ": [";
        //     for (int cellIdx : intrestingCellPair.second)
        //     {
        //         std::cerr << cellIdx << " ";
        //     }
        //     std::cerr << "]\n";
        // }
    }

    void continueBuildPath(int curCell, std::map<int, std::list<int>>& resultPathsMap)
    {
        // std::cerr << "continueBuildPath from " << curCell << std::endl;
        for (int neighCell : cells[curCell].neighArr)
        {
            if (neighCell == -1)
            {
                continue;
            }

            // std::cerr << "\tneigh " << neighCell;
            if (resultPathsMap.count(neighCell) 
                && resultPathsMap.at(neighCell).size() <= resultPathsMap.at(curCell).size() + 1)
            {
                // std::cerr << " already visited " << std::endl;
                continue;
            }
            // std::cerr << " isn't visited, save path " << std::endl;
            resultPathsMap[neighCell] = resultPathsMap.at(curCell);
            resultPathsMap[neighCell].push_back(neighCell);

            continueBuildPath(neighCell, resultPathsMap);
        }
    }

    std::map<int, std::list<int>> makePathMap(int startCell)
    {
        std::map<int, std::list<int>> resultPathsMap;
        resultPathsMap[startCell] = {};

        continueBuildPath(startCell, resultPathsMap);

        return resultPathsMap;
    }

    // {
    //     std::vector<int> distances(numberOfCells, -1);
    //     distances[startCell] = 0;
    //     makeDfs(startCell, distances);
    //     return distances;
    // }

    void readTurnState()
    {
        friendlyCellsOnTurn.clear();

        for (int cellIdx = 0; cellIdx < cells.size(); cellIdx++)
        {
            cells.at(cellIdx).readTurnState();

            if (cells.at(cellIdx).myAnts)
            {
                friendlyCellsOnTurn.insert(cellIdx);
            }
        }
    }

    void makeTurnEstimation()
    {
        turnCellWorthMap.clear();

        // --- add eggs estimation ---
        // std::cerr << "add eggs estimation" << std::endl;
        std::vector<int> newEggCellVec; // vec with updated egg cells
        for (int eggCell : eggsCells)
        {
            // skip cell without eggs
            if (cells.at(eggCell).curResources == 0)
            {
                continue;
            }
            newEggCellVec.push_back(eggCell); // save cell with egg

            // std::cerr << "\tfind nearest friendly cell to egg cell " << eggCell << std::endl;
            // 
            // find nearest friendly cell to this egg cell
            const std::map<int, std::list<int>>& pathMapFromEggCell = pathMapVec.at(eggCell);
            int distFromNearestCell = maxFieldDist;
            for (int friendlyCell : friendlyCellsOnTurn)
            {
                if (pathMapFromEggCell.at(friendlyCell).size() < distFromNearestCell)
                {
                    distFromNearestCell = pathMapFromEggCell.at(friendlyCell).size();
                    nearestFriendlyCellToAnyCells[eggCell] = friendlyCell;
                }
            }

            // std::cerr << "\tcalc egg cell estimation" << std::endl;
            // 
            // worth = distFromNearestFriendlyCell * curResources * cellTypeCoef
            // cellTypeCoef == 2 for Egg and 1 for Crystal
            turnCellWorthMap[
                (maxFieldDist - distFromNearestCell) * cells.at(eggCell).curResources * cellWorthCoef[Cell::Type::Egg] * (friendlyCellsOnTurn.count(eggCell) ? 2 : 1)
            ] = eggCell;
        }
        // save update egg vec if it updated
        if (newEggCellVec.size() != eggsCells.size())
        {
            eggsCells = newEggCellVec;
        }

        // std::cerr << "add crystal estimation" << std::endl;
        // 
        // --- add crystal estimation ---
        std::vector<int> newCrystalCellVec; // vec with updated crystal cells
        for (int crystalCell : crystalCells)
        {
            // skip cell without crystals
            if (cells.at(crystalCell).curResources == 0)
            {
                continue;
            }
            newCrystalCellVec.push_back(crystalCell); // save cell with crystal

            // find nearest friendly cell to this crystal cell
            const std::map<int, std::list<int>>& pathMapFromCrystalCell = pathMapVec.at(crystalCell);
            int distFromNearestCell = maxFieldDist;
            for (int friendlyCell : friendlyCellsOnTurn)
            {
                if (pathMapFromCrystalCell.at(friendlyCell).size() < distFromNearestCell)
                {
                    distFromNearestCell = pathMapFromCrystalCell.at(friendlyCell).size();
                    nearestFriendlyCellToAnyCells[crystalCell] = friendlyCell;
                }
            }

            // worth = distFromNearestFriendlyCell * curResources * cellTypeCoef
            // cellTypeCoef == 2 for Egg and 1 for Crystal
            turnCellWorthMap[
                (maxFieldDist - distFromNearestCell) * cells.at(crystalCell).curResources * cellWorthCoef[Cell::Type::Crystal] * (friendlyCellsOnTurn.count(crystalCell) ? 2 : 1)
            ] = crystalCell;
        }
        // save update crystal vec if it updated
        if (newCrystalCellVec.size() != crystalCells.size())
        {
            crystalCells = newCrystalCellVec;
        }

        std::cerr << "---- CELLS WORTH ESTIMATION ----\n";
        for (const auto& estimationCellPair : turnCellWorthMap)
        {
            std::cerr << "Estimation of cell " << estimationCellPair.second << " == " << estimationCellPair.first << std::endl;
        }
    }
};

class Game
{
    Field field_;
    std::vector<std::unique_ptr<Action>> curTurnActions;

private:
    void readTurnState()
    {
        field_.readTurnState();
        field_.makeTurnEstimation();
    }

    void printActions()
    {
        // empty actions case
        if (curTurnActions.empty())
        {
            curTurnActions.push_back(std::make_unique<Action>());
        }

        // print actions
        for (std::unique_ptr<Action>& actionPtr : curTurnActions)
        {
            cout << actionPtr->stringify();
        }
        cout << endl;
    }

    void makeLineForAllCrystalls()
    {
        for (int crystalCellIdx = 0; crystalCellIdx < field_.crystalCells.size(); crystalCellIdx++)
        {
            curTurnActions.push_back(std::make_unique<LineAction>(field_.friendlyBases.at(0), field_.crystalCells[crystalCellIdx], 1));
        }
    }

    void makeLineForAllEggs()
    {
        for (int eggCellIdx = 0; eggCellIdx < field_.eggsCells.size(); eggCellIdx++)
        {
            curTurnActions.push_back(std::make_unique<LineAction>(field_.friendlyBases.at(0), field_.eggsCells[eggCellIdx], 1));
        }
    }

    void makeLineForBestCell()
    {
        if (field_.turnCellWorthMap.empty())
        {
            return;
        }
        curTurnActions.push_back(std::make_unique<LineAction>(field_.friendlyBases.at(0), field_.turnCellWorthMap.rbegin()->second, 1));
    }

    void makeActions()
    {
        curTurnActions.clear();

        // makeLineForAllCrystalls();
        makeLineForBestCell();

        printActions();
    }

public:
    void init()
    {
        field_.init();
        field_.calcDistances();
    }

    void start()
    {
        while (1)
        {
            readTurnState();
            makeActions();
        }
    }
};

}

int main()
{
    SpringChallenge2023::Game game;
    game.init();
    game.start();
}
