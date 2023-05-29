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

    // syntetic
    std::vector<int> crystalCells;
    std::vector<int> eggsCells;

    std::vector<std::vector<int>> distancesFromBases;

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
        distancesFromBases.clear();
        distancesFromBases.resize(friendlyBases.size());

        for (int friendlyBaseIdx = 0; friendlyBaseIdx < friendlyBases.size(); friendlyBaseIdx++)
        {
            distancesFromBases[friendlyBaseIdx] = std::vector<int>(numberOfCells, -1);

            const int curFriendlyBaseIdx = friendlyBases[friendlyBaseIdx];
            distancesFromBases[friendlyBaseIdx][curFriendlyBaseIdx] = 0;
            makeDfs(curFriendlyBaseIdx, distancesFromBases[friendlyBaseIdx]);

            std::map<int, std::list<int>> distancesMap;
            for (int i = 0; i < numberOfCells; i++)
            {
                distancesMap[distancesFromBases[friendlyBaseIdx][i]].push_back(i);
            }
            std::cerr << "Distances from friendly base " << curFriendlyBaseIdx << ":\n";
            for (const auto& distPair : distancesMap)
            {
                std::cerr << "Distance " << distPair.first << ": [";
                for (int cellIdx : distPair.second)
                {
                    std::cerr << cellIdx << " ";
                }
                std::cerr << "]\n";
            }
        }
    }

    void readTurnState()
    {
        for (int cellIdx = 0; cellIdx < cells.size(); cellIdx++)
        {
            cells.at(cellIdx).readTurnState();
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

    void makeActions()
    {
        curTurnActions.clear();

        makeLineForAllCrystalls();

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