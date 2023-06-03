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

    // для каждой колонии мапа: cell -> cell, по которым строятся линии биконов
    std::map<int, std::map<int, int>> coloniesLinesMap;

    // для каждой колонии набор ячеек, в которых должны быть биконы
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

        // пройтись по всем ячейкам и при нахождении среди соседей минимального значения колонии - распрострянть это значение на всех соседей
        // при этом не забывать взводить флаг existNewUnions
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

        // теперь все союзные ячейки поделены на колонии

        // сохраняем колонии в мапу колоний
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

        // удаление "вырожденных" колоний
        // колония считается вырожденной, если в ней нет ни одной союзной базы
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

// если какие то яйца пропали с карты, то про них надо забыть
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

// если какие то кристаллы пропали с карты, то про них надо забыть
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

// изменение оценки кристаллов
// чем меньше кристаллов на карте осталось - тем они ценнее
        crystallEstimation = float(totalCrystalsNb) / float(onTurnMapCrystalsNb);

// если текущее кол-во муравьев позволяет собрать половину кристалов от начального
// то кристаллы собирать становится ГОРАЗДО выгоднее
        if (friendlyAntsOnCurTurn > totalCrystalsNb / 2)
        {
            crystallEstimation *= 10;
        }

// оценка стоимости ячеек для каждой колонии
        for (const auto& colonyWorthMapPair : turnColoniesMap)
        {
            const int colonyId = colonyWorthMapPair.first;

            // построение обратной мапы: cell -> worth
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

    // к оценке каждой ячейки добавляется сумма оценок соседей и записывается в мапу с оценками для текущей колонии
            // пробегаемся по локальной мапе curColonyWorthMap и считаем ПОЛНУЮ оценку отдельно
            for (const auto& worthPair : curColonyWorthMap)
            {
                float curCellFullWorth = worthPair.second;
                // добавляем оценки соседей
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

                // записываем сумму в глобальную мапу
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

        // в каждой колонии линии строятся между опорными точками - клетками с ресурсами, которые представляют какую-либо ценность
        // первоначально линия сохраняется от базы до ближайшей полезной точки
        // после того, как убдет добавлена основная линия, можно достравитьва линии от/до остальных полезных клетко

        for (const auto& colonyPair : field_.turnColoniesMap)
        {
            const int colonyId = colonyPair.first;
            const std::set<int> colonyCells = colonyPair.second;

        // определение союзной базы в данной колонии
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

            // если союзная база не нашлась, то это означает, что данная колония вырожденная и надо эту колонию слить с одной из полноценных колоний
            if (friendlyColonyBase == -1)
            {
                // std::cerr << "FIXME saveActualLinesInColonies" << std::endl;
                continue;
            }

            // набор задействованных опорных точек
            // состоит из союзной базы и всех ячеек колонии, представляющих какую-либо ценност
            std::set<int> colonyReferenceCells{ friendlyColonyBase };

            // набор НЕ использованных опорных точек колони
            std::set<int> freeReferenceCells;

            // все ячейки, задействованные в построении линий
            std::set<int> allCellsUsedInLines;

            // наполняем набор freeReferenceCells
            for (int colonyCell : colonyCells)
            {
                // игнорим пустые ячейки
                if (!field_.eggsCells.count(colonyCell) && !field_.crystalCells.count(colonyCell))
                {
                    continue;
                }

                freeReferenceCells.insert(colonyCell);
            }

        // после определния союзной базы в колонии, надо найти ближайшую к этой базе опорную точку среди freeReferenceCells
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

            // сохраняем первую опорную линию
            field_.coloniesLinesMap[colonyId][friendlyColonyBase] = nearestResourceColonyCellToFriendBase;

            // перераспределние опорных точек
            freeReferenceCells.erase(nearestResourceColonyCellToFriendBase);
            colonyReferenceCells.insert(nearestResourceColonyCellToFriendBase);

            // наполнение allCellsUsedInLines
            allCellsUsedInLines.insert(friendlyColonyBase);
            for (int cell : field_.pathMapVec[friendlyColonyBase][nearestResourceColonyCellToFriendBase])
            {
                allCellsUsedInLines.insert(cell);
            }

        // пока есть свободные опорные точки
        // пытаемся найти такую опорную точку, которая будет наиближайшей к построенным линиям внутри колонии
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

                        // ускоряем итерирование
                        if (minDistFromRefernceCell <= 1)
                        {
                            break;
                        }
                    }

                    // ускоряем итерирование
                    if (minDistFromRefernceCell <= 1)
                    {
                        break;
                    }
                }

                // если что то пошло не так
                if (nextUsedReferenceCell == -1 || nearestCellInAllCellsUsedInLines == -1)
                {
                    break;
                }


             // сохраняем опорную линию
                field_.coloniesLinesMap[colonyId][nextUsedReferenceCell] = nearestCellInAllCellsUsedInLines;

                // перераспределние опорных точек
                freeReferenceCells.erase(nextUsedReferenceCell);
                colonyReferenceCells.insert(nextUsedReferenceCell);

                // наполнение allCellsUsedInLines
                for (int cell : field_.pathMapVec[nearestCellInAllCellsUsedInLines][nextUsedReferenceCell])
                {
                    allCellsUsedInLines.insert(cell);
                }
            }

        // сохраняем полученный набор клеток для совершения хода и выставления бикново
            field_.coloniesBeaconsSetMap[colonyId] = allCellsUsedInLines;
        }

        // std::cerr << "end saveActualLinesInColonies" << std::endl;
    }

    void tryMakeNewLinesInColonies()
    {
        // std::cerr << "begin tryMakeNewLinesInColonies" << std::endl;

        // для каждой колонии надо "перестроить колонии": изменить конфигурацию маяков внутри колонии и, опционально, расширить зону колонии
        for (const auto& colonyPair : field_.turnColoniesMap)
        {
            const int colonyId = colonyPair.first;
            const std::set<int>& curColonyCellSet = colonyPair.second;
            
            // если список потенциальных таргетов пустой - делать нечего
            if (field_.worthToCellSetColonyMap[colonyId].empty())
            {
                continue;
            }

            // новые пути строятся до всех клеток, оценка которых превышает некоторый порог
            float colonyEstimationThreshold = -1.f;
            for (auto bestEstimateIter = field_.worthToCellSetColonyMap[colonyId].rbegin(); bestEstimateIter != field_.worthToCellSetColonyMap[colonyId].rend(); bestEstimateIter++)
            {
                // набор ячеек с текущим эстимейтом
                const std::set<int>& cellsSetWithCurEstimation = bestEstimateIter->second;

                for (int bestEstimateCell : cellsSetWithCurEstimation)
                {
                    // если текущей ячейки нет в мапе с таргетами для колонии, то используем эту оценку как максимальную
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

                    // набор ячеек с текущим эстимейтом
                    const std::set<int>& cellsSetWithCurEstimation = bestEstimateIter->second;

                    for (int bestEstimateCell : cellsSetWithCurEstimation)
                    {
                        // если текущей ячейки нет в мапе с таргетами дял колонии, то сохраняем эту ячейку как целевую для колонии
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

                // в текущей колонии надо найти ячейку, ближайшую к ячейке с макс оценкой
                // поиск ячейки производится среди field_.coloniesBeaconsSetMap[colonyId]

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
                // бежим по мапе с оценками ячеек для данной колонии
                // ищем ячейки, которые по пути к наилучшей ячейке
                // и строим оценку для таких ячеек в мапе radiusCellWorthMap
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
                    // наполнение allCellsUsedInLines
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
