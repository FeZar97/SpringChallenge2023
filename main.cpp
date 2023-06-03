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

    int colonyId;

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

    // synthetic
    std::set<int> crystalCells;
    std::set<int> eggsCells;

    int totalCrystalsNb{ 0 };
    int totalEggsNb{ 0 };

    int onTurnMapCrystalsNb{ 0 };
    int onTurnMapEggsNb{ 0 };

    float crystallEstimation{ 1 };
    float eggsEstimation{ 1 };

    // map srcCellIdx -> map < dstCellIdx -> shortestPath >
    std::vector<std::map<int, std::vector<int>>> pathMapVec;

    // map worth of cell to cell for every colony
    // colony id -> estimation -> cell
    std::map<int, std::map<float, std::set<int>>> worthToCellSetColonyMap;

    int friendlyAntsOnCurTurn;
    std::set<int> friendlyCellsOnTurn;

    // ��� ������ ������� ����: cell -> cell, �� ������� �������� ����� �������
    std::map<int, std::map<int, int>> coloniesLinesMap;

    // ��� ������ ������� ����� �����, � ������� ������ ���� ������
    std::map<int, std::set<int>> coloniesBeaconsSetMap;

    // colony id -> set of colonies cells
    std::map<int, std::set<int>> turnColoniesMap;

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
                crystalCells.insert(i);
                totalCrystalsNb += cells[i].initialResources;

            }

            if (cells[i].type == Cell::Egg)
            {
                eggsCells.insert(i);
                totalEggsNb += cells[i].initialResources;
            }
        }
        cin.ignore();

        eggsEstimation = float(totalCrystalsNb) / float(totalEggsNb);

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
        }

        // std::cerr << "Max field distance: " << maxFieldDist << std::endl;
    }

    void continueBuildPath(std::list<int> nextNeightList, std::map<int, std::vector<int>>& resultPathsMap)
    {
        do
        {
            std::list<int> cellsToPromoute = nextNeightList;
            nextNeightList.clear();

            for (int cellToPromoute : cellsToPromoute)
            {
                for (int neighCell : cells[cellToPromoute].neighArr)
                {
                    if (neighCell == -1 || resultPathsMap.count(neighCell))
                    {
                        // std::cerr << " already visited " << std::endl;
                        continue;
                    }

                    nextNeightList.push_back(neighCell);

                    // std::cerr << " isn't visited, save path " << std::endl;

                    resultPathsMap[neighCell] = resultPathsMap[cellToPromoute];
                    resultPathsMap[neighCell].push_back(neighCell);

                    if (resultPathsMap[neighCell].size() > maxFieldDist)
                    {
                        maxFieldDist = resultPathsMap[neighCell].size();
                    }
                }
            }
        } while (!nextNeightList.empty());
    }

    void makePathMap(int startCell)
    {
        std::map<int, std::vector<int>>& resultPathsMap = pathMapVec[startCell];
        resultPathsMap[startCell] = {};
        continueBuildPath({ startCell }, resultPathsMap);
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

    void buildColonies()
    {
        turnColoniesMap.clear();

        // map cell id -> colony id
        std::map<int, int> cellInColonies;

        // prepare: enumerate all cell by it own colony
        for (int cell : friendlyCellsOnTurn)
        {
            cells[cell].colonyId = cellInColonies.size() + 1;
            cellInColonies[cell] = cells[cell].colonyId;
        }

        // �������� �� ���� ������� � ��� ���������� ����� ������� ������������ �������� ������� - ������������� ��� �������� �� ���� �������
        // ��� ���� �� �������� �������� ���� existNewUnions
        bool existNewUnions = true;
        while (existNewUnions)
        {
            existNewUnions = false;

            for (int cell : friendlyCellsOnTurn)
            {
                int minColonyIdInNeigh = cells[cell].colonyId;
                for (int neighCellIdx : cells[cell].neighArr)
                {
                    if (friendlyCellsOnTurn.count(neighCellIdx) && cells[neighCellIdx].colonyId < minColonyIdInNeigh)
                    {
                        minColonyIdInNeigh = cells[neighCellIdx].colonyId;
                    }
                }

                if (minColonyIdInNeigh != cells[cell].colonyId)
                {
                    existNewUnions = true;
                    cells[cell].colonyId = minColonyIdInNeigh;
                }

                for (int neighCellIdx : cells[cell].neighArr)
                {
                    if (friendlyCellsOnTurn.count(neighCellIdx))
                    {
                        cells[neighCellIdx].colonyId = minColonyIdInNeigh;
                    }
                }
            }
        }

        // ������ ��� ������� ������ �������� �� �������

        // ��������� ������� � ���� �������
        for (int cell : friendlyCellsOnTurn)
        {
            turnColoniesMap[cells[cell].colonyId].insert(cell);
        }

        for (const auto& colonyPair : turnColoniesMap)
        {
            std::cerr << "Colony " << colonyPair.first << ": [";
            for (int cell : colonyPair.second)
            {
                std::cerr << cell;
                if (cell != *std::prev(colonyPair.second.end()))
                {
                    std::cerr << ", ";
                }
            }
            std::cerr << "]\n";
        }

        // �������� "�����������" �������
        // ������� ��������� �����������, ���� � ��� ��� �� ����� ������� ����
        int lastCheckedColonyId = -1;
        auto colonyToCheckIter = turnColoniesMap.upper_bound(lastCheckedColonyId);
        while (colonyToCheckIter != turnColoniesMap.end())
        {
            lastCheckedColonyId = colonyToCheckIter->first;
            // std::cerr << "Check Colony " << lastCheckedColonyId << " degeneracy..." << std::endl;

            bool isColonyWithFriendlyBase = false;
            for (int friendlyBase : friendlyBases)
            {
                if (colonyToCheckIter->second.count(friendlyBase))
                {
                    isColonyWithFriendlyBase = true;
                    break;
                }
            }

            if (!isColonyWithFriendlyBase)
            {
                // std::cerr << "Colony " << lastCheckedColonyId << " hasn't friendly base and is degenrated, forget it" << std::endl;
                turnColoniesMap.erase(lastCheckedColonyId);
            }
            colonyToCheckIter = turnColoniesMap.upper_bound(lastCheckedColonyId);
        }
    }

    void makeTurnEstimation()
    {
        worthToCellSetColonyMap.clear();
        onTurnMapCrystalsNb = 0;
        onTurnMapEggsNb = 0;

// ���� ����� �� ���� ������� � �����, �� ��� ��� ���� ������
        std::set<int> newEggCellSet; // set with updated egg cells
        for (int eggCell : eggsCells)
        {
            // skip cell without eggs
            if (cells.at(eggCell).curResources == 0)
            {
                continue;
            }
            newEggCellSet.insert(eggCell); // save cell with egg
            onTurnMapEggsNb += cells.at(eggCell).curResources;
        }
        // save update egg vec if it updated
        if (newEggCellSet.size() != eggsCells.size())
        {
            eggsCells = newEggCellSet;
        }

// ���� ����� �� ��������� ������� � �����, �� ��� ��� ���� ������
        std::set<int> newCrystalCellSet; // set with updated crystal cells
        for (int crystalCell : crystalCells)
        {
            // skip cell without crystals
            if (cells.at(crystalCell).curResources == 0)
            {
                continue;
            }
            newCrystalCellSet.insert(crystalCell); // save cell with crystal
            onTurnMapCrystalsNb += cells.at(crystalCell).curResources;
        }
        // save update crystal vec if it updated
        if (newCrystalCellSet.size() != crystalCells.size())
        {
            crystalCells = newCrystalCellSet;
        }

// ��������� ������ ����������
// ��� ������ ���������� �� ����� �������� - ��� ��� ������
        crystallEstimation = float(totalCrystalsNb) / float(onTurnMapCrystalsNb);

// ���� ������� ���-�� �������� ��������� ������� �������� ��������� �� ����������
// �� ��������� �������� ���������� ������� ��������
        if (friendlyAntsOnCurTurn > totalCrystalsNb / 2)
        {
            crystallEstimation *= 10;
        }

// ������ ��������� ����� ��� ������ �������
        for (const auto& colonyWorthMapPair : turnColoniesMap)
        {
            const int colonyId = colonyWorthMapPair.first;

            // ���������� �������� ����: cell -> worth
            std::map<int, float> curColonyWorthMap;

    // estimate egg cells
            // std::cerr << "\tEstimate egg cells" << std::endl;
            for (int eggCell : eggsCells)
            {
                const std::map<int, std::vector<int>>& pathMapFromEggCell = pathMapVec.at(eggCell);

            // find nearest friendly cell IN THIS COLONY to this egg cell
                int distFromNearestColonyCell = maxFieldDist;

                const std::set<int>& friendlyCellsInColonySet = colonyWorthMapPair.second;
                for (int friendlyColonyCell : friendlyCellsInColonySet)
                {
                    const std::vector<int>& shortestPathListFromEggToThisCell = pathMapFromEggCell.at(friendlyColonyCell);
                    if (shortestPathListFromEggToThisCell.size() < distFromNearestColonyCell)
                    {
                        distFromNearestColonyCell = int(shortestPathListFromEggToThisCell.size());
                    }
                }

                float distCoef = distFromNearestColonyCell == 0 ? 2 : (1 / float(distFromNearestColonyCell));
                curColonyWorthMap[eggCell] = float(cells.at(eggCell).curResources * eggsEstimation) * distCoef;
            }
            
    // estimate crystall cells
            // std::cerr << "\tEstimate crystal cells" << std::endl;
            for (int crystalCell : crystalCells)
            {
                const std::map<int, std::vector<int>>& pathMapFromCrystalCell = pathMapVec.at(crystalCell);

                // find nearest friendly cell IN THIS COLONY to this crystal cell
                int distFromNearestColonyCell = maxFieldDist;

                const std::set<int>& friendlyCellsInColonySet = colonyWorthMapPair.second;
                for (int friendlyColonyCell : friendlyCellsInColonySet)
                {
                    const std::vector<int>& shortestPathListFromCrystalToThisCell = pathMapFromCrystalCell.at(friendlyColonyCell);
                    if (shortestPathListFromCrystalToThisCell.size() < distFromNearestColonyCell)
                    {
                        distFromNearestColonyCell = int(shortestPathListFromCrystalToThisCell.size());
                    }
                }

                float distCoef = distFromNearestColonyCell == 0 ? 0.5f : (1 / float(distFromNearestColonyCell));
                curColonyWorthMap[crystalCell] = float(cells.at(crystalCell).curResources * crystallEstimation) * distCoef;
            }

    // � ������ ������ ������ ����������� ����� ������ ������� � ������������ � ���� � �������� ��� ������� �������
            // ����������� �� ��������� ���� curColonyWorthMap � ������� ������ ������ ��������
            for (const auto& worthPair : curColonyWorthMap)
            {
                float curCellFullWorth = worthPair.second;
                // ��������� ������ �������
                for (int neight : cells[worthPair.first].neighArr)
                {
                    if (neight == -1)
                    {
                        continue;
                    }
                    if (curColonyWorthMap.count(neight))
                    {
                        curCellFullWorth += curColonyWorthMap[neight] * 0.8;
                    }
                }

                // ���������� ����� � ���������� ����
                worthToCellSetColonyMap[colonyId][curCellFullWorth].insert(worthPair.first);
            }

    // print estimation
            std::cerr << "Estimation for colony " + std::to_string(colonyId) + ": ";
            for (auto estimationCellRIter = worthToCellSetColonyMap[colonyId].rbegin(); 
                estimationCellRIter != worthToCellSetColonyMap[colonyId].rend();
                estimationCellRIter++)
            {
                std::cerr << estimationCellRIter->first << " (";
                for (int cell : estimationCellRIter->second)
                {
                    std::cerr << cell;
                    if (cell != *std::prev(estimationCellRIter->second.end()))
                    {
                        std::cerr << ", ";
                    }
                }
                std::cerr << "), ";
            }
            std::cerr << std::endl;
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
        field_.buildColonies();
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

    void saveActualLinesInColonies()
    {
        // std::cerr << "begin saveActualLinesInColonies" << std::endl;

        // � ������ ������� ����� �������� ����� �������� ������� - �������� � ���������, ������� ������������ �����-���� ��������
        // ������������� ����� ����������� �� ���� �� ��������� �������� �����
        // ����� ����, ��� ����� ��������� �������� �����, ����� ������������ ����� ��/�� ��������� �������� ������

        for (const auto& colonyPair : field_.turnColoniesMap)
        {
            const int colonyId = colonyPair.first;
            const std::set<int> colonyCells = colonyPair.second;

        // ����������� ������� ���� � ������ �������
            int friendlyColonyBase = -1;
            for (int friendlyBase : field_.friendlyBases)
            {
                if (colonyCells.count(friendlyBase))
                {
                    friendlyColonyBase = friendlyBase;
                    break;
                }
            }

            // std::cerr << "friendly base in colony " << colonyId << ": " << friendlyColonyBase << std::endl;

            // ���� ������� ���� �� �������, �� ��� ��������, ��� ������ ������� ����������� � ���� ��� ������� ����� � ����� �� ����������� �������
            if (friendlyColonyBase == -1)
            {
                // std::cerr << "FIXME saveActualLinesInColonies" << std::endl;
                continue;
            }

            // ����� ��������������� ������� �����
            // ������� �� ������� ���� � ���� ����� �������, �������������� �����-���� �������
            std::set<int> colonyReferenceCells{ friendlyColonyBase };

            // ����� �� �������������� ������� ����� ������
            std::set<int> freeReferenceCells;

            // ��� ������, ��������������� � ���������� �����
            std::set<int> allCellsUsedInLines;

            // ��������� ����� freeReferenceCells
            for (int colonyCell : colonyCells)
            {
                // ������� ������ ������
                if (!field_.eggsCells.count(colonyCell) && !field_.crystalCells.count(colonyCell))
                {
                    continue;
                }

                freeReferenceCells.insert(colonyCell);
            }

        // ����� ���������� ������� ���� � �������, ���� ����� ��������� � ���� ���� ������� ����� ����� freeReferenceCells
            int nearestResourceColonyCellToFriendBase = -1;
            int minDist = field_.maxFieldDist;
            for (int colonyCell : freeReferenceCells)
            {
                int curDist = field_.pathMapVec[colonyCell][friendlyColonyBase].size();
                if (curDist < minDist)
                {
                    minDist = curDist;
                    nearestResourceColonyCellToFriendBase = colonyCell;
                }
            }

            // std::cerr << "Nearest Colony Cell with resources To Friendly Base base in colony " << colonyId << ": " << nearestResourceColonyCellToFriendBase << std::endl;

            if (nearestResourceColonyCellToFriendBase == -1)
            {
                field_.coloniesBeaconsSetMap[colonyId] = { friendlyColonyBase };
                continue;
            }

            // ��������� ������ ������� �����
            field_.coloniesLinesMap[colonyId][friendlyColonyBase] = nearestResourceColonyCellToFriendBase;

            // ���������������� ������� �����
            freeReferenceCells.erase(nearestResourceColonyCellToFriendBase);
            colonyReferenceCells.insert(nearestResourceColonyCellToFriendBase);

            // ���������� allCellsUsedInLines
            allCellsUsedInLines.insert(friendlyColonyBase);
            for (int cell : field_.pathMapVec[friendlyColonyBase][nearestResourceColonyCellToFriendBase])
            {
                allCellsUsedInLines.insert(cell);
            }

        // ���� ���� ��������� ������� �����
        // �������� ����� ����� ������� �����, ������� ����� ������������ � ����������� ������ ������ �������
            while (freeReferenceCells.size())
            {
                int nextUsedReferenceCell = -1;
                int nearestCellInAllCellsUsedInLines = -1;
                int minDistFromRefernceCell = field_.maxFieldDist;

                for (int colonyCell : freeReferenceCells)
                {
                    for (int linesCell : allCellsUsedInLines)
                    {
                        int curDist = field_.pathMapVec[colonyCell][linesCell].size();
                        if (curDist < minDistFromRefernceCell)
                        {
                            minDist = curDist;
                            nextUsedReferenceCell = colonyCell;
                            nearestCellInAllCellsUsedInLines = linesCell;
                        }

                        // �������� ������������
                        if (minDistFromRefernceCell <= 1)
                        {
                            break;
                        }
                    }

                    // �������� ������������
                    if (minDistFromRefernceCell <= 1)
                    {
                        break;
                    }
                }

                // ���� ��� �� ����� �� ���
                if (nextUsedReferenceCell == -1 || nearestCellInAllCellsUsedInLines == -1)
                {
                    break;
                }


             // ��������� ������� �����
                field_.coloniesLinesMap[colonyId][nextUsedReferenceCell] = nearestCellInAllCellsUsedInLines;

                // ���������������� ������� �����
                freeReferenceCells.erase(nextUsedReferenceCell);
                colonyReferenceCells.insert(nextUsedReferenceCell);

                // ���������� allCellsUsedInLines
                for (int cell : field_.pathMapVec[nearestCellInAllCellsUsedInLines][nextUsedReferenceCell])
                {
                    allCellsUsedInLines.insert(cell);
                }
            }

        // ��������� ���������� ����� ������ ��� ���������� ���� � ����������� �������
            field_.coloniesBeaconsSetMap[colonyId] = allCellsUsedInLines;
        }

        // std::cerr << "end saveActualLinesInColonies" << std::endl;
    }

    void tryMakeNewLinesInColonies()
    {
        // std::cerr << "begin tryMakeNewLinesInColonies" << std::endl;

        // ��� ������ ������� ���� "����������� �������": �������� ������������ ������ ������ ������� �, �����������, ��������� ���� �������
        for (const auto& colonyPair : field_.turnColoniesMap)
        {
            const int colonyId = colonyPair.first;
            const std::set<int>& curColonyCellSet = colonyPair.second;
            
            // ���� ������ ������������� �������� ������ - ������ ������
            if (field_.worthToCellSetColonyMap[colonyId].empty())
            {
                continue;
            }

            // ����� ���� �������� �� ���� ������, ������ ������� ��������� ��������� �����
            float colonyEstimationThreshold = -1.f;
            for (auto bestEstimateIter = field_.worthToCellSetColonyMap[colonyId].rbegin(); bestEstimateIter != field_.worthToCellSetColonyMap[colonyId].rend(); bestEstimateIter++)
            {
                // ����� ����� � ������� ����������
                const std::set<int>& cellsSetWithCurEstimation = bestEstimateIter->second;

                for (int bestEstimateCell : cellsSetWithCurEstimation)
                {
                    // ���� ������� ������ ��� � ���� � ��������� ��� �������, �� ���������� ��� ������ ��� ������������
                    if (!field_.coloniesBeaconsSetMap[colonyId].count(bestEstimateCell))
                    {
                        colonyEstimationThreshold = bestEstimateIter->first * 0.6;
                        break;
                    }
                }

                // if threshold found
                if (colonyEstimationThreshold > 0)
                {
                    break;
                }
            }

            std::cerr << "Estimation threshold for colony " + std::to_string(colonyId) + ": " << colonyEstimationThreshold << std::endl;

            bool existCellWithHighEstimate = true;
            while (existCellWithHighEstimate)
            {
                existCellWithHighEstimate = false;

                float curEstimation = -1.f;

                // find cell with max estimate for that not yet exist in field_.coloniesBeaconsSetMap[colonyId]
                int cellWithMaxEstimate = -1;
                for (auto bestEstimateIter = field_.worthToCellSetColonyMap[colonyId].rbegin(); bestEstimateIter != field_.worthToCellSetColonyMap[colonyId].rend(); bestEstimateIter++)
                {
                    if (bestEstimateIter->first < colonyEstimationThreshold)
                    {
                        break;
                    }

                    // ����� ����� � ������� ����������
                    const std::set<int>& cellsSetWithCurEstimation = bestEstimateIter->second;

                    for (int bestEstimateCell : cellsSetWithCurEstimation)
                    {
                        // ���� ������� ������ ��� � ���� � ��������� ��� �������, �� ��������� ��� ������ ��� ������� ��� �������
                        if (!field_.coloniesBeaconsSetMap[colonyId].count(bestEstimateCell))
                        {
                            cellWithMaxEstimate = bestEstimateCell;
                            curEstimation = bestEstimateIter->first;
                            break;
                        }
                    }

                    if (cellWithMaxEstimate != -1)
                    {
                        break;
                    }
                }

                // if cellWithMaxEstimate not found - continue
                if (cellWithMaxEstimate == -1)
                {
                    break;
                }

                std::cerr << "Next cell with high estimation for colony " + std::to_string(colonyId) + ": " << cellWithMaxEstimate << ", estimation " << curEstimation << std::endl;

                existCellWithHighEstimate = true;

                // � ������� ������� ���� ����� ������, ��������� � ������ � ���� �������
                // ����� ������ ������������ ����� field_.coloniesBeaconsSetMap[colonyId]

                int distToCellWithMaxEstimate = field_.maxFieldDist;
                int nearestColonyCellToBestCell = -1;
                for (int beaconCell : field_.coloniesBeaconsSetMap[colonyId])
                {
                    int curDist = field_.pathMapVec[beaconCell][cellWithMaxEstimate].size();
                    if (curDist < distToCellWithMaxEstimate)
                    {
                        distToCellWithMaxEstimate = curDist;
                        nearestColonyCellToBestCell = beaconCell;
                    }
                }

                // std::cerr << "\tdistToCellWithMaxEstimate: " << distToCellWithMaxEstimate << std::endl;
                // std::cerr << "\tnearestColonyCellToBestCell: " << nearestColonyCellToBestCell << std::endl;

                // make beacons path without intermediate cells
                if (distToCellWithMaxEstimate < 2)
                {
                    field_.coloniesBeaconsSetMap[colonyId].insert(cellWithMaxEstimate);
                    continue;
                }

                // if dist >= 2 its possible to make OPTIMAL path over other usefull cells

                int maxPossiblePathLength = distToCellWithMaxEstimate + distToCellWithMaxEstimate / 2 + 1;

                // find all usefull cells in radius (distToCellWithMaxEstimate) without cell (cellWithMaxEstimate)
                std::map<float, int> radiusCellWorthMap; // map (worth) -> (cell)
                // ����� �� ���� � �������� ����� ��� ������ �������
                // ���� ������, ������� �� ���� � ��������� ������
                // � ������ ������ ��� ����� ����� � ���� radiusCellWorthMap
                for (const auto& worthCellSetPair : field_.worthToCellSetColonyMap[colonyId])
                {
                    float curWorthCellEstimation = worthCellSetPair.first;
                    for (int curWorthCell : worthCellSetPair.second)
                    {
                        // skip cell with max estimate
                        if (curWorthCell == cellWithMaxEstimate)
                        {
                            continue;
                        }

                        int distFromNearestColonyCell = int(field_.pathMapVec[nearestColonyCellToBestCell][curWorthCell].size()),
                            distFromBestCell = int(field_.pathMapVec[cellWithMaxEstimate][curWorthCell].size());
                        int pathOverCellLength = distFromBestCell + distFromBestCell;

                        // skip cell with too long dist
                        if (distFromNearestColonyCell > distToCellWithMaxEstimate
                            || distFromBestCell > distToCellWithMaxEstimate
                            || pathOverCellLength > maxPossiblePathLength)
                        {
                            continue;
                        }

                        float distCoef = pathOverCellLength == 0 ? 2 : (1 / float(pathOverCellLength));
                        radiusCellWorthMap[curWorthCellEstimation * distCoef] = curWorthCell;
                    }
                }

                // if no more useful cells in radius make direct line to nearest colony cell
                if (radiusCellWorthMap.empty())
                {
                    // ���������� allCellsUsedInLines
                    for (int cell : field_.pathMapVec[nearestColonyCellToBestCell][cellWithMaxEstimate])
                    {
                        field_.coloniesBeaconsSetMap[colonyId].insert(cell);
                    }
                    continue;
                }

                // take max useful cell and make path over this cell with beacons

                int intermdiateCell = radiusCellWorthMap.rbegin()->second;
                std::cerr << "Intermdiate Cell for colony " << colonyId << " with max esimation : " << intermdiateCell << std::endl;

                // make line from nearest colony cell to Intermdiate Cell
                for (int cell : field_.pathMapVec[nearestColonyCellToBestCell][intermdiateCell])
                {
                    field_.coloniesBeaconsSetMap[colonyId].insert(cell);
                }
                // and then make line from Intermdiate Cell to Cell with max estimate
                for (int cell : field_.pathMapVec[intermdiateCell][cellWithMaxEstimate])
                {
                    field_.coloniesBeaconsSetMap[colonyId].insert(cell);
                }
            }
        }

        // std::cerr << "end tryMakeNewLinesInColonies" << std::endl;
    }

    void setBeacons()
    {
        // map cell -> strength
        std::map<int, int> totalBeaconsMap;
        for (const auto& colonyPair : field_.turnColoniesMap)
        {
            const int colonyId = colonyPair.first;

            for (int beaconCell : field_.coloniesBeaconsSetMap[colonyId])
            {
                totalBeaconsMap[beaconCell] = 
                    field_.cells[beaconCell].oppAnts > field_.cells[beaconCell].myAnts ? 8 : 4;
            }
        }
        for (const auto& beaconsPair : totalBeaconsMap)
        {
            curTurnActions.push_back(std::make_unique<BeaconAction>(beaconsPair.first, beaconsPair.second));
        }
    }

    void makeActions()
    {
        curTurnActions.clear();
        field_.coloniesLinesMap.clear();

        saveActualLinesInColonies();

        tryMakeNewLinesInColonies();

        setBeacons();

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
