
class DiskGroup{

};

class Disk{
private:
    int unitNum;
    int headPos;
    int* unitStart;
    Disk(int unitNum, int* unitStart = nullptr, int headPos = 0){
        this->unitNum = unitNum;
        this->unitStart = unitStart;
        this->headPos = headPos;
    }
public:

};

