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
    std::map<float, std::set<int>> worthToCellSetMap;

    int friendlyAntsOnCurTurn;
    std::set<int> friendlyCellsOnTurn;

    // save beacons path between turns
    std::map<int, std::list<int>> beaconsPathMap; // map (cell) -> list<cell>
    std::set<int> beaconsCellSet;

    std::map<Cell::Type, float> cellWorthCoef {
        {Cell::Egg,     float(2.50)},
        {Cell::Crystal, float(1.00)},
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

    void calcDistances()
    {
        pathMapVec.resize(numberOfCells);
        for (int cellIdx = 0; cellIdx < numberOfCells; cellIdx++)
        {
            makePathMap(cellIdx);

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

        // std::cerr << "Max field distance: " << maxFieldDist << std::endl;
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

            if (resultPathsMap[neighCell].size() > maxFieldDist)
            {
                maxFieldDist = int(resultPathsMap[neighCell].size());
            }

            continueBuildPath(neighCell, resultPathsMap);
        }
    }

    void makePathMap(int startCell)
    {
        std::map<int, std::list<int>>& resultPathsMap = pathMapVec[startCell];
        resultPathsMap[startCell] = {};
        continueBuildPath(startCell, resultPathsMap);
    }

    void readTurnState()
    {
        friendlyCellsOnTurn.clear();
        friendlyAntsOnCurTurn = 0;

        for (int cellIdx = 0; cellIdx < cells.size(); cellIdx++)
        {
            cells.at(cellIdx).readTurnState();

            if (cells.at(cellIdx).myAnts)
            {
                friendlyAntsOnCurTurn += cells.at(cellIdx).myAnts;
                friendlyCellsOnTurn.insert(cellIdx);
            }
        }
    }

    void makeTurnEstimation()
    {
        worthToCellSetMap.clear();

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
            
            // find nearest friendly cell to this egg cell
            const std::map<int, std::list<int>>& pathMapFromEggCell = pathMapVec.at(eggCell);
            int distFromNearestCell = maxFieldDist;
            for (int friendlyCell : friendlyCellsOnTurn)
            {
                if (pathMapFromEggCell.at(friendlyCell).size() < distFromNearestCell)
                {
                    distFromNearestCell = int(pathMapFromEggCell.at(friendlyCell).size());
                }
            }

            float distCoef = distFromNearestCell == 0 ? 0.5f : (1 / float(distFromNearestCell));
            worthToCellSetMap[
                float(cells.at(eggCell).curResources * cellWorthCoef[Cell::Type::Egg]) * distCoef
            ].insert(eggCell);
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
                    distFromNearestCell = int(pathMapFromCrystalCell.at(friendlyCell).size());
                }
            }

            float distCoef = distFromNearestCell == 0 ? 0.5f : ( 1 / float(distFromNearestCell * distFromNearestCell));
            worthToCellSetMap[
                float(cells.at(crystalCell).curResources * cellWorthCoef[Cell::Type::Crystal]) * distCoef
            ].insert(crystalCell);
        }
        // save update crystal vec if it updated
        if (newCrystalCellVec.size() != crystalCells.size())
        {
            crystalCells = newCrystalCellVec;
        }

        for (auto estimationCellRIter = worthToCellSetMap.rbegin(); estimationCellRIter != worthToCellSetMap.rend(); estimationCellRIter++)
        {
            std::cerr << "Estimation " << estimationCellRIter->first << " cells: ";
            for (int cell : estimationCellRIter->second)
            {
                std::cerr << cell << ", ";
            }
            std::cerr << std::endl;
        }

        // update beacons turn map if needed
        bool needBeaconsForget = true;
        while (needBeaconsForget)
        {
            needBeaconsForget = false;

            for (const auto& beaconsPairs : beaconsPathMap)
            {
                int beaconsPathTargetCell = beaconsPairs.first;

                // if prev beacons target disapears, need remove beacons list
                bool existBeaconTargetInWorthMap = false;
                for (const auto& worthPair : worthToCellSetMap)
                {
                    if (worthPair.second.count(beaconsPathTargetCell))
                    {
                        existBeaconTargetInWorthMap = true;
                        break;
                    }
                }
                if (!existBeaconTargetInWorthMap)
                {
                    std::cerr << "Forget beacons path to cell " << beaconsPathTargetCell << std::endl;

                    needBeaconsForget = true;
                    beaconsPathMap.erase(beaconsPathTargetCell);
                    break;
                }
            }
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

    void saveBeaconsForTargets()
    {
        for (const auto& beaconsTargetPair : field_.beaconsPathMap)
        {
            for (int cell : beaconsTargetPair.second)
            {
                field_.beaconsCellSet.insert(cell);
            }
        }
    }

    void tryMakeNewBeaconPathForBestCell()
    {
        if (field_.worthToCellSetMap.empty())
        {
            return;
        }

        // find cell with max estimate for that not yet exist beacons path
        int cellWithMaxEstimate = -1;
        for (auto bestEstimateIter = field_.worthToCellSetMap.rbegin(); bestEstimateIter != field_.worthToCellSetMap.rend(); bestEstimateIter++)
        {
            for (int bestEstimateCell : bestEstimateIter->second)
            {
                if (!field_.beaconsPathMap.count(bestEstimateCell))
                {
                    cellWithMaxEstimate = bestEstimateCell;
                    break;
                }
            }

            if (cellWithMaxEstimate != -1)
            {
                break;
            }
        }

        // if cellWithMaxEstimate not found - return
        if (cellWithMaxEstimate == -1)
        {
            return;
        }

        std::cerr << "Cell with max estimation: " << cellWithMaxEstimate << std::endl;

        // try find path from nearest friendly cell
        int nearestFriendlyCellToBestCell;
        int distToCellWithMaxEstimate = field_.maxFieldDist;
        for (int friendlyCell : field_.friendlyCellsOnTurn)
        {
            if (field_.pathMapVec[friendlyCell][cellWithMaxEstimate].size() < distToCellWithMaxEstimate)
            {
                nearestFriendlyCellToBestCell = friendlyCell;
                distToCellWithMaxEstimate = int(field_.pathMapVec[friendlyCell][cellWithMaxEstimate].size());
            }
        }

        // std::cerr << "nearestFriendlyCellToBestCell: " << nearestFriendlyCellToBestCell << std::endl;
        // std::cerr << "distToCellWithMaxEstimate: " << distToCellWithMaxEstimate << std::endl;

        // make beacons path without intermediate cells
        if (distToCellWithMaxEstimate < 2)
        {
            // nearestFriendlyCellToBestCell (start cell)
            field_.beaconsPathMap[cellWithMaxEstimate].push_back(nearestFriendlyCellToBestCell);
            field_.beaconsCellSet.insert(nearestFriendlyCellToBestCell);

            // cellWithMaxEstimate (end cell)
            field_.beaconsPathMap[cellWithMaxEstimate].push_back(cellWithMaxEstimate);
            field_.beaconsCellSet.insert(cellWithMaxEstimate);

            return;
        }

        // if dist >= 2 its possible to make OPTIMAL path over other usefull cells

        int maxPossiblePathLength = distToCellWithMaxEstimate + distToCellWithMaxEstimate / 2 + 1;

        // find all usefull cells in radius (distToCellWithMaxEstimate) without cell (cellWithMaxEstimate)
        std::map<float, int> radiusCellWorthMap; // map (worth) -> (cell)
        for (const auto& worthCellSetPair : field_.worthToCellSetMap)
        {
            float curWorthCellEstimation = worthCellSetPair.first;
            for (int curWorthCell : worthCellSetPair.second)
            {
                // skip cell with max estimate
                if (curWorthCell == cellWithMaxEstimate)
                {
                    continue;
                }

                int distFromNearestFriendlyCell = int(field_.pathMapVec[nearestFriendlyCellToBestCell][curWorthCell].size()),
                    distFromBestCell = int(field_.pathMapVec[cellWithMaxEstimate][curWorthCell].size());
                int pathOverCellLength = distFromBestCell + distFromBestCell;

                // skip cell with too long dist
                if (distFromNearestFriendlyCell > distToCellWithMaxEstimate
                    || distFromBestCell > distToCellWithMaxEstimate
                    || pathOverCellLength > maxPossiblePathLength)
                {
                    continue;
                }

                float distCoef = pathOverCellLength == 0 ? 0.5f : (1 / float(pathOverCellLength));
                radiusCellWorthMap[curWorthCellEstimation * distCoef] = curWorthCell;
            }
        }

        // if no more useful cells in radius make line
        if (radiusCellWorthMap.empty())
        {
            // nearestFriendlyCellToBestCell (start cell)
            field_.beaconsPathMap[cellWithMaxEstimate].push_back(nearestFriendlyCellToBestCell);
            field_.beaconsCellSet.insert(nearestFriendlyCellToBestCell);

            // beacons from nearestFriendlyCellToBestCell to intermediate cell
            for (int cell : field_.pathMapVec[nearestFriendlyCellToBestCell][cellWithMaxEstimate])
            {
                field_.beaconsPathMap[cellWithMaxEstimate].push_back(cell);
                field_.beaconsCellSet.insert(cell);
            }
            return;
        }

        // take max useful cell and make path over this cell with beacons

        int intermdiateCell = radiusCellWorthMap.rbegin()->second;
        std::cerr << "intermdiateCell with max esimation: " << intermdiateCell << std::endl;

        // nearestFriendlyCellToBestCell (start cell)
        field_.beaconsPathMap[cellWithMaxEstimate].push_back(nearestFriendlyCellToBestCell);
        field_.beaconsCellSet.insert(nearestFriendlyCellToBestCell);

        // beacons from nearestFriendlyCellToBestCell to intermediate cell
        for (int cell : field_.pathMapVec[nearestFriendlyCellToBestCell][intermdiateCell])
        {
            field_.beaconsPathMap[cellWithMaxEstimate].push_back(cell);
            field_.beaconsCellSet.insert(cell);
        }

        // beacons from intermediate cell to best estimate cell
        for (int cell : field_.pathMapVec[intermdiateCell][cellWithMaxEstimate])
        {
            field_.beaconsPathMap[cellWithMaxEstimate].push_back(cell);
            field_.beaconsCellSet.insert(cell);
        }
    }

    void checkBeaconPathToBase()
    {
        for (int curFriendlyBase : field_.friendlyBases)
        {
            // find nearest friendly cell to current friendly base
            int nearestFriendlyCellToCurFriendlyBase;
            int nearestDistanceToCurFriendlyBase = field_.maxFieldDist;
            for (int friendCell : field_.friendlyCellsOnTurn)
            {
                int curDist = int(field_.pathMapVec[friendCell][curFriendlyBase].size());
                if (curDist < nearestDistanceToCurFriendlyBase)
                {
                    nearestDistanceToCurFriendlyBase = curDist;
                    nearestFriendlyCellToCurFriendlyBase = friendCell;
                }
            }

            // save beacon cell to friendly base
            for (int cell : field_.pathMapVec[nearestFriendlyCellToCurFriendlyBase][curFriendlyBase])
            {
                field_.beaconsCellSet.insert(cell);
            }
        }
    }

    void setBeacons()
    {
        for (int curFriendlyBase : field_.friendlyBases)
        {
            curTurnActions.push_back(
                std::make_unique<BeaconAction>(curFriendlyBase, 1)
            );
        }

        for (int beaconCell : field_.beaconsCellSet)
        {
            curTurnActions.push_back(
                std::make_unique<BeaconAction>(beaconCell, 1)
            );
        }
    }

    void makeActions()
    {
        curTurnActions.clear();
        field_.beaconsCellSet.clear();

        saveBeaconsForTargets();

        tryMakeNewBeaconPathForBestCell();

        // checkBeaconPathToBase();

        setBeacons();

        // for (const auto& beaconPathPair : field_.beaconsPathMap)
        // {
        //     std::cerr << "Beacon path to cell " << beaconPathPair.first << ": [";
        //     for (int cell : beaconPathPair.second)
        //     {
        //         std::cerr << cell << ", ";
        //     }
        //     std::cerr << "]\n";
        // }

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
