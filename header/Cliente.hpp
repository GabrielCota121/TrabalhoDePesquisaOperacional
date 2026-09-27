#ifndef CLIENTE_HPP
#define CLIENTE_HPP
#include <vector>
#include <string>
#include <cmath>
class Cliente {
    private:
        int id;
        int xCoord;
        int yCoord;
        int demandWasteType1;
        int demandWasteType2;
        int demandWasteType3;
        int readyTime;
        int dueDateNarrow;
        int dueDate;
        int dueDateWide;
        int service;
        int bidNarrow;
        int bid;
        int bidWide;

    public:
        Cliente(int id, int xCoord, int yCoord, int demandWasteType1, int demandWasteType2, int readyTime, int dueDateNarrow, int dueDate, int dueDateWide, int service, int bidNarrow, int bid, int bidWide, int demandWasteType3 = 0) {

            this->id = id;
            this->xCoord = xCoord;
            this->yCoord = yCoord;
            this->demandWasteType1 = demandWasteType1;
            this->demandWasteType2 = demandWasteType2;
            this->demandWasteType3 = demandWasteType3;
            this->readyTime = readyTime;
            this->dueDateNarrow = dueDateNarrow;
            this->dueDate = dueDate;
            this->dueDateWide = dueDateWide;
            this->service = service;
            this->bidNarrow = bidNarrow;
            this->bid = bid;
            this->bidWide = bidWide;
        }

        int getId() const { return id; }
        int getXCoord() const { return xCoord; }
        int getYCoord() const { return yCoord; }
        int getDemandWasteType1() const { return demandWasteType1; }
        int getDemandWasteType2() const { return demandWasteType2; }
        int getDemandWasteType3() const { return demandWasteType3; }
        int getReadyTime() const { return readyTime; }
        int getDueDateNarrow() const { return dueDateNarrow; }
        int getDueDate() const { return dueDate; }
        int getDueDateWide() const { return dueDateWide; }
        int getService() const { return service; }
        int getBidNarrow() const { return bidNarrow; }
        int getBid() const { return bid; }
        int getBidWide() const { return bidWide; }

};

#endif